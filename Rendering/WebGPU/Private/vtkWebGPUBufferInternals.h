// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUBufferInternals_h
#define vtkWebGPUBufferInternals_h

#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPUBufferInternals
{
public:
  // Check whether the given device can create a buffer that is sizeBytes big.
  static bool CheckBufferSize(const wgpu::Device& device, unsigned long sizeBytes);
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPUBufferInternals.h
