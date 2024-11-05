// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPUBindGroupInternals.h"
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkWebGPUBindGroupInternals::BindingInitializationHelper::BindingInitializationHelper(
  uint32_t _binding, const wgpu::Sampler& _sampler)
  : binding(_binding)
  , sampler(_sampler)
{
}

//------------------------------------------------------------------------------
vtkWebGPUBindGroupInternals::BindingInitializationHelper::BindingInitializationHelper(
  uint32_t _binding, const wgpu::TextureView& _textureView)
  : binding(_binding)
  , textureView(_textureView)
{
}

//------------------------------------------------------------------------------
vtkWebGPUBindGroupInternals::BindingInitializationHelper::BindingInitializationHelper(
  uint32_t _binding, const wgpu::Buffer& _buffer, uint64_t _offset, uint64_t _size)
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
wgpu::BindGroupEntry vtkWebGPUBindGroupInternals::BindingInitializationHelper::GetAsBinding() const
{
  wgpu::BindGroupEntry result;

  result.binding = binding;
  result.sampler = sampler;
  result.textureView = textureView;
  result.buffer = buffer;
  result.offset = offset;
  result.size = size;

  return result;
}

//------------------------------------------------------------------------------
wgpu::BindGroup vtkWebGPUBindGroupInternals::MakeBindGroup(const wgpu::Device& device,
  const wgpu::BindGroupLayout& layout, const std::vector<wgpu::BindGroupEntry>& entries,
  std::string label /*=""*/)
{
  wgpu::BindGroupDescriptor descriptor;
  descriptor.label = label.c_str();
  descriptor.layout = layout;
  descriptor.entryCount = static_cast<uint32_t>(entries.size());
  descriptor.entries = entries.data();

  return device.CreateBindGroup(&descriptor);
}

//------------------------------------------------------------------------------
wgpu::BindGroup vtkWebGPUBindGroupInternals::MakeBindGroup(const wgpu::Device& device,
  const wgpu::BindGroupLayout& layout,
  std::initializer_list<BindingInitializationHelper> entriesInitializer, std::string label /*=""*/)
{
  std::vector<wgpu::BindGroupEntry> entries;
  for (const BindingInitializationHelper& helper : entriesInitializer)
  {
    entries.push_back(helper.GetAsBinding());
  }

  return MakeBindGroup(device, layout, entries, label);
}
VTK_ABI_NAMESPACE_END
