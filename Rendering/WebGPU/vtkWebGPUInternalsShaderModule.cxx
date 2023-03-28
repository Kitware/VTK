/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebGPUInternalsShaderModule.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWebGPUInternalsShaderModule.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
wgpu::ShaderModule vtkWebGPUInternalsShaderModule::CreateFromWGSL(
  const wgpu::Device& device, const std::string& source)
{
  wgpu::ShaderModuleWGSLDescriptor wgslDesc;
  wgslDesc.source = source.c_str();

  wgpu::ShaderModuleDescriptor descriptor;
  descriptor.nextInChain = &wgslDesc;

  return device.CreateShaderModule(&descriptor);
}

//------------------------------------------------------------------------------
wgpu::ShaderModule vtkWebGPUInternalsShaderModule::CreateFromSPIRV(
  const wgpu::Device& device, const uint32_t* code)
{
  wgpu::ShaderModuleSPIRVDescriptor sprivDescriptor;
  sprivDescriptor.code = code;

  wgpu::ShaderModuleDescriptor descriptor;
  descriptor.nextInChain = &sprivDescriptor;

  return device.CreateShaderModule(&descriptor);
}
VTK_ABI_NAMESPACE_END
