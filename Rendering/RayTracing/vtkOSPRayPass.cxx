// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtk_glad.h>

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

#include <sstream>

#ifdef _MSC_VER
#include <intrin.h>
#endif

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <sys/types.h>
#endif

VTK_ABI_NAMESPACE_BEGIN
class vtkOSPRayPassInternals : public vtkRenderPass
{
public:
  static vtkOSPRayPassInternals* New();
  vtkTypeMacro(vtkOSPRayPassInternals, vtkRenderPass);

  vtkOSPRayPassInternals() = default;

  ~vtkOSPRayPassInternals() override { delete this->QuadHelper; }

  void Init(vtkOpenGLRenderWindow* context, const std::string& renType, vtkRenderer* ren)
  {
    std::string FSSource = vtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Decl",
      "uniform sampler2D colorTexture;\n"
      "uniform sampler2D depthTexture;\n");

    std::stringstream ss;
    ss << "vec4 color = texture(colorTexture, texCoord);\n"
       << "gl_FragDepth = texture(depthTexture, texCoord).r;\n";

    if (renType == "pathtracer")
    {
      // If the background image is an hdri (= mode in environment mode)
      // we need to have an opaque background but ospray set it transparent
      // Set it to opaque to let the tone mapping be applied on the background
      auto bgMode = vtkOSPRayRendererNode::GetBackgroundMode(ren);
      bool useHdri = ren->GetUseImageBasedLighting() && ren->GetEnvironmentTexture() &&
        bgMode == vtkOSPRayRendererNode::Environment;
      ss << "gl_FragData[0] = vec4(color.rgb, " << (useHdri ? "1.0)" : "color.a)") << ";\n";
      this->BgMode = bgMode;
    }
    else
    {
      ss << "gl_FragData[0] = color;\n";
    }
    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl", ss.str());

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

    this->RendererType = renType;
  }

  void Render(const vtkRenderState* s) override { this->Parent->RenderInternal(s); }

  vtkNew<vtkOSPRayViewNodeFactory> Factory;
  vtkOSPRayPass* Parent = nullptr;
  std::string RendererType;
  vtkOSPRayRendererNode::BackgroundMode BgMode;

  // OpenGL-based display
  vtkOpenGLQuadHelper* QuadHelper = nullptr;
  vtkNew<vtkTextureObject> ColorTexture;
  vtkNew<vtkTextureObject> DepthTexture;
  vtkNew<vtkTextureObject> SharedColorTexture;
  vtkNew<vtkTextureObject> SharedDepthTexture;
};

int vtkOSPRayPass::RTDeviceRefCount = 0;

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkOSPRayPassInternals);

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkOSPRayPass);

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkOSPRayPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkOSPRayPass, SceneGraph, vtkOSPRayRendererNode);

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

    std::string renType = vtkOSPRayRendererNode::GetRendererType(ren);
    int bgMode = vtkOSPRayRendererNode::GetBackgroundMode(ren);
    if (this->Internal->QuadHelper &&
      (this->Internal->RendererType != renType || this->Internal->BgMode != bgMode))
    {
      delete this->Internal->QuadHelper;
      this->Internal->QuadHelper = nullptr;
    }

    if (!this->Internal->QuadHelper)
    {
      this->Internal->Init(windowOpenGL, renType, ren);
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
      // for visRTX, reuse existing OpenGL texture provided
      this->Internal->SharedColorTexture->AssignToExistingTexture(colorTexGL, GL_TEXTURE_2D);
      this->Internal->SharedDepthTexture->AssignToExistingTexture(depthTexGL, GL_TEXTURE_2D);

      usedColorTex = this->Internal->SharedColorTexture;
      usedDepthTex = this->Internal->SharedDepthTexture;
    }
    else
    {
      // upload to the texture
      this->Internal->ColorTexture->Create2DFromRaw(
        viewportWidth, viewportHeight, 4, VTK_FLOAT, this->SceneGraph->GetBuffer());
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

//------------------------------------------------------------------------------
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
  // other RayTracing backends). Currently the only "spoiling" detections are
  // Apple's Rosetta not supporting AVX and older processors that don't support
  // SSE4.1. Since the only other backend is OptiX today and is not supported
  // on macOS within VTK anyways, there is no conflict. Older processors
  // without SSE4.1 may be rejected here even if OptiX is supported, but such
  // old hardware with new video cards is considered a reasonable loss to avoid
  // crashing outright otherwise.
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

#ifdef __x86_64__
#if defined(__has_builtin)
#if __has_builtin(__builtin_cpu_init)
#define vtkOSPRayPass_has_builtin_cpu_init 1
#endif
#elif defined(__clang__) // Only supported in Clang 6 and up.
#if __clang_major__ >= 6
#define vtkOSPRayPass_has_builtin_cpu_init 1
#endif
#elif defined(__GNUC__) // GCC has always provided this mechanism
#define vtkOSPRayPass_has_builtin_cpu_init 1
#endif
#ifndef vtkOSPRayPass_has_builtin_cpu_init
#define vtkOSPRayPass_has_builtin_cpu_init 0
#endif

  // ISPC detects AVX2, AVX, and SSE4.1 instruction sets. If none are
  // supported, an `abort()` awaits pretty much any ISPC call. Detect SSE4.1
  // and, if missing, disable OSPRay support.
  //
  // CPU features are detected here:
  // https://github.com/ispc/ispc/blob/bf959a96af1a362b1fe16895aa2ae997355ea05b/builtins/dispatch.ll#L132-L133
#if vtkOSPRayPass_has_builtin_cpu_init
  // Most compilers have a good CPU feature abstraction, so use it if
  // available.
  {
    __builtin_cpu_init();
    if (!__builtin_cpu_supports("sse4.1"))
    {
      is_supported = false;
    }
  }
#elif defined(_MSC_VER)
  // Query the CPU for instruction support using MSVC intrinsics.
  // https://learn.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex
  {
    // Storage for `cpuid` results.
    std::array<int, 4> cpui;

    // First query how many function IDs are supported.
    __cpuid(cpui.data(), 0);
    int const nids = cpui[0];

    constexpr int FeatureBitFunctionId = 1;
    constexpr size_t Ecx = 2;
    constexpr int SSE4_1_bit = 19;

    // SSE4.1 support lives in the first function ID vector.
    // https://en.wikipedia.org/wiki/CPUID#EAX=1:_Processor_Info_and_Feature_Bits
    if (nids >= FeatureBitFunctionId)
    {
      __cpuid(cpui.data(), FeatureBitFunctionId);

      // The `ecx` return is in index 2; bit 19 holds SSE4.1 information.
      int const sse42_container = cpui[Ecx];
      if (!(sse42_container & (1 << SSE4_1_bit)))
      {
        is_supported = false;
      }
    }
    else
    {
      // No feature bit vector present? Something is up; assume the worst and
      // disable support.
      is_supported = false;
    }
  }
#endif
#endif

  //////////////////////////////////////////////////////////////////////////////
  // See the comment at the beginning of any conditions.
  //////////////////////////////////////////////////////////////////////////////

  detected = true;
  return is_supported;
}

//------------------------------------------------------------------------------
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
VTK_ABI_NAMESPACE_END
