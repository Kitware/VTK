// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPUInternalsBindGroup.h"
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkWebGPUInternalsBindGroup::BindingInitializationHelper::BindingInitializationHelper(
  uint32_t _binding, const wgpu::Sampler& _sampler)
  : binding(_binding)
  , sampler(_sampler)
{
}

//------------------------------------------------------------------------------
vtkWebGPUInternalsBindGroup::BindingInitializationHelper::BindingInitializationHelper(
  uint32_t _binding, const wgpu::TextureView& _textureView)
  : binding(_binding)
  , textureView(_textureView)
{
}

//------------------------------------------------------------------------------
vtkWebGPUInternalsBindGroup::BindingInitializationHelper::BindingInitializationHelper(
  uint32_t _binding, const wgpu::Buffer& _buffer, uint64_t _offset, uint64_t _size)
  : binding(_binding)
  , buffer(_buffer)
  , offset(_offset)
  , size(_size)
{
}

//------------------------------------------------------------------------------
vtkWebGPUInternalsBindGroup::BindingInitializationHelper::BindingInitializationHelper(
  const BindingInitializationHelper&) = default;

//------------------------------------------------------------------------------
vtkWebGPUInternalsBindGroup::BindingInitializationHelper::~BindingInitializationHelper() = default;

//------------------------------------------------------------------------------
wgpu::BindGroupEntry vtkWebGPUInternalsBindGroup::BindingInitializationHelper::GetAsBinding() const
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
wgpu::BindGroup vtkWebGPUInternalsBindGroup::MakeBindGroup(const wgpu::Device& device,
  const wgpu::BindGroupLayout& layout,
  std::initializer_list<BindingInitializationHelper> entriesInitializer)
{
  std::vector<wgpu::BindGroupEntry> entries;
  for (const BindingInitializationHelper& helper : entriesInitializer)
  {
    entries.push_back(helper.GetAsBinding());
  }

  wgpu::BindGroupDescriptor descriptor;
  descriptor.layout = layout;
  descriptor.entryCount = static_cast<uint32_t>(entries.size());
  descriptor.entries = entries.data();

  return device.CreateBindGroup(&descriptor);
}
VTK_ABI_NAMESPACE_END
