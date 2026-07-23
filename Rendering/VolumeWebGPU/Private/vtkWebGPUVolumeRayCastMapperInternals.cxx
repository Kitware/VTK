// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPUVolumeRayCastMapperInternals.h"

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkMultiVolume.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPlane.h"
#include "vtkPlaneCollection.h"
#include "vtkPointData.h"
#include "vtkRenderer.h"
#include "vtkVolume.h"
#include "vtkVolumeMapper.h"
#include "vtkVolumeProperty.h"
#include "vtkWebGPUConfiguration.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWindow.h"

#include "VolumeRayCast.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
constexpr int MaxVolumes = vtkWebGPUVolumeRayCastMapper::MaximumNumberOfVolumes;
constexpr int MaxLights = vtkWebGPUVolumeRayCastMapper::vtkInternals::MaximumLights;
constexpr int MaxClipPlanes = vtkWebGPUVolumeRayCastMapper::vtkInternals::MaximumClipPlanes;
constexpr int TableSize = vtkWebGPUVolumeRayCastMapper::vtkInternals::TransferFunctionSize;

// Flags shared with VolumeRayCast.wgsl.
enum GlobalFlag : std::uint32_t
{
  FlagJitter = 1u,
  FlagAdaptive = 2u,
  FlagSkipEmpty = 4u,
  FlagFullScreen = 8u,
  FlagHasDepth = 16u
};

enum VolumeFlag : std::uint32_t
{
  VolumeFlagShade = 1u,
  VolumeFlagGradientOpacity = 2u
};

// Must match GlobalUniforms in VolumeRayCast.wgsl.
struct GlobalUniforms
{
  float ViewProjection[16];
  float InverseViewProjection[16];
  float BoundsMinimum[3];
  float JitterScale;
  float BoundsMaximum[3];
  float EmptyThreshold;
  float CameraPosition[3];
  std::uint32_t NumberOfVolumes;
  float Viewport[4];
  std::uint32_t NumberOfLights;
  std::uint32_t Flags;
  std::uint32_t BlendMode;
  std::uint32_t MaximumSteps;
  float MinimumStepFactor;
  float MaximumStepFactor;
  std::uint32_t NumberOfClipPlanes;
  float Padding0;
  float AverageRangeMinimum;
  float AverageRangeMaximum;
  float Padding1[2];
  float LightDirections[MaxLights][4];
  float LightColors[MaxLights][4];
  float ClipPlanes[MaxClipPlanes][4];
};
static_assert(sizeof(GlobalUniforms) == 528, "GlobalUniforms must match the WGSL layout");

// Must match VolumeParams in VolumeRayCast.wgsl.
struct VolumeParams
{
  float VolumeMatrix[16];
  float InverseVolumeMatrix[16];
  float Dimensions[3];
  float BaseStep;
  float Spacing[3];
  float ValueScale;
  float BrickDimensions[3];
  float ValueBias;
  float ScalarRangeMinimum;
  float ScalarRangeScale;
  std::uint32_t BrickOffset;
  float BrickSize;
  float OpacityUnitDistance;
  float Ambient;
  float Diffuse;
  float Specular;
  float SpecularPower;
  float GradientOpacityMinimum;
  float GradientOpacityScale;
  std::uint32_t Flags;
};
static_assert(sizeof(VolumeParams) == 224, "VolumeParams must match the WGSL layout");

//------------------------------------------------------------------------------
// WGSL reads matrices column by column, VTK stores them row by row.
void CopyMatrix(vtkMatrix4x4* source, float destination[16])
{
  for (int row = 0; row < 4; ++row)
  {
    for (int column = 0; column < 4; ++column)
    {
      destination[column * 4 + row] = static_cast<float>(source->GetElement(row, column));
    }
  }
}

//------------------------------------------------------------------------------
std::uint16_t PackHalf(float value)
{
  std::uint32_t bits;
  std::memcpy(&bits, &value, sizeof(bits));
  const std::uint32_t sign = (bits >> 16) & 0x8000u;
  const std::int32_t exponent = static_cast<std::int32_t>((bits >> 23) & 0xFFu) - 127 + 15;
  const std::uint32_t mantissa = bits & 0x7FFFFFu;
  if (exponent <= 0)
  {
    return static_cast<std::uint16_t>(sign);
  }
  if (exponent >= 31)
  {
    return static_cast<std::uint16_t>(sign | 0x7C00u);
  }
  return static_cast<std::uint16_t>(
    sign | (static_cast<std::uint32_t>(exponent) << 10) | (mantissa >> 13));
}

//------------------------------------------------------------------------------
template <typename T>
void CopyFirstComponent(const T* source, int components, vtkIdType count, float* destination)
{
  for (vtkIdType i = 0; i < count; ++i)
  {
    destination[i] = static_cast<float>(source[i * components]);
  }
}

//------------------------------------------------------------------------------
// Scalar extremes of every block, including one voxel of overlap on the far side so that the
// range also covers what interpolation can produce inside the block.
template <typename T>
void MeasureBricks(const T* texels, const int dimensions[3], int brickSize,
  const int brickDimensions[3], std::vector<float>& minimum, std::vector<float>& maximum)
{
  const vtkIdType sliceStride = static_cast<vtkIdType>(dimensions[0]) * dimensions[1];
  const std::size_t count =
    static_cast<std::size_t>(brickDimensions[0]) * brickDimensions[1] * brickDimensions[2];
  minimum.assign(count, std::numeric_limits<float>::max());
  maximum.assign(count, std::numeric_limits<float>::lowest());

  for (int bz = 0; bz < brickDimensions[2]; ++bz)
  {
    const int z0 = bz * brickSize;
    const int z1 = std::min(z0 + brickSize, dimensions[2] - 1);
    for (int by = 0; by < brickDimensions[1]; ++by)
    {
      const int y0 = by * brickSize;
      const int y1 = std::min(y0 + brickSize, dimensions[1] - 1);
      for (int bx = 0; bx < brickDimensions[0]; ++bx)
      {
        const int x0 = bx * brickSize;
        const int x1 = std::min(x0 + brickSize, dimensions[0] - 1);

        float low = std::numeric_limits<float>::max();
        float high = std::numeric_limits<float>::lowest();
        for (int z = z0; z <= z1; ++z)
        {
          for (int y = y0; y <= y1; ++y)
          {
            const vtkIdType row = z * sliceStride + static_cast<vtkIdType>(y) * dimensions[0];
            for (int x = x0; x <= x1; ++x)
            {
              const float value = static_cast<float>(texels[row + x]);
              low = std::min(low, value);
              high = std::max(high, value);
            }
          }
        }
        const std::size_t index =
          static_cast<std::size_t>(bz) * brickDimensions[1] * brickDimensions[0] +
          static_cast<std::size_t>(by) * brickDimensions[0] + bx;
        minimum[index] = low;
        maximum[index] = high;
      }
    }
  }
}

