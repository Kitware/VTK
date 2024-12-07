// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUComputeBufferInternals_h
#define vtkWebGPUComputeBufferInternals_h

#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"

VTK_ABI_NAMESPACE_BEGIN

/**
 * Internal utility class for manipulating vtkWebGPUComputeBuffers
 */
class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPUComputeBufferInternals
{
public:
  /**
   * Uploads a vtkDataArray to the given wgpuBuffer
   */
  static void UploadFromDataArray(
    wgpu::Device device, wgpu::Buffer wgpuBuffer, vtkDataArray* dataArray);

  /**
   * Uploads a vtkDataArray with offset to the given wgpuBuffer
   */
  static void UploadFromDataArray(
    wgpu::Device device, wgpu::Buffer wgpuBuffer, vtkIdType byteOffset, vtkDataArray* dataArray);
};

VTK_ABI_NAMESPACE_END

#endif
