// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkTranslucentPass.h"
#include "vtkObjectFactory.h"
#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTranslucentPass);

//------------------------------------------------------------------------------
vtkTranslucentPass::vtkTranslucentPass() = default;

//------------------------------------------------------------------------------
vtkTranslucentPass::~vtkTranslucentPass() = default;

//------------------------------------------------------------------------------
void vtkTranslucentPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkTranslucentPass::Render(const vtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  this->NumberOfRenderedProps = 0;
  this->RenderFilteredTranslucentPolygonalGeometry(s);
}
VTK_ABI_NAMESPACE_END
