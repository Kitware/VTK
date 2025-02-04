// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPURenderPipelineDescriptorInternals_h
#define vtkWebGPURenderPipelineDescriptorInternals_h

#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"

#include <array>

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPURenderPipelineDescriptorInternals
  : public wgpu::RenderPipelineDescriptor
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

  wgpu::DepthStencilState* EnableDepthStencil(
    wgpu::TextureFormat format = wgpu::TextureFormat::Depth24PlusStencil8);
  void DisableDepthStencil();

  wgpu::BlendState* EnableBlending(std::size_t colorTargetId);
  void DisableBlending(std::size_t colorTargetId);

  std::array<wgpu::VertexBufferLayout, kMaxVertexBuffers> cBuffers;
  std::array<wgpu::VertexAttribute, kMaxVertexAttributes> cAttributes;
  std::array<wgpu::ColorTargetState, kMaxColorAttachments> cTargets;
  std::array<wgpu::BlendState, kMaxColorAttachments> cBlends;

  wgpu::FragmentState cFragment;
  wgpu::DepthStencilState cDepthStencil;
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPURenderPipelineDescriptorInternals.h
