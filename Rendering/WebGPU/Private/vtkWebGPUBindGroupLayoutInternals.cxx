// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"

#include "Private/vtkWebGPUHelpersPrivate.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper::LayoutEntryInitializationHelper(
  uint32_t entryBinding, WGPUShaderStage entryVisibility, WGPUBufferBindingType bufferType,
  bool bufferHasDynamicOffset, uint64_t bufferMinBindingSize)
  : WGPUBindGroupLayoutEntry(WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT)
{
  binding = entryBinding;
  visibility = entryVisibility;
  buffer.type = bufferType;
  buffer.hasDynamicOffset = bufferHasDynamicOffset;
  buffer.minBindingSize = bufferMinBindingSize;
}

//------------------------------------------------------------------------------
vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper::LayoutEntryInitializationHelper(
  uint32_t entryBinding, WGPUShaderStage entryVisibility, WGPUSamplerBindingType samplerType)
  : WGPUBindGroupLayoutEntry(WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT)
{
  binding = entryBinding;
  visibility = entryVisibility;
  sampler.type = samplerType;
}

//------------------------------------------------------------------------------
vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper::LayoutEntryInitializationHelper(
  uint32_t entryBinding, WGPUShaderStage entryVisibility, WGPUTextureSampleType textureSampleType,
  WGPUTextureViewDimension textureViewDimension, bool textureMultisampled)
  : WGPUBindGroupLayoutEntry(WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT)
{
  binding = entryBinding;
  visibility = entryVisibility;
  texture.sampleType = textureSampleType;
  texture.viewDimension = textureViewDimension;
  texture.multisampled = textureMultisampled;
}

//------------------------------------------------------------------------------
vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper::LayoutEntryInitializationHelper(
  uint32_t entryBinding, WGPUShaderStage entryVisibility,
  WGPUStorageTextureAccess storageTextureAccess, WGPUTextureFormat format,
  WGPUTextureViewDimension textureViewDimension)
  : WGPUBindGroupLayoutEntry(WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT)
{
  binding = entryBinding;
  visibility = entryVisibility;
  storageTexture.access = storageTextureAccess;
  storageTexture.format = format;
  storageTexture.viewDimension = textureViewDimension;
}

//------------------------------------------------------------------------------
vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper::LayoutEntryInitializationHelper(
  const WGPUBindGroupLayoutEntry& entry)
  : WGPUBindGroupLayoutEntry(entry)
{
}

//------------------------------------------------------------------------------
WGPUBindGroupLayout vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(const WGPUDevice& device,
  const std::vector<WGPUBindGroupLayoutEntry>& entries, std::string label /*=""*/)
{
  WGPUBindGroupLayoutDescriptor descriptor = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
  descriptor.label = vtkWebGPUMakeStringView(label);
  descriptor.entryCount = static_cast<uint32_t>(entries.size());
  descriptor.entries = entries.data();
  return wgpuDeviceCreateBindGroupLayout(device, &descriptor);
}

//------------------------------------------------------------------------------
WGPUBindGroupLayout vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(const WGPUDevice& device,
  std::initializer_list<vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper>
    entriesInitializer,
  std::string label /*=""*/)
{
  std::vector<WGPUBindGroupLayoutEntry> entries;
  entries.reserve(entriesInitializer.size());
  for (const auto& entry : entriesInitializer)
  {
    entries.push_back(entry);
  }
  return MakeBindGroupLayout(device, entries, label);
}
VTK_ABI_NAMESPACE_END
