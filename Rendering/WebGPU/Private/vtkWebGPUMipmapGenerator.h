// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkWebGPUMipmapGenerator
 * @brief Fills the mip chain of a WebGPU texture from its first level.
 */

#ifndef vtkWebGPUMipmapGenerator_h
#define vtkWebGPUMipmapGenerator_h

#include "vtkABINamespace.h" // for VTK_ABI_NAMESPACE_BEGIN
#include "vtk_wgpu.h"        // for the WebGPU C API

#include <cstdint> // for std::uint32_t

VTK_ABI_NAMESPACE_BEGIN
class vtkWebGPUConfiguration;

class vtkWebGPUMipmapGenerator
{
public:
  static void Generate(vtkWebGPUConfiguration* configuration, WGPUTexture texture,
    WGPUTextureFormat format, std::uint32_t mipLevelCount, std::uint32_t layerCount);
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkWebGPUMipmapGenerator.h
