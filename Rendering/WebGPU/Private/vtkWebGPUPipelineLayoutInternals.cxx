// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPUPipelineLayoutInternals.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
wgpu::PipelineLayout vtkWebGPUPipelineLayoutInternals::MakeBasicPipelineLayout(
  const wgpu::Device& device, const wgpu::BindGroupLayout* bindGroupLayout,
  std::string label /*=""*/)
{
  wgpu::PipelineLayoutDescriptor descriptor;
  descriptor.label = label.c_str();
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
wgpu::PipelineLayout vtkWebGPUPipelineLayoutInternals::MakePipelineLayout(
  const wgpu::Device& device, std::vector<wgpu::BindGroupLayout> bgls, std::string label /*=""*/)
{
  wgpu::PipelineLayoutDescriptor descriptor;
  descriptor.label = label.c_str();
  descriptor.bindGroupLayoutCount = uint32_t(bgls.size());
  descriptor.bindGroupLayouts = bgls.data();
  return device.CreatePipelineLayout(&descriptor);
}
VTK_ABI_NAMESPACE_END
