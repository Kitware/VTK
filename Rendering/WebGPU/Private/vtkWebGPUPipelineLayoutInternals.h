// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUPipelineLayoutInternals_h
#define vtkWebGPUPipelineLayoutInternals_h

#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"

#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPUPipelineLayoutInternals
{
public:
  static WGPUPipelineLayout MakeBasicPipelineLayout(
    WGPUDevice device, const WGPUBindGroupLayout* bindGroupLayout, std::string label = "");

  static WGPUPipelineLayout MakePipelineLayout(
    WGPUDevice device, std::vector<WGPUBindGroupLayout> bgls, std::string label = "");
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPUPipelineLayoutInternals.h
