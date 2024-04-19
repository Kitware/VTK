// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOverlayPass.h"
#include "vtkObjectFactory.h"
#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOverlayPass);

//------------------------------------------------------------------------------
vtkOverlayPass::vtkOverlayPass() = default;

//------------------------------------------------------------------------------
vtkOverlayPass::~vtkOverlayPass() = default;

//------------------------------------------------------------------------------
void vtkOverlayPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkOverlayPass::Render(const vtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  this->NumberOfRenderedProps = 0;
  this->RenderFilteredOverlay(s);
}
VTK_ABI_NAMESPACE_END
