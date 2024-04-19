// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPUInternalsBindGroupLayout.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkWebGPUInternalsBindGroupLayout::LayoutEntryInitializationHelper::LayoutEntryInitializationHelper(
  uint32_t entryBinding, wgpu::ShaderStage entryVisibility, wgpu::BufferBindingType bufferType,
  bool bufferHasDynamicOffset, uint64_t bufferMinBindingSize)
{
  binding = entryBinding;
  visibility = entryVisibility;
  buffer.type = bufferType;
  buffer.hasDynamicOffset = bufferHasDynamicOffset;
  buffer.minBindingSize = bufferMinBindingSize;
}

//------------------------------------------------------------------------------
vtkWebGPUInternalsBindGroupLayout::LayoutEntryInitializationHelper::LayoutEntryInitializationHelper(
  uint32_t entryBinding, wgpu::ShaderStage entryVisibility, wgpu::SamplerBindingType samplerType)
{
  binding = entryBinding;
  visibility = entryVisibility;
  sampler.type = samplerType;
}

//------------------------------------------------------------------------------
vtkWebGPUInternalsBindGroupLayout::LayoutEntryInitializationHelper::LayoutEntryInitializationHelper(
  uint32_t entryBinding, wgpu::ShaderStage entryVisibility,
  wgpu::TextureSampleType textureSampleType, wgpu::TextureViewDimension textureViewDimension,
  bool textureMultisampled)
{
  binding = entryBinding;
  visibility = entryVisibility;
  texture.sampleType = textureSampleType;
  texture.viewDimension = textureViewDimension;
  texture.multisampled = textureMultisampled;
}

//------------------------------------------------------------------------------
vtkWebGPUInternalsBindGroupLayout::LayoutEntryInitializationHelper::LayoutEntryInitializationHelper(
  uint32_t entryBinding, wgpu::ShaderStage entryVisibility,
  wgpu::StorageTextureAccess storageTextureAccess, wgpu::TextureFormat format,
  wgpu::TextureViewDimension textureViewDimension)
{
  binding = entryBinding;
  visibility = entryVisibility;
  storageTexture.access = storageTextureAccess;
  storageTexture.format = format;
  storageTexture.viewDimension = textureViewDimension;
}

//------------------------------------------------------------------------------
vtkWebGPUInternalsBindGroupLayout::LayoutEntryInitializationHelper::LayoutEntryInitializationHelper(
  const wgpu::BindGroupLayoutEntry& entry)
  : wgpu::BindGroupLayoutEntry(entry)
{
}

//------------------------------------------------------------------------------
wgpu::BindGroupLayout vtkWebGPUInternalsBindGroupLayout::MakeBindGroupLayout(
  const wgpu::Device& device,
  std::initializer_list<vtkWebGPUInternalsBindGroupLayout::LayoutEntryInitializationHelper>
    entriesInitializer)
{
  std::vector<wgpu::BindGroupLayoutEntry> entries;
  for (const vtkWebGPUInternalsBindGroupLayout::LayoutEntryInitializationHelper& entry :
    entriesInitializer)
  {
    entries.push_back(entry);
  }

  wgpu::BindGroupLayoutDescriptor descriptor;
  descriptor.entryCount = static_cast<uint32_t>(entries.size());
  descriptor.entries = entries.data();
  return device.CreateBindGroupLayout(&descriptor);
}
VTK_ABI_NAMESPACE_END
