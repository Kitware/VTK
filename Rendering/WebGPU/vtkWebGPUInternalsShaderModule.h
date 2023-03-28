/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebGPUInternalsShaderModule.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkWebGPUInternalsShaderModule_h
#define vtkWebGPUInternalsShaderModule_h

#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"

#include <string>

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUInternalsShaderModule
{
public:
  static wgpu::ShaderModule CreateFromWGSL(const wgpu::Device& device, const std::string& source);
  static wgpu::ShaderModule CreateFromSPIRV(const wgpu::Device& device, const uint32_t* code);
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPUInternalsShaderModule.h
