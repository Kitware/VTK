// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUBufferInternals_h
#define vtkWebGPUBufferInternals_h

#include "vtkDataArray.h"
#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPUBufferInternals
{
public:
  static wgpu::Buffer Upload(const wgpu::Device& device, unsigned long offset, void* data,
    unsigned long sizeBytes, wgpu::BufferUsage usage, const char* label = nullptr);

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

  static wgpu::Buffer CreateBuffer(const wgpu::Device& device, unsigned long sizeBytes,
    wgpu::BufferUsage usage, bool mappedAtCreation = false, const char* label = nullptr);

  // Check whether the given device can create a buffer that is sizeBytes big.
  static bool CheckBufferSize(const wgpu::Device& device, unsigned long sizeBytes);
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPUBufferInternals.h
