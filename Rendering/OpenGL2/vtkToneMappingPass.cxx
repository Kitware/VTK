/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkToneMappingPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkToneMappingPass.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLQuadHelper.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"

vtkStandardNewMacro(vtkToneMappingPass);

// ----------------------------------------------------------------------------
vtkToneMappingPass::~vtkToneMappingPass()
{
  if (this->FrameBufferObject)
  {
    vtkErrorMacro("FrameBufferObject should have been deleted in ReleaseGraphicsResources().");
  }
  if (this->ColorTexture)
  {
    vtkErrorMacro("ColorTexture should have been deleted in ReleaseGraphicsResources().");
  }
  if (this->QuadHelper)
  {
    vtkErrorMacro("QuadHelper should have been deleted in ReleaseGraphicsResources().");
  }
}

// ----------------------------------------------------------------------------
void vtkToneMappingPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FrameBufferObject:";
  if (this->FrameBufferObject != nullptr)
  {
    this->FrameBufferObject->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "ColorTexture:";
  if (this->ColorTexture != nullptr)
  {
    this->ColorTexture->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
}

// ----------------------------------------------------------------------------
void vtkToneMappingPass::Render(const vtkRenderState* s)
{
  vtkOpenGLClearErrorMacro();

  this->NumberOfRenderedProps = 0;

  vtkRenderer* r = s->GetRenderer();
  vtkOpenGLRenderWindow* renWin = static_cast<vtkOpenGLRenderWindow*>(r->GetRenderWindow());
  vtkOpenGLState* ostate = renWin->GetState();

  vtkOpenGLState::ScopedglEnableDisable bsaver(ostate, GL_BLEND);
  vtkOpenGLState::ScopedglEnableDisable dsaver(ostate, GL_DEPTH_TEST);

  if (this->DelegatePass == nullptr)
  {
    vtkWarningMacro("no delegate in vtkToneMappingPass.");
    return;
  }

  // create FBO and texture
  int x, y, w, h;
  r->GetTiledSizeAndOrigin(&w, &h, &x, &y);

  if (this->ColorTexture == nullptr)
  {
    this->ColorTexture = vtkTextureObject::New();
    this->ColorTexture->SetContext(renWin);
    this->ColorTexture->SetMinificationFilter(vtkTextureObject::Linear);
    this->ColorTexture->SetMagnificationFilter(vtkTextureObject::Linear);
    this->ColorTexture->Allocate2D(w, h, 4, VTK_FLOAT);
  }
  this->ColorTexture->Resize(w, h);

  if (this->FrameBufferObject == nullptr)
  {
    this->FrameBufferObject = vtkOpenGLFramebufferObject::New();
    this->FrameBufferObject->SetContext(renWin);
  }

  renWin->GetState()->PushFramebufferBindings();
  this->RenderDelegate(s, w, h, w, h, this->FrameBufferObject, this->ColorTexture);
  renWin->GetState()->PopFramebufferBindings();

  if (this->QuadHelper &&
    static_cast<unsigned int>(this->ToneMappingType) != this->QuadHelper->ShaderChangeValue)
  {
    delete this->QuadHelper;
    this->QuadHelper = nullptr;
  }

  if (!this->QuadHelper)
  {
    std::string FSSource = vtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Decl",
      "uniform sampler2D source;\n"
      "//VTK::FSQ::Decl");

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl",
      "vec4 pixel = texture2D(source, texCoord);\n"
      "  float Y = 0.2126 * pixel.r + 0.7152 * pixel.g + 0.0722 * pixel.b;\n"
      "  //VTK::FSQ::Impl");

    switch (this->ToneMappingType)
    {
      case Clamp:
        vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl",
          "float scale = min(Y, 1.0) / Y;\n"
          "  //VTK::FSQ::Impl");
        break;
      case Reinhard:
        vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl",
          "float scale = 1.0 / (Y + 1.0);\n"
          "  //VTK::FSQ::Impl");
        break;
      case Exponential:
        vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Decl", "uniform float exposure;\n");
        vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl",
          "float scale = (1.0 - exp(-Y*exposure)) / Y;\n"
          "  //VTK::FSQ::Impl");
        break;
    }

    vtkShaderProgram::Substitute(
      FSSource, "//VTK::FSQ::Impl", "gl_FragData[0] = vec4(pixel.rgb * scale, pixel.a);");

    this->QuadHelper = new vtkOpenGLQuadHelper(renWin,
      vtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader().c_str(), FSSource.c_str(), "");

    this->QuadHelper->ShaderChangeValue = this->ToneMappingType;
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->QuadHelper->Program);
  }

  if (!this->QuadHelper->Program || !this->QuadHelper->Program->GetCompiled())
  {
    vtkErrorMacro("Couldn't build the shader program.");
    return;
  }

  this->ColorTexture->Activate();
  this->QuadHelper->Program->SetUniformi("source", this->ColorTexture->GetTextureUnit());

  if (this->ToneMappingType == Exponential)
  {
    this->QuadHelper->Program->SetUniformf("exposure", this->Exposure);
  }

  ostate->vtkglDisable(GL_BLEND);
  ostate->vtkglDisable(GL_DEPTH_TEST);
  ostate->vtkglViewport(x, y, w, h);
  ostate->vtkglScissor(x, y, w, h);

  this->QuadHelper->Render();

  this->ColorTexture->Deactivate();

  vtkOpenGLCheckErrorMacro("failed after Render");
}

// ----------------------------------------------------------------------------
void vtkToneMappingPass::ReleaseGraphicsResources(vtkWindow* w)
{
  this->Superclass::ReleaseGraphicsResources(w);

  if (this->QuadHelper)
  {
    delete this->QuadHelper;
    this->QuadHelper = nullptr;
  }
  if (this->FrameBufferObject)
  {
    this->FrameBufferObject->Delete();
    this->FrameBufferObject = nullptr;
  }
  if (this->ColorTexture)
  {
    this->ColorTexture->Delete();
    this->ColorTexture = nullptr;
  }
}
