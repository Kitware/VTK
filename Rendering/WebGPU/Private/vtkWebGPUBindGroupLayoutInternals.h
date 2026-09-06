// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUBindGroupLayoutInternals_h
#define vtkWebGPUBindGroupLayoutInternals_h

#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"

#include <string>
#include <vector>

#include <initializer_list>

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPUBindGroupLayoutInternals
{
public:
  // Helpers to make creating bind group layouts look nicer:
  //
  //   vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device, {
  //       {0, WGPUShaderStage_Vertex, WGPUBufferBindingType_Uniform},
  //       {1, WGPUShaderStage_Fragment, WGPUSamplerBindingType_Filtering},
  //       {3, WGPUShaderStage_Fragment, WGPUTextureSampleType_Float}
  //   });
  struct LayoutEntryInitializationHelper : WGPUBindGroupLayoutEntry
  {
    // for buffers
    LayoutEntryInitializationHelper(uint32_t entryBinding, WGPUShaderStage entryVisibility,
      WGPUBufferBindingType bufferType, bool bufferHasDynamicOffset = false,
      uint64_t bufferMinBindingSize = 0);
    // for samplers
    LayoutEntryInitializationHelper(
      uint32_t entryBinding, WGPUShaderStage entryVisibility, WGPUSamplerBindingType samplerType);
    // for texture
    LayoutEntryInitializationHelper(uint32_t entryBinding, WGPUShaderStage entryVisibility,
      WGPUTextureSampleType textureSampleType,
      WGPUTextureViewDimension viewDimension = WGPUTextureViewDimension_2D,
      bool textureMultisampled = false);
    // for storage buffers
    LayoutEntryInitializationHelper(uint32_t entryBinding, WGPUShaderStage entryVisibility,
      WGPUStorageTextureAccess storageTextureAccess, WGPUTextureFormat format,
      WGPUTextureViewDimension viewDimension = WGPUTextureViewDimension_2D);

    LayoutEntryInitializationHelper(const WGPUBindGroupLayoutEntry& entry);
  };

  ///@{
  /**
   * Creates the bind group layout from a list of bind group layout entries.
   * The returned handle is owned by the caller.
   */
  static WGPUBindGroupLayout MakeBindGroupLayout(const WGPUDevice& device,
    std::initializer_list<LayoutEntryInitializationHelper> entriesInitializer,
    std::string label = "");

  static WGPUBindGroupLayout MakeBindGroupLayout(const WGPUDevice& device,
    const std::vector<WGPUBindGroupLayoutEntry>& entries, std::string label = "");
  ///@}
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPUBindGroupLayoutInternals.h
