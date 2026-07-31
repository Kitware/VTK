// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPUShaderModuleInternals.h"

#include "Private/vtkWebGPUHelpersPrivate.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
WGPUShaderModule vtkWebGPUShaderModuleInternals::CreateFromWGSL(
  WGPUDevice device, const std::string& source)
{
  WGPUShaderSourceWGSL wgslDesc = WGPU_SHADER_SOURCE_WGSL_INIT;
  wgslDesc.code = vtkWebGPUMakeStringView(source);

  WGPUShaderModuleDescriptor descriptor = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
  descriptor.nextInChain = &wgslDesc.chain;

  return wgpuDeviceCreateShaderModule(device, &descriptor);
}

//------------------------------------------------------------------------------
WGPUShaderModule vtkWebGPUShaderModuleInternals::CreateFromSPIRV(
  WGPUDevice device, const uint32_t* code)
{
  WGPUShaderSourceSPIRV sprivDescriptor = WGPU_SHADER_SOURCE_SPIRV_INIT;
  sprivDescriptor.code = code;

  WGPUShaderModuleDescriptor descriptor = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
  descriptor.nextInChain = &sprivDescriptor.chain;

  return wgpuDeviceCreateShaderModule(device, &descriptor);
}
VTK_ABI_NAMESPACE_END
