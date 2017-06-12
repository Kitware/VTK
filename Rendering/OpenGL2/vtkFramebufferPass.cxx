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
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkTextureObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLError.h"
// #include "vtkShaderProgram.h"
// #include "vtkOpenGLShaderCache.h"
// #include "vtkOpenGLRenderWindow.h"
// #include "vtkOpenGLVertexArrayObject.h"

#include "vtkOpenGLHelper.h"

vtkStandardNewMacro(vtkFramebufferPass);

// ----------------------------------------------------------------------------
vtkFramebufferPass::vtkFramebufferPass()
{
  this->FrameBufferObject=0;
  this->ColorTexture = vtkTextureObject::New();
  this->DepthTexture = vtkTextureObject::New();
  this->DepthFormat = vtkTextureObject::Float32;
}

// ----------------------------------------------------------------------------
vtkFramebufferPass::~vtkFramebufferPass()
{
  if(this->FrameBufferObject!=0)
  {
    vtkErrorMacro(<<"FrameBufferObject should have been deleted in ReleaseGraphicsResources().");
  }
   if(this->ColorTexture!=0)
   {
    this->ColorTexture->Delete();
    this->ColorTexture = nullptr;
   }
   if(this->DepthTexture !=0)
   {
    this->DepthTexture->Delete();
    this->DepthTexture = nullptr;
   }
}

// ----------------------------------------------------------------------------
void vtkFramebufferPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkFramebufferPass::Render(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  vtkOpenGLClearErrorMacro();

  this->NumberOfRenderedProps=0;

  vtkRenderer *r=s->GetRenderer();
  vtkOpenGLRenderWindow *renWin = static_cast<vtkOpenGLRenderWindow *>(r->GetRenderWindow());

  if(this->DelegatePass == 0)
  {
    vtkWarningMacro(<<" no delegate.");
    return;
  }

  // 1. Create a new render state with an FO.

  int size[2];
  s->GetWindowSize(size);

  this->ColorTexture->SetContext(renWin);
  if(this->ColorTexture->GetWidth() != static_cast<unsigned int>(size[0]) ||
     this->ColorTexture->GetHeight() != static_cast<unsigned int>(size[1]))
  {
    this->ColorTexture->Create2D(static_cast<unsigned int>(size[0]),
                          static_cast<unsigned int>(size[1]),4,
                          VTK_UNSIGNED_CHAR, false);
  }

  // Depth texture
  this->DepthTexture->SetContext(renWin);
  if (this->DepthTexture->GetWidth() != static_cast<unsigned int> (size[0])
      || this->DepthTexture->GetHeight() != static_cast<unsigned int> (size[1]))
  {
    this->DepthTexture->AllocateDepth(
      size[0], size[1], this->DepthFormat);
  }

  if(this->FrameBufferObject==0)
  {
    this->FrameBufferObject=vtkOpenGLFramebufferObject::New();
    this->FrameBufferObject->SetContext(renWin);
  }

  this->FrameBufferObject->SaveCurrentBindingsAndBuffers();
  this->RenderDelegate(s, size[0], size[1], size[0], size[1],
    this->FrameBufferObject,
    this->ColorTexture, this->DepthTexture);

  this->FrameBufferObject->RestorePreviousBindingsAndBuffers();


  // now copy the result to the outer FO
  this->FrameBufferObject->SaveCurrentBindingsAndBuffers(
    this->FrameBufferObject->GetReadMode());
  this->FrameBufferObject->Bind(
    this->FrameBufferObject->GetReadMode());

  glBlitFramebuffer(
    0, 0, size[0], size[1],
    0, 0, size[0], size[1],
    GL_COLOR_BUFFER_BIT,
    GL_LINEAR);

  this->FrameBufferObject->RestorePreviousBindingsAndBuffers(
    this->FrameBufferObject->GetReadMode());

  vtkOpenGLCheckErrorMacro("failed after Render");
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkFramebufferPass::ReleaseGraphicsResources(vtkWindow *w)
{
  assert("pre: w_exists" && w!=0);

  this->Superclass::ReleaseGraphicsResources(w);

  if(this->FrameBufferObject!=0)
  {
    this->FrameBufferObject->Delete();
    this->FrameBufferObject=0;
  }
   if(this->ColorTexture!=0)
   {
    this->ColorTexture->ReleaseGraphicsResources(w);
   }
   if(this->DepthTexture!=0)
   {
    this->DepthTexture->ReleaseGraphicsResources(w);
   }
}
