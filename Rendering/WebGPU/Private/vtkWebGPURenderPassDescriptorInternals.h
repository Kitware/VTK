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
  : public WGPURenderPassDescriptor
{
public:
  static constexpr int kMaxColorAttachments = 8u;
  vtkWebGPURenderPassDescriptorInternals(const std::vector<WGPUTextureView>& colorAttachmentInfo,
    WGPUTextureView depthStencil = nullptr, bool clearColor = true, bool clearDepth = true,
    bool clearStencil = true);
  ~vtkWebGPURenderPassDescriptorInternals();

  vtkWebGPURenderPassDescriptorInternals(
    const vtkWebGPURenderPassDescriptorInternals& otherRenderPass);
  const vtkWebGPURenderPassDescriptorInternals& operator=(
    const vtkWebGPURenderPassDescriptorInternals& otherRenderPass);

  void UnsetDepthStencilLoadStoreOpsForFormat(WGPUTextureFormat format);

  std::array<WGPURenderPassColorAttachment, kMaxColorAttachments> ColorAttachments;
  WGPURenderPassDepthStencilAttachment DepthStencilAttachmentInfo =
    WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_INIT;
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPURenderPassDescriptorInternals.h
