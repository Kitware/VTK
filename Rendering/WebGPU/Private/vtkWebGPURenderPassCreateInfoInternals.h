// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPURenderPassCreateInfoInternals_h
#define vtkWebGPURenderPassCreateInfoInternals_h

#include "Private/vtkWebGPUHandle.h"
#include "Private/vtkWebGPURenderPassDescriptorInternals.h"
#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPURenderPassCreateInfoInternals
{
public:
  vtkWebGPURenderPassCreateInfoInternals();
  vtkWebGPURenderPassCreateInfoInternals(uint32_t width, uint32_t height, WGPUTexture color,
    WGPUTextureFormat texture = DefaultColorFormat);

  static constexpr WGPUTextureFormat DefaultColorFormat = WGPUTextureFormat_RGBA8Unorm;

  static vtkWebGPURenderPassCreateInfoInternals CreateBasicRenderPass(WGPUDevice device,
    uint32_t width, uint32_t height, WGPUTextureFormat format = DefaultColorFormat);

  uint32_t width;
  uint32_t height;
  vtkWebGPU::Texture color;
  WGPUTextureFormat colorFormat;
  // renderPassInfo only borrows the view, so this keeps it alive. Declared
  // before renderPassInfo so it is constructed first.
  vtkWebGPU::TextureView colorView;
  vtkWebGPURenderPassDescriptorInternals renderPassInfo;
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPURenderPassCreateInfoInternals.h
