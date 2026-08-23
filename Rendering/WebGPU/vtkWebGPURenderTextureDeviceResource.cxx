// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPURenderTextureDeviceResource.h"
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"
#include "Private/vtkWebGPUHandle.h"
#include "vtkObjectFactory.h"
#include "vtkWebGPUConfiguration.h"
#include "vtkWebGPURenderWindow.h"

#include "Private/vtkWebGPUComputePassTextureStorageInternals.h"

#include <string_view>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPURenderTextureDeviceResource);

//------------------------------------------------------------------------------
vtkWebGPURenderTextureDeviceResource::vtkWebGPURenderTextureDeviceResource()
  // The WGPU_*_INIT macros carry the non-zero defaults the C API expects. They live
  // here rather than in the header because vtkWrapHierarchy cannot expand them.
  : TextureDescriptor(WGPU_TEXTURE_DESCRIPTOR_INIT)
  , SamplerDescriptor(WGPU_SAMPLER_DESCRIPTOR_INIT)
  , TextureViewDescriptor(WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT)
{
}

//------------------------------------------------------------------------------
vtkWebGPURenderTextureDeviceResource::~vtkWebGPURenderTextureDeviceResource()
{
  this->ReleaseGraphicsResources(nullptr);
}

//------------------------------------------------------------------------------
void vtkWebGPURenderTextureDeviceResource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Label: " << this->Label << "\n";
  os << indent << "AddressModeU: ";
  switch (this->AddressModeU)
  {
    case AddressMode::CLAMP_TO_EDGE:
      os << "CLAMP_TO_EDGE";
      break;
    case AddressMode::REPEAT:
      os << "REPEAT";
      break;
    case AddressMode::MIRROR_REPEAT:
      os << "MIRROR_REPEAT";
      break;
    default:
      os << "UNKNOWN";
      break;
  }
  os << "\n";

  os << indent << "AddressModeV: ";
  switch (this->AddressModeV)
  {
    case AddressMode::CLAMP_TO_EDGE:
      os << "CLAMP_TO_EDGE";
      break;
    case AddressMode::REPEAT:
      os << "REPEAT";
      break;
    case AddressMode::MIRROR_REPEAT:
      os << "MIRROR_REPEAT";
      break;
    default:
      os << "UNKNOWN";
      break;
  }
  os << "\n";

  os << indent << "AddressModeW: ";
  switch (this->AddressModeW)
  {
    case AddressMode::CLAMP_TO_EDGE:
      os << "CLAMP_TO_EDGE";
      break;
    case AddressMode::REPEAT:
      os << "REPEAT";
      break;
    case AddressMode::MIRROR_REPEAT:
      os << "MIRROR_REPEAT";
      break;
    default:
      os << "UNKNOWN";
      break;
  }
  os << "\n";

  os << indent << "MagFilter: ";
  switch (this->MagFilter)
  {
    case FilterMode::NEAREST:
      os << "NEAREST";
      break;
    case FilterMode::LINEAR:
      os << "LINEAR";
      break;
    default:
      os << "UNKNOWN";
      break;
  }
  os << "\n";

  os << indent << "MinFilter: ";
  switch (this->MinFilter)
  {
    case FilterMode::NEAREST:
      os << "NEAREST";
      break;
    case FilterMode::LINEAR:
      os << "LINEAR";
      break;
    default:
      os << "UNKNOWN";
      break;
  }
  os << "\n";

  os << indent << "MipmapFilter: ";
  switch (this->MipmapFilter)
  {
    case FilterMode::NEAREST:
      os << "NEAREST";
      break;
    case FilterMode::LINEAR:
      os << "LINEAR";
      break;
    default:
      os << "UNKNOWN";
      break;
  }
  os << "\n";

  os << indent << "SamplerBindingType: ";
  switch (this->SamplerBindingType)
  {
    case SamplerMode::FILTERING:
      os << "FILTERING";
      break;
    case SamplerMode::NON_FILTERING:
      os << "NON_FILTERING";
      break;
    case SamplerMode::COMPARISON:
      os << "COMPARISON";
      break;
    default:
      os << "UNKNOWN";
      break;
  }
  os << "\n";

  os << indent << "CompareFunc: ";
  switch (this->CompareFunc)
  {
    case CompareFunction::NEVER:
      os << "NEVER";
      break;
    case CompareFunction::LESS:
      os << "LESS";
      break;
    case CompareFunction::LESS_EQUAL:
      os << "LESS_EQUAL";
      break;
    case CompareFunction::GREATER:
      os << "GREATER";
      break;
    case CompareFunction::GREATER_EQUAL:
      os << "GREATER_EQUAL";
      break;
    case CompareFunction::EQUAL:
      os << "EQUAL";
      break;
    case CompareFunction::NOT_EQUAL:
      os << "NOT_EQUAL";
      break;
    case CompareFunction::ALWAYS:
      os << "ALWAYS";
      break;
    default:
      os << "UNKNOWN";
      break;
  }
  os << "\n";
  os << indent << "LODMinClamp: " << this->LODMinClamp << "\n";
  os << indent << "LODMaxClamp: " << this->LODMaxClamp << "\n";
  os << indent << "MaxAnisotropy: " << this->MaxAnisotropy << "\n";
  os << indent << "SampleCount: " << this->SampleCount << "\n";
  os << indent << "BaseMipLevel: " << this->BaseMipLevel << "\n";
}

