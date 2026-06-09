// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2025 F3D-APP Foundation
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHexagonalBokehBlurPass.h"

#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"

VTK_ABI_NAMESPACE_BEGIN

namespace
{

double BlurFuncStep(double circleOfConfusion)
{
  /* extrapolate from `step = 0.1` for `circleOfConfusion = 20`
   * but ensure at least 4 iterations for small `circleOfConfusion` values.
   */
  return 2.0 / std::max(circleOfConfusion, 8.0);
}

constexpr std::string_view GetShaderBlurFunctionSource()
{
  // clang-format off
  return R"#(
const float PI = 3.14159265359;

vec3 BlurTexture(sampler2D tex, vec2 uv, vec2 direction)
{
  vec3 color = vec3(0.0);
  float acc = 0.0;

  // fix for the Y shape artifacts
  uv += 0.5 * invViewDims * direction;

  for (float i = 0.0; i < 1.0; i += step)
  {
    vec2 offset = i * coc * direction * invViewDims;
    color += texture(tex, uv + offset).rgb;
    acc += 1.0;
  }

  return color / acc;
})#";
  // clang-format on
}

vtkSmartPointer<vtkTextureObject> CreateTextureConfig(
  vtkOpenGLRenderWindow* renderWindow, int width, int height)
{
  vtkNew<vtkTextureObject> texture;
  texture->SetContext(renderWindow);
  texture->SetFormat(GL_RGB);
  texture->SetInternalFormat(GL_RGB16F);
  texture->SetDataType(GL_HALF_FLOAT);
  texture->SetMinificationFilter(vtkTextureObject::Linear);
  texture->SetMagnificationFilter(vtkTextureObject::Linear);
  texture->SetWrapS(vtkTextureObject::ClampToEdge);
  texture->SetWrapT(vtkTextureObject::ClampToEdge);
  texture->Allocate2D(width, height, 3, VTK_FLOAT);

  return texture;
}

}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkHexagonalBokehBlurPass);

//------------------------------------------------------------------------------
void vtkHexagonalBokehBlurPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CircleOfConfusionRadius: " << this->CircleOfConfusionRadius;
}

//------------------------------------------------------------------------------
void vtkHexagonalBokehBlurPass::InitializeGraphicsResources(
  vtkOpenGLRenderWindow* renderWindow, int width, int height)
{
  if (!this->BackgroundTexture)
  {
    this->BackgroundTexture = ::CreateTextureConfig(renderWindow, width, height);
  }
  if (!this->VerticalBlurTexture)
  {
    this->VerticalBlurTexture = ::CreateTextureConfig(renderWindow, width, height);
  }
  if (!this->DiagonalBlurTexture)
  {
    this->DiagonalBlurTexture = ::CreateTextureConfig(renderWindow, width, height);
  }

  if (this->FrameBufferObject == nullptr)
  {
    this->FrameBufferObject = vtkSmartPointer<vtkOpenGLFramebufferObject>::New();
    this->FrameBufferObject->SetContext(renderWindow);
  }
}

//------------------------------------------------------------------------------
void vtkHexagonalBokehBlurPass::RenderDelegate(const vtkRenderState* state, int width, int height)
{
  this->PreRender(state);

  this->FrameBufferObject->GetContext()->GetState()->PushFramebufferBindings();
  this->FrameBufferObject->Bind();

  this->FrameBufferObject->AddColorAttachment(0, this->BackgroundTexture);
  this->FrameBufferObject->ActivateDrawBuffers(1);
  this->FrameBufferObject->StartNonOrtho(width, height);

  vtkOpenGLRenderer* glRen = vtkOpenGLRenderer::SafeDownCast(state->GetRenderer());

  glRen->GetState()->vtkglClear(GL_COLOR_BUFFER_BIT);

  this->DelegatePass->Render(state);
  this->NumberOfRenderedProps += this->DelegatePass->GetNumberOfRenderedProps();

  this->FrameBufferObject->RemoveColorAttachments(1);

  this->FrameBufferObject->GetContext()->GetState()->PopFramebufferBindings();

  this->PostRender(state);
}

