// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLLabeledContourMapper.h"

#include "vtkActor.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLHelper.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLTexture.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkTextActor3D.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenGLLabeledContourMapper);

//------------------------------------------------------------------------------
void vtkOpenGLLabeledContourMapper::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkOpenGLLabeledContourMapper::vtkOpenGLLabeledContourMapper()
{
  this->StencilBO = new vtkOpenGLHelper;
  this->TempMatrix4 = vtkMatrix4x4::New();
}

//------------------------------------------------------------------------------
vtkOpenGLLabeledContourMapper::~vtkOpenGLLabeledContourMapper()
{
  delete this->StencilBO;
  this->StencilBO = nullptr;
  this->TempMatrix4->Delete();
}

//------------------------------------------------------------------------------
bool vtkOpenGLLabeledContourMapper::CreateLabels(vtkActor* actor)
{
  if (!this->Superclass::CreateLabels(actor))
  {
    return false;
  }

  if (vtkMatrix4x4* actorMatrix = actor->GetMatrix())
  {
    for (vtkIdType i = 0; i < this->NumberOfUsedTextActors; ++i)
    {
      vtkMatrix4x4* labelMatrix = this->TextActors[i]->GetUserMatrix();
      vtkMatrix4x4::Multiply4x4(actorMatrix, labelMatrix, labelMatrix);
      this->TextActors[i]->SetUserMatrix(labelMatrix);
    }
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkOpenGLLabeledContourMapper::ReleaseGraphicsResources(vtkWindow* win)
{
  this->Superclass::ReleaseGraphicsResources(win);
  this->StencilBO->ReleaseGraphicsResources(win);
}

//------------------------------------------------------------------------------
bool vtkOpenGLLabeledContourMapper::ApplyStencil(vtkRenderer* ren, vtkActor* act)
{
  if (this->StencilQuadsSize == 0)
  {
    return true;
  }

  // Draw stencil quads into stencil buffer:
  // compile and bind it if needed
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetVTKWindow());
  vtkOpenGLState* ostate = renWin->GetState();

  if (!this->StencilBO->Program)
  {
    this->StencilBO->Program = renWin->GetShaderCache()->ReadyShaderProgram(
      // vertex shader
      "//VTK::System::Dec\n"
      "in vec4 vertexMC;\n"
      "uniform mat4 MCDCMatrix;\n"
      "void main() { gl_Position = MCDCMatrix*vertexMC; }\n",
      // fragment shader
      "//VTK::System::Dec\n"
      "//VTK::Output::Dec\n"
      "void main() { gl_FragData[0] = vec4(1.0,1.0,1.0,1.0); }",
      // geometry shader
      "");
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->StencilBO->Program);
  }

  if (!this->StencilBO->Program)
  {
    return false;
  }

  // Save some state:
  {
    vtkOpenGLState::ScopedglColorMask cmsaver(ostate);
    vtkOpenGLState::ScopedglDepthMask dmsaver(ostate);

    // Enable rendering into the stencil buffer:
    ostate->vtkglEnable(GL_STENCIL_TEST);
    ostate->vtkglStencilMask(0xFF);
    glClearStencil(0);
    ostate->vtkglClear(GL_STENCIL_BUFFER_BIT);
    ostate->vtkglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    ostate->vtkglDepthMask(GL_FALSE);
    ostate->vtkglStencilFunc(GL_ALWAYS, 1, 0xFF);
    ostate->vtkglStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

    vtkOpenGLCamera* cam = (vtkOpenGLCamera*)(ren->GetActiveCamera());
    vtkMatrix4x4* wcdc;
    vtkMatrix4x4* wcvc;
    vtkMatrix3x3* norms;
    vtkMatrix4x4* vcdc;
    cam->GetKeyMatrices(ren, wcvc, norms, vcdc, wcdc);
    if (!act->GetIsIdentity())
    {
      vtkMatrix4x4* mcwc;
      vtkMatrix3x3* anorms;
      ((vtkOpenGLActor*)act)->GetKeyMatrices(mcwc, anorms);
      vtkMatrix4x4::Multiply4x4(mcwc, wcdc, this->TempMatrix4);
      this->StencilBO->Program->SetUniformMatrix("MCDCMatrix", this->TempMatrix4);
    }
    else
    {
      this->StencilBO->Program->SetUniformMatrix("MCDCMatrix", wcdc);
    }

    vtkOpenGLRenderUtilities::RenderTriangles(this->StencilQuads, this->StencilQuadsSize / 3,
      this->StencilQuadIndices, this->StencilQuadIndicesSize, nullptr, this->StencilBO->Program,
      this->StencilBO->VAO);
  }

  // Setup GL to only draw in unstenciled regions:
  ostate->vtkglStencilMask(0x00);
  ostate->vtkglStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  ostate->vtkglStencilFunc(GL_EQUAL, 0, 0xFF);

  vtkOpenGLCheckErrorMacro("failed after ApplyStencil()");

  return this->Superclass::ApplyStencil(ren, act);
}

//------------------------------------------------------------------------------
bool vtkOpenGLLabeledContourMapper::RemoveStencil(vtkRenderer* ren)
{
  static_cast<vtkOpenGLRenderWindow*>(ren->GetVTKWindow())
    ->GetState()
    ->vtkglDisable(GL_STENCIL_TEST);
  vtkOpenGLCheckErrorMacro("failed after RemoveStencil()");
  return this->Superclass::RemoveStencil(ren);
}
VTK_ABI_NAMESPACE_END
