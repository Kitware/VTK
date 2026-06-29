// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper::LayoutEntryInitializationHelper(
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
vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper::LayoutEntryInitializationHelper(
  uint32_t entryBinding, wgpu::ShaderStage entryVisibility, wgpu::SamplerBindingType samplerType)
{
  binding = entryBinding;
  visibility = entryVisibility;
  sampler.type = samplerType;
}

//------------------------------------------------------------------------------
vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper::LayoutEntryInitializationHelper(
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
vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper::LayoutEntryInitializationHelper(
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
vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper::LayoutEntryInitializationHelper(
  const wgpu::BindGroupLayoutEntry& entry)
  : wgpu::BindGroupLayoutEntry(entry)
{
}

//------------------------------------------------------------------------------
wgpu::BindGroupLayout vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(
  const wgpu::Device& device, const std::vector<wgpu::BindGroupLayoutEntry>& entries,
  std::string label /*=""*/)
{
  wgpu::BindGroupLayoutDescriptor descriptor;
  descriptor.label = label.c_str();
  descriptor.entryCount = static_cast<uint32_t>(entries.size());
  descriptor.entries = entries.data();
  return device.CreateBindGroupLayout(&descriptor);
}

//------------------------------------------------------------------------------
wgpu::BindGroupLayout vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(
  const wgpu::Device& device,
  std::initializer_list<vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper>
    entriesInitializer,
  std::string label /*=""*/)
{
  std::vector<wgpu::BindGroupLayoutEntry> entries;
  for (const vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper& entry :
    entriesInitializer)
  {
    entries.push_back(entry);
  }

  return MakeBindGroupLayout(device, entries, label);
}

//------------------------------------------------------------------------------
WGPUBindGroupLayout vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(const WGPUDevice& device,
  const std::vector<WGPUBindGroupLayoutEntry>& entries, std::string label /*=""*/)
{
  // Convert WGPU* types to wgpu:: wrappers and call the existing implementation
  wgpu::Device wrappedDevice(device);
  std::vector<wgpu::BindGroupLayoutEntry> wrappedEntries;
  for (const auto& entry : entries)
  {
    wrappedEntries.push_back(*reinterpret_cast<const wgpu::BindGroupLayoutEntry*>(&entry));
  }
  auto result = MakeBindGroupLayout(wrappedDevice, wrappedEntries, label);
  return result.MoveToCHandle();
}
VTK_ABI_NAMESPACE_END
