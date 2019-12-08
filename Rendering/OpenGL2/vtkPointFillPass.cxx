/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointFillPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPointFillPass.h"
#include "vtkObjectFactory.h"
#include <cassert>

#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLQuadHelper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"

#include "vtkPointFillPassFS.h"
#include "vtkTextureObjectVS.h"

vtkStandardNewMacro(vtkPointFillPass);

// ----------------------------------------------------------------------------
vtkPointFillPass::vtkPointFillPass()
{
  this->FrameBufferObject = nullptr;
  this->Pass1 = nullptr;
  this->Pass1Depth = nullptr;
  this->QuadHelper = nullptr;
  this->MinimumCandidateAngle = 1.5 * vtkMath::Pi();
  this->CandidatePointRatio = 0.99;
}

// ----------------------------------------------------------------------------
vtkPointFillPass::~vtkPointFillPass()
{
  if (this->FrameBufferObject != nullptr)
  {
    vtkErrorMacro(<< "FrameBufferObject should have been deleted in ReleaseGraphicsResources().");
  }
  if (this->Pass1 != nullptr)
  {
    vtkErrorMacro(<< "Pass1 should have been deleted in ReleaseGraphicsResources().");
  }
  if (this->Pass1Depth != nullptr)
  {
    vtkErrorMacro(<< "Pass1Depth should have been deleted in ReleaseGraphicsResources().");
  }
}

// ----------------------------------------------------------------------------
void vtkPointFillPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkPointFillPass::Render(const vtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  vtkOpenGLClearErrorMacro();

  this->NumberOfRenderedProps = 0;

  vtkRenderer* r = s->GetRenderer();
  vtkOpenGLRenderWindow* renWin = static_cast<vtkOpenGLRenderWindow*>(r->GetRenderWindow());

  if (this->DelegatePass == nullptr)
  {
    vtkWarningMacro(<< " no delegate.");
    return;
  }

  // 1. Create a new render state with an FBO.

  int width;
  int height;
  int size[2];
  s->GetWindowSize(size);
  width = size[0];
  height = size[1];

  if (this->Pass1 == nullptr)
  {
    this->Pass1 = vtkTextureObject::New();
    this->Pass1->SetContext(renWin);
    this->Pass1->Create2D(static_cast<unsigned int>(width), static_cast<unsigned int>(height), 4,
      VTK_UNSIGNED_CHAR, false);
  }
  this->Pass1->Resize(width, height);

  // Depth texture
  if (this->Pass1Depth == nullptr)
  {
    this->Pass1Depth = vtkTextureObject::New();
    this->Pass1Depth->SetContext(renWin);
    this->Pass1Depth->AllocateDepth(width, height, vtkTextureObject::Float32);
  }
  this->Pass1Depth->Resize(width, height);

  if (this->FrameBufferObject == nullptr)
  {
    this->FrameBufferObject = vtkOpenGLFramebufferObject::New();
    this->FrameBufferObject->SetContext(renWin);
  }

  renWin->GetState()->PushFramebufferBindings();
  this->RenderDelegate(
    s, width, height, width, height, this->FrameBufferObject, this->Pass1, this->Pass1Depth);
  renWin->GetState()->PopFramebufferBindings();

  // has something changed that would require us to recreate the shader?
  if (!this->QuadHelper)
  {
    // build the shader source code
    // compile and bind it if needed
    this->QuadHelper = new vtkOpenGLQuadHelper(renWin, nullptr, vtkPointFillPassFS, "");
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->QuadHelper->Program);
  }

  if (!this->QuadHelper->Program)
  {
    return;
  }

  renWin->GetState()->vtkglDisable(GL_BLEND);
  //  glDisable(GL_DEPTH_TEST);

  this->Pass1->Activate();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  this->QuadHelper->Program->SetUniformi("source", this->Pass1->GetTextureUnit());

  this->Pass1Depth->Activate();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  this->QuadHelper->Program->SetUniformi("depth", this->Pass1Depth->GetTextureUnit());

  vtkCamera* cam = r->GetActiveCamera();
  double* frange = cam->GetClippingRange();
  this->QuadHelper->Program->SetUniformf("nearC", frange[0]);
  this->QuadHelper->Program->SetUniformf("farC", frange[1]);
  this->QuadHelper->Program->SetUniformf("MinimumCandidateAngle", this->MinimumCandidateAngle);
  this->QuadHelper->Program->SetUniformf("CandidatePointRatio", this->CandidatePointRatio);
  float offset[2];
  offset[0] = 1.0 / width;
  offset[1] = 1.0 / height;
  this->QuadHelper->Program->SetUniform2f("pixelToTCoord", offset);

  this->QuadHelper->Render();
  this->Pass1->Deactivate();
  this->Pass1Depth->Deactivate();

  vtkOpenGLCheckErrorMacro("failed after Render");
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkPointFillPass::ReleaseGraphicsResources(vtkWindow* w)
{
  assert("pre: w_exists" && w != nullptr);

  this->Superclass::ReleaseGraphicsResources(w);

  if (this->QuadHelper != nullptr)
  {
    delete this->QuadHelper;
    this->QuadHelper = nullptr;
  }
  if (this->FrameBufferObject != nullptr)
  {
    this->FrameBufferObject->Delete();
    this->FrameBufferObject = nullptr;
  }
  if (this->Pass1 != nullptr)
  {
    this->Pass1->Delete();
    this->Pass1 = nullptr;
  }
  if (this->Pass1Depth != nullptr)
  {
    this->Pass1Depth->Delete();
    this->Pass1Depth = nullptr;
  }
}
