// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUBindGroupLayoutInternals_h
#define vtkWebGPUBindGroupLayoutInternals_h

#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"

#include <initializer_list>

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPUBindGroupLayoutInternals
{
public:
  // Helpers to make creating bind group layouts look nicer:
  //
  //   vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device, {
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

  /**
   * Creates the bind group layout from a list of bind group layout entries
   */
  static wgpu::BindGroupLayout MakeBindGroupLayout(const wgpu::Device& device,
    std::initializer_list<LayoutEntryInitializationHelper> entriesInitializer,
    std::string label = "");

  /**
   * Creates the bind group layout from a list of bind group layout entries
   */
  static wgpu::BindGroupLayout MakeBindGroupLayout(const wgpu::Device& device,
    const std::vector<wgpu::BindGroupLayoutEntry>& entries, std::string label = "");
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPUBindGroupLayoutInternals.h
