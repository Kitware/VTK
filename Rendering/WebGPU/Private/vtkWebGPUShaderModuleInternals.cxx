// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPUShaderModuleInternals.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
wgpu::ShaderModule vtkWebGPUShaderModuleInternals::CreateFromWGSL(
  const wgpu::Device& device, const std::string& source)
{
  wgpu::ShaderSourceWGSL wgslDesc;
  wgslDesc.code = source.c_str();

  wgpu::ShaderModuleDescriptor descriptor;
  descriptor.nextInChain = &wgslDesc;

  return device.CreateShaderModule(&descriptor);
}

//------------------------------------------------------------------------------
wgpu::ShaderModule vtkWebGPUShaderModuleInternals::CreateFromSPIRV(
  const wgpu::Device& device, const uint32_t* code)
{
  wgpu::ShaderSourceSPIRV sprivDescriptor;
  sprivDescriptor.code = code;

  wgpu::ShaderModuleDescriptor descriptor;
  descriptor.nextInChain = &sprivDescriptor;

  return device.CreateShaderModule(&descriptor);
}
VTK_ABI_NAMESPACE_END
