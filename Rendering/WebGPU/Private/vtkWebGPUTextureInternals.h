// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUTextureInternals_h
#define vtkWebGPUTextureInternals_h

#include "vtkDataArray.h"
#include "vtkRenderingWebGPUModule.h"
#include "vtkWebGPUConfiguration.h"
#include "vtk_wgpu.h"

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPUTextureInternals
{
public:
  /**
   * Upload byteSize of data from the data pointer to the given texture using the given device and
   * assuming bytesPerRow bytes of data per row of the texture.
   */
  static void Upload(vtkSmartPointer<vtkWebGPUConfiguration> wgpuConfiguration,
    wgpu::Texture texture, uint32_t bytesPerRow, uint32_t byteSize, const void* data,
    const char* description = nullptr);

  /**
   * Uploads a maximum of bytesToUpload from a vtkDataArray to a texture assuming bytesPerRow bytes
   * of data per row of the texture.
   */
  static void UploadFromDataArray(vtkSmartPointer<vtkWebGPUConfiguration> wgpuConfiguration,
    wgpu::Texture texture, uint32_t bytesPerRow, vtkDataArray* dataArray,
    const char* description = nullptr);

  /**
   * Get the image copy texture from the given texture for use in uploading data to the texture
   */
  static wgpu::TexelCopyTextureInfo GetTexelCopyTextureInfo(wgpu::Texture texture);

  /**
   * Get the texture data layout from the given texture and bytes per row for use in uploading data
   * to the texture
   */
  static wgpu::TexelCopyBufferLayout GetDataLayout(wgpu::Texture texture, uint32_t bytesPerRow);
};

VTK_ABI_NAMESPACE_END

#endif