//------------------------------------------------------------------------------
void vtkHexagonalBokehBlurPass::RenderDirectionalBlur(
  vtkOpenGLRenderWindow* renderWindow, int width, int height, float coc)
{
  if (!this->BlurQuadHelper)
  {
    std::string FSSource = vtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();

    std::stringstream ssDecl;
    ssDecl << "uniform sampler2D backgroundTexture;\n";
    ssDecl << "uniform vec2 invViewDims;\n";
    ssDecl << "uniform float coc;\n";
    ssDecl << "const float step = " << ::BlurFuncStep(coc) << ";\n";
    ssDecl << ::GetShaderBlurFunctionSource();
    ssDecl << "//VTK::FSQ::Decl";

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Decl", ssDecl.str());

    std::stringstream ssImpl;

    ssImpl << "  vec2 blurDir = vec2(cos(PI/2), sin(PI/2));\n";
    ssImpl << "  vec3 color1 = BlurTexture(backgroundTexture, texCoord, blurDir).rgb;\n";
    ssImpl << "  blurDir = vec2(cos(-PI/6), sin(-PI/6));\n";
    ssImpl << "  vec3 color2 = BlurTexture(backgroundTexture, texCoord, blurDir).rgb;\n";
    ssImpl << "  gl_FragData[0] = vec4(color1, 1.0);\n";
    ssImpl << "  gl_FragData[1] = vec4(color1 + color2, 1.0);\n";

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl", ssImpl.str());

    this->BlurQuadHelper = std::make_unique<vtkOpenGLQuadHelper>(renderWindow,
      vtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader().c_str(), FSSource.c_str(), "");
  }

  renderWindow->GetShaderCache()->ReadyShaderProgram(this->BlurQuadHelper->Program);

  if (!this->BlurQuadHelper->Program || !this->BlurQuadHelper->Program->GetCompiled())
  {
    vtkErrorMacro("Couldn't build the Blur shader program.");
    return;
  }

  this->BackgroundTexture->Activate();
  this->BlurQuadHelper->Program->SetUniformi(
    "backgroundTexture", this->BackgroundTexture->GetTextureUnit());

  float invViewDims[2] = { 1.f / static_cast<float>(width), 1.f / static_cast<float>(height) };
  this->BlurQuadHelper->Program->SetUniform2f("invViewDims", invViewDims);

  this->BlurQuadHelper->Program->SetUniformf("coc", coc);

  this->FrameBufferObject->GetContext()->GetState()->PushFramebufferBindings();
  this->FrameBufferObject->Bind();

  this->FrameBufferObject->AddColorAttachment(0, this->VerticalBlurTexture);
  this->FrameBufferObject->AddColorAttachment(1, this->DiagonalBlurTexture);
  this->FrameBufferObject->ActivateDrawBuffers(2);
  this->FrameBufferObject->StartNonOrtho(width, height);

  this->BlurQuadHelper->Render();

  this->FrameBufferObject->RemoveColorAttachments(2);

  this->FrameBufferObject->GetContext()->GetState()->PopFramebufferBindings();

  this->BackgroundTexture->Deactivate();
}

//------------------------------------------------------------------------------
void vtkHexagonalBokehBlurPass::RenderRhomboidBlur(
  vtkOpenGLRenderWindow* renderWindow, int width, int height, float coc)
{
  if (!this->RhomboidQuadHelper)
  {
    std::string FSSource = vtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();

    std::stringstream ssDecl;
    ssDecl << "uniform sampler2D verticalBlurTexture;\n";
    ssDecl << "uniform sampler2D diagonalBlurTexture;\n";
    ssDecl << "uniform vec2 invViewDims;\n";
    ssDecl << "uniform float coc;\n";
    ssDecl << "const float step = " << BlurFuncStep(coc) << ";\n";
    ssDecl << ::GetShaderBlurFunctionSource();
    ssDecl << "//VTK::FSQ::Decl";

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Decl", ssDecl.str());

    std::stringstream ssImpl;

    ssImpl << "  vec2 blurDir = vec2(cos(-PI/6), sin(-PI/6));\n";
    ssImpl << "  vec3 color1 = BlurTexture(verticalBlurTexture, texCoord, blurDir).rgb;\n";
    ssImpl << "  blurDir = vec2(cos(-5*PI/6), sin(-5*PI/6));\n";
    ssImpl << "  vec3 color2 = BlurTexture(diagonalBlurTexture, texCoord, blurDir).rgb;\n";
    ssImpl << "  gl_FragData[0] = vec4((color1 + color2) / 3, 1.0);\n";

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl", ssImpl.str());

    this->RhomboidQuadHelper = std::make_unique<vtkOpenGLQuadHelper>(renderWindow,
      vtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader().c_str(), FSSource.c_str(), "");
  }

  renderWindow->GetShaderCache()->ReadyShaderProgram(this->RhomboidQuadHelper->Program);

  if (!this->RhomboidQuadHelper->Program || !this->RhomboidQuadHelper->Program->GetCompiled())
  {
    vtkErrorMacro("Couldn't build the Rhomboid Blur shader program.");
    return;
  }

  this->VerticalBlurTexture->Activate();
  this->DiagonalBlurTexture->Activate();
  this->RhomboidQuadHelper->Program->SetUniformi(
    "verticalBlurTexture", this->VerticalBlurTexture->GetTextureUnit());
  this->RhomboidQuadHelper->Program->SetUniformi(
    "diagonalBlurTexture", this->DiagonalBlurTexture->GetTextureUnit());

  float invViewDims[2] = { 1.f / static_cast<float>(width), 1.f / static_cast<float>(height) };
  this->RhomboidQuadHelper->Program->SetUniform2f("invViewDims", invViewDims);

  this->RhomboidQuadHelper->Program->SetUniformf("coc", coc);

  this->RhomboidQuadHelper->Render();

  this->BackgroundTexture->Deactivate();
}

