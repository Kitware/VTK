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

vtkCxxSetObjectMacro(vtkPBRIrradianceTexture, InputTexture, vtkOpenGLTexture);

//------------------------------------------------------------------------------
vtkPBRIrradianceTexture::~vtkPBRIrradianceTexture()
{
  if (this->InputTexture)
  {
    this->InputTexture->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkPBRIrradianceTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "IrradianceStep: " << this->IrradianceStep << "\n";
  os << indent << "IrradianceSize: " << this->IrradianceSize << endl;
}

// ---------------------------------------------------------------------------
// Release the graphics resources used by this texture.
void vtkPBRIrradianceTexture::ReleaseGraphicsResources(vtkWindow* win)
{
  if (this->InputTexture)
  {
    this->InputTexture->ReleaseGraphicsResources(win);
  }
  this->Superclass::ReleaseGraphicsResources(win);
}

//------------------------------------------------------------------------------
void vtkPBRIrradianceTexture::Load(vtkRenderer* ren)
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
    this->TextureObject->SetMinificationFilter(vtkTextureObject::Linear);
    this->TextureObject->SetMagnificationFilter(vtkTextureObject::Linear);
    this->TextureObject->CreateCubeFromRaw(
      this->IrradianceSize, this->IrradianceSize, 3, VTK_FLOAT, nullptr);

    this->RenderWindow = renWin;

    vtkOpenGLState* state = renWin->GetState();
    vtkOpenGLState::ScopedglViewport svp(state);
    vtkOpenGLState::ScopedglEnableDisable sdepth(state, GL_DEPTH_TEST);
    vtkOpenGLState::ScopedglEnableDisable sblend(state, GL_BLEND);
    vtkOpenGLState::ScopedglEnableDisable sscissor(state, GL_SCISSOR_TEST);

    std::string FSSource = vtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Decl",
      "//VTK::TEXTUREINPUT::Decl\n"
      "uniform vec3 shift;\n"
      "uniform vec3 contribX;\n"
      "uniform vec3 contribY;\n"
      "const float PI = 3.14159265359;\n"
      "vec3 GetSampleColor(vec3 dir)\n"
      "{\n"
      "  //VTK::SAMPLING::Decl\n"
      "  //VTK::COLORSPACE::Decl\n"
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
      << "  const vec3 x = vec3(1.0, 0.0, 0.0);\n"
         "  const vec3 y = vec3(0.0, 1.0, 0.0);\n"
         "  vec3 n = normalize(vec3(shift.x + contribX.x * texCoord.x + contribY.x * texCoord.y,\n"
         "    shift.y + contribX.y * texCoord.x + contribY.y * texCoord.y,\n"
         "    shift.z + contribX.z * texCoord.x + contribY.z * texCoord.y));\n"
         "  vec3 t = normalize(cross(n, y));\n"
         "  mat3 m = mat3(t, cross(n, t), n);\n"
         "  vec3 acc = vec3(0.0);\n"
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
         "      acc += GetSampleColor(m * sample) * factor;\n"
         "      nSamples = nSamples + 1.0;\n"
         "    }\n"
         "  }\n"
         "  gl_FragData[0] = vec4(acc * (PI / nSamples), 1.0);\n";

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl", fsImpl.str());

    vtkOpenGLQuadHelper quadHelper(renWin,
      vtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader().c_str(), FSSource.c_str(), "");

    vtkNew<vtkOpenGLFramebufferObject> fbo;
    fbo->SetContext(renWin);
    renWin->GetState()->PushFramebufferBindings();
    fbo->Bind();

    if (!quadHelper.Program || !quadHelper.Program->GetCompiled())
    {
      vtkErrorMacro("Couldn't build the shader program for irradiance.");
    }
    else
    {
      this->InputTexture->GetTextureObject()->Activate();
      quadHelper.Program->SetUniformi("inputTex", this->InputTexture->GetTextureUnit());

      float shift[6][3] = { { 1.f, 1.f, 1.f }, { -1.f, 1.f, -1.f }, { -1.f, 1.f, -1.f },
        { -1.f, -1.f, 1.f }, { -1.f, 1.f, 1.f }, { 1.f, 1.f, -1.f } };
      float contribX[6][3] = { { 0.f, 0.f, -2.f }, { 0.f, 0.f, 2.f }, { 2.f, 0.f, 0.f },
        { 2.f, 0.f, 0.f }, { 2.f, 0.f, 0.f }, { -2.f, 0.f, 0.f } };
      float contribY[6][3] = { { 0.f, -2.f, 0.f }, { 0.f, -2.f, 0.f }, { 0.f, 0.f, 2.f },
        { 0.f, 0.f, -2.f }, { 0.f, -2.f, 0.f }, { 0.f, -2.f, 0.f } };

      for (int i = 0; i < 6; i++)
      {
        fbo->AddColorAttachment(0, this->TextureObject, 0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
        fbo->ActivateDrawBuffers(1);
        fbo->Start(this->IrradianceSize, this->IrradianceSize);

        quadHelper.Program->SetUniform3f("shift", shift[i]);
        quadHelper.Program->SetUniform3f("contribX", contribX[i]);
        quadHelper.Program->SetUniform3f("contribY", contribY[i]);
        quadHelper.Render();
        fbo->RemoveColorAttachment(0);

        // Computing irradiance can be long depending on the GPU.
        // On Windows 7, a computation longer than 2 seconds triggers GPU timeout.
        // The following call do a glFlush() that inform the OS that the computation is finished
        // thus avoids the trigger of the GPU timeout.
        renWin->WaitForCompletion();
      }
      this->InputTexture->GetTextureObject()->Deactivate();
    }
    renWin->GetState()->PopFramebufferBindings();
    this->LoadTime.Modified();
  }

  this->TextureObject->Activate();
}
