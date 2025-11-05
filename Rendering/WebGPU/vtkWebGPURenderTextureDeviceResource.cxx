// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPURenderTextureDeviceResource.h"
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"
#include "vtkObjectFactory.h"
#include "vtkWebGPUConfiguration.h"
#include "vtkWebGPURenderWindow.h"

#include "Private/vtkWebGPUComputePassTextureStorageInternals.h"

#include <string_view>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPURenderTextureDeviceResource);

//------------------------------------------------------------------------------
vtkWebGPURenderTextureDeviceResource::vtkWebGPURenderTextureDeviceResource() = default;

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
  if (this->Texture)
  {
    this->Texture.Destroy();
    this->Texture = nullptr;
  }
  if (this->Sampler)
  {
    this->Sampler = nullptr;
  }
  if (this->TextureView)
  {
    this->TextureView = nullptr;
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
  this->TextureDescriptor.label = std::string_view(this->Label);
  this->TextureDescriptor.dimension =
    vtkWebGPUComputePassTextureStorageInternals::ComputeTextureDimensionToWebGPU(
      this->GetDimension());
  this->TextureDescriptor.size.width = this->GetWidth();
  this->TextureDescriptor.size.height = this->GetHeight();
  this->TextureDescriptor.size.depthOrArrayLayers = this->GetDepth();
  this->TextureDescriptor.format =
    vtkWebGPUComputePassTextureStorageInternals::ComputeTextureFormatToWebGPU(this->GetFormat());
  this->TextureDescriptor.mipLevelCount = this->GetMipLevelCount();
  this->TextureDescriptor.sampleCount = this->SampleCount;
  this->TextureDescriptor.usage =
    vtkWebGPUComputePassTextureStorageInternals::ComputeTextureModeToUsage(this->GetMode(),
      this->TextureDescriptor.label.IsUndefined() ? std::string("Unnamed Texture")
                                                  : std::string(this->TextureDescriptor.label));
  this->Texture = wgpuConfiguration->CreateTexture(this->TextureDescriptor);
  // upload data
  const auto bytesPerRow = this->GetWidth() * this->GetBytesPerPixel();
  const auto sizeBytes = bytesPerRow * this->GetHeight() * this->GetDepth();
  wgpu::Origin3D dstOrigin = { 0, 0, 0 };
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
  this->Sampler = wgpuConfiguration->GetDevice().CreateSampler(&this->SamplerDescriptor);
  this->TextureViewDescriptor = {};
  if (cubeMap)
  {
    this->TextureViewDescriptor.dimension = wgpu::TextureViewDimension::Cube;
  }
  this->TextureView = this->Texture.CreateView(&this->TextureViewDescriptor);
}

//------------------------------------------------------------------------------
wgpu::BindGroupLayoutEntry vtkWebGPURenderTextureDeviceResource::MakeSamplerBindGroupLayoutEntry(
  std::uint32_t binding, wgpu::ShaderStage visibility)
{
  wgpu::BindGroupLayoutEntry entry = {};
  entry.binding = binding;
  entry.visibility = visibility;
  entry.sampler.nextInChain = nullptr;
  entry.sampler.type = this->GetWebGPUSamplerBindingType(this->SamplerBindingType);
  return entry;
}

//------------------------------------------------------------------------------
wgpu::BindGroupEntry vtkWebGPURenderTextureDeviceResource::MakeSamplerBindGroupEntry(
  std::uint32_t binding)
{
  wgpu::BindGroupEntry entry = {};
  entry.binding = binding;
  entry.sampler = this->Sampler;
  return entry;
}

//------------------------------------------------------------------------------
wgpu::BindGroupLayoutEntry
vtkWebGPURenderTextureDeviceResource::MakeTextureViewBindGroupLayoutEntry(
  std::uint32_t binding, wgpu::ShaderStage visibility)
{
  wgpu::BindGroupLayoutEntry entry = {};
  entry.binding = binding;
  entry.visibility = visibility;
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
wgpu::BindGroupEntry vtkWebGPURenderTextureDeviceResource::MakeTextureViewBindGroupEntry(
  std::uint32_t binding)
{
  wgpu::BindGroupEntry entry = {};
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
wgpu::FilterMode vtkWebGPURenderTextureDeviceResource::GetWebGPUFilterMode(FilterMode mode)
{
  switch (mode)
  {
    case FilterMode::NEAREST:
      return wgpu::FilterMode::Nearest;
    case FilterMode::LINEAR:
      return wgpu::FilterMode::Linear;
    default:
      return wgpu::FilterMode::Undefined;
  }
}
//------------------------------------------------------------------------------
wgpu::MipmapFilterMode vtkWebGPURenderTextureDeviceResource::GetWGPUMipMapFilterMode(
  FilterMode mode)
{
  switch (mode)
  {
    case FilterMode::NEAREST:
      return wgpu::MipmapFilterMode::Nearest;
    case FilterMode::LINEAR:
      return wgpu::MipmapFilterMode::Linear;
    default:
      return wgpu::MipmapFilterMode::Undefined;
  }
}

//------------------------------------------------------------------------------
wgpu::AddressMode vtkWebGPURenderTextureDeviceResource::GetWebGPUAddressMode(AddressMode mode)
{
  switch (mode)
  {
    case AddressMode::CLAMP_TO_EDGE:
      return wgpu::AddressMode::ClampToEdge;
    case AddressMode::REPEAT:
      return wgpu::AddressMode::Repeat;
    case AddressMode::MIRROR_REPEAT:
      return wgpu::AddressMode::MirrorRepeat;
    default:
      return wgpu::AddressMode::Undefined;
  }
}

//------------------------------------------------------------------------------
wgpu::SamplerBindingType vtkWebGPURenderTextureDeviceResource::GetWebGPUSamplerBindingType(
  SamplerMode mode)
{
  switch (mode)
  {
    case SamplerMode::FILTERING:
      return wgpu::SamplerBindingType::Filtering;
    case SamplerMode::NON_FILTERING:
      return wgpu::SamplerBindingType::NonFiltering;
    case SamplerMode::COMPARISON:
      return wgpu::SamplerBindingType::Comparison;
    default:
      return wgpu::SamplerBindingType::Undefined;
  }
}

//------------------------------------------------------------------------------
wgpu::CompareFunction vtkWebGPURenderTextureDeviceResource::GetWebGPUCompareFunction(
  CompareFunction mode)
{
  switch (mode)
  {
    case CompareFunction::NEVER:
      return wgpu::CompareFunction::Never;
    case CompareFunction::LESS:
      return wgpu::CompareFunction::Less;
    case CompareFunction::LESS_EQUAL:
      return wgpu::CompareFunction::LessEqual;
    case CompareFunction::GREATER:
      return wgpu::CompareFunction::Greater;
    case CompareFunction::GREATER_EQUAL:
      return wgpu::CompareFunction::GreaterEqual;
    case CompareFunction::EQUAL:
      return wgpu::CompareFunction::Equal;
    case CompareFunction::NOT_EQUAL:
      return wgpu::CompareFunction::NotEqual;
    case CompareFunction::ALWAYS:
      return wgpu::CompareFunction::Always;
    default:
      return wgpu::CompareFunction::Undefined;
  }
}
VTK_ABI_NAMESPACE_END
