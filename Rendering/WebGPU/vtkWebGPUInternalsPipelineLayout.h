/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebGPUInternalsPipelineLayout.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkWebGPUInternalsPipelineLayout_h
#define vtkWebGPUInternalsPipelineLayout_h

#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUInternalsPipelineLayout
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
