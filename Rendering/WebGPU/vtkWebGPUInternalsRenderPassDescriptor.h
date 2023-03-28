/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebGPUInternalsRenderPassDescriptor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkWebGPUInternalsRenderPassDescriptor_h
#define vtkWebGPUInternalsRenderPassDescriptor_h

#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"

#include <array>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUInternalsRenderPassDescriptor
  : public wgpu::RenderPassDescriptor
{
public:
  static constexpr int kMaxColorAttachments = 8u;
  vtkWebGPUInternalsRenderPassDescriptor(const std::vector<wgpu::TextureView>& colorAttachmentInfo,
    wgpu::TextureView depthStencil = wgpu::TextureView());
  ~vtkWebGPUInternalsRenderPassDescriptor();

  vtkWebGPUInternalsRenderPassDescriptor(
    const vtkWebGPUInternalsRenderPassDescriptor& otherRenderPass);
  const vtkWebGPUInternalsRenderPassDescriptor& operator=(
    const vtkWebGPUInternalsRenderPassDescriptor& otherRenderPass);

  void UnsetDepthStencilLoadStoreOpsForFormat(wgpu::TextureFormat format);

  std::array<wgpu::RenderPassColorAttachment, kMaxColorAttachments> ColorAttachments;
  wgpu::RenderPassDepthStencilAttachment DepthStencilAttachmentInfo = {};
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPUInternalsRenderPassDescriptor.h
