// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPUInternalsPipelineLayout.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
wgpu::PipelineLayout vtkWebGPUInternalsPipelineLayout::MakeBasicPipelineLayout(
  const wgpu::Device& device, const wgpu::BindGroupLayout* bindGroupLayout)
{
  wgpu::PipelineLayoutDescriptor descriptor;
  if (bindGroupLayout != nullptr)
  {
    descriptor.bindGroupLayoutCount = 1;
    descriptor.bindGroupLayouts = bindGroupLayout;
  }
  else
  {
    descriptor.bindGroupLayoutCount = 0;
    descriptor.bindGroupLayouts = nullptr;
  }
  return device.CreatePipelineLayout(&descriptor);
}

//------------------------------------------------------------------------------
wgpu::PipelineLayout vtkWebGPUInternalsPipelineLayout::MakePipelineLayout(
  const wgpu::Device& device, std::vector<wgpu::BindGroupLayout> bgls)
{
  wgpu::PipelineLayoutDescriptor descriptor;
  descriptor.bindGroupLayoutCount = uint32_t(bgls.size());
  descriptor.bindGroupLayouts = bgls.data();
  return device.CreatePipelineLayout(&descriptor);
}
VTK_ABI_NAMESPACE_END
