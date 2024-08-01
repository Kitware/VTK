// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUTextureInternals_h
#define vtkWebGPUTextureInternals_h

#include "vtkDataArray.h"
#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPUTextureInternals
{
public:
  /**
   * Creates a WebGPU texture with the given device and returns it.
   */
  static wgpu::Texture CreateATexture(const wgpu::Device& device, wgpu::Extent3D extents,
    wgpu::TextureDimension dimension, wgpu::TextureFormat format, wgpu::TextureUsage usage,
    int mipLevelCount = 1, std::string label = "");

  /**
   * Creates a texture view of a texture
   */
  static wgpu::TextureView CreateATextureView(const wgpu::Device& device, wgpu::Texture texture,
    wgpu::TextureViewDimension dimension, wgpu::TextureAspect aspect, wgpu::TextureFormat format,
    int baseMipLevel, int mipLevelCount, std::string label);

  /**
   * Upload byteSize of data from the data pointer to the given texture using the given device and
   * assuming bytesPerRow bytes of data per row of the texture.
   */
  static void Upload(const wgpu::Device& device, wgpu::Texture texture, uint32_t bytesPerRow,
    uint32_t byteSize, const void* data);

  /**
   * Uploads a maximum of bytesToUpload from a vtkDataArray to a texure assuming bytesPerRow bytes
   * of data per row of the texture.
   */
  static void UploadFromDataArray(const wgpu::Device& device, wgpu::Texture texture,
    uint32_t bytesPerRow, vtkDataArray* dataArray);

  /**
   * Get the image copy texture from the given texture for use in uploading data to the texture
   */
  static wgpu::ImageCopyTexture GetImageCopyTexture(wgpu::Texture texture);

  /**
   * Get the texture data layout from the given texture and bytes per row for use in uploading data
   * to the texture
   */
  static wgpu::TextureDataLayout GetDataLayout(wgpu::Texture texture, uint32_t bytesPerRow);
};

VTK_ABI_NAMESPACE_END

#endif
