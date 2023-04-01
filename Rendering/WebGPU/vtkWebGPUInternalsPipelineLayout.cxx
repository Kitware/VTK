/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebGPUInternalsPipelineLayout.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
