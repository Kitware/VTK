/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFramebufferPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFramebufferPass.h"
#include "vtkObjectFactory.h"
#include <cassert>

// #include "vtkCamera.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLState.h"
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkTextureObject.h"
// #include "vtkShaderProgram.h"
// #include "vtkOpenGLShaderCache.h"
// #include "vtkOpenGLRenderWindow.h"
// #include "vtkOpenGLVertexArrayObject.h"

#include "vtkOpenGLHelper.h"

vtkStandardNewMacro(vtkFramebufferPass);

// ----------------------------------------------------------------------------
vtkFramebufferPass::vtkFramebufferPass()
{
  this->FrameBufferObject = nullptr;
  this->ColorTexture = vtkTextureObject::New();
  this->DepthTexture = vtkTextureObject::New();
  this->DepthFormat = vtkTextureObject::Float32;
  this->ColorFormat = vtkTextureObject::Fixed8;
}

// ----------------------------------------------------------------------------
vtkFramebufferPass::~vtkFramebufferPass()
{
  if (this->FrameBufferObject != nullptr)
  {
    vtkErrorMacro(<< "FrameBufferObject should have been deleted in ReleaseGraphicsResources().");
  }
  if (this->ColorTexture != nullptr)
  {
    this->ColorTexture->Delete();
    this->ColorTexture = nullptr;
  }
  if (this->DepthTexture != nullptr)
  {
    this->DepthTexture->Delete();
    this->DepthTexture = nullptr;
  }
}

// ----------------------------------------------------------------------------
void vtkFramebufferPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkFramebufferPass::Render(const vtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  vtkOpenGLClearErrorMacro();

  this->NumberOfRenderedProps = 0;

  vtkRenderer* r = s->GetRenderer();
  vtkOpenGLRenderWindow* renWin = static_cast<vtkOpenGLRenderWindow*>(r->GetRenderWindow());
  vtkOpenGLState* ostate = renWin->GetState();

  if (this->DelegatePass == nullptr)
  {
    vtkWarningMacro(<< " no delegate.");
    return;
  }

  // 1. Create a new render state with an FO.
  if (s->GetFrameBuffer() == nullptr)
  {
    // get the viewport dimensions
    r->GetTiledSizeAndOrigin(
      &this->ViewportWidth, &this->ViewportHeight, &this->ViewportX, &this->ViewportY);
  }
  else
  {
    int size[2];
    s->GetWindowSize(size);
    this->ViewportWidth = size[0];
    this->ViewportHeight = size[1];
    this->ViewportX = 0;
    this->ViewportY = 0;
  }

  this->ColorTexture->SetContext(renWin);
  if (!this->ColorTexture->GetHandle())
  {
    if (this->ColorFormat == vtkTextureObject::Float16)
    {
      this->ColorTexture->SetInternalFormat(GL_RGBA16F);
      this->ColorTexture->SetDataType(GL_FLOAT);
    }
    if (this->ColorFormat == vtkTextureObject::Float32)
    {
      this->ColorTexture->SetInternalFormat(GL_RGBA32F);
      this->ColorTexture->SetDataType(GL_FLOAT);
    }
    this->ColorTexture->Create2D(
      this->ViewportWidth, this->ViewportHeight, 4, VTK_UNSIGNED_CHAR, false);
  }
  this->ColorTexture->Resize(this->ViewportWidth, this->ViewportHeight);

  // Depth texture
  this->DepthTexture->SetContext(renWin);
  if (!this->DepthTexture->GetHandle())
  {
    this->DepthTexture->AllocateDepth(this->ViewportWidth, this->ViewportHeight, this->DepthFormat);
  }
  this->DepthTexture->Resize(this->ViewportWidth, this->ViewportHeight);

  if (this->FrameBufferObject == nullptr)
  {
    this->FrameBufferObject = vtkOpenGLFramebufferObject::New();
    this->FrameBufferObject->SetContext(renWin);
  }

  ostate->PushFramebufferBindings();
  this->RenderDelegate(s, this->ViewportWidth, this->ViewportHeight, this->ViewportWidth,
    this->ViewportHeight, this->FrameBufferObject, this->ColorTexture, this->DepthTexture);

  ostate->PopFramebufferBindings();

  // now copy the result to the outer FO
  ostate->PushReadFramebufferBinding();
  this->FrameBufferObject->Bind(this->FrameBufferObject->GetReadMode());

  ostate->vtkglViewport(
    this->ViewportX, this->ViewportY, this->ViewportWidth, this->ViewportHeight);
  ostate->vtkglScissor(this->ViewportX, this->ViewportY, this->ViewportWidth, this->ViewportHeight);

  glBlitFramebuffer(0, 0, this->ViewportWidth, this->ViewportHeight, this->ViewportX,
    this->ViewportY, this->ViewportX + this->ViewportWidth, this->ViewportY + this->ViewportHeight,
    GL_COLOR_BUFFER_BIT, GL_LINEAR);

  ostate->PopReadFramebufferBinding();

  vtkOpenGLCheckErrorMacro("failed after Render");
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkFramebufferPass::ReleaseGraphicsResources(vtkWindow* w)
{
  assert("pre: w_exists" && w != nullptr);

  this->Superclass::ReleaseGraphicsResources(w);

  if (this->FrameBufferObject != nullptr)
  {
    this->FrameBufferObject->Delete();
    this->FrameBufferObject = nullptr;
  }
  if (this->ColorTexture != nullptr)
  {
    this->ColorTexture->ReleaseGraphicsResources(w);
  }
  if (this->DepthTexture != nullptr)
  {
    this->DepthTexture->ReleaseGraphicsResources(w);
  }
}
