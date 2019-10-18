/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPBRLUTTexture.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPBRLUTTexture.h"
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

vtkStandardNewMacro(vtkPBRLUTTexture);

//------------------------------------------------------------------------------
void vtkPBRLUTTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LUTSize: " << this->LUTSize << "\n";
  os << indent << "LUTSamples: " << this->LUTSamples << endl;
}

//------------------------------------------------------------------------------
void vtkPBRLUTTexture::Load(vtkRenderer* ren)
{
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  if (!renWin)
  {
    vtkErrorMacro("No render window.");
  }

  if (this->GetMTime() > this->LoadTime.GetMTime())
  {
    if (this->TextureObject == nullptr)
    {
      this->TextureObject = vtkTextureObject::New();
    }
    this->TextureObject->SetContext(renWin);
    this->TextureObject->SetFormat(GL_RG);
    this->TextureObject->SetInternalFormat(GL_RG16F);
    this->TextureObject->SetDataType(GL_FLOAT);
    this->TextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
    this->TextureObject->SetWrapT(vtkTextureObject::ClampToEdge);
    this->TextureObject->SetMinificationFilter(vtkTextureObject::Linear);
    this->TextureObject->SetMagnificationFilter(vtkTextureObject::Linear);
    this->TextureObject->Allocate2D(this->LUTSize, this->LUTSize, 2, VTK_FLOAT);

    this->RenderWindow = renWin;

    vtkOpenGLState* state = renWin->GetState();
    vtkOpenGLState::ScopedglViewport svp(state);
    vtkOpenGLState::ScopedglEnableDisable sdepth(state, GL_DEPTH_TEST);
    vtkOpenGLState::ScopedglEnableDisable sblend(state, GL_BLEND);
    vtkOpenGLState::ScopedglEnableDisable sscissor(state, GL_SCISSOR_TEST);

    vtkNew<vtkOpenGLFramebufferObject> fbo;
    fbo->SetContext(renWin);
    renWin->GetState()->PushFramebufferBindings();
    fbo->Bind();

    fbo->AddColorAttachment(0, this->TextureObject);
    fbo->ActivateDrawBuffers(1);
    fbo->Start(this->LUTSize, this->LUTSize);

    std::string FSSource = vtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Decl",
      "const float PI = 3.14159265359;\n"
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
      "float GeometrySchlickGGX(float NdV, float k)\n"
      "{\n"
      "  return NdV / (NdV * (1.0 - k) + k);\n"
      "}\n"
      "float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)\n"
      "{\n"
      "  float k = (roughness * roughness) / 2.0;\n"
      "  float NdV = max(dot(N, V), 0.0);\n"
      "  float NdL = max(dot(N, L), 0.0);\n"
      "  float ggx2 = GeometrySchlickGGX(NdV, k);\n"
      "  float ggx1 = GeometrySchlickGGX(NdL, k);\n"
      "  return ggx1 * ggx2;\n"
      "}\n");

    std::stringstream fsImpl;
    fsImpl << "float NdV = texCoord.x;\n"
              "  float roughness = texCoord.y;\n"
              "  vec3 V = vec3(sqrt(1.0 - NdV*NdV), 0.0, NdV);\n"
              "  float A = 0.0;\n"
              "  float B = 0.0;\n"
              "  vec3 N = vec3(0.0, 0.0, 1.0);\n"
              "  for(uint i = 0u; i < "
           << this->LUTSamples
           << "u; ++i)\n"
              "  {\n"
              "    vec2 rd = Hammersley(i, "
           << this->LUTSamples
           << "u);\n"
              "    vec3 H = ImportanceSampleGGX(rd, N, roughness);\n"
              "    vec3 L = normalize(2.0 * dot(V, H) * H - V);\n"
              "    float NdL = max(L.z, 0.0);\n"
              "    float NdH = max(H.z, 0.0);\n"
              "    float VdH = max(dot(V, H), 0.0);\n"
              "    if(NdL > 0.0)\n"
              "    {\n"
              "      float G = GeometrySmith(N, V, L, roughness);\n"
              "      float G_Vis = (G * VdH) / (NdH * NdV);\n"
              "      float Fc = pow(1.0 - VdH, 5.0);\n"
              "      A += (1.0 - Fc) * G_Vis;\n"
              "      B += Fc * G_Vis;\n"
              "    }\n"
              "  }\n"
              "  A /= float("
           << this->LUTSamples
           << "u);\n"
              "  B /= float("
           << this->LUTSamples
           << "u);\n"
              "  gl_FragData[0] = vec4(A, B, 0.0, 0.0);\n";

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl", fsImpl.str());

    vtkOpenGLQuadHelper quadHelper(renWin,
      vtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader().c_str(), FSSource.c_str(), "");

    if (!quadHelper.Program || !quadHelper.Program->GetCompiled())
    {
      vtkErrorMacro("Couldn't build the shader program for irradiance.");
    }
    else
    {
      quadHelper.Render();
    }
    renWin->GetState()->PopFramebufferBindings();
    this->LoadTime.Modified();
  }

  this->TextureObject->Activate();
}
