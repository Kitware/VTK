// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAnariPass.h"

#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLQuadHelper.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkRenderState.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"

#include "vtkAnariProfiling.h"
#include "vtkAnariViewNodeFactory.h"

#include <memory>
#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

// ----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkAnariPass, SceneGraph, vtkAnariSceneGraph);

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAnariPass);

// ----------------------------------------------------------------------------
void vtkAnariPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
void vtkAnariPass::Render(const vtkRenderState* states)
{
  this->NumberOfRenderedProps = 0;

  vtkRenderer* renderer = states->GetRenderer();
  vtkOpenGLFramebufferObject* fbo =
    vtkOpenGLFramebufferObject::SafeDownCast(states->GetFrameBuffer());

  vtkFrameInformation frameInfo;
  if (fbo)
  {
    fbo->GetLastSize(frameInfo.ViewportWidth, frameInfo.ViewportHeight);
  }
  else
  {
    renderer->GetTiledSizeAndOrigin(&frameInfo.ViewportWidth, &frameInfo.ViewportHeight,
      &frameInfo.ViewportX, &frameInfo.ViewportY);
    vtkWindow* renderWindow = renderer->GetRenderWindow();
    renderWindow->GetTileViewport(frameInfo.TileViewport);
    renderWindow->GetTileScale(frameInfo.TileScale);
  }

  this->RenderAnariFrame(renderer, frameInfo);
  this->BlitAnariFrameToOpenGLFrame(renderer, frameInfo);
}

// ----------------------------------------------------------------------------
vtkAnariDevice* vtkAnariPass::GetAnariDevice()
{
  return this->Device;
}

// ----------------------------------------------------------------------------
vtkAnariRenderer* vtkAnariPass::GetAnariRenderer()
{
  return this->Renderer;
}

// ----------------------------------------------------------------------------
vtkViewNodeFactory* vtkAnariPass::GetViewNodeFactory()
{
  return this->Factory;
}

// ----------------------------------------------------------------------------
vtkAnariPass::vtkAnariPass()
{
  this->GetAnariDevice()->SetOnNewDeviceCallback(
    [&]() { this->GetAnariRenderer()->SetAnariDevice(this->GetAnariDevice()); });
}

// ----------------------------------------------------------------------------
void vtkAnariPass::RenderAnariFrame(vtkRenderer* renderer, const vtkFrameInformation& frameInfo)
{
  vtkAnariProfiling startProfiling("vtkAnariPass::Render", vtkAnariProfiling::YELLOW);

  auto* anariDevice = this->GetAnariDevice();
  auto* anariRenderer = this->GetAnariRenderer();

  if (!anariDevice->AnariInitialized())
  {
    if (!anariDevice->SetupAnariDeviceFromLibrary("environment", "default", false))
    {
      return;
    }
  }

  if (renderer)
  {
    if (!this->SceneGraph)
    {
      vtkAnariSceneGraph* sceneGraph =
        vtkAnariSceneGraph::SafeDownCast(this->Factory->CreateNode(renderer));
      this->SceneGraph = vtkSmartPointer<vtkAnariSceneGraph>::Take(sceneGraph);
      this->SceneGraph->SetAnariDevice(anariDevice);
      this->SceneGraph->SetAnariRenderer(anariRenderer);
    }
    else if (anariRenderer != this->SceneGraph->GetAnariRenderer())
    {
      this->SceneGraph->SetAnariRenderer(anariRenderer);
    }
  }

  if (!this->SceneGraph)
  {
    return;
  }

  vtkAnariSceneGraph* anariRendererNode =
    vtkAnariSceneGraph::SafeDownCast(this->SceneGraph->GetViewNodeFor(renderer));
  anariRendererNode->SetSize(frameInfo.ViewportWidth, frameInfo.ViewportHeight);
  anariRendererNode->SetViewport(frameInfo.TileViewport);
  anariRendererNode->SetScale(frameInfo.TileScale);

  this->SceneGraph->TraverseAllPasses();
}

