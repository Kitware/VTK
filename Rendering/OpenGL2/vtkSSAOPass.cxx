/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSSAOPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSSAOPass.h"

#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLPolyDataMapper.h"
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

#include <random>

vtkStandardNewMacro(vtkSSAOPass);

// ----------------------------------------------------------------------------
void vtkSSAOPass::PrintSelf(ostream& os, vtkIndent indent)
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
  os << indent << "PositionTexture:";
  if (this->PositionTexture != nullptr)
  {
    this->PositionTexture->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "NormalTexture:";
  if (this->NormalTexture != nullptr)
  {
    this->NormalTexture->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "SSAOTexture:";
  if (this->SSAOTexture != nullptr)
  {
    this->SSAOTexture->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "DepthTexture:";
  if (this->DepthTexture != nullptr)
  {
    this->DepthTexture->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
}

// ----------------------------------------------------------------------------
void vtkSSAOPass::InitializeGraphicsResources(vtkOpenGLRenderWindow* renWin, int w, int h)
{
  if (this->ColorTexture == nullptr)
  {
    this->ColorTexture = vtkTextureObject::New();
    this->ColorTexture->SetContext(renWin);
    this->ColorTexture->SetFormat(GL_RGBA);
    this->ColorTexture->SetInternalFormat(GL_RGBA32F);
    this->ColorTexture->SetDataType(GL_FLOAT);
    this->ColorTexture->SetMinificationFilter(vtkTextureObject::Linear);
    this->ColorTexture->SetMagnificationFilter(vtkTextureObject::Linear);
    this->ColorTexture->Allocate2D(w, h, 4, VTK_FLOAT);
  }

  if (this->PositionTexture == nullptr)
  {
    // This texture needs mipmapping levels in order to improve
    // texture sampling performances
    // see "Scalable ambient obscurance"
    this->PositionTexture = vtkTextureObject::New();
    this->PositionTexture->SetContext(renWin);
    this->PositionTexture->SetFormat(GL_RGB);
    this->PositionTexture->SetInternalFormat(GL_RGB16F);
    this->PositionTexture->SetDataType(GL_FLOAT);
    this->PositionTexture->SetWrapS(vtkTextureObject::ClampToEdge);
    this->PositionTexture->SetWrapT(vtkTextureObject::ClampToEdge);
    this->PositionTexture->SetMinificationFilter(vtkTextureObject::NearestMipmapNearest);
    this->PositionTexture->SetMaxLevel(10);
    this->PositionTexture->Allocate2D(w, h, 3, VTK_FLOAT);
  }

  if (this->NormalTexture == nullptr)
  {
    this->NormalTexture = vtkTextureObject::New();
    this->NormalTexture->SetContext(renWin);
    this->NormalTexture->SetFormat(GL_RGB);
    this->NormalTexture->SetInternalFormat(GL_RGB16F);
    this->NormalTexture->SetDataType(GL_FLOAT);
    this->NormalTexture->SetWrapS(vtkTextureObject::ClampToEdge);
    this->NormalTexture->SetWrapT(vtkTextureObject::ClampToEdge);
    this->NormalTexture->Allocate2D(w, h, 3, VTK_FLOAT);
  }

  if (this->SSAOTexture == nullptr)
  {
    this->SSAOTexture = vtkTextureObject::New();
    this->SSAOTexture->SetContext(renWin);
    this->SSAOTexture->SetFormat(GL_RED);
    this->SSAOTexture->SetInternalFormat(GL_R8);
    this->SSAOTexture->SetDataType(GL_UNSIGNED_BYTE);
    this->SSAOTexture->Allocate2D(w, h, 1, VTK_UNSIGNED_CHAR);
  }

  if (this->DepthTexture == nullptr)
  {
    this->DepthTexture = vtkTextureObject::New();
    this->DepthTexture->SetContext(renWin);
    this->DepthTexture->AllocateDepth(w, h, vtkTextureObject::Float32);
  }

  if (this->FrameBufferObject == nullptr)
  {
    this->FrameBufferObject = vtkOpenGLFramebufferObject::New();
    this->FrameBufferObject->SetContext(renWin);
  }
}

// ----------------------------------------------------------------------------
void vtkSSAOPass::ComputeKernel()
{
  std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
  std::default_random_engine generator;

  this->Kernel.resize(3 * this->KernelSize);

  for (unsigned int i = 0; i < this->KernelSize; ++i)
  {
    float sample[3] = { randomFloats(generator) * 2.f - 1.f, randomFloats(generator) * 2.f - 1.f,
      randomFloats(generator) };

    // reject the sample if not in the hemisphere
    if (vtkMath::Norm(sample) > 1.0)
    {
      i--;
      continue;
    }

    // more samples closer to the point
    float scale = i / static_cast<float>(this->KernelSize);
    scale = 0.1f + 0.9f * scale * scale; // lerp
    vtkMath::MultiplyScalar(sample, scale);

    this->Kernel[3 * i] = sample[0];
    this->Kernel[3 * i + 1] = sample[1];
    this->Kernel[3 * i + 2] = sample[2];
  }
}

// ----------------------------------------------------------------------------
bool vtkSSAOPass::SetShaderParameters(vtkShaderProgram* vtkNotUsed(program),
  vtkAbstractMapper* mapper, vtkProp* vtkNotUsed(prop), vtkOpenGLVertexArrayObject* vtkNotUsed(VAO))
{
  if (vtkOpenGLPolyDataMapper::SafeDownCast(mapper) != nullptr)
  {
    this->FrameBufferObject->ActivateDrawBuffers(3);
  }
  else
  {
    this->FrameBufferObject->ActivateDrawBuffers(1);
  }

  return true;
}

// ----------------------------------------------------------------------------
void vtkSSAOPass::RenderDelegate(const vtkRenderState* s, int w, int h)
{
  this->PreRender(s);

  this->FrameBufferObject->GetContext()->GetState()->PushFramebufferBindings();
  this->FrameBufferObject->Bind();

  this->FrameBufferObject->AddColorAttachment(0, this->ColorTexture);
  this->FrameBufferObject->AddColorAttachment(1, this->PositionTexture);
  this->FrameBufferObject->AddColorAttachment(2, this->NormalTexture);
  this->FrameBufferObject->ActivateDrawBuffers(3);
  this->FrameBufferObject->AddDepthAttachment(this->DepthTexture);
  this->FrameBufferObject->StartNonOrtho(w, h);

  this->DelegatePass->Render(s);
  this->NumberOfRenderedProps += this->DelegatePass->GetNumberOfRenderedProps();

  this->FrameBufferObject->GetContext()->GetState()->PopFramebufferBindings();

  this->PostRender(s);
}

// ----------------------------------------------------------------------------
void vtkSSAOPass::RenderSSAO(vtkOpenGLRenderWindow* renWin, vtkMatrix4x4* projection, int w, int h)
{
  if (this->SSAOQuadHelper && this->SSAOQuadHelper->ShaderChangeValue < this->GetMTime())
  {
    delete this->SSAOQuadHelper;
    this->SSAOQuadHelper = nullptr;
  }

  if (!this->SSAOQuadHelper)
  {
    this->ComputeKernel();

    std::string FSSource = vtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();

    std::stringstream ssDecl;
    ssDecl << "uniform sampler2D texPosition;\n"
              "uniform sampler2D texNormal;\n"
              "uniform sampler2D texNoise;\n"
              "uniform sampler2D texDepth;\n"
              "uniform float kernelRadius;\n"
              "uniform float kernelBias;\n"
              "uniform vec3 samples["
           << this->KernelSize
           << "];\n"
              "uniform mat4 matProjection;\n"
              "uniform ivec2 size;\n";

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Decl", ssDecl.str());

    std::stringstream ssImpl;
    ssImpl
      << "\n"
         "  float occlusion = 0.0;\n"
         "  float depth = texture(texDepth, texCoord).r;\n"
         "  if (depth < 1.0)\n"
         "  {\n"
         "    vec3 fragPosVC = texture(texPosition, texCoord).xyz;\n"
         "    vec4 fragPosDC = matProjection * vec4(fragPosVC, 1.0);\n"
         "    fragPosDC.xyz /= fragPosDC.w;\n"
         "    fragPosDC.xyz = fragPosDC.xyz * 0.5 + 0.5;\n"
         "    if (fragPosDC.z - depth < 0.0001)\n"
         "    {\n"
         "      vec3 normal = texture(texNormal, texCoord).rgb;\n"
         "      vec2 tilingShift = size / textureSize(texNoise, 0);\n"
         "      float randomAngle = 6.283185 * texture(texNoise, texCoord * tilingShift).r;\n"
         "      vec3 randomVec = vec3(cos(randomAngle), sin(randomAngle), 0.0);\n"
         "      vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));\n"
         "      vec3 bitangent = cross(normal, tangent);\n"
         "      mat3 TBN = mat3(tangent, bitangent, normal);\n"
         "      const int kernelSize = "
      << this->KernelSize
      << ";\n"
         "      for (int i = 0; i < kernelSize; i++)\n"
         "      {\n"
         "        vec3 sampleVC = TBN * samples[i];\n"
         "        sampleVC = fragPosVC + sampleVC * kernelRadius;\n"
         "        vec4 sampleDC = matProjection * vec4(sampleVC, 1.0);\n"
         "        sampleDC.xyz /= sampleDC.w;\n"
         "        sampleDC.xyz = sampleDC.xyz * 0.5 + 0.5;\n" // to clip space
         "        float sampleDepth = textureLod(texPosition, sampleDC.xy, 40.0 * "
         "distance(fragPosDC.xy, sampleDC.xy)).z;\n"
         "        float rangeCheck = smoothstep(0.0, 1.0, kernelRadius / abs(fragPosVC.z - "
         "sampleDepth));\n"
         "        occlusion += (sampleDepth >= sampleVC.z + kernelBias ? 1.0 : 0.0) * rangeCheck;\n"
         "      }\n"
         "      occlusion = occlusion / float(kernelSize);\n"
         "    }\n"
         "  }\n"
         "  gl_FragData[0] = vec4(vec3(1.0 - occlusion), 1.0);\n";

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl", ssImpl.str());

    this->SSAOQuadHelper = new vtkOpenGLQuadHelper(renWin,
      vtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader().c_str(), FSSource.c_str(), "");

    this->SSAOQuadHelper->ShaderChangeValue = this->GetMTime();
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->SSAOQuadHelper->Program);
  }

  if (!this->SSAOQuadHelper->Program || !this->SSAOQuadHelper->Program->GetCompiled())
  {
    vtkErrorMacro("Couldn't build the SSAO shader program.");
    return;
  }

  this->PositionTexture->Activate();
  this->NormalTexture->Activate();
  this->DepthTexture->Activate();
  this->SSAOQuadHelper->Program->SetUniformi(
    "texPosition", this->PositionTexture->GetTextureUnit());
  this->SSAOQuadHelper->Program->SetUniformi("texNormal", this->NormalTexture->GetTextureUnit());
  this->SSAOQuadHelper->Program->SetUniform3fv("samples", this->KernelSize, this->Kernel.data());
  this->SSAOQuadHelper->Program->SetUniformi("texNoise", renWin->GetNoiseTextureUnit());
  this->SSAOQuadHelper->Program->SetUniformi("texDepth", this->DepthTexture->GetTextureUnit());
  this->SSAOQuadHelper->Program->SetUniformf("kernelRadius", this->Radius);
  this->SSAOQuadHelper->Program->SetUniformf("kernelBias", this->Bias);
  this->SSAOQuadHelper->Program->SetUniformMatrix("matProjection", projection);

  int size[2] = { w, h };
  this->SSAOQuadHelper->Program->SetUniform2i("size", size);

  this->FrameBufferObject->GetContext()->GetState()->PushFramebufferBindings();
  this->FrameBufferObject->Bind();

  this->FrameBufferObject->AddColorAttachment(0, this->SSAOTexture);
  this->FrameBufferObject->ActivateDrawBuffers(1);
  this->FrameBufferObject->StartNonOrtho(w, h);

  this->SSAOQuadHelper->Render();

  this->FrameBufferObject->GetContext()->GetState()->PopFramebufferBindings();

  this->DepthTexture->Deactivate();
  this->PositionTexture->Deactivate();
  this->NormalTexture->Deactivate();
}

// ----------------------------------------------------------------------------
void vtkSSAOPass::RenderCombine(vtkOpenGLRenderWindow* renWin)
{
  vtkOpenGLState* ostate = renWin->GetState();

  if (this->CombineQuadHelper && this->CombineQuadHelper->ShaderChangeValue < this->GetMTime())
  {
    delete this->CombineQuadHelper;
    this->CombineQuadHelper = nullptr;
  }

  if (!this->CombineQuadHelper)
  {
    std::string FSSource = vtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();

    std::stringstream ssDecl;
    ssDecl << "uniform sampler2D texColor;\n"
              "uniform sampler2D texSSAO;\n"
              "uniform sampler2D texDepth;\n"
              "//VTK::FSQ::Decl";

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Decl", ssDecl.str());

    std::stringstream ssImpl;
    ssImpl << "  vec4 col = texture(texColor, texCoord);\n";

    if (this->Blur)
    {
      ssImpl << "  ivec2 size = textureSize(texSSAO, 0);"
                "  float ao = 0.195346 * texture(texSSAO, texCoord).r + \n"
                "    0.077847	* texture(texSSAO, texCoord + vec2(-1, -1) / size).r +\n"
                "    0.077847 * texture(texSSAO, texCoord + vec2(-1, 1) / size).r +\n"
                "    0.077847 * texture(texSSAO, texCoord + vec2(1, -1) / size).r +\n"
                "    0.077847 * texture(texSSAO, texCoord + vec2(1, 1) / size).r +\n"
                "    0.123317 * texture(texSSAO, texCoord + vec2(-1, 0) / size).r +\n"
                "    0.123317 * texture(texSSAO, texCoord + vec2(1, 0) / size).r +\n"
                "    0.123317 * texture(texSSAO, texCoord + vec2(0, -1) / size).r +\n"
                "    0.123317 * texture(texSSAO, texCoord + vec2(0, 1) / size).r;\n";
    }
    else
    {
      ssImpl << "  float ao = texture(texSSAO, texCoord).r;\n";
    }
    ssImpl << "  gl_FragData[0] = vec4(col.rgb * ao, col.a);\n"
              "  gl_FragDepth = texture(texDepth, texCoord).r;\n";

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl", ssImpl.str());

    this->CombineQuadHelper = new vtkOpenGLQuadHelper(renWin,
      vtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader().c_str(), FSSource.c_str(), "");

    this->CombineQuadHelper->ShaderChangeValue = this->GetMTime();
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->CombineQuadHelper->Program);
  }

  if (!this->CombineQuadHelper->Program || !this->CombineQuadHelper->Program->GetCompiled())
  {
    vtkErrorMacro("Couldn't build the SSAO Combine shader program.");
    return;
  }

  this->ColorTexture->Activate();
  this->SSAOTexture->Activate();
  this->DepthTexture->Activate();
  this->CombineQuadHelper->Program->SetUniformi("texColor", this->ColorTexture->GetTextureUnit());
  this->CombineQuadHelper->Program->SetUniformi("texSSAO", this->SSAOTexture->GetTextureUnit());
  this->CombineQuadHelper->Program->SetUniformi("texDepth", this->DepthTexture->GetTextureUnit());

  ostate->vtkglEnable(GL_DEPTH_TEST);
  ostate->vtkglClear(GL_DEPTH_BUFFER_BIT);

  this->CombineQuadHelper->Render();

  this->DepthTexture->Deactivate();
  this->ColorTexture->Deactivate();
  this->SSAOTexture->Deactivate();
}

// ----------------------------------------------------------------------------
void vtkSSAOPass::Render(const vtkRenderState* s)
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
    vtkWarningMacro("no delegate in vtkSSAOPass.");
    return;
  }

  // create FBO and texture
  int x, y, w, h;
  r->GetTiledSizeAndOrigin(&w, &h, &x, &y);

  this->InitializeGraphicsResources(renWin, w, h);

  this->ColorTexture->Resize(w, h);
  this->PositionTexture->Resize(w, h);
  this->NormalTexture->Resize(w, h);
  this->SSAOTexture->Resize(w, h);
  this->DepthTexture->Resize(w, h);

  ostate->vtkglViewport(x, y, w, h);
  ostate->vtkglScissor(x, y, w, h);

  this->RenderDelegate(s, w, h);

  ostate->vtkglDisable(GL_BLEND);
  ostate->vtkglDisable(GL_DEPTH_TEST);

  // generate mipmap levels
  this->PositionTexture->Bind();
  glGenerateMipmap(GL_TEXTURE_2D);

  vtkOpenGLCamera* cam = (vtkOpenGLCamera*)(r->GetActiveCamera());
  vtkMatrix4x4* projection = cam->GetProjectionTransformMatrix(r->GetTiledAspectRatio(), -1, 1);
  projection->Transpose();

  this->RenderSSAO(renWin, projection, w, h);
  this->RenderCombine(renWin);

  vtkOpenGLCheckErrorMacro("failed after Render");
}

