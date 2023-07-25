// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOutlineGlowPass.h"
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

// to be able to dump intermediate passes into png files for debugging.
// only for vtkOutlineGlowPass developers.
// #define VTK_OUTLINE_GLOW_PASS_DEBUG

#ifdef VTK_OUTLINE_GLOW_PASS_DEBUG
#include "vtkImageExtractComponents.h"
#include "vtkImageImport.h"
#include "vtkPNGWriter.h"
#include "vtkPixelBufferObject.h"
#endif

// Shader includes
#include "vtkOutlineGlowBlurPassFS.h"
#include "vtkOutlineGlowUpscalePassFS.h"
#include "vtkTextureObjectVS.h" // a pass through shader

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOutlineGlowPass);

// ----------------------------------------------------------------------------
vtkOutlineGlowPass::vtkOutlineGlowPass()
{
  this->FrameBufferObject = nullptr;
  this->BlurPass1 = nullptr;
  this->BlurPass2 = nullptr;
  this->ScenePass = nullptr;
  this->BlurProgram = nullptr;
  this->UpscaleProgram = nullptr;
}

// ----------------------------------------------------------------------------
vtkOutlineGlowPass::~vtkOutlineGlowPass()
{
  if (this->FrameBufferObject != nullptr)
  {
    vtkErrorMacro(<< "FrameBufferObject should have been deleted in ReleaseGraphicsResources().");
  }
  if (this->ScenePass != nullptr)
  {
    vtkErrorMacro(<< "ScenePass should have been deleted in ReleaseGraphicsResources().");
  }
  if (this->BlurPass1 != nullptr)
  {
    vtkErrorMacro(<< "BlurPass1 should have been deleted in ReleaseGraphicsResources().");
  }
  if (this->BlurPass2 != nullptr)
  {
    vtkErrorMacro(<< "BlurPass2 should have been deleted in ReleaseGraphicsResources().");
  }
}

// ----------------------------------------------------------------------------
void vtkOutlineGlowPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OutlineGlowPass:";
  os << indent << "OutlineIntensity: " << this->OutlineIntensity << endl;
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkOutlineGlowPass::Render(const vtkRenderState* s)
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
    int halfWidth = std::ceil((double)width / 2.0);
    int halfHeight = std::ceil((double)height / 2.0);

    // No extra pixels. Tex coord clamping takes care of the edges.
    int widthHalfTarget = halfWidth;
    int heightHalfTarget = halfHeight;
    int widthFullTarget = width;
    int heightFullTarget = height;

    if (this->ScenePass == nullptr)
    {
      this->ScenePass = vtkTextureObject::New();
      this->ScenePass->SetContext(renWin);
    }

    if (this->FrameBufferObject == nullptr)
    {
      this->FrameBufferObject = vtkOpenGLFramebufferObject::New();
      this->FrameBufferObject->SetContext(renWin);
    }

    // backup GL state
    vtkOpenGLState::ScopedglEnableDisable bsaver(ostate, GL_BLEND);
    vtkOpenGLState::ScopedglEnableDisable dsaver(ostate, GL_DEPTH_TEST);

    // 2. Render Scene to an offscreen render target
    this->FrameBufferObject->SaveCurrentBindingsAndBuffers();

    this->RenderDelegate(s, width, height, widthFullTarget, heightFullTarget,
      this->FrameBufferObject, this->ScenePass);

