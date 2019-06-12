/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPBRIrradianceTexture.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPBRIrradianceTexture.h"
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

vtkStandardNewMacro(vtkPBRIrradianceTexture);

vtkCxxSetObjectMacro(vtkPBRIrradianceTexture, InputCubeMap, vtkOpenGLTexture);

//------------------------------------------------------------------------------
vtkPBRIrradianceTexture::~vtkPBRIrradianceTexture()
{
  if (this->InputCubeMap)
  {
    this->InputCubeMap->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkPBRIrradianceTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "IrradianceStep: " << this->IrradianceStep << "\n";
  os << indent << "IrradianceSize: " << this->IrradianceSize << endl;
}

//------------------------------------------------------------------------------
void vtkPBRIrradianceTexture::Load(vtkRenderer* ren)
{
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  if (!renWin)
  {
    vtkErrorMacro("No render window.");
  }

  if (!this->InputCubeMap)
  {
    vtkErrorMacro("No input cubemap specified.");
  }

  this->InputCubeMap->Render(ren);

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
      this->IrradianceSize, this->IrradianceSize, 3, VTK_FLOAT, nullptr);

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
    fbo->Start(this->IrradianceSize, this->IrradianceSize);

    std::string FSSource = vtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Decl",
      "uniform samplerCube cubeMap;\n"
      "const float PI = 3.14159265359;\n"
      "//VTK::FSQ::Decl");

    std::stringstream fsImpl;
    fsImpl
      << "  const vec3 x = vec3(1.0, 0.0, 0.0);\n"
         "  const vec3 y = vec3(0.0, 1.0, 0.0);\n"
         "  vec3 n_px = normalize(vec3(1.0, 1.0 - 2.0 * texCoord.y, 1.0 - 2.0 * texCoord.x));\n"
         "  vec3 n_nx = normalize(vec3(-1.0, 1.0 - 2.0 * texCoord.y, 2.0 * texCoord.x - 1.0));\n"
         "  vec3 n_py = normalize(vec3(2.0 * texCoord.x - 1.0, 1.0, 2.0 * texCoord.y - 1.0));\n"
         "  vec3 n_ny = normalize(vec3(2.0 * texCoord.x - 1.0, -1.0, 1.0 - 2.0 * texCoord.y));\n"
         "  vec3 n_pz = normalize(vec3(2.0 * texCoord.x - 1.0, 1.0 - 2.0 * texCoord.y, 1.0));\n"
         "  vec3 n_nz = normalize(vec3(1.0 - 2.0 * texCoord.x, 1.0 - 2.0 * texCoord.y, -1.0));\n"
         "  vec3 t_px = normalize(cross(n_px, y));\n"
         "  vec3 t_nx = normalize(cross(n_nx, y));\n"
         "  vec3 t_py = normalize(cross(n_py, x));\n"
         "  vec3 t_ny = normalize(cross(n_ny, x));\n"
         "  vec3 t_pz = normalize(cross(n_pz, x));\n"
         "  vec3 t_nz = normalize(cross(n_nz, x));\n"
         "  mat3 m_px = mat3(t_px, cross(n_px, t_px), n_px);\n"
         "  mat3 m_nx = mat3(t_nx, cross(n_nx, t_nx), n_nx);\n"
         "  mat3 m_py = mat3(t_py, cross(n_py, t_py), n_py);\n"
         "  mat3 m_ny = mat3(t_ny, cross(n_ny, t_ny), n_ny);\n"
         "  mat3 m_pz = mat3(t_pz, cross(n_pz, t_pz), n_pz);\n"
         "  mat3 m_nz = mat3(t_nz, cross(n_nz, t_nz), n_nz);\n"
         "  vec3 i_px = vec3(0.0);\n"
         "  vec3 i_nx = vec3(0.0);\n"
         "  vec3 i_py = vec3(0.0);\n"
         "  vec3 i_ny = vec3(0.0);\n"
         "  vec3 i_pz = vec3(0.0);\n"
         "  vec3 i_nz = vec3(0.0);\n"
         "  float nSamples = 0.0;\n"
         "  for (float phi = 0.0; phi < 2.0 * PI; phi += "
      << this->IrradianceStep
      << ")\n"
         "  {\n"
         "    for (float theta = 0.0; theta < 0.5 * PI; theta += "
      << this->IrradianceStep
      << ")\n"
         "    {\n"
         "      vec3 sample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));\n"
         "      float factor = cos(theta) * sin(theta);\n"
         "      i_px += texture(cubeMap, m_px * sample).rgb * factor;\n"
         "      i_nx += texture(cubeMap, m_nx * sample).rgb * factor;\n"
         "      i_py += texture(cubeMap, m_py * sample).rgb * factor;\n"
         "      i_ny += texture(cubeMap, m_ny * sample).rgb * factor;\n"
         "      i_pz += texture(cubeMap, m_pz * sample).rgb * factor;\n"
         "      i_nz += texture(cubeMap, m_nz * sample).rgb * factor;\n"
         "      nSamples = nSamples + 1.0;\n"
         "    }\n"
         "  }\n"
         "  float denom = 1.0 / nSamples;\n"
         "  gl_FragData[0] = vec4(i_px * PI * denom, 1.0);\n"
         "  gl_FragData[1] = vec4(i_nx * PI * denom, 1.0);\n"
         "  gl_FragData[2] = vec4(i_py * PI * denom, 1.0);\n"
         "  gl_FragData[3] = vec4(i_ny * PI * denom, 1.0);\n"
         "  gl_FragData[4] = vec4(i_pz * PI * denom, 1.0);\n"
         "  gl_FragData[5] = vec4(i_nz * PI * denom, 1.0);\n";

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl", fsImpl.str());

    vtkOpenGLQuadHelper quadHelper(renWin,
      vtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader().c_str(), FSSource.c_str(), "");

    if (!quadHelper.Program || !quadHelper.Program->GetCompiled())
    {
      vtkErrorMacro("Couldn't build the shader program for irradiance.");
    }
    else
    {
      this->InputCubeMap->GetTextureObject()->Activate();
      quadHelper.Program->SetUniformi("cubeMap", this->InputCubeMap->GetTextureUnit());
      quadHelper.Render();
      this->InputCubeMap->GetTextureObject()->Deactivate();
    }
    fbo->RestorePreviousBindingsAndBuffers();
    this->LoadTime.Modified();
  }

  this->TextureObject->Activate();
}
