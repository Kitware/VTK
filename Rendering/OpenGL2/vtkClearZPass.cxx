// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkClearZPass.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLState.h"
#include "vtkRenderState.h"
#include "vtk_glad.h"
#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkClearZPass);

//------------------------------------------------------------------------------
vtkClearZPass::vtkClearZPass()
{
  this->Depth = 1.0;
}

//------------------------------------------------------------------------------
vtkClearZPass::~vtkClearZPass() = default;

//------------------------------------------------------------------------------
void vtkClearZPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Depth:" << this->Depth << endl;
}

//------------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkClearZPass::Render(const vtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);
  (void)s;
  this->NumberOfRenderedProps = 0;

  vtkOpenGLState* ostate = static_cast<vtkOpenGLRenderer*>(s->GetRenderer())->GetState();
  ostate->vtkglDepthMask(GL_TRUE);
  ostate->vtkglClearDepth(this->Depth);
  ostate->vtkglClear(GL_DEPTH_BUFFER_BIT);
}
VTK_ABI_NAMESPACE_END
