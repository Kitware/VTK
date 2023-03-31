/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebGPURenderPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
