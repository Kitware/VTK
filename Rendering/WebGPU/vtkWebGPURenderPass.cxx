// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPURenderPass.h"
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkWebGPURenderWindow.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkWebGPURenderPass::vtkWebGPURenderPass() = default;

//------------------------------------------------------------------------------
vtkWebGPURenderPass::~vtkWebGPURenderPass() = default;

//------------------------------------------------------------------------------
void vtkWebGPURenderPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkWebGPURenderPass::End(const vtkRenderState*, wgpu::RenderPassEncoder&& pass)
{
  pass.End();
  pass = nullptr;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderPass::Render(const vtkRenderState*) {}
VTK_ABI_NAMESPACE_END
