// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSSAAPass.h"
#include "vtkObjectFactory.h"
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
#include <cassert>

#include "vtkOpenGLHelper.h"

#include "vtkSSAAPassFS.h"
#include "vtkTextureObjectVS.h" // a pass through shader

#include "vtkRenderbuffer.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkSSAAPass);

vtkCxxSetObjectMacro(vtkSSAAPass, DelegatePass, vtkRenderPass);

//------------------------------------------------------------------------------
vtkSSAAPass::vtkSSAAPass()
{
  this->FrameBufferObject = nullptr;
  this->Pass1 = nullptr;
  this->Pass1Depth = nullptr;
  this->Pass2 = nullptr;
  this->DepthTexture1 = nullptr;
  this->DepthTexture2 = nullptr;
  this->SSAAHelper = nullptr;
  this->DelegatePass = nullptr;
  this->ColorFormat = vtkTextureObject::Fixed8;
  this->DepthFormat = vtkTextureObject::Fixed24;
  this->SSAAHelper = new vtkOpenGLHelper;
}

//------------------------------------------------------------------------------
vtkSSAAPass::~vtkSSAAPass()
{
  if (this->DelegatePass != nullptr)
  {
    this->DelegatePass->Delete();
  }

  if (this->FrameBufferObject != nullptr)
  {
    this->FrameBufferObject->Delete();
  }

  if (this->Pass1 != nullptr)
  {
    this->Pass1->Delete();
  }

  if (this->Pass1Depth != nullptr)
  {
    this->Pass1Depth->Delete();
  }

  if (this->Pass2 != nullptr)
  {
    this->Pass2->Delete();
  }

  if (this->DepthTexture1 != nullptr)
  {
    this->DepthTexture1->Delete();
  }

  if (this->DepthTexture2 != nullptr)
  {
    this->DepthTexture2->Delete();
  }

  delete this->SSAAHelper;
}

//------------------------------------------------------------------------------
void vtkSSAAPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "DelegatePass:";
  if (this->DelegatePass != nullptr)
  {
    this->DelegatePass->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
}

