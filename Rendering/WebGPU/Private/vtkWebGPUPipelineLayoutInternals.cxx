// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPUPipelineLayoutInternals.h"

#include "Private/vtkWebGPUHelpersPrivate.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
WGPUPipelineLayout vtkWebGPUPipelineLayoutInternals::MakeBasicPipelineLayout(
  WGPUDevice device, const WGPUBindGroupLayout* bindGroupLayout, std::string label /*=""*/)
{
  WGPUPipelineLayoutDescriptor descriptor = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
  descriptor.label = vtkWebGPUMakeStringView(label);
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
  return wgpuDeviceCreatePipelineLayout(device, &descriptor);
}

//------------------------------------------------------------------------------
WGPUPipelineLayout vtkWebGPUPipelineLayoutInternals::MakePipelineLayout(
  WGPUDevice device, std::vector<WGPUBindGroupLayout> bgls, std::string label /*=""*/)
{
  WGPUPipelineLayoutDescriptor descriptor = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
  descriptor.label = vtkWebGPUMakeStringView(label);
  descriptor.bindGroupLayoutCount = uint32_t(bgls.size());
  descriptor.bindGroupLayouts = bgls.data();
  return wgpuDeviceCreatePipelineLayout(device, &descriptor);
}
VTK_ABI_NAMESPACE_END