// ----------------------------------------------------------------------------
bool vtkSSAOPass::PreReplaceShaderValues(std::string& vtkNotUsed(vertexShader),
  std::string& vtkNotUsed(geometryShader), std::string& fragmentShader, vtkAbstractMapper* mapper,
  vtkProp* vtkNotUsed(prop))
{
  if (vtkOpenGLPolyDataMapper::SafeDownCast(mapper) != nullptr)
  {
    // apply SSAO after lighting
    vtkShaderProgram::Substitute(fragmentShader, "//VTK::Light::Impl",
      "//VTK::Light::Impl\n"
      "  //VTK::SSAO::Impl\n",
      false);
  }

  return true;
}

// ----------------------------------------------------------------------------
bool vtkSSAOPass::PostReplaceShaderValues(std::string& vtkNotUsed(vertexShader),
  std::string& vtkNotUsed(geometryShader), std::string& fragmentShader, vtkAbstractMapper* mapper,
  vtkProp* vtkNotUsed(prop))
{
  if (vtkOpenGLPolyDataMapper::SafeDownCast(mapper) != nullptr)
  {
    if (fragmentShader.find("vertexVC") != std::string::npos &&
      fragmentShader.find("normalVCVSOutput") != std::string::npos)
    {
      vtkShaderProgram::Substitute(fragmentShader, "  //VTK::SSAO::Impl",
        "  gl_FragData[1] = vec4(vertexVC.xyz, 1.0);\n"
        "  gl_FragData[2] = vec4(normalVCVSOutput, 1.0);\n"
        "\n",
        false);
    }
    else
    {
      vtkShaderProgram::Substitute(fragmentShader, "  //VTK::SSAO::Impl",
        "  gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);\n"
        "  gl_FragData[2] = vec4(0.0, 0.0, 0.0, 0.0);\n"
        "\n",
        false);
    }
  }

  return true;
}

// ----------------------------------------------------------------------------
void vtkSSAOPass::ReleaseGraphicsResources(vtkWindow* w)
{
  this->Superclass::ReleaseGraphicsResources(w);

  if (this->SSAOQuadHelper)
  {
    delete this->SSAOQuadHelper;
    this->SSAOQuadHelper = nullptr;
  }
  if (this->CombineQuadHelper)
  {
    delete this->CombineQuadHelper;
    this->CombineQuadHelper = nullptr;
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
  if (this->PositionTexture)
  {
    this->PositionTexture->Delete();
    this->PositionTexture = nullptr;
  }
  if (this->NormalTexture)
  {
    this->NormalTexture->Delete();
    this->NormalTexture = nullptr;
  }
  if (this->SSAOTexture)
  {
    this->SSAOTexture->Delete();
    this->SSAOTexture = nullptr;
  }
  if (this->DepthTexture)
  {
    this->DepthTexture->Delete();
    this->DepthTexture = nullptr;
  }
}
