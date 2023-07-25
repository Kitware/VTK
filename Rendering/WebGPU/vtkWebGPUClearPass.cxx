// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUClearPass.h"
#include "vtkObjectFactory.h"
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkWebGPUInternalsRenderPassDescriptor.h"
#include "vtkWebGPURenderWindow.h"
#include <chrono>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPUClearPass);

//------------------------------------------------------------------------------
vtkWebGPUClearPass::vtkWebGPUClearPass() = default;

//------------------------------------------------------------------------------
vtkWebGPUClearPass::~vtkWebGPUClearPass() = default;

//------------------------------------------------------------------------------
void vtkWebGPUClearPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
wgpu::RenderPassEncoder vtkWebGPUClearPass::Begin(const vtkRenderState* state)
{
  auto renderer = state->GetRenderer();
  auto wgpuRenWin = vtkWebGPURenderWindow::SafeDownCast(state->GetRenderer()->GetRenderWindow());

  std::vector<wgpu::TextureView> backBufferViews = {
    wgpuRenWin->GetOffscreenColorAttachmentView()
  };
  wgpu::TextureView depthStencilView = wgpuRenWin->GetDepthStencilView();

  vtkWebGPUInternalsRenderPassDescriptor renderPassDescriptor(backBufferViews, depthStencilView);
  renderPassDescriptor.label = "vtkWebGPUClearPass::Begin";

  double* bgColor = renderer->GetBackground();
  for (auto& colorAttachment : renderPassDescriptor.ColorAttachments)
  {
    colorAttachment.clearValue.r = bgColor[0];
    colorAttachment.clearValue.g = bgColor[1];
    colorAttachment.clearValue.b = bgColor[2];
    colorAttachment.clearValue.a = 1.0f;
  }
  return wgpuRenWin->NewRenderPass(renderPassDescriptor);
}

//------------------------------------------------------------------------------
void vtkWebGPUClearPass::Render(const vtkRenderState* state)
{
  if (!state->IsValid())
  {
    return;
  }
  this->End(state, this->Begin(state));
}
VTK_ABI_NAMESPACE_END
