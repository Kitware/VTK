// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUPipelineLayoutInternals_h
#define vtkWebGPUPipelineLayoutInternals_h

#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPUPipelineLayoutInternals
{
public:
  static wgpu::PipelineLayout MakeBasicPipelineLayout(const wgpu::Device& device,
    const wgpu::BindGroupLayout* bindGroupLayout, std::string label = "");

  static wgpu::PipelineLayout MakePipelineLayout(
    const wgpu::Device& device, std::vector<wgpu::BindGroupLayout> bgls, std::string label = "");
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPUPipelineLayoutInternals.h
