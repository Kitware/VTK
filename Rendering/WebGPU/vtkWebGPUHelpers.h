// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUHelpers_h
#define vtkWebGPUHelpers_h

#include "vtkRenderingWebGPUModule.h"
#include "vtkWebGPUComputePass.h"
#include "vtkWebGPUComputeTexture.h"

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUHelpers
{
public:
  /**
   * Writes a certain mip level of a texture from a compute path to a PNG file on the disk.
   *
   * This function behaves like Dispatch(), ReadBufferFromGPU(), ReadTextureFromGPU(), .. in the
   * sense that it will only be executed after a call to vtkWebGPUComputePipeline::Update().
   *
   * The flipY parameter is used the flip the output along the Y-axis. Useful if the texture's
   * internal storage is reversed along the Y-axis
   */
  static void WriteComputeTextureToDisk(const std::string& filepath,
    vtkSmartPointer<vtkWebGPUComputePass>, int textureIndex, int mipLevel, bool flipY = false);

  static std::string StringViewToStdString(wgpu::StringView sv);

private:
  /**
   * Returns the data type that would be appropriate to create a vtkImageData from the data of a
   * texture. This function can typically be called to get the 'dataType' argument of
   * vtkImageData::AllocateScalars()
   */
  static int ComputeTextureFormatToVTKDataType(vtkWebGPUComputeTexture::TextureFormat format);
};

VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPUHelpers.h
