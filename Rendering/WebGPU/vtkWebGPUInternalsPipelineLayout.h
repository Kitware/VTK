// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUInternalsPipelineLayout_h
#define vtkWebGPUInternalsPipelineLayout_h

#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPUInternalsPipelineLayout
{
public:
  static wgpu::PipelineLayout MakeBasicPipelineLayout(
    const wgpu::Device& device, const wgpu::BindGroupLayout* bindGroupLayout);

  static wgpu::PipelineLayout MakePipelineLayout(
    const wgpu::Device& device, std::vector<wgpu::BindGroupLayout> bgls);
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPUInternalsPipelineLayout.h
