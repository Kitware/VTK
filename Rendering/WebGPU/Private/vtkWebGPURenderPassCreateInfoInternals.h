// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPURenderPassCreateInfoInternals_h
#define vtkWebGPURenderPassCreateInfoInternals_h

#include "Private/vtkWebGPURenderPassDescriptorInternals.h"
#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPURenderPassCreateInfoInternals
{
public:
  vtkWebGPURenderPassCreateInfoInternals();
  vtkWebGPURenderPassCreateInfoInternals(uint32_t width, uint32_t height, wgpu::Texture color,
    wgpu::TextureFormat texture = DefaultColorFormat);

  static constexpr wgpu::TextureFormat DefaultColorFormat = wgpu::TextureFormat::RGBA8Unorm;

  static vtkWebGPURenderPassCreateInfoInternals CreateBasicRenderPass(const wgpu::Device& device,
    uint32_t width, uint32_t height, wgpu::TextureFormat format = DefaultColorFormat);

  uint32_t width;
  uint32_t height;
  wgpu::Texture color;
  wgpu::TextureFormat colorFormat;
  vtkWebGPURenderPassDescriptorInternals renderPassInfo;
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPURenderPassCreateInfoInternals.h
