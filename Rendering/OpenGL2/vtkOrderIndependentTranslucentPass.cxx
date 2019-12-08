/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOrderIndependentTranslucentPass.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOrderIndependentTranslucentPass.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLQuadHelper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkProp.h"
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"
#include <cassert>
#include <list>

#include "vtkRenderStepsPass.h"

#include "vtkOpenGLHelper.h"

// the 2D blending shaders we use
#include "vtkOrderIndependentTranslucentPassFinalFS.h"

vtkStandardNewMacro(vtkOrderIndependentTranslucentPass);
vtkCxxSetObjectMacro(vtkOrderIndependentTranslucentPass, TranslucentPass, vtkRenderPass);

// ----------------------------------------------------------------------------
vtkOrderIndependentTranslucentPass::vtkOrderIndependentTranslucentPass()
  : Framebuffer(nullptr)
  , State(nullptr)
{
  this->TranslucentPass = nullptr;

  this->FinalBlend = nullptr;

  this->TranslucentZTexture = vtkTextureObject::New();

  this->TranslucentRGBATexture = vtkTextureObject::New();
  this->TranslucentRTexture = vtkTextureObject::New();

  this->ViewportX = 0;
  this->ViewportY = 0;
  this->ViewportWidth = 100;
  this->ViewportHeight = 100;
}

// ----------------------------------------------------------------------------
vtkOrderIndependentTranslucentPass::~vtkOrderIndependentTranslucentPass()
{
  if (this->TranslucentPass != nullptr)
  {
    this->TranslucentPass->Delete();
  }
  if (this->TranslucentZTexture)
  {
    this->TranslucentZTexture->UnRegister(this);
    this->TranslucentZTexture = nullptr;
  }
  if (this->TranslucentRGBATexture)
  {
    this->TranslucentRGBATexture->UnRegister(this);
    this->TranslucentRGBATexture = nullptr;
  }
  if (this->TranslucentRTexture)
  {
    this->TranslucentRTexture->UnRegister(this);
    this->TranslucentRTexture = nullptr;
  }
  if (this->Framebuffer)
  {
    this->Framebuffer->UnRegister(this);
    this->Framebuffer = nullptr;
  }
}

//-----------------------------------------------------------------------------
// Description:
// Destructor. Delete SourceCode if any.
void vtkOrderIndependentTranslucentPass::ReleaseGraphicsResources(vtkWindow* w)
{
  assert("pre: w_exists" && w != nullptr);

  if (this->FinalBlend != nullptr)
  {
    delete this->FinalBlend;
    this->FinalBlend = nullptr;
  }
  if (this->TranslucentPass)
  {
    this->TranslucentPass->ReleaseGraphicsResources(w);
  }
  if (this->TranslucentZTexture)
  {
    this->TranslucentZTexture->ReleaseGraphicsResources(w);
  }
  if (this->TranslucentRGBATexture)
  {
    this->TranslucentRGBATexture->ReleaseGraphicsResources(w);
  }
  if (this->TranslucentRTexture)
  {
    this->TranslucentRTexture->ReleaseGraphicsResources(w);
  }
  if (this->Framebuffer)
  {
    this->Framebuffer->ReleaseGraphicsResources(w);
    this->Framebuffer->UnRegister(this);
    this->Framebuffer = nullptr;
  }
}

// ----------------------------------------------------------------------------
void vtkOrderIndependentTranslucentPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "TranslucentPass:";
  if (this->TranslucentPass != nullptr)
  {
    this->TranslucentPass->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
}