//------------------------------------------------------------------------------
// Extremes of the table between two entries in constant time, which turns block classification
// from a per block scan into a lookup.
class RangeTable
{
public:
  explicit RangeTable(const std::array<float, TableSize>& values)
  {
    this->Levels = 1;
    while ((1 << this->Levels) <= TableSize)
    {
      ++this->Levels;
    }
    this->Minimum.resize(this->Levels * TableSize);
    this->Maximum.resize(this->Levels * TableSize);
    for (int i = 0; i < TableSize; ++i)
    {
      this->Minimum[i] = values[i];
      this->Maximum[i] = values[i];
    }
    for (int level = 1; level < this->Levels; ++level)
    {
      const int span = 1 << level;
      const int half = span >> 1;
      for (int i = 0; i + span <= TableSize; ++i)
      {
        this->Minimum[level * TableSize + i] = std::min(this->Minimum[(level - 1) * TableSize + i],
          this->Minimum[(level - 1) * TableSize + i + half]);
        this->Maximum[level * TableSize + i] = std::max(this->Maximum[(level - 1) * TableSize + i],
          this->Maximum[(level - 1) * TableSize + i + half]);
      }
    }
  }

  void Query(int first, int last, float& low, float& high) const
  {
    first = std::clamp(first, 0, TableSize - 1);
    last = std::clamp(last, first, TableSize - 1);
    int level = 0;
    while ((1 << (level + 1)) <= (last - first + 1))
    {
      ++level;
    }
    const int offset = level * TableSize;
    const int second = last - (1 << level) + 1;
    low = std::min(this->Minimum[offset + first], this->Minimum[offset + second]);
    high = std::max(this->Maximum[offset + first], this->Maximum[offset + second]);
  }

private:
  int Levels = 1;
  std::vector<float> Minimum;
  std::vector<float> Maximum;
};

//------------------------------------------------------------------------------
wgpu::TextureFormat DepthOnlyFormat(wgpu::TextureFormat format)
{
  switch (format)
  {
    case wgpu::TextureFormat::Depth24PlusStencil8:
      return wgpu::TextureFormat::Depth24Plus;
    case wgpu::TextureFormat::Depth32FloatStencil8:
      return wgpu::TextureFormat::Depth32Float;
    default:
      return format;
  }
}
} // anonymous namespace

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::vtkInternals::ReleaseGraphicsResources()
{
  for (auto& entry : this->InputCache)
  {
    entry.second.Texture = nullptr;
    entry.second.TextureView = nullptr;
    entry.second.DataTime = 0;
  }
  this->GlobalUniformBuffer = nullptr;
  this->VolumeParamsBuffer = nullptr;
  this->BrickBuffer = nullptr;
  this->BrickBufferElements = 0;
  this->ColorTransferFunctions = nullptr;
  this->ColorTransferFunctionsView = nullptr;
  this->OpacityTransferFunctions = nullptr;
  this->OpacityTransferFunctionsView = nullptr;
  this->GradientTransferFunctions = nullptr;
  this->GradientTransferFunctionsView = nullptr;
  this->TransferFunctionSampler = nullptr;
  this->PlaceholderVolume = nullptr;
  this->PlaceholderVolumeView = nullptr;
  this->BindGroupLayout = nullptr;
  this->BindGroup = nullptr;
  this->Pipeline = nullptr;
  this->PipelineColorFormat = wgpu::TextureFormat::Undefined;
  this->OpaqueDepthView = nullptr;
  this->TransferFunctionTime = 0;
  this->BrickClassificationTime = 0;
}

