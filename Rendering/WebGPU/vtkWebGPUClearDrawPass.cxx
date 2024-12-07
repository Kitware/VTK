// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUClearDrawPass.h"
#include "Private/vtkWebGPURenderPassDescriptorInternals.h"
#include "vtkObjectFactory.h"
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkWebGPURenderWindow.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPUClearDrawPass);

//------------------------------------------------------------------------------
vtkWebGPUClearDrawPass::vtkWebGPUClearDrawPass() = default;

//------------------------------------------------------------------------------
vtkWebGPUClearDrawPass::~vtkWebGPUClearDrawPass() = default;

//------------------------------------------------------------------------------
void vtkWebGPUClearDrawPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
wgpu::RenderPassEncoder vtkWebGPUClearDrawPass::Begin(const vtkRenderState* state)
{
  auto wgpuRenderWindow =
    vtkWebGPURenderWindow::SafeDownCast(state->GetRenderer()->GetRenderWindow());

  std::vector<wgpu::TextureView> backBufferViews = {
    wgpuRenderWindow->GetOffscreenColorAttachmentView()
  };
  wgpu::TextureView depthStencilView = wgpuRenderWindow->GetDepthStencilView();

  vtkWebGPURenderPassDescriptorInternals renderPassDescriptor(
    backBufferViews, depthStencilView, this->ClearColor, this->ClearDepth, this->ClearStencil);
  renderPassDescriptor.label = "vtkWebGPUClearDrawPass::Begin";
  return wgpuRenderWindow->NewRenderPass(renderPassDescriptor);
}

//------------------------------------------------------------------------------
void vtkWebGPUClearDrawPass::Render(const vtkRenderState* state)
{
  if (!state->IsValid())
  {
    return;
  }
  this->End(state, this->Begin(state));
}
VTK_ABI_NAMESPACE_END
