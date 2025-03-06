// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtk_glad.h"

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN
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
  os << indent << "PrefilterLevels: " << this->PrefilterLevels << endl;
  os << indent << "PrefilterSize: " << this->PrefilterSize << endl;
}

//------------------------------------------------------------------------------
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
    return;
  }

  if (!this->InputTexture)
  {
    vtkErrorMacro("No input cubemap specified.");
    return;
  }

#ifdef GL_ES_VERSION_3_0
  // Mipmap generation is not supported for most texture formats (like GL_RGB32F)
  this->InputTexture->MipmapOff();
  this->InputTexture->InterpolateOff();
#endif
  this->InputTexture->Render(ren);
  this->PrefilterSize = this->InputTexture->GetTextureObject()->GetHeight();

  if (this->GetMTime() > this->LoadTime.GetMTime() ||
    this->InputTexture->GetMTime() > this->LoadTime.GetMTime())
  {
    if (this->TextureObject == nullptr)
    {
      this->TextureObject = vtkTextureObject::New();
    }
    this->TextureObject->SetContext(renWin);
    this->TextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
    this->TextureObject->SetWrapT(vtkTextureObject::ClampToEdge);
    this->TextureObject->SetWrapR(vtkTextureObject::ClampToEdge);
    this->TextureObject->SetMinificationFilter(vtkTextureObject::LinearMipmapLinear);
    this->TextureObject->SetMagnificationFilter(vtkTextureObject::Linear);
    this->TextureObject->SetGenerateMipmap(true);
    this->TextureObject->SetMaxLevel(this->PrefilterLevels - 1);
#ifdef GL_ES_VERSION_3_0
    this->TextureObject->SetFormat(GL_RGB);
    this->TextureObject->SetDataType(GL_UNSIGNED_BYTE);
    this->TextureObject->SetInternalFormat(GL_RGB8);
    this->TextureObject->CreateCubeFromRaw(
      this->PrefilterSize, this->PrefilterSize, 3, VTK_UNSIGNED_CHAR, nullptr);
#else
    this->TextureObject->SetFormat(GL_RGB);
    this->TextureObject->SetInternalFormat(this->HalfPrecision ? GL_RGB16F : GL_RGB32F);
    this->TextureObject->CreateCubeFromRaw(
      this->PrefilterSize, this->PrefilterSize, 3, VTK_FLOAT, nullptr);
#endif

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
      "uniform int nbSamples;\n"
      "const float PI = 3.14159265359;\n"
      // The solid angle of the texel = 4*PI/(6*w*h)
      // We remove the 4, simplification with saSample
      "const float saTexel  = PI / (6.0 * " +
        std::to_string(this->PrefilterSize * this->PrefilterSize) +
        ".0);\n"
        "vec3 GetSampleColor(vec3 dir, float mipLevel)\n"
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
        "}\n"
        "// Normal Distribution\n"
        "float D_GGX(float NdH, float roughness)\n"
        "{\n"
        "    float alpha = roughness * roughness;\n"
        "    float alpha2 = alpha * alpha;\n"
        "    float denom = NdH * NdH * (alpha2 - 1.0) + 1.0;\n"
        "    return alpha2 / (PI * denom * denom); \n"
        "}\n"
        "void AccumulateColorAndWeight(inout vec3 p, inout float w, vec2 rd, vec3 n, float "
        "roughness)\n"
        "{\n"
        "  vec3 h = ImportanceSampleGGX(rd, n, roughness);\n"
        "  float NdH = max(dot(n,h), 0.0);\n"
        "  // Should be HdV here, but we assume V = N\n"
        "  vec3 l = normalize(2.0 * NdH * h - n);\n"
        "  float NdL = max(dot(n, l), 0.0);\n"
        "  if (NdL > 0.0)\n"
        "  {\n"
        "    // sample from the environment's mip level based on roughness/pdf\n"
        "    float D = D_GGX(NdH, roughness);\n"
        // The pdf of the ggx distribution with importance sampling
        // pdf = (D * NdH) / (VdH * 4). Here NdH = VdH as we assume V=N
        // we removed the 4, simplification with saTexel
        "    float pdf = D;\n"
        // The solid angle of the sample
        "    float nbSamplesF = float(nbSamples);"
        "    float saSample = 1.0 / ( nbSamplesF * pdf);\n"
        // miplevel = 0.5 * (log2(K) + log2(saSample) - log2(saTexel))
        // K=4 so log2(K)=2
        "    float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * (2.0 + log2(saSample) - "
        "log2(saTexel));\n"
        "    p += GetSampleColor(l, mipLevel) * NdL;\n"
        "    w += NdL;\n"
        "  }\n"
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
      vtkShaderProgram::Substitute(FSSource, "//VTK::TEXTUREINPUT::Decl",
        "uniform samplerCube inputTex;\n"
        "uniform vec3 floorPlane;\n" // floor plane eqn
        "uniform vec3 floorRight;\n" // floor plane right
        "uniform vec3 floorFront;\n" // floor plane front
      );

      vtkShaderProgram::Substitute(FSSource, "//VTK::SAMPLING::Decl",
        "  dir = normalize(dir);\n"
        "  vec3 dirv = vec3(dot(dir,floorRight),\n"
        "    dot(dir,floorPlane),\n"
        "    dot(dir,floorFront));\n"
        "  vec3 col = textureLod(inputTex, dirv, mipLevel).rgb;\n");
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
        "  vec3 col = textureLod(inputTex, p, mipLevel).rgb;\n");
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
         "  uint nbSamplesU = uint(nbSamples);\n"
         "  for (uint i = 0u; i < nbSamplesU; i++)\n"
         "  {\n"
         "    vec2 rd = Hammersley(i, nbSamplesU);\n"
         "    AccumulateColorAndWeight(p_px, w_px, rd, n_px, roughness);\n"
         "    AccumulateColorAndWeight(p_nx, w_nx, rd, n_nx, roughness);\n"
         "    AccumulateColorAndWeight(p_py, w_py, rd, n_py, roughness);\n"
         "    AccumulateColorAndWeight(p_ny, w_ny, rd, n_ny, roughness);\n"
         "    AccumulateColorAndWeight(p_pz, w_pz, rd, n_pz, roughness);\n"
         "    AccumulateColorAndWeight(p_nz, w_nz, rd, n_nz, roughness);\n"
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
      vtkErrorMacro("Couldn't build the shader program for prefilter texture.");
    }
    else
    {
#ifndef GL_ES_VERSION_3_0
      // For GLES 3.0, we forcefully turn these off, so don't warn about it.
      if (!this->InputTexture->GetInterpolate() || !this->InputTexture->GetMipmap())
      {
        vtkWarningMacro("The input texture of vtkPBRPrefilterTexture should have mipmap and "
                        "interpolate set to ON.");
      }
#endif

      this->InputTexture->GetTextureObject()->Activate();
      quadHelper.Program->SetUniformi("inputTex", this->InputTexture->GetTextureUnit());

      if (this->InputTexture->GetCubeMap())
      {
        float plane[3], right[3];
        for (unsigned int i = 0; i < 3; i++)
        {
          plane[i] = ren->GetEnvironmentUp()[i];
          right[i] = ren->GetEnvironmentRight()[i];
        }
        quadHelper.Program->SetUniform3f("floorPlane", plane);
        quadHelper.Program->SetUniform3f("floorRight", right);
        float front[3];
        vtkMath::Cross(plane, right, front);
        quadHelper.Program->SetUniform3f("floorFront", front);
      }

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

        float roughness = static_cast<float>(mip) / static_cast<float>(this->PrefilterLevels);
        quadHelper.Program->SetUniformf("roughness", roughness);

        // Heuristic to choose the number of samples according to the roughness
        const float a = 0.65;
        const float b = 1.0 - a;
        int nbSamples = roughness * this->PrefilterMaxSamples / (a * roughness + b) + 1;

        quadHelper.Program->SetUniformi("nbSamples", nbSamples);

        quadHelper.Render();
      }

      renWin->GetState()->PopFramebufferBindings();

      this->InputTexture->GetTextureObject()->Deactivate();
    }
    this->LoadTime.Modified();
  }

  this->TextureObject->Activate();
}
VTK_ABI_NAMESPACE_END
