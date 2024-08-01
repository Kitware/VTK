// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUShaderModuleInternals_h
#define vtkWebGPUShaderModuleInternals_h

#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"

#include <string>

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPUShaderModuleInternals
{
public:
  static wgpu::ShaderModule CreateFromWGSL(const wgpu::Device& device, const std::string& source);
  static wgpu::ShaderModule CreateFromSPIRV(const wgpu::Device& device, const uint32_t* code);
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPUShaderModuleInternals.h