#ifdef VTK_OUTLINE_GLOW_PASS_DEBUG
    // Save first pass in file for debugging.
    vtkPixelBufferObject* pbo = this->BlurPass1->Download();

    unsigned char* openglRawData = new unsigned char[4 * widthHalfTarget * heightHalfTarget];
    unsigned int dims[2];
    dims[0] = widthHalfTarget;
    dims[1] = heightHalfTarget;
    vtkIdType incs[2];
    incs[0] = 0;
    incs[1] = 0;
    bool status = pbo->Download2D(VTK_UNSIGNED_CHAR, openglRawData, dims, 4, incs);
    assert("check" && status);
    pbo->Delete();

    // no pbo
    this->BlurPass1->Bind();
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, openglRawData);
    this->BlurPass1->Deactivate();

    vtkImageImport* importer = vtkImageImport::New();
    importer->CopyImportVoidPointer(
      openglRawData, 4 * widthHalfTarget * heightHalfTarget * sizeof(unsigned char));
    importer->SetDataScalarTypeToUnsignedChar();
    importer->SetNumberOfScalarComponents(4);
    importer->SetWholeExtent(0, widthHalfTarget - 1, 0, heightHalfTarget - 1, 0, 0);
    importer->SetDataExtentToWholeExtent();
    delete[] openglRawData;

    vtkImageExtractComponents* rgbaToRgb = vtkImageExtractComponents::New();
    rgbaToRgb->SetInputConnection(importer->GetOutputPort());
    rgbaToRgb->SetComponents(0, 1, 2);

    vtkPNGWriter* writer = vtkPNGWriter::New();
    writer->SetFileName("BlurPass1.png");
    writer->SetInputConnection(rgbaToRgb->GetOutputPort());
    importer->Delete();
    rgbaToRgb->Delete();
    writer->Write();
    writer->Delete();
#endif

    // 3. Render Scene to Buffer1 while applying horizontal blur
    if (this->BlurPass1 == nullptr)
    {
      this->BlurPass1 = vtkTextureObject::New();
      this->BlurPass1->SetContext(this->FrameBufferObject->GetContext());
    }

    if (this->BlurPass1->GetWidth() != static_cast<unsigned int>(widthHalfTarget) ||
      this->BlurPass1->GetHeight() != static_cast<unsigned int>(heightHalfTarget))
    {
      this->BlurPass1->Create2D(static_cast<unsigned int>(widthHalfTarget),
        static_cast<unsigned int>(heightHalfTarget), 4, VTK_UNSIGNED_CHAR, false);
    }

    this->FrameBufferObject->AddColorAttachment(0, this->BlurPass1);
    this->FrameBufferObject->Start(widthHalfTarget, heightHalfTarget);

#ifdef VTK_OUTLINE_GLOW_PASS_DEBUG
    cout << "outline finish2" << endl;
    glFinish();
#endif

    // Use a blur shader, do it horizontally. this->ScenePass is the source
    // (this->BlurPass1 is the fbo render target)

    // has something changed that would require us to recreate the shader?
    if (!this->BlurProgram)
    {
      this->BlurProgram = new vtkOpenGLHelper;
      // build the shader source code
      std::string VSSource = vtkTextureObjectVS;
      std::string FSSource = vtkOutlineGlowBlurPassFS;
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
      this->FrameBufferObject->UnBind();
      this->FrameBufferObject->RestorePreviousBindingsAndBuffers();
      return;
    }

    this->ScenePass->Activate();
    int sourceId = this->ScenePass->GetTextureUnit();
    this->ScenePass->SetMinificationFilter(vtkTextureObject::Linear);
    this->ScenePass->SetMagnificationFilter(vtkTextureObject::Linear);
    // Clamp the texture coordinates so we don't get an outline at the opposite end of the screen
    this->ScenePass->SetWrapS(vtkTextureObject::ClampToEdge);
    this->ScenePass->SetWrapT(vtkTextureObject::ClampToEdge);
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
    fvalues[0] = static_cast<float>(2.2 / widthHalfTarget);
    this->BlurProgram->Program->SetUniformf("offsetx", fvalues[0]);
    fvalues[0] = 0.0f;
    this->BlurProgram->Program->SetUniformf("offsety", fvalues[0]);

#ifdef VTK_OUTLINE_GLOW_PASS_DEBUG
    cout << "outline finish3-" << endl;
    glFinish();
#endif

    ostate->vtkglDisable(GL_BLEND);
    ostate->vtkglDisable(GL_DEPTH_TEST);

    this->FrameBufferObject->RenderQuad(0, widthHalfTarget - 1, 0, heightHalfTarget - 1,
      this->BlurProgram->Program, this->BlurProgram->VAO);