// ----------------------------------------------------------------------------
void vtkAnariPass::BlitAnariFrameToOpenGLFrame(
  vtkRenderer* renderer, const vtkFrameInformation& frameInfo)
{
  if (!this->SceneGraph)
  {
    return;
  }

  vtkRenderWindow* renderWindow = vtkRenderWindow::SafeDownCast(renderer->GetVTKWindow());
  vtkOpenGLRenderWindow* windowOpenGL = vtkOpenGLRenderWindow::SafeDownCast(renderWindow);

  this->SetupFrame(windowOpenGL, renderer, this->GetAnariDevice()->GetExtensions());
  if (!this->OpenGLQuadHelper->Program || !this->OpenGLQuadHelper->Program->GetCompiled())
  {
    vtkErrorMacro("Couldn't build the shader program.");
    return;
  }

  windowOpenGL->MakeCurrent();

  // upload to the texture //
  this->ColorTexture->Create2DFromRaw(frameInfo.ViewportWidth, frameInfo.ViewportHeight, 4,
    VTK_UNSIGNED_CHAR, const_cast<unsigned char*>(this->SceneGraph->GetBuffer()));
  this->DepthTexture->CreateDepthFromRaw(frameInfo.ViewportWidth, frameInfo.ViewportHeight,
    vtkTextureObject::Float32, VTK_FLOAT, const_cast<float*>(this->SceneGraph->GetZBuffer()));

  this->ColorTexture->Activate();
  this->DepthTexture->Activate();

  this->OpenGLQuadHelper->Program->SetUniformi(
    "colorTexture", this->ColorTexture->GetTextureUnit());
  this->OpenGLQuadHelper->Program->SetUniformi(
    "depthTexture", this->DepthTexture->GetTextureUnit());

  vtkOpenGLState* openGLState = windowOpenGL->GetState();
  vtkOpenGLState::ScopedglEnableDisable dsaver(openGLState, GL_DEPTH_TEST);
  vtkOpenGLState::ScopedglEnableDisable bsaver(openGLState, GL_BLEND);
  vtkOpenGLState::ScopedglDepthFunc dfsaver(openGLState);
  vtkOpenGLState::ScopedglBlendFuncSeparate bfsaver(openGLState);

  openGLState->vtkglViewport(
    frameInfo.ViewportX, frameInfo.ViewportY, frameInfo.ViewportWidth, frameInfo.ViewportHeight);
  openGLState->vtkglScissor(
    frameInfo.ViewportX, frameInfo.ViewportY, frameInfo.ViewportWidth, frameInfo.ViewportHeight);
  openGLState->vtkglEnable(GL_DEPTH_TEST);

  vtkOpenGLState::ScopedglDepthFunc depthFunctionSaver(openGLState);
  openGLState->vtkglDepthFunc(GL_ALWAYS);

  if (renderer->GetLayer() == 0)
  {
    openGLState->vtkglDisable(GL_BLEND);
  }
  else
  {
    openGLState->vtkglEnable(GL_BLEND);

    if (vtkAnariSceneGraph::GetCompositeOnGL(renderer))
    {
      openGLState->vtkglBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
    }
    else
    {
      openGLState->vtkglBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
    }
  }

  this->OpenGLQuadHelper->Render();

  this->ColorTexture->Deactivate();
  this->DepthTexture->Deactivate();
}

// ----------------------------------------------------------------------------
void vtkAnariPass::SetupFrame(vtkOpenGLRenderWindow* openGLRenderWindow, vtkRenderer* renderer,
  const anari::Extensions& extensions)
{
  std::string fragShader = vtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();
  vtkShaderProgram::Substitute(fragShader, "//VTK::FSQ::Decl",
    "uniform sampler2D colorTexture;\n"
    "uniform sampler2D depthTexture;\n");

  std::stringstream ss;
  ss << "vec4 color = texture(colorTexture, texCoord);\n"
     << "gl_FragDepth = texture(depthTexture, texCoord).r;\n";

  bool useHDRI = renderer->GetUseImageBasedLighting() && renderer->GetEnvironmentTexture() &&
    extensions.ANARI_KHR_LIGHT_HDRI;
  ss << "gl_FragData[0] = vec4(color.rgb, " << (useHDRI ? "1.0)" : "color.a)") << ";\n";

  vtkShaderProgram::Substitute(fragShader, "//VTK::FSQ::Impl", ss.str());
  this->OpenGLQuadHelper.reset(new vtkOpenGLQuadHelper(openGLRenderWindow,
    vtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader().c_str(), fragShader.c_str(), ""));

  this->ColorTexture->SetContext(openGLRenderWindow);
  this->ColorTexture->AutoParametersOff();
  this->DepthTexture->SetContext(openGLRenderWindow);
  this->DepthTexture->AutoParametersOff();
  this->SharedColorTexture->SetContext(openGLRenderWindow);
  this->SharedColorTexture->AutoParametersOff();
  this->SharedDepthTexture->SetContext(openGLRenderWindow);
  this->SharedDepthTexture->AutoParametersOff();
}

VTK_ABI_NAMESPACE_END
