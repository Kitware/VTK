// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpaquePass.h"
#include "vtkObjectFactory.h"
#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpaquePass);

//------------------------------------------------------------------------------
vtkOpaquePass::vtkOpaquePass() = default;

//------------------------------------------------------------------------------
vtkOpaquePass::~vtkOpaquePass() = default;

//------------------------------------------------------------------------------
void vtkOpaquePass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkOpaquePass::Render(const vtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  this->NumberOfRenderedProps = 0;
  this->RenderFilteredOpaqueGeometry(s);
}
VTK_ABI_NAMESPACE_END
