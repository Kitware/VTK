/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPBRPrefilterTexture.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPBRPrefilterTexture.h"
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

vtkStandardNewMacro(vtkPBRPrefilterTexture);

vtkCxxSetObjectMacro(vtkPBRPrefilterTexture, InputTexture, vtkOpenGLTexture);

//------------------------------------------------------------------------------
vtkPBRPrefilterTexture::~vtkPBRPrefilterTexture()
{
  if (this->InputTexture)
  {
    this->InputTexture->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkPBRPrefilterTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PrefilterSize: " << this->PrefilterSize << "\n";
  os << indent << "PrefilterLevels: " << this->PrefilterLevels << "\n";
  os << indent << "PrefilterSamples: " << this->PrefilterSamples << endl;
}

// ---------------------------------------------------------------------------
// Release the graphics resources used by this texture.
void vtkPBRPrefilterTexture::ReleaseGraphicsResources(vtkWindow* win)
{
  if (this->InputTexture)
  {
    this->InputTexture->ReleaseGraphicsResources(win);
  }
  this->Superclass::ReleaseGraphicsResources(win);
}

//------------------------------------------------------------------------------
void vtkPBRPrefilterTexture::Load(vtkRenderer* ren)
{
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  if (!renWin)
  {
    vtkErrorMacro("No render window.");
  }

  if (!this->InputTexture)
  {
    vtkErrorMacro("No input cubemap specified.");
  }

  this->InputTexture->Render(ren);

  if (this->GetMTime() > this->LoadTime.GetMTime() ||
    this->InputTexture->GetMTime() > this->LoadTime.GetMTime())
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
    this->TextureObject->SetMinificationFilter(vtkTextureObject::LinearMipmapLinear);
    this->TextureObject->SetMagnificationFilter(vtkTextureObject::Linear);
    this->TextureObject->SetGenerateMipmap(true);
    this->TextureObject->SetMaxLevel(this->PrefilterLevels - 1);
    this->TextureObject->CreateCubeFromRaw(
      this->PrefilterSize, this->PrefilterSize, 3, VTK_FLOAT, nullptr);

    this->RenderWindow = renWin;

    vtkOpenGLState* state = renWin->GetState();
    vtkOpenGLState::ScopedglViewport svp(state);
    vtkOpenGLState::ScopedglEnableDisable sdepth(state, GL_DEPTH_TEST);
    vtkOpenGLState::ScopedglEnableDisable sblend(state, GL_BLEND);
    vtkOpenGLState::ScopedglEnableDisable sscissor(state, GL_SCISSOR_TEST);

    std::string FSSource = vtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Decl",
      "//VTK::TEXTUREINPUT::Decl\n"
      "uniform float roughness;\n"
      "const float PI = 3.14159265359;\n"
      "vec3 GetSampleColor(vec3 dir)\n"
      "{\n"
      "  //VTK::SAMPLING::Decl\n"
      "  //VTK::COLORSPACE::Decl\n"
      "}\n"
      "float RadicalInverse_VdC(uint bits)\n"
      "{\n"
      "  bits = (bits << 16u) | (bits >> 16u);\n"
      "  bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);\n"
      "  bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);\n"
      "  bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);\n"
      "  bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);\n"
      "  return float(bits) * 2.3283064365386963e-10; // / 0x100000000\n"
      "}\n"
      "vec2 Hammersley(uint i, uint N)\n"
      "{\n"
      "  return vec2(float(i)/float(N), RadicalInverse_VdC(i));\n"
      "}\n"
      "vec3 ImportanceSampleGGX(vec2 rd, vec3 N, float roughness)\n"
      "{\n"
      "  float a = roughness*roughness;\n"
      "  float phi = 2.0 * PI * rd.x;\n"
      "  float cosTheta = sqrt((1.0 - rd.y) / (1.0 + (a*a - 1.0) * rd.y));\n"
      "  float sinTheta = sqrt(1.0 - cosTheta*cosTheta);\n"
      "  vec3 H;\n"
      "  H.x = cos(phi) * sinTheta;\n"
      "  H.y = sin(phi) * sinTheta;\n"
      "  H.z = cosTheta;\n"
      "  vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);\n"
      "  vec3 tangent = normalize(cross(up, N));\n"
      "  vec3 bitangent = cross(N, tangent);\n"
      "  vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;\n"
      "  return normalize(sampleVec);\n"
      "}\n");

    if (this->ConvertToLinear)
    {
      vtkShaderProgram::Substitute(
        FSSource, "//VTK::COLORSPACE::Decl", "return pow(col, vec3(2.2));");
    }
    else
    {
      vtkShaderProgram::Substitute(FSSource, "//VTK::COLORSPACE::Decl", "return col;");
    }

    if (this->InputTexture->GetCubeMap())
    {
      vtkShaderProgram::Substitute(
        FSSource, "//VTK::TEXTUREINPUT::Decl", "uniform samplerCube inputTex;");

      vtkShaderProgram::Substitute(
        FSSource, "//VTK::SAMPLING::Decl", "vec3 col = texture(inputTex, dir).rgb;");
    }
    else
    {
      vtkShaderProgram::Substitute(
        FSSource, "//VTK::TEXTUREINPUT::Decl", "uniform sampler2D inputTex;");

      vtkShaderProgram::Substitute(FSSource, "//VTK::SAMPLING::Decl",
        "  dir = normalize(dir);\n"
        "  float theta = atan(dir.z, dir.x);\n"
        "  float phi = asin(dir.y);\n"
        "  vec2 p = vec2(theta * 0.1591 + 0.5, phi * 0.3183 + 0.5);\n"
        "  vec3 col = texture(inputTex, p).rgb;\n");
    }

    std::stringstream fsImpl;
    fsImpl
      << "vec3 n_px = normalize(vec3(1.0, 1.0 - 2.0 * texCoord.y, 1.0 - 2.0 * texCoord.x));\n"
         "  vec3 n_nx = normalize(vec3(-1.0, 1.0 - 2.0 * texCoord.y, 2.0 * texCoord.x - 1.0));\n"
         "  vec3 n_py = normalize(vec3(2.0 * texCoord.x - 1.0, 1.0, 2.0 * texCoord.y - 1.0));\n"
         "  vec3 n_ny = normalize(vec3(2.0 * texCoord.x - 1.0, -1.0, 1.0 - 2.0 * texCoord.y));\n"
         "  vec3 n_pz = normalize(vec3(2.0 * texCoord.x - 1.0, 1.0 - 2.0 * texCoord.y, 1.0));\n"
         "  vec3 n_nz = normalize(vec3(1.0 - 2.0 * texCoord.x, 1.0 - 2.0 * texCoord.y, -1.0));\n"
         "  vec3 p_px = vec3(0.0);\n"
         "  vec3 p_nx = vec3(0.0);\n"
         "  vec3 p_py = vec3(0.0);\n"
         "  vec3 p_ny = vec3(0.0);\n"
         "  vec3 p_pz = vec3(0.0);\n"
         "  vec3 p_nz = vec3(0.0);\n"
         "  float w_px = 0.0;\n"
         "  float w_nx = 0.0;\n"
         "  float w_py = 0.0;\n"
         "  float w_ny = 0.0;\n"
         "  float w_pz = 0.0;\n"
         "  float w_nz = 0.0;\n"
         "  for (uint i = 0u; i < "
      << this->PrefilterSamples
      << "u; i++)\n"
         "  {\n"
         "    vec2 rd = Hammersley(i, "
      << this->PrefilterSamples
      << "u);\n"
         "    vec3 h_px = ImportanceSampleGGX(rd, n_px, roughness);\n"
         "    vec3 h_nx = ImportanceSampleGGX(rd, n_nx, roughness);\n"
         "    vec3 h_py = ImportanceSampleGGX(rd, n_py, roughness);\n"
         "    vec3 h_ny = ImportanceSampleGGX(rd, n_ny, roughness);\n"
         "    vec3 h_pz = ImportanceSampleGGX(rd, n_pz, roughness);\n"
         "    vec3 h_nz = ImportanceSampleGGX(rd, n_nz, roughness);\n"
         "    vec3 l_px = normalize(2.0 * dot(n_px, h_px) * h_px - n_px);\n"
         "    vec3 l_nx = normalize(2.0 * dot(n_nx, h_nx) * h_nx - n_nx);\n"
         "    vec3 l_py = normalize(2.0 * dot(n_py, h_py) * h_py - n_py);\n"
         "    vec3 l_ny = normalize(2.0 * dot(n_ny, h_ny) * h_ny - n_ny);\n"
         "    vec3 l_pz = normalize(2.0 * dot(n_pz, h_pz) * h_pz - n_pz);\n"
         "    vec3 l_nz = normalize(2.0 * dot(n_nz, h_nz) * h_nz - n_nz);\n"
         "    float d_px = max(dot(n_px, l_px), 0.0);\n"
         "    float d_nx = max(dot(n_nx, l_nx), 0.0);\n"
         "    float d_py = max(dot(n_py, l_py), 0.0);\n"
         "    float d_ny = max(dot(n_ny, l_ny), 0.0);\n"
         "    float d_pz = max(dot(n_pz, l_pz), 0.0);\n"
         "    float d_nz = max(dot(n_nz, l_nz), 0.0);\n"
         "    p_px += GetSampleColor(l_px) * d_px;\n"
         "    p_nx += GetSampleColor(l_nx) * d_nx;\n"
         "    p_py += GetSampleColor(l_py) * d_py;\n"
         "    p_ny += GetSampleColor(l_ny) * d_ny;\n"
         "    p_pz += GetSampleColor(l_pz) * d_pz;\n"
         "    p_nz += GetSampleColor(l_nz) * d_nz;\n"
         "    w_px += d_px;\n"
         "    w_nx += d_nx;\n"
         "    w_py += d_py;\n"
         "    w_ny += d_ny;\n"
         "    w_pz += d_pz;\n"
         "    w_nz += d_nz;\n"
         "  }\n"
         "  gl_FragData[0] = vec4(p_px / w_px, 1.0);\n"
         "  gl_FragData[1] = vec4(p_nx / w_nx, 1.0);\n"
         "  gl_FragData[2] = vec4(p_py / w_py, 1.0);\n"
         "  gl_FragData[3] = vec4(p_ny / w_ny, 1.0);\n"
         "  gl_FragData[4] = vec4(p_pz / w_pz, 1.0);\n"
         "  gl_FragData[5] = vec4(p_nz / w_nz, 1.0);\n";

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl", fsImpl.str());

    vtkOpenGLQuadHelper quadHelper(renWin,
      vtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader().c_str(), FSSource.c_str(), "");

    if (!quadHelper.Program || !quadHelper.Program->GetCompiled())
    {
      vtkErrorMacro("Couldn't build the shader program for irradiance.");
    }
    else
    {
      this->InputTexture->GetTextureObject()->Activate();
      quadHelper.Program->SetUniformi("inputTex", this->InputTexture->GetTextureUnit());

      vtkNew<vtkOpenGLFramebufferObject> fbo;
      fbo->SetContext(renWin);

      renWin->GetState()->PushFramebufferBindings();
      fbo->Bind();

      for (unsigned int mip = 0; mip < this->PrefilterLevels; mip++)
      {
        fbo->RemoveColorAttachments(6);
        for (int i = 0; i < 6; i++)
        {
          fbo->AddColorAttachment(
            i, this->TextureObject, 0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mip);
        }
        fbo->ActivateDrawBuffers(6);
        unsigned currentSize = this->PrefilterSize >> mip;
        fbo->Start(currentSize, currentSize);

        float roughness = static_cast<float>(mip) / static_cast<float>(this->PrefilterLevels - 1);
        quadHelper.Program->SetUniformf("roughness", roughness);

        quadHelper.Render();
      }

      renWin->GetState()->PopFramebufferBindings();

      this->InputTexture->GetTextureObject()->Deactivate();
    }
    this->LoadTime.Modified();
  }

  this->TextureObject->Activate();
}