#ifdef VTK_OUTLINE_GLOW_PASS_DEBUG
    cout << "outline finish3" << endl;
    glFinish();
#endif

    this->BlurPass1->Deactivate();

#ifdef VTK_OUTLINE_GLOW_PASS_DEBUG

    // Save second pass in file for debugging.
    pbo = this->BlurPass2->Download();
    openglRawData = new unsigned char[4 * widthHalfTarget * heightHalfTarget];
    status = pbo->Download2D(VTK_UNSIGNED_CHAR, openglRawData, dims, 4, incs);
    assert("check2" && status);
    pbo->Delete();

    // no pbo
    this->BlurPass2->Bind();
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, openglRawData);
    this->BlurPass2->Deactivate();

    importer = vtkImageImport::New();
    importer->CopyImportVoidPointer(
      openglRawData, 4 * widthHalfTarget * heightHalfTarget * sizeof(unsigned char));
    importer->SetDataScalarTypeToUnsignedChar();
    importer->SetNumberOfScalarComponents(4);
    importer->SetWholeExtent(0, widthHalfTarget - 1, 0, heightHalfTarget - 1, 0, 0);
    importer->SetDataExtentToWholeExtent();
    delete[] openglRawData;

    rgbaToRgb = vtkImageExtractComponents::New();
    rgbaToRgb->SetInputConnection(importer->GetOutputPort());
    rgbaToRgb->SetComponents(0, 1, 2);

    writer = vtkPNGWriter::New();
    writer->SetFileName("BlurPass2.png");
    writer->SetInputConnection(rgbaToRgb->GetOutputPort());
    importer->Delete();
    rgbaToRgb->Delete();
    writer->Write();
    writer->Delete();
