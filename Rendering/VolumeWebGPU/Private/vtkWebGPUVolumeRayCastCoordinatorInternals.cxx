// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPUVolumeRayCastCoordinatorInternals.h"

#include "Private/vtkWebGPUHelpersPrivate.h"

#include "vtkWebGPUVolumeRayCastMapper.h"

#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPlaneCollection.h"
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

VTK_ABI_NAMESPACE_BEGIN

namespace
{
using Internals = vtkWebGPUVolumeRayCastCoordinator::vtkInternals;
constexpr int MaxVolumes = Internals::MaximumVolumes;
constexpr int MaxLights = Internals::MaximumLights;
constexpr int MaxClipPlanes = Internals::MaximumClipPlanes;
constexpr int TableSize = Internals::TableSize;

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
  float AverageRangeMinimum;
  float AverageRangeMaximum;
  float LightDirections[MaxLights][4];
  float LightColors[MaxLights][4];
};
static_assert(sizeof(GlobalUniforms) == 416, "GlobalUniforms must match the WGSL layout");

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
  std::uint32_t NumberOfClipPlanes;
  float Padding0[3];
  float ClipPlanes[MaxClipPlanes][4];
};
static_assert(sizeof(VolumeParams) == 336, "VolumeParams must match the WGSL layout");

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
WGPUTextureFormat DepthOnlyFormat(WGPUTextureFormat format)
{
  switch (format)
  {
    case WGPUTextureFormat_Depth24PlusStencil8:
      return WGPUTextureFormat_Depth24Plus;
    case WGPUTextureFormat_Depth32FloatStencil8:
      return WGPUTextureFormat_Depth32Float;
    default:
      return format;
  }
}
} // anonymous namespace

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastCoordinator::vtkInternals::BeginFrame()
{
  this->Contributions.clear();
  this->Rendered = false;
}

