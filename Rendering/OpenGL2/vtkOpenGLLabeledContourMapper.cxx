/*==============================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLLabeledContourMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

==============================================================================*/
#include "vtkOpenGLLabeledContourMapper.h"

#include "vtkActor.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLTexture.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkTextActor3D.h"
#include "vtkOpenGLHelper.h"



//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLLabeledContourMapper)

//------------------------------------------------------------------------------
void vtkOpenGLLabeledContourMapper::PrintSelf(std::ostream &os,
                                              vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkOpenGLLabeledContourMapper::vtkOpenGLLabeledContourMapper()
{
  this->StencilBO =  new vtkOpenGLHelper;
  this->TempMatrix4 = vtkMatrix4x4::New();
}

//------------------------------------------------------------------------------
vtkOpenGLLabeledContourMapper::~vtkOpenGLLabeledContourMapper()
{
  delete this->StencilBO;
  this->StencilBO = 0;
  this->TempMatrix4->Delete();
}

//------------------------------------------------------------------------------
bool vtkOpenGLLabeledContourMapper::CreateLabels(vtkActor *actor)
{
  if (!this->Superclass::CreateLabels(actor))
  {
    return false;
  }

  if (vtkMatrix4x4 *actorMatrix = actor->GetMatrix())
  {
    for (vtkIdType i = 0; i < this->NumberOfUsedTextActors; ++i)
    {
      vtkMatrix4x4 *labelMatrix = this->TextActors[i]->GetUserMatrix();
      vtkMatrix4x4::Multiply4x4(actorMatrix, labelMatrix, labelMatrix);
      this->TextActors[i]->SetUserMatrix(labelMatrix);
    }
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkOpenGLLabeledContourMapper::ReleaseGraphicsResources(vtkWindow *win)
{
  this->Superclass::ReleaseGraphicsResources(win);
  this->StencilBO->ReleaseGraphicsResources(win);
}

//------------------------------------------------------------------------------
bool vtkOpenGLLabeledContourMapper::ApplyStencil(vtkRenderer *ren,
                                                 vtkActor *act)
{
  // Draw stencil quads into stencil buffer:
  // compile and bind it if needed
  vtkOpenGLRenderWindow *renWin =
    vtkOpenGLRenderWindow::SafeDownCast(ren->GetVTKWindow());

  if (!this->StencilBO->Program)
  {
    this->StencilBO->Program  =
        renWin->GetShaderCache()->ReadyShaderProgram(
        // vertex shader
        "//VTK::System::Dec\n"
        "attribute vec4 vertexMC;\n"
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
  GLboolean colorMask[4];
  glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);
  GLboolean depthMask;
  glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);

  // Enable rendering into the stencil buffer:
  glEnable(GL_STENCIL_TEST);
  glStencilMask(0xFF);
  glClearStencil(0);
  glClear(GL_STENCIL_BUFFER_BIT);
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  glDepthMask(GL_FALSE);
  glStencilFunc(GL_ALWAYS, 1, 0xFF);
  glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

  vtkOpenGLCamera *cam = (vtkOpenGLCamera *)(ren->GetActiveCamera());
  vtkMatrix4x4 *wcdc;
  vtkMatrix4x4 *wcvc;
  vtkMatrix3x3 *norms;
  vtkMatrix4x4 *vcdc;
  cam->GetKeyMatrices(ren,wcvc,norms,vcdc,wcdc);
  if (!act->GetIsIdentity())
  {
    vtkMatrix4x4 *mcwc;
    vtkMatrix3x3 *anorms;
    ((vtkOpenGLActor *)act)->GetKeyMatrices(mcwc,anorms);
    vtkMatrix4x4::Multiply4x4(mcwc, wcdc, this->TempMatrix4);
    this->StencilBO->Program->SetUniformMatrix("MCDCMatrix", this->TempMatrix4);
  }
  else
  {
    this->StencilBO->Program->SetUniformMatrix("MCDCMatrix", wcdc);
  }

  vtkOpenGLRenderUtilities::RenderTriangles(
    this->StencilQuads,
    this->StencilQuadsSize/3,
    this->StencilQuadIndices,
    this->StencilQuadIndicesSize,
    NULL,
    this->StencilBO->Program,
    this->StencilBO->VAO);

  // Restore state:
  glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
  glDepthMask(depthMask);

  // Setup GL to only draw in unstenciled regions:
  glStencilMask(0x00);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  glStencilFunc(GL_EQUAL, 0, 0xFF);

  vtkOpenGLCheckErrorMacro("failed after ApplyStencil()");

  return this->Superclass::ApplyStencil(ren, act);
}

//------------------------------------------------------------------------------
bool vtkOpenGLLabeledContourMapper::RemoveStencil()
{
  glDisable(GL_STENCIL_TEST);
  vtkOpenGLCheckErrorMacro("failed after RemoveStencil()");
  return this->Superclass::RemoveStencil();
}
