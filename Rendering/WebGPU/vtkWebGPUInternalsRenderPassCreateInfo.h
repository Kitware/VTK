// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUInternalsRenderPassCreateInfo_h
#define vtkWebGPUInternalsRenderPassCreateInfo_h

#include "vtkRenderingWebGPUModule.h"
#include "vtkWebGPUInternalsRenderPassDescriptor.h"
#include "vtk_wgpu.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPUInternalsRenderPassCreateInfo
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