//------------------------------------------------------------------------------
bool vtkWebGPUVolumeRayCastCoordinator::vtkInternals::Offer(
  const vtkWebGPUVolumeRayCastContribution& contribution)
{
  const bool alreadyHere = std::any_of(this->Contributions.begin(), this->Contributions.end(),
    [&contribution](const vtkWebGPUVolumeRayCastContribution& other)
    { return other.Mapper == contribution.Mapper && other.Port == contribution.Port; });
  if (this->Rendered || alreadyHere)
  {
    this->BeginFrame();
  }

  if (static_cast<int>(this->Contributions.size()) >= MaxVolumes)
  {
    return false;
  }
  this->Contributions.push_back(contribution);
  return true;
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastCoordinator::vtkInternals::Forget(vtkWebGPUVolumeRayCastMapper* mapper)
{
  this->Contributions.erase(std::remove_if(this->Contributions.begin(), this->Contributions.end(),
                              [mapper](const vtkWebGPUVolumeRayCastContribution& contribution)
                              { return contribution.Mapper == mapper; }),
    this->Contributions.end());
  // Whatever was uploaded for that mapper is gone with it.
  this->Layers.clear();
  this->BrickLayers.clear();
  this->BindGroup = nullptr;
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastCoordinator::vtkInternals::ReleaseGraphicsResources()
{
  this->Contributions.clear();
  this->ReleaseDeviceResources();
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastCoordinator::vtkInternals::ReleaseDeviceResources()
{
  this->Layers.clear();
  this->BrickLayers.clear();
  this->BrickOffsets.clear();
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
  this->OpaqueDepthView = nullptr;
  this->BindGroupLayout = nullptr;
  this->BindGroup = nullptr;
  this->Pipeline = nullptr;
  this->PipelineColorFormat = WGPUTextureFormat_Undefined;
  this->DeviceIdentity = nullptr;
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastCoordinator::vtkInternals::EnsurePlaceholders(
  vtkWebGPURenderWindow* window)
{
  if (this->PlaceholderVolume != nullptr)
  {
    return;
  }
  auto configuration = window->GetWGPUConfiguration();

  WGPUTextureDescriptor descriptor = WGPU_TEXTURE_DESCRIPTOR_INIT;
  descriptor.label = WGPUStringView{ "PlaceholderVolume", WGPU_STRLEN };
  descriptor.dimension = WGPUTextureDimension_3D;
  descriptor.size = { 1, 1, 1 };
  descriptor.format = WGPUTextureFormat_R32Float;
  descriptor.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
  descriptor.mipLevelCount = 1;
  descriptor.sampleCount = 1;
  this->PlaceholderVolume = vtkWebGPU::Texture::Acquire(configuration->CreateTexture(descriptor));

  const float zero = 0.0f;
  configuration->WriteTexture(
    this->PlaceholderVolume, 4, 4, &zero, 0, { 0, 0, 0 }, 0, "PlaceholderVolume");

  WGPUTextureViewDescriptor viewDescriptor = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
  viewDescriptor.label = WGPUStringView{ "PlaceholderVolume", WGPU_STRLEN };
  viewDescriptor.format = WGPUTextureFormat_R32Float;
  viewDescriptor.dimension = WGPUTextureViewDimension_3D;
  viewDescriptor.baseMipLevel = 0;
  viewDescriptor.mipLevelCount = 1;
  viewDescriptor.baseArrayLayer = 0;
  viewDescriptor.arrayLayerCount = 1;
  this->PlaceholderVolumeView = vtkWebGPU::TextureView::Acquire(
    configuration->CreateView(this->PlaceholderVolume, viewDescriptor));

  if (this->TransferFunctionSampler == nullptr)
  {
    WGPUSamplerDescriptor samplerDescriptor = WGPU_SAMPLER_DESCRIPTOR_INIT;
    samplerDescriptor.label = WGPUStringView{ "TransferFunctionSampler", WGPU_STRLEN };
    samplerDescriptor.addressModeU = WGPUAddressMode_ClampToEdge;
    samplerDescriptor.addressModeV = WGPUAddressMode_ClampToEdge;
    samplerDescriptor.addressModeW = WGPUAddressMode_ClampToEdge;
    samplerDescriptor.magFilter = WGPUFilterMode_Linear;
    samplerDescriptor.minFilter = WGPUFilterMode_Linear;
    this->TransferFunctionSampler =
      vtkWebGPU::Sampler::Acquire(wgpuDeviceCreateSampler(window->GetDevice(), &samplerDescriptor));
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastCoordinator::vtkInternals::UpdateTransferFunctions(
  vtkWebGPURenderWindow* window)
{
  auto configuration = window->GetWGPUConfiguration();

  if (this->ColorTransferFunctions == nullptr)
  {
    const auto makeArray = [&](const char* label)
    {
      WGPUTextureDescriptor descriptor = WGPU_TEXTURE_DESCRIPTOR_INIT;
      descriptor.label = WGPUStringView{ label, WGPU_STRLEN };
      descriptor.dimension = WGPUTextureDimension_2D;
      descriptor.size = { TableSize, 1, MaxVolumes };
      descriptor.format = WGPUTextureFormat_RGBA16Float;
      descriptor.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
      descriptor.mipLevelCount = 1;
      descriptor.sampleCount = 1;
      return vtkWebGPU::Texture::Acquire(configuration->CreateTexture(descriptor));
    };
    this->ColorTransferFunctions = makeArray("ColorTransferFunctions");
    this->OpacityTransferFunctions = makeArray("OpacityTransferFunctions");
    this->GradientTransferFunctions = makeArray("GradientTransferFunctions");

    const auto makeView = [&](vtkWebGPU::Texture& texture, const char* label)
    {
      WGPUTextureViewDescriptor descriptor = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
      descriptor.label = WGPUStringView{ label, WGPU_STRLEN };
      descriptor.format = WGPUTextureFormat_RGBA16Float;
      descriptor.dimension = WGPUTextureViewDimension_2DArray;
      descriptor.baseMipLevel = 0;
      descriptor.mipLevelCount = 1;
      descriptor.baseArrayLayer = 0;
      descriptor.arrayLayerCount = MaxVolumes;
      return vtkWebGPU::TextureView::Acquire(configuration->CreateView(texture, descriptor));
    };
    this->ColorTransferFunctionsView = makeView(this->ColorTransferFunctions, "ColorTF");
    this->OpacityTransferFunctionsView = makeView(this->OpacityTransferFunctions, "OpacityTF");
    this->GradientTransferFunctionsView = makeView(this->GradientTransferFunctions, "GradientTF");

    this->Layers.clear();
    this->BindGroup = nullptr;
  }

  this->Layers.resize(MaxVolumes);

  const std::uint32_t bytesPerRow = TableSize * 4 * sizeof(std::uint16_t);
  for (std::size_t index = 0; index < this->Contributions.size(); ++index)
  {
    const auto& contribution = this->Contributions[index];
    LayerState& layer = this->Layers[index];
    if (layer.Mapper == contribution.Mapper && layer.Port == contribution.Port &&
      layer.ContentTime == contribution.ContentTime)
    {
      continue;
    }

    const WGPUOrigin3D destination = { 0, 0, static_cast<std::uint32_t>(index) };
    configuration->WriteTexture(this->ColorTransferFunctions, bytesPerRow, bytesPerRow,
      contribution.ColorTable->data(), 0, destination, 0, "Color transfer function");
    configuration->WriteTexture(this->OpacityTransferFunctions, bytesPerRow, bytesPerRow,
      contribution.OpacityTable->data(), 0, destination, 0, "Opacity transfer function");
    configuration->WriteTexture(this->GradientTransferFunctions, bytesPerRow, bytesPerRow,
      contribution.GradientTable->data(), 0, destination, 0, "Gradient opacity transfer function");

    layer.Mapper = contribution.Mapper;
    layer.Port = contribution.Port;
    layer.ContentTime = contribution.ContentTime;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastCoordinator::vtkInternals::UpdateBricks(vtkWebGPURenderWindow* window)
{
  this->BrickOffsets.assign(this->Contributions.size(), 0);

  std::size_t total = 0;
  for (std::size_t index = 0; index < this->Contributions.size(); ++index)
  {
    this->BrickOffsets[index] = static_cast<std::uint32_t>(total);
    const auto* blocks = this->Contributions[index].BrickClassification;
    total += blocks != nullptr ? blocks->size() / 2 : 0;
  }
  total = std::max<std::size_t>(total, 1);

  // Nothing changed hands and nothing was rebuilt, so the buffer still holds what it should.
  bool stale = this->BrickBuffer == nullptr || this->BrickBufferElements != total ||
    this->BrickLayers.size() != this->Contributions.size();
  for (std::size_t index = 0; !stale && index < this->Contributions.size(); ++index)
  {
    const auto& contribution = this->Contributions[index];
    const LayerState& layer = this->BrickLayers[index];
    stale = layer.Mapper != contribution.Mapper || layer.Port != contribution.Port ||
      layer.ContentTime != contribution.ContentTime;
  }
  if (!stale)
  {
    return;
  }

  std::vector<float> classification(total * 2, 0.0f);
  // A single opaque, featureless entry keeps a pass with no blocks from skipping everything.
  classification[0] = 1.0f;
  classification[1] = 1.0f;

  this->BrickLayers.resize(this->Contributions.size());
  for (std::size_t index = 0; index < this->Contributions.size(); ++index)
  {
    const auto& contribution = this->Contributions[index];
    if (contribution.BrickClassification != nullptr && !contribution.BrickClassification->empty())
    {
      std::copy(contribution.BrickClassification->begin(), contribution.BrickClassification->end(),
        classification.begin() + 2 * this->BrickOffsets[index]);
    }
    this->BrickLayers[index] = { contribution.Mapper, contribution.Port, contribution.ContentTime };
  }

  auto configuration = window->GetWGPUConfiguration();
  const std::size_t bytes = classification.size() * sizeof(float);
  if (this->BrickBuffer == nullptr || this->BrickBufferElements != total)
  {
    this->BrickBuffer = vtkWebGPU::Buffer::Acquire(configuration->CreateBuffer(
      bytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, false, "VolumeBlocks"));
    this->BrickBufferElements = total;
    this->BindGroup = nullptr;
  }
  configuration->WriteBuffer(this->BrickBuffer, 0, classification.data(), bytes, "VolumeBlocks");
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastCoordinator::vtkInternals::UpdateDepthView(vtkWebGPURenderWindow* window)
{
  this->OpaqueDepthView = nullptr;
  this->DepthAvailable = false;

  auto depthTexture = window->GetDepthStencilTexture();
  if (depthTexture == nullptr)
  {
    return;
  }

  WGPUTextureViewDescriptor descriptor = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
  descriptor.label = WGPUStringView{ "OpaqueDepth", WGPU_STRLEN };
  descriptor.aspect = WGPUTextureAspect_DepthOnly;
  descriptor.format = DepthOnlyFormat(window->GetDepthStencilFormat());
  descriptor.dimension = WGPUTextureViewDimension_2D;
  descriptor.baseMipLevel = 0;
  descriptor.mipLevelCount = 1;
  descriptor.baseArrayLayer = 0;
  descriptor.arrayLayerCount = 1;
  this->OpaqueDepthView = vtkWebGPU::TextureView::Acquire(
    window->GetWGPUConfiguration()->CreateView(depthTexture, descriptor));
  this->DepthAvailable = this->OpaqueDepthView != nullptr;
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastCoordinator::vtkInternals::UpdateUniforms(
  vtkWebGPURenderWindow* window, vtkRenderer* renderer)
{
  auto configuration = window->GetWGPUConfiguration();

  // Settings that describe the march rather than one volume are taken from the mapper of the
  // first input; a pass can only be marched one way.
  vtkWebGPUVolumeRayCastMapper* primary = this->Contributions.front().Mapper;
  const int blendMode = primary->GetBlendMode();
  for (const auto& contribution : this->Contributions)
  {
    if (contribution.Mapper->GetBlendMode() != blendMode)
    {
      vtkWarningWithObjectMacro(this->Self,
        << "Volumes that share a renderer are ray cast together and can only be blended one way. "
           "Using the blend mode of "
        << primary->GetObjectDescription() << " for all of them.");
      break;
    }
  }

  double bounds[6] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN, VTK_DOUBLE_MAX, VTK_DOUBLE_MIN,
    VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };

  std::vector<VolumeParams> parameters(MaxVolumes);
  std::memset(parameters.data(), 0, parameters.size() * sizeof(VolumeParams));

  bool interactive = false;
  double smallestStep = VTK_DOUBLE_MAX;

  for (std::size_t index = 0; index < this->Contributions.size(); ++index)
  {
    const auto& contribution = this->Contributions[index];
    VolumeParams& params = parameters[index];
    vtkVolumeProperty* property = contribution.Property;
    vtkWebGPUVolumeRayCastMapper* mapper = contribution.Mapper;

    interactive = interactive || contribution.Volume->GetAllocatedRenderTime() < 1.0;

    double extent[3];
    for (int i = 0; i < 3; ++i)
    {
      extent[i] = std::max(contribution.Dimensions[i] - 1, 1);
    }

    vtkNew<vtkMatrix4x4> textureToIndex;
    textureToIndex->Identity();
    for (int i = 0; i < 3; ++i)
    {
      textureToIndex->SetElement(i, i, extent[i]);
    }

    vtkNew<vtkMatrix4x4> indexToWorld;
    vtkMatrix4x4::Multiply4x4(
      contribution.WorldMatrix, contribution.Data->GetIndexToPhysicalMatrix(), indexToWorld);

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
    if (mapper->GetAutoAdjustSampleDistances() == 0 && mapper->GetSampleDistance() > 0.0f)
    {
      step = mapper->GetSampleDistance();
    }
    params.BaseStep = static_cast<float>(step);
    smallestStep = std::min(smallestStep, step);

    for (int i = 0; i < 3; ++i)
    {
      params.Dimensions[i] = static_cast<float>(contribution.Dimensions[i]);
      params.BrickDimensions[i] = static_cast<float>(contribution.BrickDimensions[i]);
    }
    params.ValueScale = contribution.ValueScale;
    params.ValueBias = contribution.ValueBias;
    params.ScalarRangeMinimum = static_cast<float>(contribution.ScalarRange[0]);
    params.ScalarRangeScale =
      static_cast<float>(1.0 / (contribution.ScalarRange[1] - contribution.ScalarRange[0]));
    params.BrickOffset = this->BrickOffsets[index];
    params.BrickSize = static_cast<float>(std::max(contribution.BrickSize, 1));
    params.OpacityUnitDistance =
      static_cast<float>(std::max(property->GetScalarOpacityUnitDistance(0), 1.0e-6));
    params.Ambient = static_cast<float>(property->GetAmbient(0));
    params.Diffuse = static_cast<float>(property->GetDiffuse(0));
    params.Specular = static_cast<float>(property->GetSpecular(0));
    params.SpecularPower = static_cast<float>(property->GetSpecularPower(0));

    const double gradientExtent = contribution.GradientRange[1] - contribution.GradientRange[0];
    params.GradientOpacityMinimum = static_cast<float>(contribution.GradientRange[0]);
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

    // Clipping planes belong to the mapper, so each input is clipped by its own.
    params.NumberOfClipPlanes = 0;
    if (vtkPlaneCollection* planes = contribution.ClippingPlanes)
    {
      vtkCollectionSimpleIterator iterator;
      planes->InitTraversal(iterator);
      while (vtkPlane* plane = planes->GetNextPlane(iterator))
      {
        if (params.NumberOfClipPlanes >= static_cast<std::uint32_t>(MaxClipPlanes))
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
        const std::uint32_t slot = params.NumberOfClipPlanes;
        for (int i = 0; i < 3; ++i)
        {
          params.ClipPlanes[slot][i] = static_cast<float>(normal[i]);
        }
        params.ClipPlanes[slot][3] = static_cast<float>(-vtkMath::Dot(normal, origin));
        ++params.NumberOfClipPlanes;
      }
    }
  }

  const double diagonal = std::sqrt(std::pow(bounds[1] - bounds[0], 2.0) +
    std::pow(bounds[3] - bounds[2], 2.0) + std::pow(bounds[5] - bounds[4], 2.0));

  // While the scene is being interacted with, cap how many samples a ray may take by spreading
  // them over the whole box instead of refusing to finish the frame.
  if (interactive && primary->GetAutoAdjustSampleDistances() != 0)
  {
    const double coarse = diagonal / primary->GetInteractiveMaximumNumberOfSamples();
    if (coarse > smallestStep)
    {
      const double factor = coarse / smallestStep;
      for (std::size_t index = 0; index < this->Contributions.size(); ++index)
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

  globals.NumberOfVolumes = static_cast<std::uint32_t>(this->Contributions.size());
  globals.JitterScale = static_cast<float>(smallestStep);
  globals.EmptyThreshold = static_cast<float>(primary->GetEmptySpaceOpacityThreshold());
  globals.BlendMode = static_cast<std::uint32_t>(blendMode);
  globals.MinimumStepFactor = static_cast<float>(primary->GetMinimumSampleDistanceFactor());
  globals.MaximumStepFactor = static_cast<float>(
    std::max(primary->GetMaximumSampleDistanceFactor(), primary->GetMinimumSampleDistanceFactor()));
  globals.MaximumSteps = static_cast<std::uint32_t>(
    std::clamp(2.5 * diagonal / std::max(smallestStep, 1.0e-9) + 32.0, 64.0, 10000.0));

  double averageRange[2];
  primary->GetAverageIPScalarRange(averageRange);
  globals.AverageRangeMinimum = static_cast<float>(averageRange[0]);
  globals.AverageRangeMaximum = static_cast<float>(averageRange[1]);

  globals.Flags = 0;
  if (primary->GetJittering())
  {
    globals.Flags |= FlagJitter;
  }
  // The intensity blend modes weigh every sample equally, so they only mean what they say when
  // the samples are evenly spaced.
  if (primary->GetAdaptiveSampling() && blendMode == vtkVolumeMapper::COMPOSITE_BLEND)
  {
    globals.Flags |= FlagAdaptive;
    if (primary->GetEmptySpaceSkipping())
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

  if (this->GlobalUniformBuffer == nullptr)
  {
    this->GlobalUniformBuffer =
      vtkWebGPU::Buffer::Acquire(configuration->CreateBuffer(sizeof(GlobalUniforms),
        WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst, false, "VolumeGlobals"));
    this->BindGroup = nullptr;
  }
  if (this->VolumeParamsBuffer == nullptr)
  {
    this->VolumeParamsBuffer =
      vtkWebGPU::Buffer::Acquire(configuration->CreateBuffer(MaxVolumes * sizeof(VolumeParams),
        WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst, false, "VolumeParams"));
    this->BindGroup = nullptr;
  }

  configuration->WriteBuffer(
    this->GlobalUniformBuffer, 0, &globals, sizeof(globals), "VolumeGlobals");
  configuration->WriteBuffer(this->VolumeParamsBuffer, 0, parameters.data(),
    parameters.size() * sizeof(VolumeParams), "VolumeParams");
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastCoordinator::vtkInternals::UpdateBindGroup(vtkWebGPURenderWindow* window)
{
  auto device = window->GetDevice();

  if (this->BindGroupLayout == nullptr)
  {
    std::vector<WGPUBindGroupLayoutEntry> entries(
      8 + MaxVolumes, WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT);

    entries[0].binding = 0;
    entries[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    entries[0].buffer.type = WGPUBufferBindingType_Uniform;

    entries[1].binding = 1;
    entries[1].visibility = WGPUShaderStage_Fragment;
    entries[1].buffer.type = WGPUBufferBindingType_Uniform;

    entries[2].binding = 2;
    entries[2].visibility = WGPUShaderStage_Fragment;
    entries[2].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;

    for (int i = 3; i <= 5; ++i)
    {
      entries[i].binding = static_cast<std::uint32_t>(i);
      entries[i].visibility = WGPUShaderStage_Fragment;
      entries[i].texture.sampleType = WGPUTextureSampleType_Float;
      entries[i].texture.viewDimension = WGPUTextureViewDimension_2DArray;
    }

    entries[6].binding = 6;
    entries[6].visibility = WGPUShaderStage_Fragment;
    entries[6].sampler.type = WGPUSamplerBindingType_Filtering;

    entries[7].binding = 7;
    entries[7].visibility = WGPUShaderStage_Fragment;
    entries[7].texture.sampleType = WGPUTextureSampleType_Depth;
    entries[7].texture.viewDimension = WGPUTextureViewDimension_2D;

    for (int i = 0; i < MaxVolumes; ++i)
    {
      auto& entry = entries[8 + i];
      entry.binding = static_cast<std::uint32_t>(8 + i);
      entry.visibility = WGPUShaderStage_Fragment;
      entry.texture.sampleType = WGPUTextureSampleType_UnfilterableFloat;
      entry.texture.viewDimension = WGPUTextureViewDimension_3D;
    }

    WGPUBindGroupLayoutDescriptor descriptor = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
    descriptor.label = WGPUStringView{ "VolumeRayCastBindGroupLayout", WGPU_STRLEN };
    descriptor.entryCount = static_cast<std::uint32_t>(entries.size());
    descriptor.entries = entries.data();
    this->BindGroupLayout =
      vtkWebGPU::BindGroupLayout::Acquire(wgpuDeviceCreateBindGroupLayout(device, &descriptor));
  }

  if (!this->DepthAvailable)
  {
    // The depth binding cannot be left empty, so there is nothing to draw into either.
    this->BindGroup = nullptr;
    return;
  }

  std::vector<WGPUBindGroupEntry> entries(8 + MaxVolumes, WGPU_BIND_GROUP_ENTRY_INIT);

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
  for (std::size_t index = 0; index < this->Contributions.size(); ++index)
  {
    if (this->Contributions[index].VolumeTexture != nullptr)
    {
      entries[8 + index].textureView = this->Contributions[index].VolumeTexture;
    }
  }

  WGPUBindGroupDescriptor descriptor = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
  descriptor.label = WGPUStringView{ "VolumeRayCastBindGroup", WGPU_STRLEN };
  descriptor.layout = this->BindGroupLayout;
  descriptor.entryCount = static_cast<std::uint32_t>(entries.size());
  descriptor.entries = entries.data();
  this->BindGroup = vtkWebGPU::BindGroup::Acquire(wgpuDeviceCreateBindGroup(device, &descriptor));
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastCoordinator::vtkInternals::EnsurePipeline(vtkWebGPURenderWindow* window)
{
  const WGPUTextureFormat colorFormat = window->GetPreferredSurfaceTextureFormat();
  if (this->Pipeline != nullptr && this->PipelineColorFormat == colorFormat)
  {
    return;
  }

  auto device = window->GetDevice();
  const std::string source = window->PreprocessShaderSource(VolumeRayCast);

  WGPUShaderSourceWGSL wgsl = WGPU_SHADER_SOURCE_WGSL_INIT;
  wgsl.code = vtkWebGPUMakeStringView(source);
  WGPUShaderModuleDescriptor moduleDescriptor = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
  moduleDescriptor.label = WGPUStringView{ "VolumeRayCast", WGPU_STRLEN };
  moduleDescriptor.nextInChain = &wgsl.chain;
  auto shaderModule =
    vtkWebGPU::ShaderModule::Acquire(wgpuDeviceCreateShaderModule(device, &moduleDescriptor));

  // The shader writes premultiplied color, so source over is a plain add.
  WGPUBlendComponent blend = WGPU_BLEND_COMPONENT_INIT;
  blend.srcFactor = WGPUBlendFactor_One;
  blend.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
  blend.operation = WGPUBlendOperation_Add;
  WGPUBlendState blendState = WGPU_BLEND_STATE_INIT;
  blendState.color = blend;
  blendState.alpha = blend;

  WGPUColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_INIT;
  colorTarget.format = colorFormat;
  colorTarget.blend = &blendState;
  colorTarget.writeMask = WGPUColorWriteMask_All;

  WGPUFragmentState fragment = WGPU_FRAGMENT_STATE_INIT;
  fragment.module = shaderModule;
  fragment.entryPoint = WGPUStringView{ "fragmentMain", WGPU_STRLEN };
  fragment.targetCount = 1;
  fragment.targets = &colorTarget;

  WGPUPipelineLayoutDescriptor layoutDescriptor = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
  layoutDescriptor.label = WGPUStringView{ "VolumeRayCastPipelineLayout", WGPU_STRLEN };
  layoutDescriptor.bindGroupLayoutCount = 1;
  const WGPUBindGroupLayout bindGroupLayout = this->BindGroupLayout;
  layoutDescriptor.bindGroupLayouts = &bindGroupLayout;
  auto layout =
    vtkWebGPU::PipelineLayout::Acquire(wgpuDeviceCreatePipelineLayout(device, &layoutDescriptor));

  WGPURenderPipelineDescriptor descriptor = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
  descriptor.label = WGPUStringView{ "VolumeRayCastPipeline", WGPU_STRLEN };
  descriptor.layout = layout;
  descriptor.vertex.module = shaderModule;
  descriptor.vertex.entryPoint = WGPUStringView{ "vertexMain", WGPU_STRLEN };
  descriptor.vertex.bufferCount = 0;
  descriptor.fragment = &fragment;
  descriptor.primitive.topology = WGPUPrimitiveTopology_TriangleList;
  // Keep the faces of the proxy box that point away from the camera so that a ray is cast once
  // per pixel even when the camera sits inside the box.
  descriptor.primitive.frontFace = WGPUFrontFace_CCW;
  descriptor.primitive.cullMode = WGPUCullMode_Back;
  descriptor.multisample.count = 1;
  descriptor.multisample.mask = 0xFFFFFFFF;

  this->Pipeline =
    vtkWebGPU::RenderPipeline::Acquire(wgpuDeviceCreateRenderPipeline(device, &descriptor));
  this->PipelineColorFormat = colorFormat;
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastCoordinator::vtkInternals::RecordRenderPass(
  vtkWebGPURenderWindow* window)
{
  if (this->Pipeline == nullptr || this->BindGroup == nullptr)
  {
    return;
  }

  WGPURenderPassColorAttachment attachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
  attachment.view = window->GetOffscreenColorAttachmentView();
  attachment.loadOp = WGPULoadOp_Load;
  attachment.storeOp = WGPUStoreOp_Store;

  WGPURenderPassDescriptor descriptor = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
  descriptor.label = WGPUStringView{ "vtkWebGPUVolumeRayCastCoordinator", WGPU_STRLEN };
  descriptor.colorAttachmentCount = 1;
  descriptor.colorAttachments = &attachment;
  descriptor.depthStencilAttachment = nullptr;

  auto pass = vtkWebGPU::RenderPassEncoder::Acquire(window->NewRenderPass(descriptor));
  if (pass == nullptr)
  {
    return;
  }

  wgpuRenderPassEncoderSetViewport(pass, this->ViewportRectangle[0], this->ViewportRectangle[1],
    this->ViewportRectangle[2], this->ViewportRectangle[3], 0.0f, 1.0f);
  wgpuRenderPassEncoderSetScissorRect(pass, static_cast<std::uint32_t>(this->ViewportRectangle[0]),
    static_cast<std::uint32_t>(this->ViewportRectangle[1]),
    static_cast<std::uint32_t>(this->ViewportRectangle[2]),
    static_cast<std::uint32_t>(this->ViewportRectangle[3]));
  wgpuRenderPassEncoderSetPipeline(pass, this->Pipeline);
  wgpuRenderPassEncoderSetBindGroup(pass, 0, this->BindGroup, 0, nullptr);
  wgpuRenderPassEncoderDraw(pass, this->DrawFullScreen ? 3 : 36, 1, 0, 0);
  wgpuRenderPassEncoderEnd(pass);
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastCoordinator::vtkInternals::Render(
  vtkWebGPURenderWindow* window, vtkRenderer* renderer)
{
  if (this->Rendered || this->Contributions.empty())
  {
    return;
  }
  this->Rendered = true;

  // A renderer can be moved to another render window, and nothing built on the old device may be
  // touched by the new one.
  WGPUDevice device = window->GetDevice();
  if (this->DeviceIdentity != nullptr && this->DeviceIdentity != device)
  {
    this->ReleaseDeviceResources();
  }
  this->DeviceIdentity = device;

  this->EnsurePlaceholders(window);
  this->UpdateTransferFunctions(window);
  this->UpdateBricks(window);
  this->UpdateDepthView(window);
  this->UpdateUniforms(window, renderer);
  this->UpdateBindGroup(window);
  this->EnsurePipeline(window);
  this->RecordRenderPass(window);
}

VTK_ABI_NAMESPACE_END
