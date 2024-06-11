// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUInternalsComputeBuffer_h
#define vtkWebGPUInternalsComputeBuffer_h

#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"

VTK_ABI_NAMESPACE_BEGIN

/**
 * Internal utility class for manipulating vtkWebGPUComputeBuffers
 */
class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPUInternalsComputeBuffer
{
public:
  /**
   * Uploads a vtkDataArray to the given wgpuBuffer
   */
  static void UploadFromDataArray(
    wgpu::Device device, wgpu::Buffer wgpuBuffer, vtkDataArray* dataArray);
};

VTK_ABI_NAMESPACE_END
#endif
