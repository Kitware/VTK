/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebGPUInternalsRenderPassCreateInfo.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkWebGPUInternalsRenderPassCreateInfo_h
#define vtkWebGPUInternalsRenderPassCreateInfo_h

#include "vtkRenderingWebGPUModule.h"
#include "vtkWebGPUInternalsRenderPassDescriptor.h"
#include "vtk_wgpu.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUInternalsRenderPassCreateInfo
{
public:
  vtkWebGPUInternalsRenderPassCreateInfo();
  vtkWebGPUInternalsRenderPassCreateInfo(uint32_t width, uint32_t height, wgpu::Texture color,
    wgpu::TextureFormat texture = DefaultColorFormat);

  static constexpr wgpu::TextureFormat DefaultColorFormat = wgpu::TextureFormat::RGBA8Unorm;

  static vtkWebGPUInternalsRenderPassCreateInfo CreateBasicRenderPass(const wgpu::Device& device,
    uint32_t width, uint32_t height, wgpu::TextureFormat format = DefaultColorFormat);

  uint32_t width;
  uint32_t height;
  wgpu::Texture color;
  wgpu::TextureFormat colorFormat;
  vtkWebGPUInternalsRenderPassDescriptor renderPassInfo;
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPUInternalsRenderPassCreateInfo.h
