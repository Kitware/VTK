/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimpleMotionBlurPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSimpleMotionBlurPass.h"
#include "vtkObjectFactory.h"
#include <cassert>

#include "vtkOpenGLError.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"

#include "vtkOpenGLHelper.h"

#include "vtkSimpleMotionBlurPassFS.h"
#include "vtkTextureObjectVS.h"

vtkStandardNewMacro(vtkSimpleMotionBlurPass);

// ----------------------------------------------------------------------------
vtkSimpleMotionBlurPass::vtkSimpleMotionBlurPass()
{
  this->SubFrames = 30;
  this->CurrentSubFrame = 0;
  this->BlendProgram = nullptr;

  this->FrameBufferObject = nullptr;
  this->AccumulationTexture[0] = vtkTextureObject::New();
  this->AccumulationTexture[1] = vtkTextureObject::New();
  this->ActiveAccumulationTexture = 0;
  this->ColorTexture = vtkTextureObject::New();
  this->DepthTexture = vtkTextureObject::New();
  this->DepthFormat = vtkTextureObject::Float32;
  this->ColorFormat = vtkTextureObject::Fixed8;
}

// ----------------------------------------------------------------------------
vtkSimpleMotionBlurPass::~vtkSimpleMotionBlurPass()
{
  if (this->FrameBufferObject != nullptr)
  {
    vtkErrorMacro(<< "FrameBufferObject should have been deleted in ReleaseGraphicsResources().");
  }
  if (this->AccumulationTexture[0] != nullptr)
  {
    this->AccumulationTexture[0]->Delete();
    this->AccumulationTexture[0] = nullptr;
  }
  if (this->AccumulationTexture[1] != nullptr)
  {
    this->AccumulationTexture[1]->Delete();
    this->AccumulationTexture[1] = nullptr;
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

//----------------------------------------------------------------------------
void vtkSimpleMotionBlurPass::SetSubFrames(int subFrames)
{
  if (this->SubFrames != subFrames)
  {
    this->SubFrames = subFrames;
    if (this->CurrentSubFrame >= this->SubFrames)
    {
      this->CurrentSubFrame = 0;
    }
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting SubFrames to "
                  << subFrames);
    this->Modified();
  }
}

// ----------------------------------------------------------------------------
void vtkSimpleMotionBlurPass::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "SubFrames: " << this->SubFrames << "\n";
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkSimpleMotionBlurPass::Render(const vtkRenderState* s)
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
    this->ColorTexture->Allocate2D(this->ViewportWidth, this->ViewportHeight, 4, VTK_UNSIGNED_CHAR);
  }
  this->ColorTexture->Resize(this->ViewportWidth, this->ViewportHeight);

  for (int i = 0; i < 2; i++)
  {
    this->AccumulationTexture[i]->SetContext(renWin);
    if (!this->AccumulationTexture[i]->GetHandle())
    {
      this->AccumulationTexture[i]->SetInternalFormat(GL_RGBA16F);
      this->AccumulationTexture[i]->SetDataType(GL_FLOAT);
      this->AccumulationTexture[i]->Allocate2D(
        this->ViewportWidth, this->ViewportHeight, 4, VTK_UNSIGNED_CHAR);
    }
    this->AccumulationTexture[i]->Resize(this->ViewportWidth, this->ViewportHeight);
  }
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

  renWin->GetState()->PushFramebufferBindings();
  this->RenderDelegate(s, this->ViewportWidth, this->ViewportHeight, this->ViewportWidth,
    this->ViewportHeight, this->FrameBufferObject, this->ColorTexture, this->DepthTexture);

  // has something changed that would require us to recreate the shader?
  if (!this->BlendProgram)
  {
    this->BlendProgram = new vtkOpenGLHelper;
    // build the shader source code
    std::string VSSource = vtkTextureObjectVS;
    std::string FSSource = vtkSimpleMotionBlurPassFS;
    std::string GSSource;

    // compile and bind it if needed
    vtkShaderProgram* newShader = renWin->GetShaderCache()->ReadyShaderProgram(
      VSSource.c_str(), FSSource.c_str(), GSSource.c_str());

    // if the shader changed reinitialize the VAO
    if (newShader != this->BlendProgram->Program)
    {
      this->BlendProgram->Program = newShader;
      this->BlendProgram->VAO->ShaderProgramChanged(); // reset the VAO as the shader has changed
    }

    this->BlendProgram->ShaderSourceTime.Modified();
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->BlendProgram->Program);
  }

  this->FrameBufferObject->AddColorAttachment(
    0, this->AccumulationTexture[this->ActiveAccumulationTexture]);

  ostate->vtkglViewport(0, 0, this->ViewportWidth, this->ViewportHeight);
  ostate->vtkglScissor(0, 0, this->ViewportWidth, this->ViewportHeight);

  // clear the accumulator on 0
  if (this->CurrentSubFrame == 0)
  {
    ostate->vtkglClearColor(0.0, 0.0, 0.0, 0.0);
    ostate->vtkglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    ostate->vtkglClear(GL_COLOR_BUFFER_BIT);
  }

  this->ColorTexture->Activate();
  int sourceId = this->ColorTexture->GetTextureUnit();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  this->BlendProgram->Program->SetUniformi("source", sourceId);
  this->BlendProgram->Program->SetUniformf("blendScale", 1.0 / this->SubFrames);
  ostate->vtkglDisable(GL_DEPTH_TEST);

  // save off current state of src / dst blend functions
  // local scope for bfsaver
  {
    vtkOpenGLState::ScopedglBlendFuncSeparate bfsaver(ostate);
    ostate->vtkglBlendFunc(GL_ONE, GL_ONE);
    this->FrameBufferObject->RenderQuad(0, this->ViewportWidth - 1, 0, this->ViewportHeight - 1,
      this->BlendProgram->Program, this->BlendProgram->VAO);
    this->ColorTexture->Deactivate();
    // restore blend func on scope exit
  }

  // blit either the last or the current FO
  this->CurrentSubFrame++;
  if (this->CurrentSubFrame < this->SubFrames)
  {
    this->FrameBufferObject->AddColorAttachment(
      0, this->AccumulationTexture[this->ActiveAccumulationTexture == 0 ? 1 : 0]);
  }
  else
  {
    this->CurrentSubFrame = 0;
    this->ActiveAccumulationTexture = (this->ActiveAccumulationTexture == 0 ? 1 : 0);
  }

  renWin->GetState()->PopFramebufferBindings();

  // now copy the result to the outer FO
  renWin->GetState()->PushReadFramebufferBinding();
  this->FrameBufferObject->Bind(this->FrameBufferObject->GetReadMode());

  ostate->vtkglViewport(
    this->ViewportX, this->ViewportY, this->ViewportWidth, this->ViewportHeight);
  ostate->vtkglScissor(this->ViewportX, this->ViewportY, this->ViewportWidth, this->ViewportHeight);

  glBlitFramebuffer(0, 0, this->ViewportWidth, this->ViewportHeight, this->ViewportX,
    this->ViewportY, this->ViewportX + this->ViewportWidth, this->ViewportY + this->ViewportHeight,
    GL_COLOR_BUFFER_BIT, GL_LINEAR);

  renWin->GetState()->PopReadFramebufferBinding();

  vtkOpenGLCheckErrorMacro("failed after Render");
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkSimpleMotionBlurPass::ReleaseGraphicsResources(vtkWindow* w)
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
  if (this->AccumulationTexture[0] != nullptr)
  {
    this->AccumulationTexture[0]->ReleaseGraphicsResources(w);
  }
  if (this->AccumulationTexture[1] != nullptr)
  {
    this->AccumulationTexture[1]->ReleaseGraphicsResources(w);
  }
  if (this->BlendProgram != nullptr)
  {
    this->BlendProgram->ReleaseGraphicsResources(w);
    delete this->BlendProgram;
    this->BlendProgram = nullptr;
  }
}
