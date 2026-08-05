// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPUVolumeRayCastMapperInternals.h"

#include "Private/vtkWebGPUVolumeRayCastCoordinatorInternals.h"

#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkMultiVolume.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkWebGPUConfiguration.h"
#include "vtkWebGPURenderWindow.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
constexpr int TableSize = vtkWebGPUVolumeTransferFunctionSize;

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
} // anonymous namespace

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::vtkInternals::ReleaseGraphicsResources()
{
  for (auto& entry : this->InputCache)
  {
    entry.second.Texture = nullptr;
    entry.second.TextureView = nullptr;
    entry.second.DataTime = 0;
    entry.second.TransferFunctionTime = 0;
  }
}

//------------------------------------------------------------------------------
bool vtkWebGPUVolumeRayCastMapper::vtkInternals::CollectInputs(vtkVolume* volume)
{
  this->ActivePorts.clear();

  auto* multiVolume = vtkMultiVolume::SafeDownCast(volume);
  const std::vector<int> ports = this->Self->GetPorts();

  for (int port : ports)
  {
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

  // Let go of the state of ports that are no longer connected.
  for (auto it = this->InputCache.begin(); it != this->InputCache.end();)
  {
    const bool active = std::find(this->ActivePorts.begin(), this->ActivePorts.end(), it->first) !=
      this->ActivePorts.end();
    it = active ? std::next(it) : this->InputCache.erase(it);
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
  input.TransferFunctionTime = 0;
  input.ContentTime.Modified();
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::vtkInternals::UpdateTransferFunctions(Input& input)
{
  vtkVolumeProperty* property = input.Property;
  if (!input.ColorTable.empty() && input.TransferFunctionTime >= property->GetMTime())
  {
    return;
  }

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

  input.ColorTable.assign(TableSize * 4, 0);
  input.OpacityTable.assign(TableSize * 4, 0);
  input.GradientTable.assign(TableSize * 4, 0);

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
    input.ColorTable[i * 4 + 0] = PackHalf(static_cast<float>(rgbValue[0]));
    input.ColorTable[i * 4 + 1] = PackHalf(static_cast<float>(rgbValue[1]));
    input.ColorTable[i * 4 + 2] = PackHalf(static_cast<float>(rgbValue[2]));
    input.ColorTable[i * 4 + 3] = PackHalf(1.0f);

    const float alpha =
      scalarOpacity != nullptr ? static_cast<float>(scalarOpacity->GetValue(value)) : 1.0f;
    input.OpacityCurve[i] = std::clamp(alpha, 0.0f, 1.0f);
    input.OpacityTable[i * 4 + 0] = PackHalf(alpha);
    input.OpacityTable[i * 4 + 3] = PackHalf(1.0f);
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
    input.GradientTable[i * 4 + 0] = PackHalf(value);
    input.GradientTable[i * 4 + 3] = PackHalf(1.0f);
  }

  input.TransferFunctionTime = property->GetMTime();
  input.BrickClassification.clear();
  input.ContentTime.Modified();
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::vtkInternals::UpdateBrickClassification(Input& input)
{
  if (!input.BrickClassification.empty() || input.BrickMinimum.empty())
  {
    return;
  }

  input.BrickClassification.assign(input.BrickMinimum.size() * 2, 0.0f);

  const RangeTable table(input.OpacityCurve);
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
    const float detail = std::clamp(
      sensitivity * std::max(static_cast<float>(high - low), opacityHigh - opacityLow), 0.0f, 1.0f);

    input.BrickClassification[brick * 2] = opacityHigh;
    input.BrickClassification[brick * 2 + 1] = detail;
  }

  input.ContentTime.Modified();
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::vtkInternals::SyncDeviceResources(
  vtkWebGPURenderWindow* window, vtkWebGPUVolumeRayCastCoordinator* coordinator, vtkVolume* volume)
{
  if (coordinator == nullptr || !this->CollectInputs(volume))
  {
    return;
  }

  for (int port : this->ActivePorts)
  {
    Input& input = this->InputCache[port];
    this->UpdateVolumeTexture(window, input);
    if (input.TextureView == nullptr)
    {
      continue;
    }
    this->UpdateTransferFunctions(input);
    this->UpdateBrickClassification(input);

    vtkWebGPUVolumeRayCastContribution contribution;
    contribution.Mapper = this->Self;
    contribution.Volume = volume;
    contribution.Port = port;
    contribution.Data = input.Data;
    contribution.Property = input.Property;
    contribution.WorldMatrix = input.WorldMatrix;
    contribution.ClippingPlanes = this->Self->GetClippingPlanes();
    contribution.VolumeTexture = input.TextureView;
    std::copy_n(input.Dimensions, 3, contribution.Dimensions);
    contribution.ValueScale = input.ValueScale;
    contribution.ValueBias = input.ValueBias;
    std::copy_n(input.ScalarRange, 2, contribution.ScalarRange);
    std::copy_n(input.GradientRange, 2, contribution.GradientRange);
    contribution.BrickSize = input.BrickSize;
    std::copy_n(input.BrickDimensions, 3, contribution.BrickDimensions);
    contribution.BrickClassification = &input.BrickClassification;
    contribution.ColorTable = &input.ColorTable;
    contribution.OpacityTable = &input.OpacityTable;
    contribution.GradientTable = &input.GradientTable;
    contribution.ContentTime = input.ContentTime.GetMTime();

    if (!coordinator->GetInternals()->Offer(contribution))
    {
      vtkWarningWithObjectMacro(
        this->Self, << "Only " << vtkWebGPUVolumeRayCastCoordinator::MaximumNumberOfVolumes
                    << " volumes can be ray cast together, the rest are not drawn.");
      break;
    }
  }
}

VTK_ABI_NAMESPACE_END