//------------------------------------------------------------------------------
void vtkWebGPURenderTextureDeviceResource::ReleaseGraphicsResources(vtkWindow* vtkNotUsed(window))
{
  // These are raw C handles. Destroy() frees the GPU allocation but does not
  // drop our reference, and assigning nullptr on its own drops nothing, so each
  // one has to be released explicitly.
  if (this->TextureView)
  {
    wgpuTextureViewRelease(this->TextureView);
    this->TextureView = nullptr;
  }
  if (this->Sampler)
  {
    wgpuSamplerRelease(this->Sampler);
    this->Sampler = nullptr;
  }
  if (this->Texture)
  {
    wgpuTextureDestroy(this->Texture);
    wgpuTextureRelease(this->Texture);
    this->Texture = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderTextureDeviceResource::SendToWebGPUDevice(std::vector<void*> dataPlanes,
  vtkWebGPUConfiguration* wgpuConfiguration, bool cubeMap /*= false*/)
{
  vtkLog(TRACE, "Sending texture to WebGPU device: " << this->GetLabel());
  // Assumptions made on inputs:
  // - inputs.size() = 1 for 1D/2D/3D textures
  // - inputs.size() = 6, and cubeMap = true for cube maps

  const int dimension = this->GetDimension();
  if (cubeMap && (dataPlanes.size() != 6 || dimension != DIMENSION_2D))
  {
    vtkErrorMacro("Cube maps require 6 data planes, each a 2D texture. There are "
      << dataPlanes.size() << " data planes and dimension is " << static_cast<int>(dimension));
    return;
  }
  // create texture.
  this->TextureDescriptor = {};
  this->TextureDescriptor.label.data = this->Label.c_str();
  this->TextureDescriptor.label.length = this->Label.size();
  this->TextureDescriptor.dimension = static_cast<WGPUTextureDimension>(
    vtkWebGPUComputePassTextureStorageInternals::ComputeTextureDimensionToWebGPU(
      this->GetDimension()));
  this->TextureDescriptor.size.width = this->GetWidth();
  this->TextureDescriptor.size.height = this->GetHeight();
  this->TextureDescriptor.size.depthOrArrayLayers = cubeMap ? 6 : this->GetDepth();
  this->TextureDescriptor.format = static_cast<WGPUTextureFormat>(
    vtkWebGPUComputePassTextureStorageInternals::ComputeTextureFormatToWebGPU(this->GetFormat()));
  this->TextureDescriptor.mipLevelCount = this->GetMipLevelCount();
  this->TextureDescriptor.sampleCount = this->SampleCount;
  this->TextureDescriptor.usage = static_cast<WGPUTextureUsage>(
    vtkWebGPUComputePassTextureStorageInternals::ComputeTextureModeToUsage(
      this->GetMode(), this->Label.empty() ? std::string("Unnamed Texture") : this->Label));
  this->Texture = wgpuConfiguration->CreateTexture(this->TextureDescriptor);
  // upload data
  const auto bytesPerRow = this->GetWidth() * this->GetBytesPerPixel();
  const auto sizeBytes = bytesPerRow * this->GetHeight() * this->GetDepth();
  WGPUOrigin3D dstOrigin = { 0, 0, 0 };
  std::uint32_t srcOffset = 0;
  for (size_t i = 0; i < dataPlanes.size(); ++i)
  {
    vtkLog(TRACE,
      "  Uploading data plane " << i << " of size " << sizeBytes << " bytes"
                                << (cubeMap ? " for cube map face." : "."));
    std::uint32_t dstMipLevel = 0;
    if (cubeMap)
    {
      dstOrigin.z = static_cast<std::uint32_t>(i);
    }
    wgpuConfiguration->WriteTexture(this->Texture, bytesPerRow, sizeBytes, dataPlanes[i], srcOffset,
      dstOrigin, dstMipLevel, cubeMap ? "Upload Cube Map Face" : "Upload Texture Data");
  }
  this->SamplerDescriptor = {};
  this->SamplerDescriptor.addressModeU = this->GetWebGPUAddressMode(this->AddressModeU);
  this->SamplerDescriptor.addressModeV = this->GetWebGPUAddressMode(this->AddressModeV);
  this->SamplerDescriptor.addressModeW = this->GetWebGPUAddressMode(this->AddressModeW);
  this->SamplerDescriptor.magFilter = this->GetWebGPUFilterMode(this->MagFilter);
  this->SamplerDescriptor.minFilter = this->GetWebGPUFilterMode(this->MinFilter);
  this->SamplerDescriptor.mipmapFilter = this->GetWGPUMipMapFilterMode(this->MipmapFilter);
  this->SamplerDescriptor.lodMinClamp = this->LODMinClamp;
  this->SamplerDescriptor.lodMaxClamp = this->LODMaxClamp;
  this->SamplerDescriptor.compare = this->GetWebGPUCompareFunction(this->CompareFunc);
  this->SamplerDescriptor.maxAnisotropy = this->MaxAnisotropy;
  vtkWebGPU::ReleaseAndNull(this->Sampler, wgpuSamplerRelease);
  this->Sampler = wgpuDeviceCreateSampler(wgpuConfiguration->GetDevice(), &this->SamplerDescriptor);
  // The INIT macro carries the defaults an empty initializer would drop, notably
  // the "undefined" mip/layer sentinels that let the implementation resolve them
  // to the texture's full counts.
  this->TextureViewDescriptor = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
  if (cubeMap)
  {
    this->TextureViewDescriptor.dimension =
      static_cast<WGPUTextureViewDimension>(WGPUTextureViewDimension_Cube);
    this->TextureViewDescriptor.arrayLayerCount = 6;
  }
  vtkWebGPU::ReleaseAndNull(this->TextureView, wgpuTextureViewRelease);
  this->TextureView = wgpuTextureCreateView(this->Texture, &this->TextureViewDescriptor);
  this->Modified();
}

//------------------------------------------------------------------------------
WGPUBindGroupLayoutEntry vtkWebGPURenderTextureDeviceResource::MakeSamplerBindGroupLayoutEntry(
  std::uint32_t binding, WGPUShaderStage visibility)
{
  WGPUBindGroupLayoutEntry entry = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
  entry.binding = binding;
  entry.visibility = static_cast<WGPUShaderStage>(visibility);
  entry.sampler.nextInChain = nullptr;
  entry.sampler.type = static_cast<WGPUSamplerBindingType>(
    this->GetWebGPUSamplerBindingType(this->SamplerBindingType));
  return entry;
}

//------------------------------------------------------------------------------
WGPUBindGroupEntry vtkWebGPURenderTextureDeviceResource::MakeSamplerBindGroupEntry(
  std::uint32_t binding)
{
  WGPUBindGroupEntry entry = WGPU_BIND_GROUP_ENTRY_INIT;
  entry.binding = binding;
  entry.sampler = this->Sampler;
  return entry;
}

//------------------------------------------------------------------------------
WGPUBindGroupLayoutEntry vtkWebGPURenderTextureDeviceResource::MakeTextureViewBindGroupLayoutEntry(
  std::uint32_t binding, WGPUShaderStage visibility)
{
  WGPUBindGroupLayoutEntry entry = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
  entry.binding = binding;
  entry.visibility = static_cast<WGPUShaderStage>(visibility);
  entry.texture.nextInChain = nullptr;
  entry.texture.sampleType =
    vtkWebGPUComputePassTextureStorageInternals::ComputeTextureSampleTypeToWebGPU(
      this->GetSampleType());
  entry.texture.viewDimension =
    vtkWebGPUComputePassTextureStorageInternals::ComputeTextureDimensionToViewDimension(
      this->GetDimension());
  entry.texture.multisampled = (this->SampleCount > 1);
  return entry;
}

//------------------------------------------------------------------------------
WGPUBindGroupEntry vtkWebGPURenderTextureDeviceResource::MakeTextureViewBindGroupEntry(
  std::uint32_t binding)
{
  WGPUBindGroupEntry entry = WGPU_BIND_GROUP_ENTRY_INIT;
  entry.binding = binding;
  entry.textureView = this->TextureView;
  return entry;
}

//------------------------------------------------------------------------------
const char* vtkWebGPURenderTextureDeviceResource::GetTextureSampleTypeString(TextureSampleType type)
{
  switch (type)
  {
    case vtkWebGPUTextureDeviceResource::FLOAT:
      return "f32";
    case vtkWebGPUTextureDeviceResource::UNFILTERABLE_FLOAT:
      return "unfilterable-float";
    case vtkWebGPUTextureDeviceResource::DEPTH:
      return "depth";
    case vtkWebGPUTextureDeviceResource::SIGNED_INT:
      return "sint";
    case vtkWebGPUTextureDeviceResource::UNSIGNED_INT:
      return "uint";
  }
  return "f32";
}

//------------------------------------------------------------------------------
WGPUFilterMode vtkWebGPURenderTextureDeviceResource::GetWebGPUFilterMode(FilterMode mode)
{
  switch (mode)
  {
    case FilterMode::NEAREST:
      return static_cast<WGPUFilterMode>(WGPUFilterMode_Nearest);
    case FilterMode::LINEAR:
      return static_cast<WGPUFilterMode>(WGPUFilterMode_Linear);
    default:
      return static_cast<WGPUFilterMode>(WGPUFilterMode_Undefined);
  }
}
//------------------------------------------------------------------------------
WGPUMipmapFilterMode vtkWebGPURenderTextureDeviceResource::GetWGPUMipMapFilterMode(FilterMode mode)
{
  switch (mode)
  {
    case FilterMode::NEAREST:
      return static_cast<WGPUMipmapFilterMode>(WGPUMipmapFilterMode_Nearest);
    case FilterMode::LINEAR:
      return static_cast<WGPUMipmapFilterMode>(WGPUMipmapFilterMode_Linear);
    default:
      return static_cast<WGPUMipmapFilterMode>(WGPUMipmapFilterMode_Undefined);
  }
}

//------------------------------------------------------------------------------
WGPUAddressMode vtkWebGPURenderTextureDeviceResource::GetWebGPUAddressMode(AddressMode mode)
{
  switch (mode)
  {
    case AddressMode::CLAMP_TO_EDGE:
      return static_cast<WGPUAddressMode>(WGPUAddressMode_ClampToEdge);
    case AddressMode::REPEAT:
      return static_cast<WGPUAddressMode>(WGPUAddressMode_Repeat);
    case AddressMode::MIRROR_REPEAT:
      return static_cast<WGPUAddressMode>(WGPUAddressMode_MirrorRepeat);
    default:
      return static_cast<WGPUAddressMode>(WGPUAddressMode_Undefined);
  }
}

//------------------------------------------------------------------------------
WGPUSamplerBindingType vtkWebGPURenderTextureDeviceResource::GetWebGPUSamplerBindingType(
  SamplerMode mode)
{
  switch (mode)
  {
    case SamplerMode::FILTERING:
      return static_cast<WGPUSamplerBindingType>(WGPUSamplerBindingType_Filtering);
    case SamplerMode::NON_FILTERING:
      return static_cast<WGPUSamplerBindingType>(WGPUSamplerBindingType_NonFiltering);
    case SamplerMode::COMPARISON:
      return static_cast<WGPUSamplerBindingType>(WGPUSamplerBindingType_Comparison);
    default:
      return static_cast<WGPUSamplerBindingType>(WGPUSamplerBindingType_Undefined);
  }
}

//------------------------------------------------------------------------------
WGPUCompareFunction vtkWebGPURenderTextureDeviceResource::GetWebGPUCompareFunction(
  CompareFunction mode)
{
  switch (mode)
  {
    case CompareFunction::NEVER:
      return static_cast<WGPUCompareFunction>(WGPUCompareFunction_Never);
    case CompareFunction::LESS:
      return static_cast<WGPUCompareFunction>(WGPUCompareFunction_Less);
    case CompareFunction::LESS_EQUAL:
      return static_cast<WGPUCompareFunction>(WGPUCompareFunction_LessEqual);
    case CompareFunction::GREATER:
      return static_cast<WGPUCompareFunction>(WGPUCompareFunction_Greater);
    case CompareFunction::GREATER_EQUAL:
      return static_cast<WGPUCompareFunction>(WGPUCompareFunction_GreaterEqual);
    case CompareFunction::EQUAL:
      return static_cast<WGPUCompareFunction>(WGPUCompareFunction_Equal);
    case CompareFunction::NOT_EQUAL:
      return static_cast<WGPUCompareFunction>(WGPUCompareFunction_NotEqual);
    case CompareFunction::ALWAYS:
      return static_cast<WGPUCompareFunction>(WGPUCompareFunction_Always);
    default:
      return static_cast<WGPUCompareFunction>(WGPUCompareFunction_Undefined);
  }
}
VTK_ABI_NAMESPACE_END
