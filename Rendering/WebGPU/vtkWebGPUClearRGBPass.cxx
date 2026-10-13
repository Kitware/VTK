// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUClearRGBPass.h"
#include "vtkObjectFactory.h"
#include "vtkOverrideAttribute.h"
#include "vtkRenderState.h"
#include "vtkRenderer.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkWebGPUClearRGBPass);

//------------------------------------------------------------------------------
vtkWebGPUClearRGBPass::vtkWebGPUClearRGBPass() = default;

//------------------------------------------------------------------------------
vtkWebGPUClearRGBPass::~vtkWebGPUClearRGBPass() = default;

//------------------------------------------------------------------------------
vtkOverrideAttribute* vtkWebGPUClearRGBPass::CreateOverrideAttributes()
{
  auto* renderingBackendAttribute =
    vtkOverrideAttribute::CreateAttributeChain("RenderingBackend", "WebGPU", nullptr);
  return renderingBackendAttribute;
}

//------------------------------------------------------------------------------
void vtkWebGPUClearRGBPass::Render(const vtkRenderState* s)
{
  this->NumberOfRenderedProps = 0;

  s->GetRenderer()->Clear();
}

//------------------------------------------------------------------------------
void vtkWebGPUClearRGBPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
