/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebGPUInternalsBindGroup.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWebGPUInternalsBindGroup.h"
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkWebGPUInternalsBindGroup::BindingInitializationHelper::BindingInitializationHelper(
  uint32_t binding, const wgpu::Sampler& sampler)
  : binding(binding)
  , sampler(sampler)
{
}

//------------------------------------------------------------------------------
vtkWebGPUInternalsBindGroup::BindingInitializationHelper::BindingInitializationHelper(
  uint32_t binding, const wgpu::TextureView& textureView)
  : binding(binding)
  , textureView(textureView)
{
}

//------------------------------------------------------------------------------
vtkWebGPUInternalsBindGroup::BindingInitializationHelper::BindingInitializationHelper(
  uint32_t binding, const wgpu::Buffer& buffer, uint64_t offset, uint64_t size)
  : binding(binding)
  , buffer(buffer)
  , offset(offset)
  , size(size)
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
