// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUInternalsBindGroupLayout_h
#define vtkWebGPUInternalsBindGroupLayout_h

#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"

#include <initializer_list>

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPUInternalsBindGroupLayout
{
public:
  // Helpers to make creating bind group layouts look nicer:
  //
  //   vtkWebGPUInternalsBindGroupLayout::MakeBindGroupLayout(device, {
  //       {0, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::Uniform},
  //       {1, wgpu::ShaderStage::Fragment, wgpu::SamplerBindingType::Filtering},
  //       {3, wgpu::ShaderStage::Fragment, wgpu::TextureSampleType::Float}
  //   });
  struct LayoutEntryInitializationHelper : wgpu::BindGroupLayoutEntry
  {
    // for buffers
    LayoutEntryInitializationHelper(uint32_t entryBinding, wgpu::ShaderStage entryVisibility,
      wgpu::BufferBindingType bufferType, bool bufferHasDynamicOffset = false,
      uint64_t bufferMinBindingSize = 0);
    // for samplers
    LayoutEntryInitializationHelper(uint32_t entryBinding, wgpu::ShaderStage entryVisibility,
      wgpu::SamplerBindingType samplerType);
    // for texture
    LayoutEntryInitializationHelper(uint32_t entryBinding, wgpu::ShaderStage entryVisibility,
      wgpu::TextureSampleType textureSampleType,
      wgpu::TextureViewDimension viewDimension = wgpu::TextureViewDimension::e2D,
      bool textureMultisampled = false);
    // for storage buffers
    LayoutEntryInitializationHelper(uint32_t entryBinding, wgpu::ShaderStage entryVisibility,
      wgpu::StorageTextureAccess storageTextureAccess, wgpu::TextureFormat format,
      wgpu::TextureViewDimension viewDimension = wgpu::TextureViewDimension::e2D);

    LayoutEntryInitializationHelper(const wgpu::BindGroupLayoutEntry& entry);
  };

  static wgpu::BindGroupLayout MakeBindGroupLayout(const wgpu::Device& device,
    std::initializer_list<LayoutEntryInitializationHelper> entriesInitializer);
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPUInternalsBindGroupLayout.h
