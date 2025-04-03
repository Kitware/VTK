// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAnariPass.h"
#include "vtkAnariProfiling.h"
#include "vtkAnariSceneGraph.h"
#include "vtkAnariViewNodeFactory.h"

#include "vtkCamera.h"
#include "vtkCameraPass.h"
#include "vtkFrameBufferObjectBase.h"
#include "vtkObjectFactory.h"
#include "vtkOverlayPass.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderState.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSequencePass.h"

#include "vtkOpenGLQuadHelper.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"

#include <memory>
#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

// ----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkAnariPass, SceneGraph, vtkAnariSceneGraph);

// ----------------------------------------------------------------------------
class vtkAnariPassInternals : public vtkRenderPass
{
public:
  static vtkAnariPassInternals* New();
  vtkTypeMacro(vtkAnariPassInternals, vtkRenderPass);

  vtkAnariPassInternals() = default;
  ~vtkAnariPassInternals() override = default;

  void SetupFrame(vtkOpenGLRenderWindow* openGLRenderWindow, const anari::Extensions& extensions,
    vtkRenderer* ren);
  void Render(const vtkRenderState* s) override;

  vtkAnariPass* Parent{ nullptr };
  std::unique_ptr<vtkOpenGLQuadHelper> OpenGLQuadHelper;

  vtkNew<vtkAnariDevice> Device;
  vtkNew<vtkAnariRenderer> Renderer;

  vtkNew<vtkAnariViewNodeFactory> Factory;
  vtkNew<vtkTextureObject> ColorTexture;
  vtkNew<vtkTextureObject> DepthTexture;
  vtkNew<vtkTextureObject> SharedColorTexture;
  vtkNew<vtkTextureObject> SharedDepthTexture;
};

// ----------------------------------------------------------------------------
void vtkAnariPassInternals::SetupFrame(
  vtkOpenGLRenderWindow* openGLRenderWindow, const anari::Extensions& extensions, vtkRenderer* ren)
{
  std::string fragShader = vtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();
  vtkShaderProgram::Substitute(fragShader, "//VTK::FSQ::Decl",
    "uniform sampler2D colorTexture;\n"
    "uniform sampler2D depthTexture;\n");

  std::stringstream ss;
  ss << "vec4 color = texture(colorTexture, texCoord);\n"
     << "gl_FragDepth = texture(depthTexture, texCoord).r;\n";

  bool useHDRI = ren->GetUseImageBasedLighting() && ren->GetEnvironmentTexture() &&
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

// ----------------------------------------------------------------------------
void vtkAnariPassInternals::Render(const vtkRenderState* s)
{
  vtkAnariProfiling startProfiling("vtkAnariPass::RenderInternal", vtkAnariProfiling::YELLOW);
  this->NumberOfRenderedProps = 0;

  auto* sceneGraph = this->Parent->SceneGraph;

  if (!sceneGraph)
  {
    return;
  }

  vtkRenderer* ren = s->GetRenderer();
  vtkFrameBufferObjectBase* fbo = s->GetFrameBuffer();
  int viewportX, viewportY;
  int viewportWidth, viewportHeight;
  double tileViewport[4];
  int tileScale[2];

  if (fbo)
  {
    viewportX = 0;
    viewportY = 0;
    fbo->GetLastSize(viewportWidth, viewportHeight);

    tileViewport[0] = tileViewport[1] = 0.0;
    tileViewport[2] = tileViewport[3] = 1.0;
    tileScale[0] = tileScale[1] = 1;
  }
  else
  {
    ren->GetTiledSizeAndOrigin(&viewportWidth, &viewportHeight, &viewportX, &viewportY);
    vtkWindow* win = ren->GetVTKWindow();
    win->GetTileViewport(tileViewport);
    win->GetTileScale(tileScale);
  }

  vtkAnariSceneGraph* anariRendererNode =
    vtkAnariSceneGraph::SafeDownCast(sceneGraph->GetViewNodeFor(ren));
  anariRendererNode->SetSize(viewportWidth, viewportHeight);
  anariRendererNode->SetViewport(tileViewport);
  anariRendererNode->SetScale(tileScale);

  sceneGraph->TraverseAllPasses();

  // Copy result to the window //

  vtkRenderWindow* rwin = vtkRenderWindow::SafeDownCast(ren->GetVTKWindow());
  vtkOpenGLRenderWindow* windowOpenGL = vtkOpenGLRenderWindow::SafeDownCast(rwin);

  this->SetupFrame(windowOpenGL, anariRendererNode->GetAnariDeviceExtensions(), ren);
  if (!this->OpenGLQuadHelper->Program || !this->OpenGLQuadHelper->Program->GetCompiled())
  {
    vtkErrorMacro("Couldn't build the shader program.");
    return;
  }

  windowOpenGL->MakeCurrent();

  // upload to the texture //

  this->ColorTexture->Create2DFromRaw(viewportWidth, viewportHeight, 4, VTK_UNSIGNED_CHAR,
    const_cast<unsigned char*>(sceneGraph->GetBuffer()));
  this->DepthTexture->CreateDepthFromRaw(viewportWidth, viewportHeight, vtkTextureObject::Float32,
    VTK_FLOAT, const_cast<float*>(sceneGraph->GetZBuffer()));

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

  openGLState->vtkglViewport(viewportX, viewportY, viewportWidth, viewportHeight);
  openGLState->vtkglScissor(viewportX, viewportY, viewportWidth, viewportHeight);
  openGLState->vtkglEnable(GL_DEPTH_TEST);

  if (ren->GetLayer() == 0)
  {
    openGLState->vtkglDisable(GL_BLEND);
    openGLState->vtkglDepthFunc(GL_ALWAYS);
  }
  else
  {
    openGLState->vtkglEnable(GL_BLEND);
    openGLState->vtkglDepthFunc(GL_LESS);

    if (vtkAnariSceneGraph::GetCompositeOnGL(ren))
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
vtkStandardNewMacro(vtkAnariPassInternals);

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAnariPass);

// ----------------------------------------------------------------------------
void vtkAnariPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
void vtkAnariPass::Render(const vtkRenderState* s)
{
  vtkAnariProfiling startProfiling("vtkAnariPass::Render", vtkAnariProfiling::YELLOW);

  auto* ad = this->GetAnariDevice();
  auto* ar = this->GetAnariRenderer();

  if (!ad->AnariInitialized())
  {
    ad->SetupAnariDeviceFromLibrary("environment", "default", false);
  }

  anari::Device device = ad->GetHandle();

  vtkRenderer* ren = s->GetRenderer();
  if (ren)
  {
    const bool rebuildSceneGraph =
      !this->SceneGraph || this->SceneGraph->GetDeviceHandle() != device;
    if (rebuildSceneGraph)
    {
      this->SceneGraph = vtkAnariSceneGraph::SafeDownCast(this->Internal->Factory->CreateNode(ren));
      this->SceneGraph->SetAnariDevice(
        ad, ad->GetAnariDeviceExtensions(), ad->GetAnariDeviceExtensionStrings());
      this->SceneGraph->SetAnariRenderer(ar->GetHandle());
    }
    else if (ar->GetHandle() != this->SceneGraph->GetRendererHandle())
    {
      this->SceneGraph->SetAnariRenderer(ar->GetHandle());
    }
  }

  this->CameraPass->Render(s);
}

// ----------------------------------------------------------------------------
vtkAnariDevice* vtkAnariPass::GetAnariDevice()
{
  return this->Internal->Device;
}

// ----------------------------------------------------------------------------
vtkAnariRenderer* vtkAnariPass::GetAnariRenderer()
{
  return this->Internal->Renderer;
}

// ----------------------------------------------------------------------------
vtkViewNodeFactory* vtkAnariPass::GetViewNodeFactory()
{
  return this->Internal->Factory;
}

// ----------------------------------------------------------------------------
vtkAnariPass::vtkAnariPass()
{
  this->Internal = vtkAnariPassInternals::New();
  this->Internal->Parent = this;

  vtkNew<vtkRenderPassCollection> renderPassCollection;

  renderPassCollection->AddItem(this->Internal);

  vtkNew<vtkOverlayPass> overlayPass;
  renderPassCollection->AddItem(overlayPass);

  vtkNew<vtkSequencePass> sequencePass;
  sequencePass->SetPasses(renderPassCollection);

  this->CameraPass->SetDelegatePass(sequencePass);

  this->GetAnariDevice()->SetOnNewDeviceCallback(
    [&](anari::Device d) { this->GetAnariRenderer()->SetAnariDevice(d); });
}

// ----------------------------------------------------------------------------
vtkAnariPass::~vtkAnariPass()
{
  this->SetSceneGraph(nullptr);
  this->Internal->Delete();
  this->Internal = nullptr;
}

VTK_ABI_NAMESPACE_END