#endif

    // 4. Render BlurrPass1 to BlurrPass2 while applying vertical blur
    if (this->BlurPass2 == nullptr)
    {
      this->BlurPass2 = vtkTextureObject::New();
      this->BlurPass2->SetContext(this->FrameBufferObject->GetContext());
    }

    if (this->BlurPass2->GetWidth() != static_cast<unsigned int>(widthHalfTarget) ||
      this->BlurPass2->GetHeight() != static_cast<unsigned int>(heightHalfTarget))
    {
      this->BlurPass2->Create2D(static_cast<unsigned int>(widthHalfTarget),
        static_cast<unsigned int>(heightHalfTarget), 4, VTK_UNSIGNED_CHAR, false);
    }

    this->FrameBufferObject->AddColorAttachment(0, this->BlurPass2);
    this->FrameBufferObject->Start(widthHalfTarget, heightHalfTarget);

    // BlurrPass1 is the source
    this->BlurPass1->Activate();
    sourceId = this->BlurPass1->GetTextureUnit();
    this->BlurPass1->SetMinificationFilter(vtkTextureObject::Linear);
    this->BlurPass1->SetMagnificationFilter(vtkTextureObject::Linear);
    // Clamp the texture coordinates so we don't get an outline at the opposite end of the screen
    this->BlurPass1->SetWrapS(vtkTextureObject::ClampToEdge);
    this->BlurPass1->SetWrapT(vtkTextureObject::ClampToEdge);
    this->BlurProgram->Program->SetUniformi("source", sourceId);

    // Use the same blur shader, do it vertically.

    // vertical offset.
    fvalues[0] = 0.0f;
    this->BlurProgram->Program->SetUniformf("offsetx", fvalues[0]);
    fvalues[0] = static_cast<float>(2.2 / heightHalfTarget);
    this->BlurProgram->Program->SetUniformf("offsety", fvalues[0]);

    ostate->vtkglDisable(GL_BLEND);
    ostate->vtkglDisable(GL_DEPTH_TEST);

    this->FrameBufferObject->RenderQuad(0, widthHalfTarget - 1, 0, heightHalfTarget - 1,
      this->BlurProgram->Program, this->BlurProgram->VAO);

    // 5. Render the blurred image back to the back buffer
    this->FrameBufferObject->UnBind();
    this->FrameBufferObject->RestorePreviousBindingsAndBuffers();

    // has something changed that would require us to recreate the shader?
    if (!this->UpscaleProgram)
    {
      this->UpscaleProgram = new vtkOpenGLHelper;
      // build the shader source code
      std::string VSSource = vtkTextureObjectVS;
      std::string FSSource = vtkOutlineGlowUpscalePassFS;
      std::string GSSource;

      // compile and bind it if needed
      vtkShaderProgram* newShader = renWin->GetShaderCache()->ReadyShaderProgram(
        VSSource.c_str(), FSSource.c_str(), GSSource.c_str());

      // if the shader changed reinitialize the VAO
      if (newShader != this->UpscaleProgram->Program)
      {
        this->UpscaleProgram->Program = newShader;
        this->UpscaleProgram->VAO
          ->ShaderProgramChanged(); // reset the VAO as the shader has changed
      }

      this->UpscaleProgram->ShaderSourceTime.Modified();
    }
    else
    {
      renWin->GetShaderCache()->ReadyShaderProgram(this->UpscaleProgram->Program);
    }

    // Set the textures. Scene contains the original unaltered scene in full resolution,
    // source the blurred downsampled image.
    this->ScenePass->Activate();
    sourceId = this->ScenePass->GetTextureUnit();
    this->UpscaleProgram->Program->SetUniformi("scene", sourceId);
    this->BlurPass2->Activate();
    sourceId = this->BlurPass2->GetTextureUnit();
    this->UpscaleProgram->Program->SetUniformi("source", sourceId);
    this->UpscaleProgram->Program->SetUniformf("outlineIntensity", this->OutlineIntensity);

    this->BlurPass2->SetMinificationFilter(vtkTextureObject::Linear);
    this->BlurPass2->SetMagnificationFilter(vtkTextureObject::Linear);

    // If this is a transparent (layered) renderer enable blending
    if (s->GetRenderer()->Transparent())
    {
      ostate->vtkglEnable(GL_BLEND);
      ostate->vtkglBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
      ostate->vtkglBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
    }

    this->BlurPass2->CopyToFrameBuffer(0, 0, widthHalfTarget - 1, heightHalfTarget - 1, 0, 0,
      width - 1, height - 1, width,
      height, // Not used, but only by calling this method
              // directly will the correct texture coordinates be used
      this->UpscaleProgram->Program, this->UpscaleProgram->VAO);

    this->BlurPass2->Deactivate();

#ifdef VTK_OUTLINE_GLOW_PASS_DEBUG
    cout << "outline finish4" << endl;
    glFinish();
#endif
  }
  else
  {
    vtkWarningMacro(<< " no delegate.");
  }

  vtkOpenGLCheckErrorMacro("failed after Render");
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkOutlineGlowPass::ReleaseGraphicsResources(vtkWindow* w)
{
  assert("pre: w_exists" && w != nullptr);

  this->Superclass::ReleaseGraphicsResources(w);

  if (this->BlurProgram != nullptr)
  {
    this->BlurProgram->ReleaseGraphicsResources(w);
    delete this->BlurProgram;
    this->BlurProgram = nullptr;
  }
  if (this->UpscaleProgram != nullptr)
  {
    this->UpscaleProgram->ReleaseGraphicsResources(w);
    delete this->UpscaleProgram;
    this->UpscaleProgram = nullptr;
  }
  if (this->FrameBufferObject != nullptr)
  {
    this->FrameBufferObject->Delete();
    this->FrameBufferObject = nullptr;
  }
  if (this->ScenePass != nullptr)
  {
    this->ScenePass->Delete();
    this->ScenePass = nullptr;
  }
  if (this->BlurPass1 != nullptr)
  {
    this->BlurPass1->Delete();
    this->BlurPass1 = nullptr;
  }
  if (this->BlurPass2 != nullptr)
  {
    this->BlurPass2->Delete();
    this->BlurPass2 = nullptr;
  }
}
VTK_ABI_NAMESPACE_END
