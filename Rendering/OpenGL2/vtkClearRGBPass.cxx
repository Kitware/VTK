// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkClearRGBPass.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLState.h"
#include "vtkRenderState.h"
#include "vtk_glad.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkClearRGBPass);

//------------------------------------------------------------------------------
vtkClearRGBPass::vtkClearRGBPass()
{
  this->Background[0] = 0;
  this->Background[1] = 0;
  this->Background[2] = 0;
}

//------------------------------------------------------------------------------
vtkClearRGBPass::~vtkClearRGBPass() = default;

//------------------------------------------------------------------------------
void vtkClearRGBPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Background:" << this->Background[0] << "," << this->Background[1] << ","
     << this->Background[2] << endl;
}

//------------------------------------------------------------------------------
void vtkClearRGBPass::Render(const vtkRenderState* s)
{
  this->NumberOfRenderedProps = 0;

  vtkOpenGLState* ostate = static_cast<vtkOpenGLRenderer*>(s->GetRenderer())->GetState();
  ostate->vtkglClearColor(static_cast<GLclampf>(this->Background[0]),
    static_cast<GLclampf>(this->Background[1]), static_cast<GLclampf>(this->Background[2]),
    static_cast<GLclampf>(0.0));
  ostate->vtkglClear(GL_COLOR_BUFFER_BIT);
}
VTK_ABI_NAMESPACE_END
