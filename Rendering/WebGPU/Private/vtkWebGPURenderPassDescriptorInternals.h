// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPURenderPassDescriptorInternals_h
#define vtkWebGPURenderPassDescriptorInternals_h

#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"

#include <array>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPURenderPassDescriptorInternals
  : public wgpu::RenderPassDescriptor
{
public:
  static constexpr int kMaxColorAttachments = 8u;
  vtkWebGPURenderPassDescriptorInternals(const std::vector<wgpu::TextureView>& colorAttachmentInfo,
    wgpu::TextureView depthStencil = wgpu::TextureView(), bool clearColor = true,
    bool clearDepth = true, bool clearStencil = true);
  ~vtkWebGPURenderPassDescriptorInternals();

  vtkWebGPURenderPassDescriptorInternals(
    const vtkWebGPURenderPassDescriptorInternals& otherRenderPass);
  const vtkWebGPURenderPassDescriptorInternals& operator=(
    const vtkWebGPURenderPassDescriptorInternals& otherRenderPass);

  void UnsetDepthStencilLoadStoreOpsForFormat(wgpu::TextureFormat format);

  std::array<wgpu::RenderPassColorAttachment, kMaxColorAttachments> ColorAttachments;
  wgpu::RenderPassDepthStencilAttachment DepthStencilAttachmentInfo = {};
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPURenderPassDescriptorInternals.h
