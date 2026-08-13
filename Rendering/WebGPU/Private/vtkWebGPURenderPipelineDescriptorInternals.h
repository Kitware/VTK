// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPURenderPipelineDescriptorInternals_h
#define vtkWebGPURenderPipelineDescriptorInternals_h

#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"

#include <array>

VTK_ABI_NAMESPACE_BEGIN
/**
 * A WGPURenderPipelineDescriptor that owns the storage its pointer members refer
 * to, and that starts out with VTK's preferred defaults rather than the plain C
 * zero-initialization.
 *
 * It derives from the C descriptor so that `&descriptor` can be handed straight
 * to the C API - no cast is needed or wanted.
 */
class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPURenderPipelineDescriptorInternals
  : public WGPURenderPipelineDescriptor
{
public:
  static constexpr int kMaxVertexBuffers = 8u;
  static constexpr int kMaxVertexAttributes = 16u;
  static constexpr int kMaxColorAttachments = 8u;

  vtkWebGPURenderPipelineDescriptorInternals();

  vtkWebGPURenderPipelineDescriptorInternals(
    const vtkWebGPURenderPipelineDescriptorInternals&) = delete;
  vtkWebGPURenderPipelineDescriptorInternals& operator=(
    const vtkWebGPURenderPipelineDescriptorInternals&) = delete;
  vtkWebGPURenderPipelineDescriptorInternals(vtkWebGPURenderPipelineDescriptorInternals&&) = delete;
  vtkWebGPURenderPipelineDescriptorInternals& operator=(
    vtkWebGPURenderPipelineDescriptorInternals&&) = delete;

  WGPUDepthStencilState* EnableDepthStencil(
    WGPUTextureFormat format = WGPUTextureFormat_Depth24PlusStencil8);
  void DisableDepthStencil();

  WGPUBlendState* EnableBlending(std::size_t colorTargetId);
  void DisableBlending(std::size_t colorTargetId);

  std::array<WGPUVertexBufferLayout, kMaxVertexBuffers> cBuffers;
  std::array<WGPUVertexAttribute, kMaxVertexAttributes> cAttributes;
  std::array<WGPUColorTargetState, kMaxColorAttachments> cTargets;
  std::array<WGPUBlendState, kMaxColorAttachments> cBlends;

  WGPUFragmentState cFragment;
  WGPUDepthStencilState cDepthStencil;
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPURenderPipelineDescriptorInternals.h
