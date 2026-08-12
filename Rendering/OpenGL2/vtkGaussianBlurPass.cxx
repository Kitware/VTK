// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkGaussianBlurPass.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLHelper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"

#include "vtkGaussianBlurPassFS.h"
#include "vtkGaussianBlurPassVS.h"

#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkGaussianBlurPass);

//------------------------------------------------------------------------------
vtkGaussianBlurPass::vtkGaussianBlurPass()
{
  this->FrameBufferObject = nullptr;
  this->Pass1 = nullptr;
  this->Pass2 = nullptr;
  this->BlurProgram = nullptr;
}

//------------------------------------------------------------------------------
vtkGaussianBlurPass::~vtkGaussianBlurPass()
{
  if (this->FrameBufferObject != nullptr)
  {
    vtkErrorMacro(<< "FrameBufferObject should have been deleted in ReleaseGraphicsResources().");
  }
  if (this->Pass1 != nullptr)
  {
    vtkErrorMacro(<< "Pass1 should have been deleted in ReleaseGraphicsResources().");
  }
  if (this->Pass2 != nullptr)
  {
    vtkErrorMacro(<< "Pass2 should have been deleted in ReleaseGraphicsResources().");
  }
}

//------------------------------------------------------------------------------
void vtkGaussianBlurPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkGaussianBlurPass::Render(const vtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  vtkOpenGLClearErrorMacro();

  this->NumberOfRenderedProps = 0;

  vtkRenderer* r = s->GetRenderer();
  vtkOpenGLRenderWindow* renWin = static_cast<vtkOpenGLRenderWindow*>(r->GetRenderWindow());
  vtkOpenGLState* ostate = renWin->GetState();

  if (this->DelegatePass != nullptr)
  {
    // 1. Create a new render state with an FBO.

    int width;
    int height;
    int size[2];
    s->GetWindowSize(size);
    width = size[0];
    height = size[1];

    // I suggest set this to 100 for debugging, makes some errors
    // much easier to find
    constexpr int extraPixels = 2; // two on each side, as the kernel is 5x5

    int w = width + extraPixels * 2;
    int h = height + extraPixels * 2;

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

    // backup GL state
    vtkOpenGLState::ScopedglEnableDisable bsaver(ostate, GL_BLEND);
    vtkOpenGLState::ScopedglEnableDisable dsaver(ostate, GL_DEPTH_TEST);

    ostate->PushFramebufferBindings();
    this->RenderDelegate(s, width, height, w, h, this->FrameBufferObject, this->Pass1);

    // 3. Same FBO, but new color attachment (new TO).
    if (this->Pass2 == nullptr)
    {
      this->Pass2 = vtkTextureObject::New();
      this->Pass2->SetContext(this->FrameBufferObject->GetContext());
    }

    if (this->Pass2->GetWidth() != static_cast<unsigned int>(w) ||
      this->Pass2->GetHeight() != static_cast<unsigned int>(h))
    {
      this->Pass2->Create2D(
        static_cast<unsigned int>(w), static_cast<unsigned int>(h), 4, VTK_UNSIGNED_CHAR, false);
    }

    this->FrameBufferObject->AddColorAttachment(0, this->Pass2);
    this->FrameBufferObject->Start(w, h);

    // Use a blur shader, do it horizontally. this->Pass1 is the source
    // (this->Pass2 is the fbo render target)

    // has something changed that would require us to recreate the shader?
    if (!this->BlurProgram)
    {
      this->BlurProgram = new vtkOpenGLHelper;
      // build the shader source code
      std::string VSSource = vtkGaussianBlurPassVS;
      std::string FSSource = vtkGaussianBlurPassFS;
      std::string GSSource;

      // compile and bind it if needed
      vtkShaderProgram* newShader = renWin->GetShaderCache()->ReadyShaderProgram(
        VSSource.c_str(), FSSource.c_str(), GSSource.c_str());

      // if the shader changed reinitialize the VAO
      if (newShader != this->BlurProgram->Program)
      {
        this->BlurProgram->Program = newShader;
        this->BlurProgram->VAO->ShaderProgramChanged(); // reset the VAO as the shader has changed
      }

      this->BlurProgram->ShaderSourceTime.Modified();
    }
    else
    {
      renWin->GetShaderCache()->ReadyShaderProgram(this->BlurProgram->Program);
    }

    if (!this->BlurProgram->Program || !this->BlurProgram->Program->GetCompiled())
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
    this->BlurProgram->Program->SetUniformi("source", sourceId);
    float fvalues[3];

    static float kernel[3] = { 5.0f, 6.0f, 5.0f };

    int i = 0;
    float sum = 0.0f;
    while (i < 3)
    {
      sum += kernel[i];
      ++i;
    }

    // kernel
    i = 0;
    while (i < 3)
    {
      fvalues[i] = kernel[i] / sum;
      ++i;
    }
    this->BlurProgram->Program->SetUniform1fv("coef", 3, fvalues);

    // horizontal offset
    fvalues[0] = static_cast<float>(1.2 / w);
    this->BlurProgram->Program->SetUniformf("offsetx", fvalues[0]);
    fvalues[0] = 0.0f;
    this->BlurProgram->Program->SetUniformf("offsety", fvalues[0]);

    ostate->vtkglDisable(GL_BLEND);
    ostate->vtkglDisable(GL_DEPTH_TEST);

    this->FrameBufferObject->RenderQuad(
      0, w - 1, 0, h - 1, this->BlurProgram->Program, this->BlurProgram->VAO);

    this->Pass1->Deactivate();

    // 4. Render in original FB (from renderstate in arg)

    ostate->PopFramebufferBindings();

    // to2 is the source
    this->Pass2->Activate();
    sourceId = this->Pass2->GetTextureUnit();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    this->BlurProgram->Program->SetUniformi("source", sourceId);

    // Use the same blur shader, do it vertically.

    // vertical offset.
    fvalues[0] = 0.0f;
    this->BlurProgram->Program->SetUniformf("offsetx", fvalues[0]);
    fvalues[0] = static_cast<float>(1.2 / h);
    this->BlurProgram->Program->SetUniformf("offsety", fvalues[0]);

    this->Pass2->CopyToFrameBuffer(extraPixels, extraPixels, w - 1 - extraPixels,
      h - 1 - extraPixels, 0, 0, width, height, this->BlurProgram->Program, this->BlurProgram->VAO);

    this->Pass2->Deactivate();
  }
  else
  {
    vtkWarningMacro(<< " no delegate.");
  }

  vtkOpenGLCheckErrorMacro("failed after Render");
}

//------------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkGaussianBlurPass::ReleaseGraphicsResources(vtkWindow* w)
{
  assert("pre: w_exists" && w != nullptr);

  this->Superclass::ReleaseGraphicsResources(w);

  if (this->BlurProgram != nullptr)
  {
    this->BlurProgram->ReleaseGraphicsResources(w);
    delete this->BlurProgram;
    this->BlurProgram = nullptr;
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
  if (this->Pass2 != nullptr)
  {
    this->Pass2->Delete();
    this->Pass2 = nullptr;
  }
}
VTK_ABI_NAMESPACE_END
