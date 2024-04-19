// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAnariPass.h"
#include "vtkAnariProfiling.h"
#include "vtkAnariRendererNode.h"
#include "vtkAnariViewNodeFactory.h"

#include "vtkCamera.h"
#include "vtkCameraPass.h"
#include "vtkFrameBufferObjectBase.h"
#include "vtkLightsPass.h"
#include "vtkObjectFactory.h"
#include "vtkOpaquePass.h"
#include "vtkOverlayPass.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderState.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSequencePass.h"
#include "vtkVolumetricPass.h"

#include "vtkOpenGLQuadHelper.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"

#include <sstream>
#include <stdexcept>

VTK_ABI_NAMESPACE_BEGIN

// ----------------------------------------------------------------------------
class vtkAnariPassInternals : public vtkRenderPass
{
public:
  static vtkAnariPassInternals* New();
  vtkTypeMacro(vtkAnariPassInternals, vtkRenderPass);

  vtkAnariPassInternals()
    : Parent(nullptr)
    , OpenGLQuadHelper(nullptr)
  {
  }

  ~vtkAnariPassInternals() { delete this->OpenGLQuadHelper; }

  void Init(vtkOpenGLRenderWindow* openGLRenderWindow, const anari::Extensions& extensions,
    vtkRenderer* ren)
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
    this->OpenGLQuadHelper = new vtkOpenGLQuadHelper(openGLRenderWindow,
      vtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader().c_str(), fragShader.c_str(), "");

    this->ColorTexture->SetContext(openGLRenderWindow);
    this->ColorTexture->AutoParametersOff();
    this->DepthTexture->SetContext(openGLRenderWindow);
    this->DepthTexture->AutoParametersOff();
    this->SharedColorTexture->SetContext(openGLRenderWindow);
    this->SharedColorTexture->AutoParametersOff();
    this->SharedDepthTexture->SetContext(openGLRenderWindow);
    this->SharedDepthTexture->AutoParametersOff();
  }

  void Render(const vtkRenderState* s) override { this->Parent->RenderInternal(s); }

  vtkAnariPass* Parent;
  vtkOpenGLQuadHelper* OpenGLQuadHelper;

  vtkNew<vtkAnariViewNodeFactory> Factory;
  vtkNew<vtkTextureObject> ColorTexture;
  vtkNew<vtkTextureObject> DepthTexture;
  vtkNew<vtkTextureObject> SharedColorTexture;
  vtkNew<vtkTextureObject> SharedDepthTexture;
};

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAnariPassInternals);

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAnariPass);

// ----------------------------------------------------------------------------
vtkAnariPass::vtkAnariPass()
  : SceneGraph(nullptr)
{
  this->Internal = vtkAnariPassInternals::New();
  this->Internal->Parent = this;
  this->PreviousRendererSubtype = nullptr;

  vtkNew<vtkRenderPassCollection> renderPassCollection;

  vtkNew<vtkLightsPass> lightPass;
  renderPassCollection->AddItem(lightPass);
  renderPassCollection->AddItem(this->Internal);

  vtkNew<vtkOverlayPass> overlayPass;
  renderPassCollection->AddItem(overlayPass);

  vtkNew<vtkSequencePass> sequencePass;
  sequencePass->SetPasses(renderPassCollection);

  this->CameraPass->SetDelegatePass(sequencePass);
}

// ----------------------------------------------------------------------------
vtkAnariPass::~vtkAnariPass()
{
  this->SetSceneGraph(nullptr);
  this->Internal->Delete();
  this->Internal = nullptr;
}

// ----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkAnariPass, SceneGraph, vtkAnariRendererNode);

// ----------------------------------------------------------------------------
void vtkAnariPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
void vtkAnariPass::Render(const vtkRenderState* s)
{
  vtkAnariProfiling startProfiling("vtkAnariPass::Render", vtkAnariProfiling::YELLOW);
  vtkRenderer* ren = s->GetRenderer();

  if (ren)
  {
    const char* rendererSubtype = vtkAnariRendererNode::GetRendererSubtype(ren);

    if (this->PreviousRendererSubtype && this->SceneGraph &&
      strcmp(this->PreviousRendererSubtype, rendererSubtype) != 0)
    {
      this->SceneGraph->Delete();
      this->SceneGraph = nullptr;
    }

    if (!this->SceneGraph)
    {
      this->SceneGraph =
        vtkAnariRendererNode::SafeDownCast(this->Internal->Factory->CreateNode(ren));
    }

    this->PreviousRendererSubtype = rendererSubtype;
  }

  this->CameraPass->Render(s);
}