//------------------------------------------------------------------------------
void vtkHexagonalBokehBlurPass::Render(const vtkRenderState* states)
{
  vtkOpenGLClearErrorMacro();

  this->NumberOfRenderedProps = 0;

  vtkRenderer* renderer = states->GetRenderer();
  vtkOpenGLRenderWindow* renWin = static_cast<vtkOpenGLRenderWindow*>(renderer->GetRenderWindow());
  vtkOpenGLState* ostate = renWin->GetState();

  vtkOpenGLState::ScopedglEnableDisable bsaver(ostate, GL_BLEND);
  vtkOpenGLState::ScopedglEnableDisable dsaver(ostate, GL_DEPTH_TEST);

  assert(this->DelegatePass != nullptr);

  // create FBO and texture
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
  vtkFrameBufferObjectBase* fbo = states->GetFrameBuffer();
  if (fbo)
  {
    fbo->GetLastSize(width, height);
  }
  else
  {
    renderer->GetTiledSizeAndOrigin(&width, &height, &x, &y);
  }

  this->InitializeGraphicsResources(renWin, width, height);

  this->BackgroundTexture->Resize(width, height);
  this->VerticalBlurTexture->Resize(width, height);
  this->DiagonalBlurTexture->Resize(width, height);

  ostate->vtkglViewport(x, y, width, height);
  ostate->vtkglScissor(x, y, width, height);

  this->RenderDelegate(states, width, height);

  ostate->vtkglDisable(GL_BLEND);
  ostate->vtkglDisable(GL_DEPTH_TEST);

  // Ensure the value is above 0
  if (this->CircleOfConfusionRadius < 0.0f)
  {
    vtkLogF(
      WARNING, "Circle of confusion radius should be a positive value. Clamping value to 0.0.");
  }
  float coc = std::max(this->CircleOfConfusionRadius, 0.0f);

  this->RenderDirectionalBlur(renWin, width, height, coc);
  this->RenderRhomboidBlur(renWin, width, height, coc);

  vtkOpenGLCheckErrorMacro("failed after Render");
}

//------------------------------------------------------------------------------
void vtkHexagonalBokehBlurPass::ReleaseGraphicsResources(vtkWindow* renderWindow)
{
  assert(renderWindow != nullptr);

  this->Superclass::ReleaseGraphicsResources(renderWindow);

  if (this->BlurQuadHelper != nullptr)
  {
    this->BlurQuadHelper->ReleaseGraphicsResources(renderWindow);
  }
  if (this->RhomboidQuadHelper != nullptr)
  {
    this->RhomboidQuadHelper->ReleaseGraphicsResources(renderWindow);
  }

  if (this->FrameBufferObject != nullptr)
  {
    this->FrameBufferObject->ReleaseGraphicsResources(renderWindow);
  }

  if (this->DiagonalBlurTexture != nullptr)
  {
    this->DiagonalBlurTexture->ReleaseGraphicsResources(renderWindow);
  }
  if (this->VerticalBlurTexture != nullptr)
  {
    this->VerticalBlurTexture->ReleaseGraphicsResources(renderWindow);
  }
  if (this->BackgroundTexture != nullptr)
  {
    this->BackgroundTexture->ReleaseGraphicsResources(renderWindow);
  }
}

VTK_ABI_NAMESPACE_END
