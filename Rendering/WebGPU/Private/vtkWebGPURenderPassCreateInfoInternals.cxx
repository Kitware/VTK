// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPURenderPassCreateInfoInternals.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkWebGPURenderPassCreateInfoInternals::vtkWebGPURenderPassCreateInfoInternals()
  : width(0)
  , height(0)
  , color(nullptr)
  , colorFormat(WGPUTextureFormat_RGBA8Unorm)
  , renderPassInfo({})
{
}

//------------------------------------------------------------------------------
vtkWebGPURenderPassCreateInfoInternals::vtkWebGPURenderPassCreateInfoInternals(uint32_t texWidth,
  uint32_t texHeight, WGPUTexture colorAttachment, WGPUTextureFormat textureFormat)
  : width(texWidth)
  , height(texHeight)
  , color(colorAttachment)
  , colorFormat(textureFormat)
  , renderPassInfo({ wgpuTextureCreateView(colorAttachment, nullptr) })
{
}

//------------------------------------------------------------------------------
vtkWebGPURenderPassCreateInfoInternals
vtkWebGPURenderPassCreateInfoInternals::CreateBasicRenderPass(
  WGPUDevice device, uint32_t width, uint32_t height, WGPUTextureFormat format)
{
  WGPUTextureDescriptor descriptor = WGPU_TEXTURE_DESCRIPTOR_INIT;
  descriptor.dimension = WGPUTextureDimension_2D;
  descriptor.size.width = width;
  descriptor.size.height = height;
  descriptor.size.depthOrArrayLayers = 1;
  descriptor.sampleCount = 1;
  descriptor.format = format;
  descriptor.mipLevelCount = 1;
  descriptor.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
  WGPUTexture colorAttachment = wgpuDeviceCreateTexture(device, &descriptor);

  return vtkWebGPURenderPassCreateInfoInternals(width, height, colorAttachment);
}
VTK_ABI_NAMESPACE_END
