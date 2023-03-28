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
#include "vtkWebGPUInternalsRenderPassCreateInfo.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkWebGPUInternalsRenderPassCreateInfo::vtkWebGPUInternalsRenderPassCreateInfo()
  : width(0)
  , height(0)
  , color(nullptr)
  , colorFormat(wgpu::TextureFormat::RGBA8Unorm)
  , renderPassInfo({})
{
}

//------------------------------------------------------------------------------
vtkWebGPUInternalsRenderPassCreateInfo::vtkWebGPUInternalsRenderPassCreateInfo(uint32_t texWidth,
  uint32_t texHeight, wgpu::Texture colorAttachment, wgpu::TextureFormat textureFormat)
  : width(texWidth)
  , height(texHeight)
  , color(colorAttachment)
  , colorFormat(textureFormat)
  , renderPassInfo({ colorAttachment.CreateView() })
{
}

//------------------------------------------------------------------------------
vtkWebGPUInternalsRenderPassCreateInfo
vtkWebGPUInternalsRenderPassCreateInfo::CreateBasicRenderPass(
  const wgpu::Device& device, uint32_t width, uint32_t height, wgpu::TextureFormat format)
{
  wgpu::TextureDescriptor descriptor;
  descriptor.dimension = wgpu::TextureDimension::e2D;
  descriptor.size.width = width;
  descriptor.size.height = height;
  descriptor.size.depthOrArrayLayers = 1;
  descriptor.sampleCount = 1;
  descriptor.format = format;
  descriptor.mipLevelCount = 1;
  descriptor.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc;
  wgpu::Texture colorAttachment = device.CreateTexture(&descriptor);

  return vtkWebGPUInternalsRenderPassCreateInfo(width, height, colorAttachment);
}
VTK_ABI_NAMESPACE_END