//------------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkSSAAPass::Render(const vtkRenderState* s)
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

  // backup GL state
  vtkOpenGLState::ScopedglEnableDisable dsaver(ostate, GL_DEPTH_TEST);
  vtkOpenGLState::ScopedglEnableDisable bsaver(ostate, GL_BLEND);

  // 1. Create a new render state with an FBO.
  int width;
  int height;
  int size[2];
  s->GetWindowSize(size);
  width = size[0];
  height = size[1];

  int modifiedWidth = width * sqrt(5.0);
  int modifiedHeight = height * sqrt(5.0);

  if (this->Pass1 == nullptr)
  {
    this->Pass1 = vtkTextureObject::New();
    this->Pass1->SetContext(renWin);
  }

  if (this->FrameBufferObject == nullptr)
  {
    this->FrameBufferObject = vtkOpenGLFramebufferObject::New();
    this->FrameBufferObject->SetContext(renWin);
  }

  if (this->Pass1->GetWidth() != static_cast<unsigned int>(modifiedWidth) ||
    this->Pass1->GetHeight() != static_cast<unsigned int>(modifiedHeight))
  {
    if (this->ColorFormat == vtkTextureObject::Float16)
    {
      this->Pass1->SetInternalFormat(GL_RGBA16F);
      this->Pass1->SetDataType(GL_FLOAT);
    }
    if (this->ColorFormat == vtkTextureObject::Float32)
    {
      this->Pass1->SetInternalFormat(GL_RGBA32F);
      this->Pass1->SetDataType(GL_FLOAT);
    }
    this->Pass1->Create2D(static_cast<unsigned int>(modifiedWidth),
      static_cast<unsigned int>(modifiedHeight), 4, VTK_UNSIGNED_CHAR, false);
  }

  ostate->PushFramebufferBindings();
  vtkRenderState s2(r);
  s2.SetPropArrayAndCount(s->GetPropArray(), s->GetPropArrayCount());
  s2.SetFrameBuffer(this->FrameBufferObject);
  this->FrameBufferObject->Bind();
  this->FrameBufferObject->AddColorAttachment(0, this->Pass1);
  this->FrameBufferObject->ActivateDrawBuffer(0);

  if (this->DepthTexture1 == nullptr)
  {
    this->DepthTexture1 = vtkTextureObject::New();
    this->DepthTexture1->SetContext(renWin);
  }
  if (!this->DepthTexture1->GetHandle())
  {
    this->DepthTexture1->AllocateDepth(modifiedWidth, modifiedHeight, this->DepthFormat);
    this->DepthTexture1->Activate();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    this->DepthTexture1->Deactivate();
  }
  this->DepthTexture1->Resize(modifiedWidth, modifiedHeight);
  this->FrameBufferObject->AddDepthAttachment(this->DepthTexture1);
  this->FrameBufferObject->StartNonOrtho(modifiedWidth, modifiedHeight);
  ostate->vtkglViewport(0, 0, modifiedWidth, modifiedHeight);
  ostate->vtkglScissor(0, 0, modifiedWidth, modifiedHeight);

  ostate->vtkglEnable(GL_DEPTH_TEST);
  this->DelegatePass->Render(&s2);
  this->NumberOfRenderedProps += this->DelegatePass->GetNumberOfRenderedProps();

  // 3. Same FBO, but new color attachment (new TO).
  if (this->Pass2 == nullptr)
  {
    this->Pass2 = vtkTextureObject::New();
    this->Pass2->SetContext(this->FrameBufferObject->GetContext());
  }

  if (this->Pass2->GetWidth() != static_cast<unsigned int>(width) ||
    this->Pass2->GetHeight() != static_cast<unsigned int>(modifiedHeight))
  {
    if (this->ColorFormat == vtkTextureObject::Float16)
    {
      this->Pass2->SetInternalFormat(GL_RGBA16F);
      this->Pass2->SetDataType(GL_FLOAT);
    }
    if (this->ColorFormat == vtkTextureObject::Float32)
    {
      this->Pass2->SetInternalFormat(GL_RGBA32F);
      this->Pass2->SetDataType(GL_FLOAT);
    }
    this->Pass2->Create2D(static_cast<unsigned int>(width),
      static_cast<unsigned int>(modifiedHeight), 4, VTK_UNSIGNED_CHAR, false);
  }

  if (this->DepthTexture2 == nullptr)
  {
    this->DepthTexture2 = vtkTextureObject::New();
    this->DepthTexture2->SetContext(renWin);
  }
  if (!this->DepthTexture2->GetHandle())
  {
    this->DepthTexture2->AllocateDepth(width, modifiedHeight, this->DepthFormat);
    this->DepthTexture2->Activate();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    this->DepthTexture2->Deactivate();
  }
  this->DepthTexture2->Resize(width, modifiedHeight);
  this->FrameBufferObject->AddColorAttachment(0, this->Pass2);
  this->FrameBufferObject->AddDepthAttachment(this->DepthTexture2);
  this->FrameBufferObject->Start(width, modifiedHeight);

  // Start() calls InitializeViewport() which disables GL_DEPTH_TEST. Re-enable it with
  // GL_ALWAYS so the Lanczos-filtered depth is always written (negative kernel weights
  // can produce values slightly outside [0,1], which would fail GL_LESS).
  vtkOpenGLState::ScopedglDepthFunc dfsaver(ostate);
  ostate->vtkglEnable(GL_DEPTH_TEST);
  ostate->vtkglDepthFunc(GL_ALWAYS);

  // Use a subsample shader, do it horizontally. this->Pass1 is the source
  // (this->Pass2 is the fbo render target)

  if (!this->SSAAHelper->Program)
  {
    // compile and bind it if needed
    vtkShaderProgram* newShader =
      renWin->GetShaderCache()->ReadyShaderProgram(vtkTextureObjectVS, vtkSSAAPassFS, nullptr);

    this->SSAAHelper->Program = newShader;
    this->SSAAHelper->VAO->ShaderProgramChanged(); // reset the VAO as the shader has changed
    this->SSAAHelper->ShaderSourceTime.Modified();
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->SSAAHelper->Program);
  }

  if (!this->SSAAHelper->Program)
  {
    vtkErrorMacro("Couldn't build the shader program. At this point , it can be an error in a "
                  "shader or a driver bug.");

    // restore some state.
    ostate->PopFramebufferBindings();
    return;
  }

  this->Pass1->Activate();
  int sourceId = this->Pass1->GetTextureUnit();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  this->DepthTexture1->Activate();
  int depthSourceId = this->DepthTexture1->GetTextureUnit();
  this->SSAAHelper->Program->SetUniformi("source", sourceId);
  this->SSAAHelper->Program->SetUniformi("depthSource", depthSourceId);
  // The implementation uses four steps to cover 1.5 destination pixels
  // so the offset is 1.5/4.0 = 0.375
  this->SSAAHelper->Program->SetUniformf("texelWidthOffset", 0.375 / width);
  this->SSAAHelper->Program->SetUniformf("texelHeightOffset", 0.0);

  ostate->vtkglDisable(GL_BLEND);

  this->FrameBufferObject->RenderQuad(
    0, width - 1, 0, modifiedHeight - 1, this->SSAAHelper->Program, this->SSAAHelper->VAO);

  this->Pass1->Deactivate();
  this->DepthTexture1->Deactivate();

  // 4. Render in original FB (from renderstate in arg)

  ostate->PopFramebufferBindings();

  // to2 is the source
  this->Pass2->Activate();
  sourceId = this->Pass2->GetTextureUnit();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  this->DepthTexture2->Activate();
  depthSourceId = this->DepthTexture2->GetTextureUnit();
  this->SSAAHelper->Program->SetUniformi("source", sourceId);
  this->SSAAHelper->Program->SetUniformi("depthSource", depthSourceId);
  this->SSAAHelper->Program->SetUniformf("texelWidthOffset", 0.0);
  this->SSAAHelper->Program->SetUniformf("texelHeightOffset", 0.375 / height);

  // Use the same sample shader, this time vertical
  int originX = 0;
  int originY = 0;
  if (s->GetFrameBuffer() == nullptr)
  {
    int tmpWidth, tmpHeight;
    r->GetTiledSizeAndOrigin(&tmpWidth, &tmpHeight, &originX, &originY);
  }
  this->Pass2->CopyToFrameBuffer(0, 0, width - 1, modifiedHeight - 1, originX, originY,
    originX + width - 1, originY + height - 1, width, height, this->SSAAHelper->Program,
    this->SSAAHelper->VAO);

  this->Pass2->Deactivate();
  this->DepthTexture2->Deactivate();

  vtkOpenGLCheckErrorMacro("failed after Render");
}

//------------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkSSAAPass::ReleaseGraphicsResources(vtkWindow* w)
{
  assert("pre: w_exists" && w != nullptr);

  this->Superclass::ReleaseGraphicsResources(w);

  if (this->SSAAHelper != nullptr)
  {
    this->SSAAHelper->ReleaseGraphicsResources(w);
  }
  if (this->FrameBufferObject != nullptr)
  {
    this->FrameBufferObject->ReleaseGraphicsResources(w);
  }
  if (this->Pass1 != nullptr)
  {
    this->Pass1->ReleaseGraphicsResources(w);
  }
  if (this->Pass1Depth != nullptr)
  {
    this->Pass1Depth->ReleaseGraphicsResources(w);
  }
  if (this->Pass2 != nullptr)
  {
    this->Pass2->ReleaseGraphicsResources(w);
  }
  if (this->DepthTexture1 != nullptr)
  {
    this->DepthTexture1->ReleaseGraphicsResources(w);
  }
  if (this->DepthTexture2 != nullptr)
  {
    this->DepthTexture2->ReleaseGraphicsResources(w);
  }
  if (this->DelegatePass != nullptr)
  {
    this->DelegatePass->ReleaseGraphicsResources(w);
  }
}
VTK_ABI_NAMESPACE_END
