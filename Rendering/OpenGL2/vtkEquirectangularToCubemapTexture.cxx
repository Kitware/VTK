/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEquirectangularToCubemapTexture.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkEquirectangularToCubemapTexture.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLQuadHelper.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLState.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"

#include "vtk_glew.h"

#include <sstream>

vtkStandardNewMacro(vtkEquirectangularToCubemapTexture);
vtkCxxSetObjectMacro(vtkEquirectangularToCubemapTexture, InputTexture, vtkOpenGLTexture);

//------------------------------------------------------------------------------
vtkEquirectangularToCubemapTexture::~vtkEquirectangularToCubemapTexture()
{
  if (this->InputTexture)
  {
    this->InputTexture->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkEquirectangularToCubemapTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CubemapSize: " << this->CubemapSize << endl;
}

//------------------------------------------------------------------------------
void vtkEquirectangularToCubemapTexture::Load(vtkRenderer* ren)
{
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  if (!renWin)
  {
    vtkErrorMacro("No render window.");
  }

  if (!this->InputTexture)
  {
    vtkErrorMacro("No input texture specified.");
  }

  this->InputTexture->Render(ren);
  this->CubeMapOn();

  if (this->GetMTime() > this->LoadTime.GetMTime())
  {
    if (this->TextureObject == nullptr)
    {
      this->TextureObject = vtkTextureObject::New();
    }
    this->TextureObject->SetContext(renWin);
    this->TextureObject->SetFormat(GL_RGB);
    this->TextureObject->SetInternalFormat(GL_RGB16F);
    this->TextureObject->SetDataType(GL_FLOAT);
    this->TextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
    this->TextureObject->SetWrapT(vtkTextureObject::ClampToEdge);
    this->TextureObject->SetWrapR(vtkTextureObject::ClampToEdge);
    this->TextureObject->SetMinificationFilter(vtkTextureObject::Linear);
    this->TextureObject->SetMagnificationFilter(vtkTextureObject::Linear);
    this->TextureObject->CreateCubeFromRaw(
      this->CubemapSize, this->CubemapSize, 3, VTK_FLOAT, nullptr);

    this->RenderWindow = renWin;

    vtkOpenGLState* state = renWin->GetState();
    vtkOpenGLState::ScopedglViewport svp(state);
    vtkOpenGLState::ScopedglEnableDisable sdepth(state, GL_DEPTH_TEST);
    vtkOpenGLState::ScopedglEnableDisable sblend(state, GL_DEPTH_TEST);
    vtkOpenGLState::ScopedglEnableDisable sscissor(state, GL_SCISSOR_TEST);

    vtkNew<vtkOpenGLFramebufferObject> fbo;
    fbo->SetContext(renWin);
    fbo->Bind();
    fbo->SaveCurrentBindingsAndBuffers();

    for (int i = 0; i < 6; i++)
    {
      fbo->AddColorAttachment(i, this->TextureObject, 0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
    }
    fbo->ActivateDrawBuffers(6);
    fbo->Start(this->CubemapSize, this->CubemapSize);

    std::string FSSource = vtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Decl",
      "uniform sampler2D equiTex;\n"
      "vec2 toSpherical(vec3 v)\n"
      "{\n"
      "  v = normalize(v);\n"
      "  float theta = atan(v.z, v.x);\n"
      "  float phi = asin(v.y);\n"
      "  return vec2(theta * 0.1591 + 0.5, phi * 0.3183 + 0.5);\n"
      "}\n"
      "//VTK::FSQ::Decl");

    std::stringstream fsImpl;
    fsImpl
      << "  \n"
         "  float x = 2.0 * texCoord.x - 1.0;\n"
         "  float y = 1.0 - 2.0 * texCoord.y;\n"
         "  gl_FragData[0] = texture(equiTex, toSpherical(vec3(1, y, -x)));\n"
         "  gl_FragData[1] = texture(equiTex, toSpherical(vec3(-1, y, x)));\n"
         "  gl_FragData[2] = texture(equiTex, toSpherical(vec3(x, 1, -y)));\n"
         "  gl_FragData[3] = texture(equiTex, toSpherical(vec3(x, -1, y)));\n"
         "  gl_FragData[4] = texture(equiTex, toSpherical(vec3(x, y, 1)));\n"
         "  gl_FragData[5] = texture(equiTex, toSpherical(vec3(-x, y, -1)));\n";

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl", fsImpl.str());

    vtkOpenGLQuadHelper quadHelper(renWin,
      vtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader().c_str(), FSSource.c_str(), "");

    if (!quadHelper.Program || !quadHelper.Program->GetCompiled())
    {
      vtkErrorMacro("Couldn't build the shader program for equirectangular to cubemap texture.");
    }
    else
    {
      this->InputTexture->GetTextureObject()->Activate();
      quadHelper.Program->SetUniformi("cubeMap", this->InputTexture->GetTextureUnit());
      quadHelper.Render();
      this->InputTexture->GetTextureObject()->Deactivate();
    }
    fbo->RestorePreviousBindingsAndBuffers();
    this->LoadTime.Modified();
  }

  this->TextureObject->Activate();
}
