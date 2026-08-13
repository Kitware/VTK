// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPUBindGroupInternals.h"

#include "Private/vtkWebGPUHelpersPrivate.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkWebGPUBindGroupInternals::BindingInitializationHelper::BindingInitializationHelper(
  uint32_t _binding, WGPUSampler _sampler)
  : binding(_binding)
  , sampler(_sampler)
{
}

//------------------------------------------------------------------------------
vtkWebGPUBindGroupInternals::BindingInitializationHelper::BindingInitializationHelper(
  uint32_t _binding, WGPUTextureView _textureView)
  : binding(_binding)
  , textureView(_textureView)
{
}

//------------------------------------------------------------------------------
vtkWebGPUBindGroupInternals::BindingInitializationHelper::BindingInitializationHelper(
  uint32_t _binding, WGPUBuffer _buffer, uint64_t _offset, uint64_t _size)
  : binding(_binding)
  , buffer(_buffer)
  , offset(_offset)
  , size(_size)
{
}

//------------------------------------------------------------------------------
vtkWebGPUBindGroupInternals::BindingInitializationHelper::BindingInitializationHelper(
  const BindingInitializationHelper&) = default;

//------------------------------------------------------------------------------
vtkWebGPUBindGroupInternals::BindingInitializationHelper::~BindingInitializationHelper() = default;

//------------------------------------------------------------------------------
WGPUBindGroupEntry vtkWebGPUBindGroupInternals::BindingInitializationHelper::GetAsBinding() const
{
  WGPUBindGroupEntry result = WGPU_BIND_GROUP_ENTRY_INIT;

  result.binding = binding;
  result.sampler = sampler;
  result.textureView = textureView;
  result.buffer = buffer;
  result.offset = offset;
  result.size = size;

  return result;
}

//------------------------------------------------------------------------------
WGPUBindGroup vtkWebGPUBindGroupInternals::MakeBindGroup(const WGPUDevice& device,
  const WGPUBindGroupLayout& layout, const std::vector<WGPUBindGroupEntry>& entries,
  std::string label /*=""*/)
{
  WGPUBindGroupDescriptor descriptor = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
  descriptor.label = vtkWebGPUMakeStringView(label);
  descriptor.layout = layout;
  descriptor.entryCount = static_cast<uint32_t>(entries.size());
  descriptor.entries = entries.data();

  return wgpuDeviceCreateBindGroup(device, &descriptor);
}

//------------------------------------------------------------------------------
WGPUBindGroup vtkWebGPUBindGroupInternals::MakeBindGroup(const WGPUDevice& device,
  const WGPUBindGroupLayout& layout,
  std::initializer_list<BindingInitializationHelper> entriesInitializer, std::string label /*=""*/)
{
  std::vector<WGPUBindGroupEntry> entries;
  entries.reserve(entriesInitializer.size());
  for (const BindingInitializationHelper& helper : entriesInitializer)
  {
    entries.push_back(helper.GetAsBinding());
  }

  return MakeBindGroup(device, layout, entries, label);
}
VTK_ABI_NAMESPACE_END