// ----------------------------------------------------------------------------
void vtkAnariPass::RenderInternal(const vtkRenderState* s)
{
  vtkAnariProfiling startProfiling("vtkAnariPass::RenderInternal", vtkAnariProfiling::YELLOW);
  this->NumberOfRenderedProps = 0;

  if (this->SceneGraph)
  {
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

    vtkAnariRendererNode* anariRendererNode =
      vtkAnariRendererNode::SafeDownCast(this->SceneGraph->GetViewNodeFor(ren));
    anariRendererNode->SetSize(viewportWidth, viewportHeight);
    anariRendererNode->SetViewport(tileViewport);
    anariRendererNode->SetScale(tileScale);

    this->SceneGraph->TraverseAllPasses();

    // Copy result to the window
    const int colorTexGL = this->SceneGraph->GetColorBufferTextureGL();
    const int depthTexGL = this->SceneGraph->GetDepthBufferTextureGL();

    vtkRenderWindow* rwin = vtkRenderWindow::SafeDownCast(ren->GetVTKWindow());
    vtkOpenGLRenderWindow* windowOpenGL = vtkOpenGLRenderWindow::SafeDownCast(rwin);

    if (this->Internal->OpenGLQuadHelper)
    {
      delete this->Internal->OpenGLQuadHelper;
      this->Internal->OpenGLQuadHelper = nullptr;
    }

    this->Internal->Init(windowOpenGL, anariRendererNode->GetAnariDeviceExtensions(), ren);
    //  windowOpenGL->GetShaderCache()->ReadyShaderProgram(this->Internal->QuadHelper->Program);

    if (!this->Internal->OpenGLQuadHelper->Program ||
      !this->Internal->OpenGLQuadHelper->Program->GetCompiled())
    {
      vtkErrorMacro("Couldn't build the shader program.");
      return;
    }

    windowOpenGL->MakeCurrent();

    vtkTextureObject* usedColorTex = nullptr;
    vtkTextureObject* usedDepthTex = nullptr;

    if (colorTexGL != 0 && depthTexGL != 0)
    {
      // for visRTX, re-use existing OpenGL texture provided
      this->Internal->SharedColorTexture->AssignToExistingTexture(colorTexGL, GL_TEXTURE_2D);
      this->Internal->SharedDepthTexture->AssignToExistingTexture(depthTexGL, GL_TEXTURE_2D);

      usedColorTex = this->Internal->SharedColorTexture;
      usedDepthTex = this->Internal->SharedDepthTexture;
    }
    else
    {
      // upload to the texture
      this->Internal->ColorTexture->Create2DFromRaw(viewportWidth, viewportHeight, 4,
        VTK_UNSIGNED_CHAR, const_cast<unsigned char*>(this->SceneGraph->GetBuffer()));
      this->Internal->DepthTexture->CreateDepthFromRaw(viewportWidth, viewportHeight,
        vtkTextureObject::Float32, VTK_FLOAT, const_cast<float*>(this->SceneGraph->GetZBuffer()));

      usedColorTex = this->Internal->ColorTexture;
      usedDepthTex = this->Internal->DepthTexture;
    }

    usedColorTex->Activate();
    usedDepthTex->Activate();

    this->Internal->OpenGLQuadHelper->Program->SetUniformi(
      "colorTexture", usedColorTex->GetTextureUnit());
    this->Internal->OpenGLQuadHelper->Program->SetUniformi(
      "depthTexture", usedDepthTex->GetTextureUnit());

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

      if (vtkAnariRendererNode::GetCompositeOnGL(ren))
      {
        openGLState->vtkglBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
      }
      else
      {
        openGLState->vtkglBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
      }
    }

    this->Internal->OpenGLQuadHelper->Render();

    usedColorTex->Deactivate();
    usedDepthTex->Deactivate();
  }
}

// ----------------------------------------------------------------------------
vtkViewNodeFactory* vtkAnariPass::GetViewNodeFactory()
{
  return this->Internal->Factory;
}

VTK_ABI_NAMESPACE_END