//------------------------------------------------------------------------------
bool vtkWebGPUVolumeRayCastMapper::vtkInternals::CollectInputs(vtkVolume* volume)
{
  const std::vector<int> previous = this->ActivePorts;
  this->ActivePorts.clear();

  auto* multiVolume = vtkMultiVolume::SafeDownCast(volume);
  const std::vector<int> ports = this->Self->GetPorts();

  for (int port : ports)
  {
    if (static_cast<int>(this->ActivePorts.size()) >= MaxVolumes)
    {
      vtkWarningWithObjectMacro(this->Self,
        << "Only " << MaxVolumes << " inputs can be ray cast together, the rest are ignored.");
      break;
    }

    auto* image = vtkImageData::SafeDownCast(this->Self->GetTransformedInput(port));
    if (image == nullptr || image->GetPointData()->GetScalars() == nullptr)
    {
      continue;
    }

    vtkVolume* portVolume = multiVolume != nullptr ? multiVolume->GetVolume(port) : nullptr;
    const auto override = this->PortProperties.find(port);
    vtkVolumeProperty* property =
      override != this->PortProperties.end() ? override->second.Get() : nullptr;
    if (property == nullptr && portVolume != nullptr)
    {
      property = portVolume->GetProperty();
    }
    if (property == nullptr)
    {
      property = volume->GetProperty();
    }
    if (property == nullptr)
    {
      continue;
    }

    Input& input = this->InputCache[port];
    input.Port = port;
    input.Data = image;
    input.Property = property;
    input.WorldMatrix = portVolume != nullptr ? portVolume->GetMatrix() : volume->GetMatrix();
    this->ActivePorts.push_back(port);
  }

  if (this->ActivePorts != previous)
  {
    this->TransferFunctionTime = 0;
    this->BrickClassificationTime = 0;
    this->BindGroup = nullptr;
  }

  return !this->ActivePorts.empty();
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::vtkInternals::UpdateVolumeTexture(
  vtkWebGPURenderWindow* window, Input& input)
{
  vtkDataArray* scalars = input.Data->GetPointData()->GetScalars();
  const int brickSize = this->Self->GetBrickSize();
  if (input.Texture != nullptr && input.DataTime >= input.Data->GetMTime() &&
    input.BrickSize == brickSize)
  {
    return;
  }

  input.Data->GetDimensions(input.Dimensions);
  input.Data->GetSpacing(input.Spacing);
  scalars->GetRange(input.DataRange, 0);

  const int* dimensions = input.Dimensions;
  const vtkIdType count = static_cast<vtkIdType>(dimensions[0]) * dimensions[1] * dimensions[2];
  if (count <= 0)
  {
    return;
  }

  const int components = scalars->GetNumberOfComponents();
  if (components > 1)
  {
    vtkWarningWithObjectMacro(this->Self, << "Input " << input.Port << " has " << components
                                          << " components, only the first one is rendered.");
  }

  // Eight bit data stays eight bit on the device and is scaled back up in the shader. Everything
  // else becomes single precision float, which is exact for the types that fit.
  const bool asBytes = scalars->GetDataType() == VTK_UNSIGNED_CHAR;
  input.ValueScale = asBytes ? 255.0f : 1.0f;
  input.ValueBias = 0.0f;

  input.BrickSize = brickSize;
  for (int i = 0; i < 3; ++i)
  {
    input.BrickDimensions[i] = std::max(1, (dimensions[i] - 1) / brickSize + 1);
  }

  const wgpu::TextureFormat format =
    asBytes ? wgpu::TextureFormat::R8Unorm : wgpu::TextureFormat::R32Float;
  const std::uint32_t bytesPerVoxel = asBytes ? 1u : 4u;

  wgpu::TextureDescriptor descriptor;
  const std::string label = "VolumeTexture-" + std::to_string(input.Port);
  descriptor.label = label.c_str();
  descriptor.dimension = wgpu::TextureDimension::e3D;
  descriptor.size = { static_cast<std::uint32_t>(dimensions[0]),
    static_cast<std::uint32_t>(dimensions[1]), static_cast<std::uint32_t>(dimensions[2]) };
  descriptor.format = format;
  descriptor.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
  descriptor.mipLevelCount = 1;
  descriptor.sampleCount = 1;

  auto configuration = window->GetWGPUConfiguration();
  input.Texture = configuration->CreateTexture(descriptor);
  if (input.Texture == nullptr)
  {
    return;
  }

  const std::uint32_t bytesPerRow = static_cast<std::uint32_t>(dimensions[0]) * bytesPerVoxel;
  if (asBytes)
  {
    std::vector<unsigned char> texels(static_cast<std::size_t>(count));
    const auto* source = static_cast<const unsigned char*>(scalars->GetVoidPointer(0));
    for (vtkIdType i = 0; i < count; ++i)
    {
      texels[i] = source[i * components];
    }
    configuration->WriteTexture(input.Texture, bytesPerRow,
      static_cast<std::uint32_t>(texels.size()), texels.data(), 0, { 0, 0, 0 }, 0,
      "Volume texture upload");
    MeasureBricks(texels.data(), dimensions, brickSize, input.BrickDimensions, input.BrickMinimum,
      input.BrickMaximum);
  }
  else
  {
    std::vector<float> texels(static_cast<std::size_t>(count));
    void* source = scalars->GetVoidPointer(0);
    switch (scalars->GetDataType())
    {
      vtkTemplateMacro(
        CopyFirstComponent(static_cast<const VTK_TT*>(source), components, count, texels.data()));
      default:
        vtkErrorWithObjectMacro(
          this->Self, << "Unsupported scalar type on input " << input.Port << ".");
        return;
    }
    configuration->WriteTexture(input.Texture, bytesPerRow,
      static_cast<std::uint32_t>(texels.size() * sizeof(float)), texels.data(), 0, { 0, 0, 0 }, 0,
      "Volume texture upload");
    MeasureBricks(texels.data(), dimensions, brickSize, input.BrickDimensions, input.BrickMinimum,
      input.BrickMaximum);
  }

  wgpu::TextureViewDescriptor viewDescriptor;
  viewDescriptor.label = label.c_str();
  viewDescriptor.format = format;
  viewDescriptor.dimension = wgpu::TextureViewDimension::e3D;
  viewDescriptor.baseMipLevel = 0;
  viewDescriptor.mipLevelCount = 1;
  viewDescriptor.baseArrayLayer = 0;
  viewDescriptor.arrayLayerCount = 1;
  input.TextureView = input.Texture.CreateView(&viewDescriptor);

  input.DataTime = input.Data->GetMTime();
  // The blocks were measured again, so their classification is stale.
  this->BrickClassificationTime = 0;
  this->BindGroup = nullptr;
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::vtkInternals::UpdateTransferFunctions(
  vtkWebGPURenderWindow* window)
{
  vtkMTimeType latest = 0;
  for (int port : this->ActivePorts)
  {
    latest = std::max(latest, this->InputCache[port].Property->GetMTime());
  }
  if (this->ColorTransferFunctions != nullptr && latest <= this->TransferFunctionTime &&
    this->OpacityTables.size() == this->ActivePorts.size())
  {
    return;
  }

  auto configuration = window->GetWGPUConfiguration();

  if (this->ColorTransferFunctions == nullptr)
  {
    const auto makeArray = [&](const char* label)
    {
      wgpu::TextureDescriptor descriptor;
      descriptor.label = label;
      descriptor.dimension = wgpu::TextureDimension::e2D;
      descriptor.size = { TableSize, 1, MaxVolumes };
      descriptor.format = wgpu::TextureFormat::RGBA16Float;
      descriptor.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
      descriptor.mipLevelCount = 1;
      descriptor.sampleCount = 1;
      return configuration->CreateTexture(descriptor);
    };
    this->ColorTransferFunctions = makeArray("ColorTransferFunctions");
    this->OpacityTransferFunctions = makeArray("OpacityTransferFunctions");
    this->GradientTransferFunctions = makeArray("GradientTransferFunctions");

    const auto makeView = [&](wgpu::Texture& texture, const char* label)
    {
      wgpu::TextureViewDescriptor descriptor;
      descriptor.label = label;
      descriptor.format = wgpu::TextureFormat::RGBA16Float;
      descriptor.dimension = wgpu::TextureViewDimension::e2DArray;
      descriptor.baseMipLevel = 0;
      descriptor.mipLevelCount = 1;
      descriptor.baseArrayLayer = 0;
      descriptor.arrayLayerCount = MaxVolumes;
      return texture.CreateView(&descriptor);
    };
    this->ColorTransferFunctionsView = makeView(this->ColorTransferFunctions, "ColorTF");
    this->OpacityTransferFunctionsView = makeView(this->OpacityTransferFunctions, "OpacityTF");
    this->GradientTransferFunctionsView = makeView(this->GradientTransferFunctions, "GradientTF");
    this->BindGroup = nullptr;
  }

  this->OpacityTables.assign(this->ActivePorts.size(), {});

  std::vector<std::uint16_t> color(TableSize * 4);
  std::vector<std::uint16_t> opacity(TableSize * 4);
  std::vector<std::uint16_t> gradient(TableSize * 4);

  for (std::size_t index = 0; index < this->ActivePorts.size(); ++index)
  {
    Input& input = this->InputCache[this->ActivePorts[index]];
    vtkVolumeProperty* property = input.Property;

    // Tabulate over whatever the transfer functions actually cover, falling back to the data.
    vtkColorTransferFunction* rgb =
      property->GetColorChannels(0) == 1 ? nullptr : property->GetRGBTransferFunction(0);
    vtkPiecewiseFunction* gray =
      property->GetColorChannels(0) == 1 ? property->GetGrayTransferFunction(0) : nullptr;
    vtkPiecewiseFunction* scalarOpacity = property->GetScalarOpacity(0);

    double range[2] = { vtkMath::Inf(), -vtkMath::Inf() };
    const auto widen = [&range](const double other[2])
    {
      if (other[1] > other[0])
      {
        range[0] = std::min(range[0], other[0]);
        range[1] = std::max(range[1], other[1]);
      }
    };
    if (rgb != nullptr)
    {
      widen(rgb->GetRange());
    }
    if (gray != nullptr)
    {
      widen(gray->GetRange());
    }
    if (scalarOpacity != nullptr)
    {
      widen(scalarOpacity->GetRange());
    }
    if (range[1] <= range[0])
    {
      range[0] = input.DataRange[0];
      range[1] = input.DataRange[1];
    }
    if (range[1] <= range[0])
    {
      range[1] = range[0] + 1.0;
    }
    input.ScalarRange[0] = range[0];
    input.ScalarRange[1] = range[1];

    const double step = (range[1] - range[0]) / (TableSize - 1);
    for (int i = 0; i < TableSize; ++i)
    {
      const double value = range[0] + i * step;

      double rgbValue[3] = { 1.0, 1.0, 1.0 };
      if (rgb != nullptr)
      {
        rgb->GetColor(value, rgbValue);
      }
      else if (gray != nullptr)
      {
        const double level = gray->GetValue(value);
        rgbValue[0] = rgbValue[1] = rgbValue[2] = level;
      }
      color[i * 4 + 0] = PackHalf(static_cast<float>(rgbValue[0]));
      color[i * 4 + 1] = PackHalf(static_cast<float>(rgbValue[1]));
      color[i * 4 + 2] = PackHalf(static_cast<float>(rgbValue[2]));
      color[i * 4 + 3] = PackHalf(1.0f);

      const float alpha =
        scalarOpacity != nullptr ? static_cast<float>(scalarOpacity->GetValue(value)) : 1.0f;
      this->OpacityTables[index][i] = std::clamp(alpha, 0.0f, 1.0f);
      opacity[i * 4 + 0] = PackHalf(alpha);
      opacity[i * 4 + 1] = 0;
      opacity[i * 4 + 2] = 0;
      opacity[i * 4 + 3] = PackHalf(1.0f);
    }

    vtkPiecewiseFunction* gradientOpacity =
      property->GetDisableGradientOpacity(0) != 0 ? nullptr : property->GetGradientOpacity(0);
    double gradientRange[2] = { 0.0, 1.0 };
    if (gradientOpacity != nullptr)
    {
      gradientOpacity->GetRange(gradientRange);
      if (gradientRange[1] <= gradientRange[0])
      {
        gradientRange[1] = gradientRange[0] + 1.0;
      }
    }
    input.GradientRange[0] = gradientRange[0];
    input.GradientRange[1] = gradientRange[1];

    const double gradientStep = (gradientRange[1] - gradientRange[0]) / (TableSize - 1);
    for (int i = 0; i < TableSize; ++i)
    {
      const float value = gradientOpacity != nullptr
        ? static_cast<float>(gradientOpacity->GetValue(gradientRange[0] + i * gradientStep))
        : 1.0f;
      gradient[i * 4 + 0] = PackHalf(value);
      gradient[i * 4 + 1] = 0;
      gradient[i * 4 + 2] = 0;
      gradient[i * 4 + 3] = PackHalf(1.0f);
    }

    const std::uint32_t bytesPerRow = TableSize * 4 * sizeof(std::uint16_t);
    const wgpu::Origin3D layer = { 0, 0, static_cast<std::uint32_t>(index) };
    configuration->WriteTexture(this->ColorTransferFunctions, bytesPerRow, bytesPerRow,
      color.data(), 0, layer, 0, "Color transfer function");
    configuration->WriteTexture(this->OpacityTransferFunctions, bytesPerRow, bytesPerRow,
      opacity.data(), 0, layer, 0, "Opacity transfer function");
    configuration->WriteTexture(this->GradientTransferFunctions, bytesPerRow, bytesPerRow,
      gradient.data(), 0, layer, 0, "Gradient opacity transfer function");
  }

  if (this->TransferFunctionSampler == nullptr)
  {
    wgpu::SamplerDescriptor descriptor;
    descriptor.label = "TransferFunctionSampler";
    descriptor.addressModeU = wgpu::AddressMode::ClampToEdge;
    descriptor.addressModeV = wgpu::AddressMode::ClampToEdge;
    descriptor.addressModeW = wgpu::AddressMode::ClampToEdge;
    descriptor.magFilter = wgpu::FilterMode::Linear;
    descriptor.minFilter = wgpu::FilterMode::Linear;
    this->TransferFunctionSampler = window->GetDevice().CreateSampler(&descriptor);
  }

  this->TransferFunctionTime = latest;
  this->BrickClassificationTime = 0;
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::vtkInternals::UpdateBrickClassification(
  vtkWebGPURenderWindow* window)
{
  if (this->BrickClassificationTime != 0 &&
    this->BrickClassificationTime >= this->TransferFunctionTime)
  {
    return;
  }

  std::size_t total = 0;
  for (int port : this->ActivePorts)
  {
    Input& input = this->InputCache[port];
    input.BrickOffset = static_cast<std::uint32_t>(total);
    total += input.BrickMinimum.size();
  }
  total = std::max<std::size_t>(total, 1);

  std::vector<float> classification(total * 2, 0.0f);
  // A single opaque, featureless entry keeps a mapper with no blocks from skipping everything.
  classification[0] = 1.0f;
  classification[1] = 1.0f;

  for (std::size_t index = 0; index < this->ActivePorts.size(); ++index)
  {
    Input& input = this->InputCache[this->ActivePorts[index]];
    if (input.BrickMinimum.empty())
    {
      continue;
    }

    const RangeTable table(this->OpacityTables[index]);
    const double origin = input.ScalarRange[0];
    const double scale = 1.0 / (input.ScalarRange[1] - input.ScalarRange[0]);

    for (std::size_t brick = 0; brick < input.BrickMinimum.size(); ++brick)
    {
      const double low = std::clamp((input.BrickMinimum[brick] - origin) * scale, 0.0, 1.0);
      const double high = std::clamp((input.BrickMaximum[brick] - origin) * scale, 0.0, 1.0);

      float opacityLow = 0.0f;
      float opacityHigh = 0.0f;
      table.Query(static_cast<int>(std::floor(low * (TableSize - 1))),
        static_cast<int>(std::ceil(high * (TableSize - 1))), opacityLow, opacityHigh);

      // How much the block varies, either in the data or in what the transfer function makes of
      // it. Amplified so that a block only counts as featureless when it really is almost flat,
      // because coarse steps through a slowly changing region are what banding is made of.
      constexpr float sensitivity = 4.0f;
      const float detail =
        std::clamp(sensitivity * std::max(static_cast<float>(high - low), opacityHigh - opacityLow),
          0.0f, 1.0f);

      const std::size_t slot = (input.BrickOffset + brick) * 2;
      classification[slot] = opacityHigh;
      classification[slot + 1] = detail;
    }
  }

  auto configuration = window->GetWGPUConfiguration();
  const std::size_t bytes = classification.size() * sizeof(float);
  if (this->BrickBuffer == nullptr || this->BrickBufferElements != total)
  {
    this->BrickBuffer = configuration->CreateBuffer(
      bytes, wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst, false, "VolumeBlocks");
    this->BrickBufferElements = total;
    this->BindGroup = nullptr;
  }
  configuration->WriteBuffer(this->BrickBuffer, 0, classification.data(), bytes, "VolumeBlocks");

  this->BrickClassificationTime = std::max<vtkMTimeType>(this->TransferFunctionTime, 1);
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::vtkInternals::EnsurePlaceholders(vtkWebGPURenderWindow* window)
{
  if (this->PlaceholderVolume != nullptr)
  {
    return;
  }
  auto configuration = window->GetWGPUConfiguration();

  wgpu::TextureDescriptor descriptor;
  descriptor.label = "PlaceholderVolume";
  descriptor.dimension = wgpu::TextureDimension::e3D;
  descriptor.size = { 1, 1, 1 };
  descriptor.format = wgpu::TextureFormat::R32Float;
  descriptor.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
  descriptor.mipLevelCount = 1;
  descriptor.sampleCount = 1;
  this->PlaceholderVolume = configuration->CreateTexture(descriptor);

  const float zero = 0.0f;
  configuration->WriteTexture(
    this->PlaceholderVolume, 4, 4, &zero, 0, { 0, 0, 0 }, 0, "PlaceholderVolume");

  wgpu::TextureViewDescriptor viewDescriptor;
  viewDescriptor.label = "PlaceholderVolume";
  viewDescriptor.format = wgpu::TextureFormat::R32Float;
  viewDescriptor.dimension = wgpu::TextureViewDimension::e3D;
  viewDescriptor.baseMipLevel = 0;
  viewDescriptor.mipLevelCount = 1;
  viewDescriptor.baseArrayLayer = 0;
  viewDescriptor.arrayLayerCount = 1;
  this->PlaceholderVolumeView = this->PlaceholderVolume.CreateView(&viewDescriptor);
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::vtkInternals::SyncDeviceResources(
  vtkWebGPURenderWindow* window, vtkVolume* volume)
{
  if (!this->CollectInputs(volume))
  {
    return;
  }
  this->EnsurePlaceholders(window);
  for (int port : this->ActivePorts)
  {
    this->UpdateVolumeTexture(window, this->InputCache[port]);
  }
  this->UpdateTransferFunctions(window);
  this->UpdateBrickClassification(window);
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::vtkInternals::UpdateUniforms(
  vtkWebGPURenderWindow* window, vtkRenderer* renderer, vtkVolume* volume)
{
  auto configuration = window->GetWGPUConfiguration();

  // The world space box that every input fits in doubles as proxy geometry.
  double bounds[6] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN, VTK_DOUBLE_MAX, VTK_DOUBLE_MIN,
    VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };

  std::vector<VolumeParams> parameters(MaxVolumes);
  std::memset(parameters.data(), 0, parameters.size() * sizeof(VolumeParams));

  const bool interactive = volume->GetAllocatedRenderTime() < 1.0;
  double smallestStep = VTK_DOUBLE_MAX;

  for (std::size_t index = 0; index < this->ActivePorts.size(); ++index)
  {
    Input& input = this->InputCache[this->ActivePorts[index]];
    VolumeParams& params = parameters[index];
    vtkVolumeProperty* property = input.Property;

    double extent[3];
    for (int i = 0; i < 3; ++i)
    {
      extent[i] = std::max(input.Dimensions[i] - 1, 1);
    }

    vtkNew<vtkMatrix4x4> textureToIndex;
    textureToIndex->Identity();
    for (int i = 0; i < 3; ++i)
    {
      textureToIndex->SetElement(i, i, extent[i]);
    }

    vtkNew<vtkMatrix4x4> indexToWorld;
    vtkMatrix4x4::Multiply4x4(
      input.WorldMatrix, input.Data->GetIndexToPhysicalMatrix(), indexToWorld);

    vtkNew<vtkMatrix4x4> volumeMatrix;
    vtkMatrix4x4::Multiply4x4(indexToWorld, textureToIndex, volumeMatrix);
    vtkNew<vtkMatrix4x4> inverseVolumeMatrix;
    vtkMatrix4x4::Invert(volumeMatrix, inverseVolumeMatrix);

    CopyMatrix(volumeMatrix, params.VolumeMatrix);
    CopyMatrix(inverseVolumeMatrix, params.InverseVolumeMatrix);

    // Grow the proxy box by the eight corners of this input.
    for (int corner = 0; corner < 8; ++corner)
    {
      const double point[4] = { static_cast<double>(corner & 1),
        static_cast<double>((corner >> 1) & 1), static_cast<double>((corner >> 2) & 1), 1.0 };
      double world[4];
      volumeMatrix->MultiplyPoint(point, world);
      for (int i = 0; i < 3; ++i)
      {
        bounds[2 * i] = std::min(bounds[2 * i], world[i]);
        bounds[2 * i + 1] = std::max(bounds[2 * i + 1], world[i]);
      }
    }

    // How long one voxel is once the volume is placed in the world.
    double voxel[3];
    for (int axis = 0; axis < 3; ++axis)
    {
      double column[3];
      for (int row = 0; row < 3; ++row)
      {
        column[row] = indexToWorld->GetElement(row, axis);
      }
      voxel[axis] = std::max(vtkMath::Norm(column), 1.0e-9);
      params.Spacing[axis] = static_cast<float>(voxel[axis]);
    }

    // One voxel is the sample distance vtkOpenGLGPUVolumeRayCastMapper settles on, which keeps
    // the two backends comparable. Adaptive sampling refines it from here.
    double step = std::min({ voxel[0], voxel[1], voxel[2] });
    if (this->Self->GetAutoAdjustSampleDistances() == 0 && this->Self->GetSampleDistance() > 0.0f)
    {
      step = this->Self->GetSampleDistance();
    }
    params.BaseStep = static_cast<float>(step);
    smallestStep = std::min(smallestStep, step);

    for (int i = 0; i < 3; ++i)
    {
      params.Dimensions[i] = static_cast<float>(input.Dimensions[i]);
      params.BrickDimensions[i] = static_cast<float>(input.BrickDimensions[i]);
    }
    params.ValueScale = input.ValueScale;
    params.ValueBias = input.ValueBias;
    params.ScalarRangeMinimum = static_cast<float>(input.ScalarRange[0]);
    params.ScalarRangeScale =
      static_cast<float>(1.0 / (input.ScalarRange[1] - input.ScalarRange[0]));
    params.BrickOffset = input.BrickOffset;
    params.BrickSize = static_cast<float>(std::max(input.BrickSize, 1));
    params.OpacityUnitDistance =
      static_cast<float>(std::max(property->GetScalarOpacityUnitDistance(0), 1.0e-6));
    params.Ambient = static_cast<float>(property->GetAmbient(0));
    params.Diffuse = static_cast<float>(property->GetDiffuse(0));
    params.Specular = static_cast<float>(property->GetSpecular(0));
    params.SpecularPower = static_cast<float>(property->GetSpecularPower(0));

    const double gradientExtent = input.GradientRange[1] - input.GradientRange[0];
    params.GradientOpacityMinimum = static_cast<float>(input.GradientRange[0]);
    params.GradientOpacityScale =
      static_cast<float>(gradientExtent > 0.0 ? 1.0 / gradientExtent : 1.0);

    params.Flags = 0;
    if (property->GetShade(0) != 0)
    {
      params.Flags |= VolumeFlagShade;
    }
    if (property->GetDisableGradientOpacity(0) == 0)
    {
      params.Flags |= VolumeFlagGradientOpacity;
    }
  }

  const double diagonal = std::sqrt(std::pow(bounds[1] - bounds[0], 2.0) +
    std::pow(bounds[3] - bounds[2], 2.0) + std::pow(bounds[5] - bounds[4], 2.0));

  // While the scene is being interacted with, cap how many samples a ray may take by spreading
  // them over the whole box instead of refusing to finish the frame.
  if (interactive && this->Self->GetAutoAdjustSampleDistances() != 0)
  {
    const double coarse = diagonal / this->Self->GetInteractiveMaximumNumberOfSamples();
    if (coarse > smallestStep)
    {
      const double factor = coarse / smallestStep;
      for (std::size_t index = 0; index < this->ActivePorts.size(); ++index)
      {
        parameters[index].BaseStep = static_cast<float>(parameters[index].BaseStep * factor);
      }
      smallestStep = coarse;
    }
  }

  GlobalUniforms globals;
  std::memset(&globals, 0, sizeof(globals));

  vtkCamera* camera = renderer->GetActiveCamera();
  vtkNew<vtkMatrix4x4> view;
  view->DeepCopy(camera->GetModelViewTransformMatrix());
  vtkNew<vtkMatrix4x4> projection;
  projection->DeepCopy(
    camera->GetProjectionTransformMatrix(renderer->GetTiledAspectRatio(), 0.0, 1.0));

  vtkNew<vtkMatrix4x4> viewProjection;
  vtkMatrix4x4::Multiply4x4(projection, view, viewProjection);
  vtkNew<vtkMatrix4x4> inverseViewProjection;
  vtkMatrix4x4::Invert(viewProjection, inverseViewProjection);
  CopyMatrix(viewProjection, globals.ViewProjection);
  CopyMatrix(inverseViewProjection, globals.InverseViewProjection);

  for (int i = 0; i < 3; ++i)
  {
    globals.BoundsMinimum[i] = static_cast<float>(bounds[2 * i]);
    globals.BoundsMaximum[i] = static_cast<float>(bounds[2 * i + 1]);
  }

  double cameraPosition[3];
  camera->GetPosition(cameraPosition);
  for (int i = 0; i < 3; ++i)
  {
    globals.CameraPosition[i] = static_cast<float>(cameraPosition[i]);
  }

  int viewportSize[2];
  int viewportOrigin[2];
  renderer->GetTiledSizeAndOrigin(
    &viewportSize[0], &viewportSize[1], &viewportOrigin[0], &viewportOrigin[1]);
  if (auto* vtkWindow = renderer->GetVTKWindow())
  {
    const int* actual = vtkWindow->GetActualSize();
    viewportOrigin[1] = actual[1] - viewportOrigin[1] - viewportSize[1];
  }
  this->ViewportRectangle[0] = static_cast<float>(viewportOrigin[0]);
  this->ViewportRectangle[1] = static_cast<float>(viewportOrigin[1]);
  this->ViewportRectangle[2] = static_cast<float>(std::max(viewportSize[0], 1));
  this->ViewportRectangle[3] = static_cast<float>(std::max(viewportSize[1], 1));
  std::copy(std::begin(this->ViewportRectangle), std::end(this->ViewportRectangle),
    std::begin(globals.Viewport));

  // The back faces of the proxy box stop covering every pixel once the near plane cuts into it,
  // so fall back to a full screen triangle when the two can meet.
  double nearBounds[6] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN, VTK_DOUBLE_MAX, VTK_DOUBLE_MIN,
    VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
  for (int corner = 0; corner < 4; ++corner)
  {
    const double clip[4] = { (corner & 1) != 0 ? 1.0 : -1.0, (corner & 2) != 0 ? 1.0 : -1.0, 0.0,
      1.0 };
    double world[4];
    inverseViewProjection->MultiplyPoint(clip, world);
    for (int i = 0; i < 3; ++i)
    {
      const double value = world[i] / world[3];
      nearBounds[2 * i] = std::min(nearBounds[2 * i], value);
      nearBounds[2 * i + 1] = std::max(nearBounds[2 * i + 1], value);
    }
  }
  const double margin = 1.0e-4 * std::max(diagonal, 1.0);
  this->DrawFullScreen = true;
  for (int i = 0; i < 3; ++i)
  {
    if (nearBounds[2 * i] > bounds[2 * i + 1] + margin ||
      nearBounds[2 * i + 1] < bounds[2 * i] - margin)
    {
      this->DrawFullScreen = false;
      break;
    }
  }

  globals.NumberOfVolumes = static_cast<std::uint32_t>(this->ActivePorts.size());
  globals.JitterScale = static_cast<float>(smallestStep);
  globals.EmptyThreshold = static_cast<float>(this->Self->GetEmptySpaceOpacityThreshold());
  globals.BlendMode = static_cast<std::uint32_t>(this->Self->GetBlendMode());
  globals.MinimumStepFactor = static_cast<float>(this->Self->GetMinimumSampleDistanceFactor());
  globals.MaximumStepFactor = static_cast<float>(std::max(
    this->Self->GetMaximumSampleDistanceFactor(), this->Self->GetMinimumSampleDistanceFactor()));
  globals.MaximumSteps = static_cast<std::uint32_t>(
    std::clamp(2.5 * diagonal / std::max(smallestStep, 1.0e-9) + 32.0, 64.0, 10000.0));

  double averageRange[2];
  this->Self->GetAverageIPScalarRange(averageRange);
  globals.AverageRangeMinimum = static_cast<float>(averageRange[0]);
  globals.AverageRangeMaximum = static_cast<float>(averageRange[1]);

  globals.Flags = 0;
  if (this->Self->GetJittering())
  {
    globals.Flags |= FlagJitter;
  }
  // The intensity blend modes weigh every sample equally, so they only mean what they say when
  // the samples are evenly spaced.
  const bool compositing = this->Self->GetBlendMode() == vtkVolumeMapper::COMPOSITE_BLEND;
  if (this->Self->GetAdaptiveSampling() && compositing)
  {
    globals.Flags |= FlagAdaptive;
    if (this->Self->GetEmptySpaceSkipping())
    {
      globals.Flags |= FlagSkipEmpty;
    }
  }
  if (this->DrawFullScreen)
  {
    globals.Flags |= FlagFullScreen;
  }
  if (this->DepthAvailable)
  {
    globals.Flags |= FlagHasDepth;
  }

  globals.NumberOfLights = 0;
  if (vtkLightCollection* lights = renderer->GetLights())
  {
    vtkCollectionSimpleIterator iterator;
    lights->InitTraversal(iterator);
    while (vtkLight* light = lights->GetNextLight(iterator))
    {
      if (globals.NumberOfLights >= static_cast<std::uint32_t>(MaxLights))
      {
        break;
      }
      if (light->GetSwitch() == 0)
      {
        continue;
      }
      double position[3];
      double focalPoint[3];
      light->GetTransformedPosition(position);
      light->GetTransformedFocalPoint(focalPoint);
      double direction[3] = { position[0] - focalPoint[0], position[1] - focalPoint[1],
        position[2] - focalPoint[2] };
      if (vtkMath::Normalize(direction) <= 0.0)
      {
        continue;
      }
      const std::uint32_t slot = globals.NumberOfLights;
      const double* color = light->GetDiffuseColor();
      const double intensity = light->GetIntensity();
      for (int i = 0; i < 3; ++i)
      {
        globals.LightDirections[slot][i] = static_cast<float>(direction[i]);
        globals.LightColors[slot][i] = static_cast<float>(color[i] * intensity);
      }
      ++globals.NumberOfLights;
    }
  }
  if (globals.NumberOfLights == 0)
  {
    // Stand in for the head light the renderer would have created.
    double focalPoint[3];
    camera->GetFocalPoint(focalPoint);
    double direction[3] = { cameraPosition[0] - focalPoint[0], cameraPosition[1] - focalPoint[1],
      cameraPosition[2] - focalPoint[2] };
    vtkMath::Normalize(direction);
    for (int i = 0; i < 3; ++i)
    {
      globals.LightDirections[0][i] = static_cast<float>(direction[i]);
      globals.LightColors[0][i] = 1.0f;
    }
    globals.NumberOfLights = 1;
  }

  globals.NumberOfClipPlanes = 0;
  if (vtkPlaneCollection* planes = this->Self->GetClippingPlanes())
  {
    vtkCollectionSimpleIterator iterator;
    planes->InitTraversal(iterator);
    while (vtkPlane* plane = planes->GetNextPlane(iterator))
    {
      if (globals.NumberOfClipPlanes >= static_cast<std::uint32_t>(MaxClipPlanes))
      {
        vtkWarningWithObjectMacro(
          this->Self, << "Only " << MaxClipPlanes << " clipping planes are supported.");
        break;
      }
      double normal[3];
      double origin[3];
      plane->GetNormal(normal);
      plane->GetOrigin(origin);
      if (vtkMath::Normalize(normal) <= 0.0)
      {
        continue;
      }
      const std::uint32_t slot = globals.NumberOfClipPlanes;
      for (int i = 0; i < 3; ++i)
      {
        globals.ClipPlanes[slot][i] = static_cast<float>(normal[i]);
      }
      globals.ClipPlanes[slot][3] = static_cast<float>(-vtkMath::Dot(normal, origin));
      ++globals.NumberOfClipPlanes;
    }
  }

  if (this->GlobalUniformBuffer == nullptr)
  {
    this->GlobalUniformBuffer = configuration->CreateBuffer(sizeof(GlobalUniforms),
      wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, false, "VolumeGlobals");
    this->BindGroup = nullptr;
  }
  if (this->VolumeParamsBuffer == nullptr)
  {
    this->VolumeParamsBuffer = configuration->CreateBuffer(MaxVolumes * sizeof(VolumeParams),
      wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, false, "VolumeParams");
    this->BindGroup = nullptr;
  }

  configuration->WriteBuffer(
    this->GlobalUniformBuffer, 0, &globals, sizeof(globals), "VolumeGlobals");
  configuration->WriteBuffer(this->VolumeParamsBuffer, 0, parameters.data(),
    parameters.size() * sizeof(VolumeParams), "VolumeParams");
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::vtkInternals::UpdateDepthView(vtkWebGPURenderWindow* window)
{
  this->OpaqueDepthView = nullptr;
  this->DepthAvailable = false;

  auto depthTexture = window->GetDepthStencilTexture();
  if (depthTexture == nullptr)
  {
    return;
  }

  wgpu::TextureViewDescriptor descriptor;
  descriptor.label = "OpaqueDepth";
  descriptor.aspect = wgpu::TextureAspect::DepthOnly;
  descriptor.format = DepthOnlyFormat(window->GetDepthStencilFormat());
  descriptor.dimension = wgpu::TextureViewDimension::e2D;
  descriptor.baseMipLevel = 0;
  descriptor.mipLevelCount = 1;
  descriptor.baseArrayLayer = 0;
  descriptor.arrayLayerCount = 1;
  this->OpaqueDepthView = depthTexture.CreateView(&descriptor);
  this->DepthAvailable = this->OpaqueDepthView != nullptr;
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::vtkInternals::UpdateBindGroup(vtkWebGPURenderWindow* window)
{
  auto device = window->GetDevice();

  if (this->BindGroupLayout == nullptr)
  {
    std::vector<wgpu::BindGroupLayoutEntry> entries(8 + MaxVolumes);
    for (auto& entry : entries)
    {
      entry = {};
    }

    entries[0].binding = 0;
    entries[0].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

    entries[1].binding = 1;
    entries[1].visibility = wgpu::ShaderStage::Fragment;
    entries[1].buffer.type = wgpu::BufferBindingType::Uniform;

    entries[2].binding = 2;
    entries[2].visibility = wgpu::ShaderStage::Fragment;
    entries[2].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

    for (int i = 3; i <= 5; ++i)
    {
      entries[i].binding = static_cast<std::uint32_t>(i);
      entries[i].visibility = wgpu::ShaderStage::Fragment;
      entries[i].texture.sampleType = wgpu::TextureSampleType::Float;
      entries[i].texture.viewDimension = wgpu::TextureViewDimension::e2DArray;
    }

    entries[6].binding = 6;
    entries[6].visibility = wgpu::ShaderStage::Fragment;
    entries[6].sampler.type = wgpu::SamplerBindingType::Filtering;

    entries[7].binding = 7;
    entries[7].visibility = wgpu::ShaderStage::Fragment;
    entries[7].texture.sampleType = wgpu::TextureSampleType::Depth;
    entries[7].texture.viewDimension = wgpu::TextureViewDimension::e2D;

    for (int i = 0; i < MaxVolumes; ++i)
    {
      auto& entry = entries[8 + i];
      entry.binding = static_cast<std::uint32_t>(8 + i);
      entry.visibility = wgpu::ShaderStage::Fragment;
      entry.texture.sampleType = wgpu::TextureSampleType::UnfilterableFloat;
      entry.texture.viewDimension = wgpu::TextureViewDimension::e3D;
    }

    wgpu::BindGroupLayoutDescriptor descriptor;
    descriptor.label = "VolumeRayCastBindGroupLayout";
    descriptor.entryCount = static_cast<std::uint32_t>(entries.size());
    descriptor.entries = entries.data();
    this->BindGroupLayout = device.CreateBindGroupLayout(&descriptor);
  }

  if (!this->DepthAvailable)
  {
    // The depth binding cannot be left empty, so there is nothing to draw into either.
    this->BindGroup = nullptr;
    return;
  }

  std::vector<wgpu::BindGroupEntry> entries(8 + MaxVolumes);
  for (auto& entry : entries)
  {
    entry = {};
  }

  entries[0].binding = 0;
  entries[0].buffer = this->GlobalUniformBuffer;
  entries[0].size = sizeof(GlobalUniforms);

  entries[1].binding = 1;
  entries[1].buffer = this->VolumeParamsBuffer;
  entries[1].size = MaxVolumes * sizeof(VolumeParams);

  entries[2].binding = 2;
  entries[2].buffer = this->BrickBuffer;
  entries[2].size = this->BrickBufferElements * 2 * sizeof(float);

  entries[3].binding = 3;
  entries[3].textureView = this->ColorTransferFunctionsView;
  entries[4].binding = 4;
  entries[4].textureView = this->OpacityTransferFunctionsView;
  entries[5].binding = 5;
  entries[5].textureView = this->GradientTransferFunctionsView;

  entries[6].binding = 6;
  entries[6].sampler = this->TransferFunctionSampler;

  entries[7].binding = 7;
  entries[7].textureView = this->OpaqueDepthView;

  for (int i = 0; i < MaxVolumes; ++i)
  {
    entries[8 + i].binding = static_cast<std::uint32_t>(8 + i);
    entries[8 + i].textureView = this->PlaceholderVolumeView;
  }
  for (std::size_t index = 0; index < this->ActivePorts.size(); ++index)
  {
    const Input& input = this->InputCache[this->ActivePorts[index]];
    if (input.TextureView != nullptr)
    {
      entries[8 + index].textureView = input.TextureView;
    }
  }

  wgpu::BindGroupDescriptor descriptor;
  descriptor.label = "VolumeRayCastBindGroup";
  descriptor.layout = this->BindGroupLayout;
  descriptor.entryCount = static_cast<std::uint32_t>(entries.size());
  descriptor.entries = entries.data();
  this->BindGroup = device.CreateBindGroup(&descriptor);
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::vtkInternals::EnsurePipeline(vtkWebGPURenderWindow* window)
{
  const wgpu::TextureFormat colorFormat = window->GetPreferredSurfaceTextureFormat();
  if (this->Pipeline != nullptr && this->PipelineColorFormat == colorFormat)
  {
    return;
  }

  auto device = window->GetDevice();
  const std::string source = window->PreprocessShaderSource(VolumeRayCast);

  wgpu::ShaderSourceWGSL wgsl;
  wgsl.code = source.c_str();
  wgpu::ShaderModuleDescriptor moduleDescriptor;
  moduleDescriptor.label = "VolumeRayCast";
  moduleDescriptor.nextInChain = &wgsl;
  wgpu::ShaderModule shaderModule = device.CreateShaderModule(&moduleDescriptor);

  // The shader writes premultiplied color, so source over is a plain add.
  wgpu::BlendComponent blend;
  blend.srcFactor = wgpu::BlendFactor::One;
  blend.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
  blend.operation = wgpu::BlendOperation::Add;
  wgpu::BlendState blendState;
  blendState.color = blend;
  blendState.alpha = blend;

  wgpu::ColorTargetState colorTarget;
  colorTarget.format = colorFormat;
  colorTarget.blend = &blendState;
  colorTarget.writeMask = wgpu::ColorWriteMask::All;

  wgpu::FragmentState fragment;
  fragment.module = shaderModule;
  fragment.entryPoint = "fragmentMain";
  fragment.targetCount = 1;
  fragment.targets = &colorTarget;

  wgpu::PipelineLayoutDescriptor layoutDescriptor;
  layoutDescriptor.label = "VolumeRayCastPipelineLayout";
  layoutDescriptor.bindGroupLayoutCount = 1;
  layoutDescriptor.bindGroupLayouts = &this->BindGroupLayout;
  wgpu::PipelineLayout layout = device.CreatePipelineLayout(&layoutDescriptor);

  wgpu::RenderPipelineDescriptor descriptor;
  descriptor.label = "VolumeRayCastPipeline";
  descriptor.layout = layout;
  descriptor.vertex.module = shaderModule;
  descriptor.vertex.entryPoint = "vertexMain";
  descriptor.vertex.bufferCount = 0;
  descriptor.fragment = &fragment;
  descriptor.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
  // Keep the faces of the proxy box that point away from the camera so that a ray is cast once
  // per pixel even when the camera sits inside the box.
  descriptor.primitive.frontFace = wgpu::FrontFace::CCW;
  descriptor.primitive.cullMode = wgpu::CullMode::Back;
  descriptor.multisample.count = 1;
  descriptor.multisample.mask = 0xFFFFFFFF;

  this->Pipeline = device.CreateRenderPipeline(&descriptor);
  this->PipelineColorFormat = colorFormat;
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::vtkInternals::RecordRenderPass(vtkWebGPURenderWindow* window)
{
  if (this->Pipeline == nullptr || this->BindGroup == nullptr)
  {
    return;
  }

  wgpu::RenderPassColorAttachment attachment;
  attachment.view = window->GetOffscreenColorAttachmentView();
  attachment.loadOp = wgpu::LoadOp::Load;
  attachment.storeOp = wgpu::StoreOp::Store;

  wgpu::RenderPassDescriptor descriptor;
  descriptor.label = "vtkWebGPUVolumeRayCastMapper";
  descriptor.colorAttachmentCount = 1;
  descriptor.colorAttachments = &attachment;
  descriptor.depthStencilAttachment = nullptr;

  wgpu::RenderPassEncoder pass = window->NewRenderPass(descriptor);
  if (pass == nullptr)
  {
    return;
  }

  pass.SetViewport(this->ViewportRectangle[0], this->ViewportRectangle[1],
    this->ViewportRectangle[2], this->ViewportRectangle[3], 0.0f, 1.0f);
  pass.SetScissorRect(static_cast<std::uint32_t>(this->ViewportRectangle[0]),
    static_cast<std::uint32_t>(this->ViewportRectangle[1]),
    static_cast<std::uint32_t>(this->ViewportRectangle[2]),
    static_cast<std::uint32_t>(this->ViewportRectangle[3]));
  pass.SetPipeline(this->Pipeline);
  pass.SetBindGroup(0, this->BindGroup);
  pass.Draw(this->DrawFullScreen ? 3 : 36);
  pass.End();
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::vtkInternals::Render(
  vtkWebGPURenderWindow* window, vtkRenderer* renderer, vtkVolume* volume)
{
  if (this->ActivePorts.empty() || this->ColorTransferFunctions == nullptr)
  {
    return;
  }

  this->UpdateDepthView(window);
  this->UpdateUniforms(window, renderer, volume);
  this->UpdateBindGroup(window);
  this->EnsurePipeline(window);
  this->RecordRenderPass(window);
}

VTK_ABI_NAMESPACE_END
