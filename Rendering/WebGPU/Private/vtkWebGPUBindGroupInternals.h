// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUBindGroupInternals_h
#define vtkWebGPUBindGroupInternals_h

#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"

#include <string>
#include <vector>

#include <initializer_list>

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPUBindGroupInternals
{
public:
  // Helpers to make creating bind groups look nicer:
  //
  //   vtkWebGPUBindGroupInternals::MakeBindGroup(
  //       device,
  //       layout,
  //   {
  //       {0, mySampler},
  //       {1, myBuffer, offset, size},
  //       {3, myTextureView}
  //   });

  // Structure with one constructor per-type of bindings, so that the initializer_list accepts
  // bindings with the right type and no extra information.
  struct BindingInitializationHelper
  {
    BindingInitializationHelper(uint32_t binding, WGPUSampler sampler);
    BindingInitializationHelper(uint32_t binding, WGPUTextureView textureView);
    BindingInitializationHelper(
      uint32_t binding, WGPUBuffer buffer, uint64_t offset = 0, uint64_t size = WGPU_WHOLE_SIZE);
    BindingInitializationHelper(const BindingInitializationHelper&);
    ~BindingInitializationHelper();

    WGPUBindGroupEntry GetAsBinding() const;

    uint32_t binding;
    WGPUSampler sampler = nullptr;
    WGPUTextureView textureView = nullptr;
    WGPUBuffer buffer = nullptr;
    uint64_t offset = 0;
    uint64_t size = 0;
  };

  /**
   * Creates a bind group given the bind group layout and a list of BindGroupEntry
   */
  static WGPUBindGroup MakeBindGroup(const WGPUDevice& device, const WGPUBindGroupLayout& layout,
    std::initializer_list<BindingInitializationHelper> entriesInitializer, std::string label = "");

  /**
   * Creates a bind group given the bind group layout and a list of BindGroupEntry
   */
  static WGPUBindGroup MakeBindGroup(const WGPUDevice& device, const WGPUBindGroupLayout& layout,
    const std::vector<WGPUBindGroupEntry>& entries, std::string label = "");
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPUBindGroupInternals.h
