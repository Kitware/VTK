// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLClearRGBPass.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLState.h"
#include "vtkOverrideAttribute.h"
#include "vtkRenderState.h"
#include "vtk_glad.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenGLClearRGBPass);

//------------------------------------------------------------------------------
vtkOpenGLClearRGBPass::vtkOpenGLClearRGBPass() = default;

//------------------------------------------------------------------------------
vtkOpenGLClearRGBPass::~vtkOpenGLClearRGBPass() = default;

//------------------------------------------------------------------------------
vtkOverrideAttribute* vtkOpenGLClearRGBPass::CreateOverrideAttributes()
{
  auto* renderingBackendAttribute =
    vtkOverrideAttribute::CreateAttributeChain("RenderingBackend", "OpenGL", nullptr);
  return renderingBackendAttribute;
}

//------------------------------------------------------------------------------
void vtkOpenGLClearRGBPass::Render(const vtkRenderState* s)
{
  this->NumberOfRenderedProps = 0;

  vtkOpenGLState* ostate = static_cast<vtkOpenGLRenderer*>(s->GetRenderer())->GetState();
  ostate->vtkglClearColor(static_cast<GLclampf>(this->Background[0]),
    static_cast<GLclampf>(this->Background[1]), static_cast<GLclampf>(this->Background[2]),
    static_cast<GLclampf>(0.0));
  ostate->vtkglClear(GL_COLOR_BUFFER_BIT);
}

//------------------------------------------------------------------------------
void vtkOpenGLClearRGBPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
