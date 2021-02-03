/*=========================================================================

  Prograxq:   Visualization Toolkit
  Module:    vtkOSPRayPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtk_glew.h>

#include "vtkCamera.h"
#include "vtkCameraPass.h"
#include "vtkFrameBufferObjectBase.h"
#include "vtkLightsPass.h"
#include "vtkOSPRayPass.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkOSPRayViewNodeFactory.h"
#include "vtkObjectFactory.h"
#include "vtkOpaquePass.h"
#include "vtkOpenGLQuadHelper.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOverlayPass.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkSequencePass.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"
#include "vtkVolumetricPass.h"

#include "RTWrapper/RTWrapper.h"

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <sys/types.h>
#endif

class vtkOSPRayPassInternals : public vtkRenderPass
{
public:
  static vtkOSPRayPassInternals* New();
  vtkTypeMacro(vtkOSPRayPassInternals, vtkRenderPass);

  vtkOSPRayPassInternals() = default;

  ~vtkOSPRayPassInternals() override { delete this->QuadHelper; }

  void Init(vtkOpenGLRenderWindow* context)
  {
    std::string FSSource = vtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Decl",
      "uniform sampler2D colorTexture;\n"
      "uniform sampler2D depthTexture;\n");

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl",
      "gl_FragData[0] = texture(colorTexture, texCoord);\n"
      "gl_FragDepth = texture(depthTexture, texCoord).r;\n");

    this->QuadHelper = new vtkOpenGLQuadHelper(context,
      vtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader().c_str(), FSSource.c_str(), "");

    this->ColorTexture->SetContext(context);
    this->ColorTexture->AutoParametersOff();
    this->DepthTexture->SetContext(context);
    this->DepthTexture->AutoParametersOff();
    this->SharedColorTexture->SetContext(context);
    this->SharedColorTexture->AutoParametersOff();
    this->SharedDepthTexture->SetContext(context);
    this->SharedDepthTexture->AutoParametersOff();
  }

  void Render(const vtkRenderState* s) override { this->Parent->RenderInternal(s); }

  vtkNew<vtkOSPRayViewNodeFactory> Factory;
  vtkOSPRayPass* Parent = nullptr;

  // OpenGL-based display
  vtkOpenGLQuadHelper* QuadHelper = nullptr;
  vtkNew<vtkTextureObject> ColorTexture;
  vtkNew<vtkTextureObject> DepthTexture;
  vtkNew<vtkTextureObject> SharedColorTexture;
  vtkNew<vtkTextureObject> SharedDepthTexture;
};

int vtkOSPRayPass::RTDeviceRefCount = 0;

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOSPRayPassInternals);

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOSPRayPass);

// ----------------------------------------------------------------------------
vtkOSPRayPass::vtkOSPRayPass()
{
  this->SceneGraph = nullptr;

  vtkOSPRayPass::RTInit();

  this->Internal = vtkOSPRayPassInternals::New();
  this->Internal->Parent = this;

  this->CameraPass = vtkCameraPass::New();
  this->LightsPass = vtkLightsPass::New();
  this->SequencePass = vtkSequencePass::New();
  this->VolumetricPass = vtkVolumetricPass::New();
  this->OverlayPass = vtkOverlayPass::New();

  this->RenderPassCollection = vtkRenderPassCollection::New();
  this->RenderPassCollection->AddItem(this->LightsPass);
  this->RenderPassCollection->AddItem(this->Internal);
  this->RenderPassCollection->AddItem(this->OverlayPass);

  this->SequencePass->SetPasses(this->RenderPassCollection);
  this->CameraPass->SetDelegatePass(this->SequencePass);

  this->PreviousType = "none";
}

// ----------------------------------------------------------------------------
vtkOSPRayPass::~vtkOSPRayPass()
{
  this->SetSceneGraph(nullptr);
  this->Internal->Delete();
  this->Internal = 0;
  if (this->CameraPass)
  {
    this->CameraPass->Delete();
    this->CameraPass = 0;
  }
  if (this->LightsPass)
  {
    this->LightsPass->Delete();
    this->LightsPass = 0;
  }
  if (this->SequencePass)
  {
    this->SequencePass->Delete();
    this->SequencePass = 0;
  }
  if (this->VolumetricPass)
  {
    this->VolumetricPass->Delete();
    this->VolumetricPass = 0;
  }
  if (this->OverlayPass)
  {
    this->OverlayPass->Delete();
    this->OverlayPass = 0;
  }
  if (this->RenderPassCollection)
  {
    this->RenderPassCollection->Delete();
    this->RenderPassCollection = 0;
  }
  vtkOSPRayPass::RTShutdown();
}

// ----------------------------------------------------------------------------
void vtkOSPRayPass::RTInit()
{
  if (!vtkOSPRayPass::IsSupported())
  {
    return;
  }
  if (RTDeviceRefCount == 0)
  {
    rtwInit();
  }
  RTDeviceRefCount++;
}

// ----------------------------------------------------------------------------
void vtkOSPRayPass::RTShutdown()
{
  if (!vtkOSPRayPass::IsSupported())
  {
    return;
  }
  --RTDeviceRefCount;
  if (RTDeviceRefCount == 0)
  {
    rtwShutdown();
  }
}

// ----------------------------------------------------------------------------
void vtkOSPRayPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkOSPRayPass, SceneGraph, vtkOSPRayRendererNode);

// ----------------------------------------------------------------------------
void vtkOSPRayPass::Render(const vtkRenderState* s)
{
  if (!vtkOSPRayPass::IsSupported())
  {
    static bool warned = false;
    if (!warned)
    {
      vtkWarningMacro(<< "Ignoring render request because OSPRay is not supported.");
      warned = true;
    }
    return;
  }

  vtkRenderer* ren = s->GetRenderer();
  if (ren)
  {
    std::string type = vtkOSPRayRendererNode::GetRendererType(ren);
    if (this->PreviousType != type && this->SceneGraph)
    {
      this->SceneGraph->Delete();
      this->SceneGraph = nullptr;
    }
    if (!this->SceneGraph)
    {
      this->SceneGraph =
        vtkOSPRayRendererNode::SafeDownCast(this->Internal->Factory->CreateNode(ren));
    }
    this->PreviousType = type;
  }

  this->CameraPass->Render(s);
}

// ----------------------------------------------------------------------------
void vtkOSPRayPass::RenderInternal(const vtkRenderState* s)
{
  if (!vtkOSPRayPass::IsSupported())
  {
    static bool warned = false;
    if (!warned)
    {
      vtkWarningMacro(<< "Ignoring render request because OSPRay is not supported.");
      warned = true;
    }
    return;
  }

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

    vtkOSPRayRendererNode* oren =
      vtkOSPRayRendererNode::SafeDownCast(this->SceneGraph->GetViewNodeFor(ren));

    oren->SetSize(viewportWidth, viewportHeight);
    oren->SetViewport(tileViewport);
    oren->SetScale(tileScale);

    this->SceneGraph->TraverseAllPasses();

    if (oren->GetBackend() == nullptr)
    {
      return;
    }

    // copy the result to the window

    vtkRenderWindow* rwin = vtkRenderWindow::SafeDownCast(ren->GetVTKWindow());

    const int colorTexGL = this->SceneGraph->GetColorBufferTextureGL();
    const int depthTexGL = this->SceneGraph->GetDepthBufferTextureGL();

    vtkOpenGLRenderWindow* windowOpenGL = vtkOpenGLRenderWindow::SafeDownCast(rwin);

    if (!this->Internal->QuadHelper)
    {
      this->Internal->Init(windowOpenGL);
    }
    else
    {
      windowOpenGL->GetShaderCache()->ReadyShaderProgram(this->Internal->QuadHelper->Program);
    }

    if (!this->Internal->QuadHelper->Program || !this->Internal->QuadHelper->Program->GetCompiled())
    {
      vtkErrorMacro("Couldn't build the shader program.");
      return;
    }

    windowOpenGL->MakeCurrent();

    vtkTextureObject* usedColorTex = nullptr;
    vtkTextureObject* usedDepthTex = nullptr;

    if (colorTexGL != 0 && depthTexGL != 0 && windowOpenGL != nullptr)
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
#ifdef VTKOSPRAY_ENABLE_DENOISER
      this->Internal->ColorTexture->Create2DFromRaw(
        viewportWidth, viewportHeight, 4, VTK_FLOAT, this->SceneGraph->GetBuffer());
#else
      this->Internal->ColorTexture->Create2DFromRaw(
        viewportWidth, viewportHeight, 4, VTK_UNSIGNED_CHAR, this->SceneGraph->GetBuffer());
#endif
      this->Internal->DepthTexture->CreateDepthFromRaw(viewportWidth, viewportHeight,
        vtkTextureObject::Float32, VTK_FLOAT, this->SceneGraph->GetZBuffer());

      usedColorTex = this->Internal->ColorTexture;
      usedDepthTex = this->Internal->DepthTexture;
    }

    usedColorTex->Activate();
    usedDepthTex->Activate();

    this->Internal->QuadHelper->Program->SetUniformi(
      "colorTexture", usedColorTex->GetTextureUnit());
    this->Internal->QuadHelper->Program->SetUniformi(
      "depthTexture", usedDepthTex->GetTextureUnit());

    vtkOpenGLState* ostate = windowOpenGL->GetState();

    vtkOpenGLState::ScopedglEnableDisable dsaver(ostate, GL_DEPTH_TEST);
    vtkOpenGLState::ScopedglEnableDisable bsaver(ostate, GL_BLEND);
    vtkOpenGLState::ScopedglDepthFunc dfsaver(ostate);
    vtkOpenGLState::ScopedglBlendFuncSeparate bfsaver(ostate);

    ostate->vtkglViewport(viewportX, viewportY, viewportWidth, viewportHeight);
    ostate->vtkglScissor(viewportX, viewportY, viewportWidth, viewportHeight);

    ostate->vtkglEnable(GL_DEPTH_TEST);

    if (ren->GetLayer() == 0)
    {
      ostate->vtkglDisable(GL_BLEND);
      ostate->vtkglDepthFunc(GL_ALWAYS);
    }
    else
    {
      ostate->vtkglEnable(GL_BLEND);
      ostate->vtkglDepthFunc(GL_LESS);
      if (vtkOSPRayRendererNode::GetCompositeOnGL(ren))
      {
        ostate->vtkglBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
      }
      else
      {
        ostate->vtkglBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
      }
    }

    this->Internal->QuadHelper->Render();

    usedDepthTex->Deactivate();
    usedColorTex->Deactivate();
  }
}

// ----------------------------------------------------------------------------
bool vtkOSPRayPass::IsSupported()
{
  static bool detected = false;
  static bool is_supported = true;

  // Short-circuit to avoid querying on every call.
  if (detected)
  {
    return is_supported;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Note that this class is used for OSPRay and OptiX (in addition to any
  // other RayTracing backends). Currently the only "spoiling" detection is
  // Apple's Rosetta. Since the only other backend is OptiX today and is not
  // supported on macOS within VTK anyways, there is no conflict.
  //////////////////////////////////////////////////////////////////////////////

#ifdef __APPLE__
  // Detect if we are being translated by Rosetta. OSPRay uses AVX instructions
  // which are not supported.
  {
    int is_translated = 0;
    size_t size = sizeof(is_translated);
    if (sysctlbyname("sysctl.proc_translated", &is_translated, &size, nullptr, 0) == -1)
    {
      if (errno == ENOENT)
      {
        is_translated = 0;
      }
      else
      {
        // Error occurred. Just continue and let it work if it can or crash if
        // it doesn't.
      }
    }
    if (is_translated)
    {
      is_supported = false;
    }
  }
#endif

  //////////////////////////////////////////////////////////////////////////////
  // See the comment at the beginning of any conditions.
  //////////////////////////////////////////////////////////////////////////////

  detected = true;
  return is_supported;
}

// ----------------------------------------------------------------------------
bool vtkOSPRayPass::IsBackendAvailable(const char* choice)
{
  if (!vtkOSPRayPass::IsSupported())
  {
    return false;
  }
  std::set<RTWBackendType> bends = rtwGetAvailableBackends();
  if (!strcmp(choice, "OSPRay raycaster"))
  {
    return (bends.find(RTW_BACKEND_OSPRAY) != bends.end());
  }
  if (!strcmp(choice, "OSPRay pathtracer"))
  {
    return (bends.find(RTW_BACKEND_OSPRAY) != bends.end());
  }
  if (!strcmp(choice, "OptiX pathtracer"))
  {
    return (bends.find(RTW_BACKEND_VISRTX) != bends.end());
  }
  return false;
}
