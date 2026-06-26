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
    wgpu::Texture(this->Texture).Destroy();
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
  this->Sampler =
    wgpu::Device(wgpuConfiguration->GetDevice())
      .CreateSampler(reinterpret_cast<wgpu::SamplerDescriptor*>(&this->SamplerDescriptor))
      .Get();
  this->TextureViewDescriptor = {};
  if (cubeMap)
  {
    this->TextureViewDescriptor.dimension =
      static_cast<WGPUTextureViewDimension>(wgpu::TextureViewDimension::Cube);
    this->TextureViewDescriptor.arrayLayerCount = 6;
  }
  this->TextureView =
    wgpu::Texture(this->Texture)
      .CreateView(reinterpret_cast<wgpu::TextureViewDescriptor*>(&this->TextureViewDescriptor))
      .Get();
  this->Modified();
}

//------------------------------------------------------------------------------
WGPUBindGroupLayoutEntry vtkWebGPURenderTextureDeviceResource::MakeSamplerBindGroupLayoutEntry(
  std::uint32_t binding, WGPUShaderStage visibility)
{
  wgpu::BindGroupLayoutEntry entry = {};
  entry.binding = binding;
  entry.visibility = static_cast<wgpu::ShaderStage>(visibility);
  entry.sampler.nextInChain = nullptr;
  entry.sampler.type = static_cast<wgpu::SamplerBindingType>(
    this->GetWebGPUSamplerBindingType(this->SamplerBindingType));
  return *reinterpret_cast<WGPUBindGroupLayoutEntry*>(&entry);
}

//------------------------------------------------------------------------------
WGPUBindGroupEntry vtkWebGPURenderTextureDeviceResource::MakeSamplerBindGroupEntry(
  std::uint32_t binding)
{
  wgpu::BindGroupEntry entry = {};
  entry.binding = binding;
  entry.sampler = wgpu::Sampler(this->Sampler);
  return *reinterpret_cast<WGPUBindGroupEntry*>(&entry);
}

//------------------------------------------------------------------------------
WGPUBindGroupLayoutEntry vtkWebGPURenderTextureDeviceResource::MakeTextureViewBindGroupLayoutEntry(
  std::uint32_t binding, WGPUShaderStage visibility)
{
  wgpu::BindGroupLayoutEntry entry = {};
  entry.binding = binding;
  entry.visibility = static_cast<wgpu::ShaderStage>(visibility);
  entry.texture.nextInChain = nullptr;
  entry.texture.sampleType =
    vtkWebGPUComputePassTextureStorageInternals::ComputeTextureSampleTypeToWebGPU(
      this->GetSampleType());
  entry.texture.viewDimension =
    vtkWebGPUComputePassTextureStorageInternals::ComputeTextureDimensionToViewDimension(
      this->GetDimension());
  entry.texture.multisampled = (this->SampleCount > 1);
  return *reinterpret_cast<WGPUBindGroupLayoutEntry*>(&entry);
}

//------------------------------------------------------------------------------
WGPUBindGroupEntry vtkWebGPURenderTextureDeviceResource::MakeTextureViewBindGroupEntry(
  std::uint32_t binding)
{
  wgpu::BindGroupEntry entry = {};
  entry.binding = binding;
  entry.textureView = wgpu::TextureView(this->TextureView);
  return *reinterpret_cast<WGPUBindGroupEntry*>(&entry);
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
      return static_cast<WGPUFilterMode>(wgpu::FilterMode::Nearest);
    case FilterMode::LINEAR:
      return static_cast<WGPUFilterMode>(wgpu::FilterMode::Linear);
    default:
      return static_cast<WGPUFilterMode>(wgpu::FilterMode::Undefined);
  }
}
//------------------------------------------------------------------------------
WGPUMipmapFilterMode vtkWebGPURenderTextureDeviceResource::GetWGPUMipMapFilterMode(FilterMode mode)
{
  switch (mode)
  {
    case FilterMode::NEAREST:
      return static_cast<WGPUMipmapFilterMode>(wgpu::MipmapFilterMode::Nearest);
    case FilterMode::LINEAR:
      return static_cast<WGPUMipmapFilterMode>(wgpu::MipmapFilterMode::Linear);
    default:
      return static_cast<WGPUMipmapFilterMode>(wgpu::MipmapFilterMode::Undefined);
  }
}

//------------------------------------------------------------------------------
WGPUAddressMode vtkWebGPURenderTextureDeviceResource::GetWebGPUAddressMode(AddressMode mode)
{
  switch (mode)
  {
    case AddressMode::CLAMP_TO_EDGE:
      return static_cast<WGPUAddressMode>(wgpu::AddressMode::ClampToEdge);
    case AddressMode::REPEAT:
      return static_cast<WGPUAddressMode>(wgpu::AddressMode::Repeat);
    case AddressMode::MIRROR_REPEAT:
      return static_cast<WGPUAddressMode>(wgpu::AddressMode::MirrorRepeat);
    default:
      return static_cast<WGPUAddressMode>(wgpu::AddressMode::Undefined);
  }
}

//------------------------------------------------------------------------------
WGPUSamplerBindingType vtkWebGPURenderTextureDeviceResource::GetWebGPUSamplerBindingType(
  SamplerMode mode)
{
  switch (mode)
  {
    case SamplerMode::FILTERING:
      return static_cast<WGPUSamplerBindingType>(wgpu::SamplerBindingType::Filtering);
    case SamplerMode::NON_FILTERING:
      return static_cast<WGPUSamplerBindingType>(wgpu::SamplerBindingType::NonFiltering);
    case SamplerMode::COMPARISON:
      return static_cast<WGPUSamplerBindingType>(wgpu::SamplerBindingType::Comparison);
    default:
      return static_cast<WGPUSamplerBindingType>(wgpu::SamplerBindingType::Undefined);
  }
}

//------------------------------------------------------------------------------
WGPUCompareFunction vtkWebGPURenderTextureDeviceResource::GetWebGPUCompareFunction(
  CompareFunction mode)
{
  switch (mode)
  {
    case CompareFunction::NEVER:
      return static_cast<WGPUCompareFunction>(wgpu::CompareFunction::Never);
    case CompareFunction::LESS:
      return static_cast<WGPUCompareFunction>(wgpu::CompareFunction::Less);
    case CompareFunction::LESS_EQUAL:
      return static_cast<WGPUCompareFunction>(wgpu::CompareFunction::LessEqual);
    case CompareFunction::GREATER:
      return static_cast<WGPUCompareFunction>(wgpu::CompareFunction::Greater);
    case CompareFunction::GREATER_EQUAL:
      return static_cast<WGPUCompareFunction>(wgpu::CompareFunction::GreaterEqual);
    case CompareFunction::EQUAL:
      return static_cast<WGPUCompareFunction>(wgpu::CompareFunction::Equal);
    case CompareFunction::NOT_EQUAL:
      return static_cast<WGPUCompareFunction>(wgpu::CompareFunction::NotEqual);
    case CompareFunction::ALWAYS:
      return static_cast<WGPUCompareFunction>(wgpu::CompareFunction::Always);
    default:
      return static_cast<WGPUCompareFunction>(wgpu::CompareFunction::Undefined);
  }
}
VTK_ABI_NAMESPACE_END