void vtkOrderIndependentTranslucentPass::BlendFinalPeel(vtkOpenGLRenderWindow* renWin)
{
  if (!this->FinalBlend)
  {
    this->FinalBlend =
      new vtkOpenGLQuadHelper(renWin, nullptr, vtkOrderIndependentTranslucentPassFinalFS, "");
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->FinalBlend->Program);
  }

  if (this->FinalBlend->Program)
  {
    this->TranslucentRGBATexture->Activate();
    this->TranslucentRTexture->Activate();

    this->FinalBlend->Program->SetUniformi(
      "translucentRGBATexture", this->TranslucentRGBATexture->GetTextureUnit());
    this->FinalBlend->Program->SetUniformi(
      "translucentRTexture", this->TranslucentRTexture->GetTextureUnit());

    this->FinalBlend->Render();
  }
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkOrderIndependentTranslucentPass::Render(const vtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  this->NumberOfRenderedProps = 0;

  if (this->TranslucentPass == nullptr)
  {
    vtkWarningMacro(<< "No TranslucentPass delegate set. Nothing can be rendered.");
    return;
  }

  // Any prop to render?
  bool hasTranslucentPolygonalGeometry = false;
  int i = 0;
  while (!hasTranslucentPolygonalGeometry && i < s->GetPropArrayCount())
  {
    hasTranslucentPolygonalGeometry = s->GetPropArray()[i]->HasTranslucentPolygonalGeometry() == 1;
    ++i;
  }
  if (!hasTranslucentPolygonalGeometry)
  {
    return; // nothing to render.
  }

  vtkOpenGLRenderWindow* renWin =
    vtkOpenGLRenderWindow::SafeDownCast(s->GetRenderer()->GetRenderWindow());
  this->State = renWin->GetState();

  vtkRenderer* r = s->GetRenderer();
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

  // create textures we need if not done already
  if (this->TranslucentRGBATexture->GetHandle() == 0)
  {
    this->TranslucentRGBATexture->SetInternalFormat(GL_RGBA16F);
    this->TranslucentRGBATexture->SetFormat(GL_RGBA);
    this->TranslucentRGBATexture->SetDataType(GL_HALF_FLOAT);
    this->TranslucentRGBATexture->SetContext(renWin);
    this->TranslucentRGBATexture->Allocate2D(
      this->ViewportWidth, this->ViewportHeight, 4, VTK_FLOAT);

    this->TranslucentRTexture->SetInternalFormat(GL_R16F);
    this->TranslucentRTexture->SetFormat(GL_RED);
    this->TranslucentRTexture->SetDataType(GL_HALF_FLOAT);
    this->TranslucentRTexture->SetContext(renWin);
    this->TranslucentRTexture->Allocate2D(this->ViewportWidth, this->ViewportHeight, 1, VTK_FLOAT);

    // what depth format should we use?
    this->TranslucentZTexture->SetContext(renWin);
    int dbits = renWin->GetDepthBufferSize();
    if (dbits == 32)
    {
      this->TranslucentZTexture->AllocateDepth(
        this->ViewportWidth, this->ViewportHeight, vtkTextureObject::Fixed32);
    }
    else
    {
      this->TranslucentZTexture->AllocateDepth(
        this->ViewportWidth, this->ViewportHeight, vtkTextureObject::Fixed24);
    }
    this->TranslucentZTexture->SetWrapS(vtkTextureObject::ClampToEdge);
    this->TranslucentZTexture->SetWrapT(vtkTextureObject::ClampToEdge);
  }
  else
  {
    // make sure texture sizes are up to date
    this->TranslucentRGBATexture->Resize(this->ViewportWidth, this->ViewportHeight);
    this->TranslucentRTexture->Resize(this->ViewportWidth, this->ViewportHeight);
    this->TranslucentZTexture->Resize(this->ViewportWidth, this->ViewportHeight);
  }

  // create framebuffer if not done already
  if (!this->Framebuffer)
  {
    this->Framebuffer = vtkOpenGLFramebufferObject::New();
    this->Framebuffer->SetContext(renWin);
    this->State->PushFramebufferBindings();
    this->Framebuffer->Bind();
    this->Framebuffer->AddDepthAttachment(this->TranslucentZTexture);
    this->Framebuffer->AddColorAttachment(0, this->TranslucentRGBATexture);
    this->Framebuffer->AddColorAttachment(1, this->TranslucentRTexture);
    this->State->PopFramebufferBindings();
  }

  this->State->vtkglViewport(0, 0, this->ViewportWidth, this->ViewportHeight);
  bool saveScissorTestState = this->State->GetEnumState(GL_SCISSOR_TEST);
  this->State->vtkglDisable(GL_SCISSOR_TEST);

  // bind the draw mode but leave read as the previous FO
  this->State->PushFramebufferBindings();
  this->Framebuffer->Bind(this->Framebuffer->GetDrawMode());
  this->Framebuffer->ActivateDrawBuffers(2);

#ifdef GL_MULTISAMPLE
  bool multiSampleStatus = this->State->GetEnumState(GL_MULTISAMPLE);
  this->State->vtkglDisable(GL_MULTISAMPLE);
#endif

  this->State->vtkglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  this->State->vtkglClearColor(0.0, 0.0, 0.0, 1.0);
  this->State->vtkglDepthMask(GL_TRUE);
  this->State->vtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#if defined(__APPLE__)
  this->State->vtkglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  // apple fails if not the upper left corenr of the window
  // blit on apple is fubar, so rerender opaque
  // to get a good depth buffer
  r->DeviceRenderOpaqueGeometry();
  this->State->vtkglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
#else
  // blit read buffer depth to FO depth texture
  glBlitFramebuffer(this->ViewportX, this->ViewportY, this->ViewportX + this->ViewportWidth,
    this->ViewportY + this->ViewportHeight, 0, 0, this->ViewportWidth, this->ViewportHeight,
    GL_DEPTH_BUFFER_BIT, GL_NEAREST);
#endif

  // now bind both read and draw
  this->Framebuffer->Bind();

  // Setup property keys for actors:
  this->PreRender(s);

  // Enable the depth buffer (otherwise it's disabled for translucent geometry)
  assert("Render state valid." && s);

  this->State->vtkglEnable(GL_DEPTH_TEST);
  this->State->vtkglEnable(GL_BLEND);

  // basic gist is we accumulate color into RGB
  // We compute final opacity into A
  // We store accumulated opacity into R of the
  // R texture.
  this->State->vtkglBlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);

  // render the translucent data into the FO
  this->TranslucentPass->Render(s);

  // back to the original FO
  this->State->PopFramebufferBindings();

  this->State->vtkglBlendFuncSeparate(
    GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);

  // Restore the original viewport and scissor test settings
  this->State->vtkglViewport(
    this->ViewportX, this->ViewportY, this->ViewportWidth, this->ViewportHeight);
  if (saveScissorTestState)
  {
    this->State->vtkglEnable(GL_SCISSOR_TEST);
  }
  else
  {
    this->State->vtkglDisable(GL_SCISSOR_TEST);
  }

  // do not write zvalues on final blend
  this->State->vtkglDepthMask(GL_FALSE);
  this->State->vtkglDepthFunc(GL_ALWAYS);
  this->BlendFinalPeel(renWin);

  // unload the textures
  this->TranslucentRGBATexture->Deactivate();
  this->TranslucentRTexture->Deactivate();
  this->TranslucentZTexture->Deactivate();

  this->State->vtkglDepthFunc(GL_LEQUAL);

#ifdef GL_MULTISAMPLE
  if (multiSampleStatus)
  {
    this->State->vtkglEnable(GL_MULTISAMPLE);
  }
#endif

  // Restore blending parameters:
  this->State->vtkglBlendFuncSeparate(
    GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  this->PostRender(s);

  this->NumberOfRenderedProps = this->TranslucentPass->GetNumberOfRenderedProps();

  vtkOpenGLCheckErrorMacro("failed after Render");
}

//------------------------------------------------------------------------------
bool vtkOrderIndependentTranslucentPass::PostReplaceShaderValues(
  std::string&, std::string&, std::string& fragmentShader, vtkAbstractMapper*, vtkProp*)
{
  vtkShaderProgram::Substitute(fragmentShader, "//VTK::DepthPeeling::Impl",
    "  gl_FragData[0] = vec4(gl_FragData[0].rgb*gl_FragData[0].a, gl_FragData[0].a);\n"
    "  gl_FragData[1].r = gl_FragData[0].a;\n");

  return true;
}
