/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClearZPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkClearZPass.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLState.h"
#include "vtkRenderState.h"
#include "vtk_glew.h"
#include <cassert>

vtkStandardNewMacro(vtkClearZPass);

// ----------------------------------------------------------------------------
vtkClearZPass::vtkClearZPass()
{
  this->Depth = 1.0;
}

// ----------------------------------------------------------------------------
vtkClearZPass::~vtkClearZPass() = default;

// ----------------------------------------------------------------------------
void vtkClearZPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Depth:" << this->Depth << endl;
}

// ----------------------------------------------------------------------------
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
