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
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
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

#include <memory>
#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

// ----------------------------------------------------------------------------
static void AnariStatusCallback(const void* userData, anari::Device device, anari::Object source,
  anari::DataType sourceType, anari::StatusSeverity severity, anari::StatusCode code,
  const char* message)
{
  if (severity == ANARI_SEVERITY_FATAL_ERROR)
  {
    vtkLogF(ERROR, "[ANARI::FATAL] %s\n", message);
  }
  else if (severity == ANARI_SEVERITY_ERROR)
  {
    vtkLogF(ERROR, "[ANARI::ERROR] %s, DataType: %d\n", message, (int)sourceType);
  }
  else if (severity == ANARI_SEVERITY_WARNING)
  {
    vtkLogF(WARNING, "[ANARI::WARN] %s, DataType: %d\n", message, (int)sourceType);
  }
  else if (severity == ANARI_SEVERITY_PERFORMANCE_WARNING)
  {
    vtkLogF(WARNING, "[ANARI::PERF] %s\n", message);
  }
  else if (severity == ANARI_SEVERITY_INFO)
  {
    vtkLogF(INFO, "[ANARI::INFO] %s\n", message);
  }
  else if (severity == ANARI_SEVERITY_DEBUG)
  {
    vtkLogF(TRACE, "[ANARI::DEBUG] %s\n", message);
  }
  else
  {
    vtkLogF(INFO, "[ANARI::STATUS] %s\n", message);
  }

  (void)userData;
  (void)device;
  (void)source;
  (void)code;
}

// ----------------------------------------------------------------------------
class vtkAnariPassInternals : public vtkRenderPass
{
public:
  static vtkAnariPassInternals* New();
  vtkTypeMacro(vtkAnariPassInternals, vtkRenderPass);

  vtkAnariPassInternals() = default;
  ~vtkAnariPassInternals() = default;

  bool IsInitialized() const;
  bool InitAnari(bool useDebugDevice = false, const char* libraryName = "environment",
    const char* deviceName = "default");
  void SetupFrame(vtkOpenGLRenderWindow* openGLRenderWindow, const anari::Extensions& extensions,
    vtkRenderer* ren);
  void Render(const vtkRenderState* s) override;
  void CleanupAnariObjects();

  vtkAnariPass* Parent{ nullptr };
  std::unique_ptr<vtkOpenGLQuadHelper> OpenGLQuadHelper;

  vtkNew<vtkAnariViewNodeFactory> Factory;
  vtkNew<vtkTextureObject> ColorTexture;
  vtkNew<vtkTextureObject> DepthTexture;
  vtkNew<vtkTextureObject> SharedColorTexture;
  vtkNew<vtkTextureObject> SharedDepthTexture;

  std::string AnariLibraryName;
  std::string AnariDeviceName;
  std::string AnariDebugTraceDir;
  std::string AnariDebugTraceMode;
  bool AnariDebugDeviceEnabled{ false };
  anari::Library AnariLibrary{ nullptr };
  anari::Device AnariDevice{ nullptr };
  anari::Extensions AnariExtensions{};
};

// ----------------------------------------------------------------------------
bool vtkAnariPassInternals::IsInitialized() const
{
  return this->AnariDevice != nullptr;
}

// ----------------------------------------------------------------------------
bool vtkAnariPassInternals::InitAnari(
  bool useDebugDevice, const char* libraryName, const char* deviceName)
{
  vtkAnariProfiling startProfiling("vtkAnariPassInternals::InitAnari", vtkAnariProfiling::YELLOW);

  const bool configIsTheSame = IsInitialized() && libraryName == this->AnariLibraryName &&
    deviceName == this->AnariDeviceName && useDebugDevice == this->AnariDebugDeviceEnabled;
  if (configIsTheSame)
  {
    return true;
  }

  this->CleanupAnariObjects();

  vtkDebugMacro(<< "VTK Anari Library name: "
                << ((libraryName != nullptr) ? libraryName : "nullptr"));
  vtkDebugMacro(<< "VTK Anari Device type: " << deviceName);

  this->AnariLibrary = anari::loadLibrary(libraryName, AnariStatusCallback);

  if (!this->AnariLibrary)
  {
    this->CleanupAnariObjects();
    vtkErrorMacro(<< "[ANARI::" << libraryName << "] Could not load " << libraryName
                  << " library.\n");
    return false;
  }

  this->AnariDevice = anari::newDevice(this->AnariLibrary, deviceName);
  if (!this->AnariDevice)
  {
    this->CleanupAnariObjects();
    vtkErrorMacro(<< "[ANARI::" << libraryName << "] Could not load " << deviceName
                  << " device.\n");
    return false;
  }

  anari::Library debugLibrary{};
  anari::Device debugDevice{};

  if (useDebugDevice)
  {
    debugLibrary = anari::loadLibrary("debug", AnariStatusCallback);
    if (!debugLibrary)
    {
      this->CleanupAnariObjects();
      vtkErrorMacro(<< "[ANARI::" << libraryName << "] Could not load debug library.");
      return false;
    }

    debugDevice = anari::newDevice(debugLibrary, "default");
    if (!debugDevice)
    {
      this->CleanupAnariObjects();
      vtkErrorMacro(<< "[ANARI::" << libraryName << "] Could not load debug device.");
      return false;
    }

    if (!this->AnariDebugTraceDir.empty())
    {
      anari::setParameter(debugDevice, debugDevice, "traceDir", this->AnariDebugTraceDir);
    }

    if (!this->AnariDebugTraceMode.empty())
    {
      anari::setParameter(debugDevice, debugDevice, "traceMode", this->AnariDebugTraceMode);
    }

    anari::setParameter(debugDevice, debugDevice, "wrappedDevice", this->AnariDevice);
    anari::commitParameters(debugDevice, debugDevice);
    this->AnariDevice = debugDevice;
  }

  auto list = (const char* const*)anariGetDeviceExtensions(this->AnariLibrary, deviceName);
  for (const auto* i = list; list != nullptr && *i != nullptr; ++i)
  {
    vtkDebugMacro(<< "[" << libraryName << ":" << deviceName << "] Feature => " << *i);
  }

  anariGetDeviceExtensionStruct(&this->AnariExtensions, this->AnariLibrary, deviceName);

  if ((this->AnariExtensions.ANARI_KHR_GEOMETRY_CYLINDER ||
        this->AnariExtensions.ANARI_KHR_GEOMETRY_CURVE) &&
    this->AnariExtensions.ANARI_KHR_GEOMETRY_SPHERE &&
    this->AnariExtensions.ANARI_KHR_GEOMETRY_TRIANGLE &&
    this->AnariExtensions.ANARI_KHR_INSTANCE_TRANSFORM)
  {
    vtkDebugMacro(<< "[ANARI::" << libraryName << "] Loaded " << deviceName << " device.\n");
  }
  else
  {
    vtkDebugMacro(<< "[ANARI::" << libraryName << "] Loaded " << deviceName
                  << " device doesn't have the minimum required features.\n");
  }

  this->AnariLibraryName = libraryName;
  this->AnariDeviceName = deviceName;
  this->AnariDebugDeviceEnabled = useDebugDevice;

  return true;
}

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

  vtkAnariRendererNode* anariRendererNode =
    vtkAnariRendererNode::SafeDownCast(sceneGraph->GetViewNodeFor(ren));
  anariRendererNode->SetSize(viewportWidth, viewportHeight);
  anariRendererNode->SetViewport(tileViewport);
  anariRendererNode->SetScale(tileScale);

  sceneGraph->TraverseAllPasses();

  // Copy result to the window
  const int colorTexGL = sceneGraph->GetColorBufferTextureGL();
  const int depthTexGL = sceneGraph->GetDepthBufferTextureGL();

  vtkRenderWindow* rwin = vtkRenderWindow::SafeDownCast(ren->GetVTKWindow());
  vtkOpenGLRenderWindow* windowOpenGL = vtkOpenGLRenderWindow::SafeDownCast(rwin);

  this->SetupFrame(windowOpenGL, anariRendererNode->GetAnariDeviceExtensions(), ren);
  if (!this->OpenGLQuadHelper->Program || !this->OpenGLQuadHelper->Program->GetCompiled())
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
    this->SharedColorTexture->AssignToExistingTexture(colorTexGL, GL_TEXTURE_2D);
    this->SharedDepthTexture->AssignToExistingTexture(depthTexGL, GL_TEXTURE_2D);

    usedColorTex = this->SharedColorTexture;
    usedDepthTex = this->SharedDepthTexture;
  }
  else
  {
    // upload to the texture
    this->ColorTexture->Create2DFromRaw(viewportWidth, viewportHeight, 4, VTK_UNSIGNED_CHAR,
      const_cast<unsigned char*>(sceneGraph->GetBuffer()));
    this->DepthTexture->CreateDepthFromRaw(viewportWidth, viewportHeight, vtkTextureObject::Float32,
      VTK_FLOAT, const_cast<float*>(sceneGraph->GetZBuffer()));

    usedColorTex = this->ColorTexture;
    usedDepthTex = this->DepthTexture;
  }

  usedColorTex->Activate();
  usedDepthTex->Activate();

  this->OpenGLQuadHelper->Program->SetUniformi("colorTexture", usedColorTex->GetTextureUnit());
  this->OpenGLQuadHelper->Program->SetUniformi("depthTexture", usedDepthTex->GetTextureUnit());

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

  this->OpenGLQuadHelper->Render();

  usedColorTex->Deactivate();
  usedDepthTex->Deactivate();
}

// ----------------------------------------------------------------------------
void vtkAnariPassInternals::CleanupAnariObjects()
{
  if (this->AnariLibrary)
  {
    anari::unloadLibrary(this->AnariLibrary);
  }

  if (this->AnariDevice)
  {
    anari::release(this->AnariDevice, this->AnariDevice);
  }

  this->AnariLibraryName = "";
  this->AnariDeviceName = "";
  this->AnariDebugDeviceEnabled = false;
  this->AnariLibrary = nullptr;
  this->AnariDevice = nullptr;
  this->AnariExtensions = {};
}

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAnariPassInternals);

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAnariPass);

// ----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkAnariPass, SceneGraph, vtkAnariRendererNode);

// ----------------------------------------------------------------------------
void vtkAnariPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
void vtkAnariPass::SetAnariDebugConfig(const char* traceDir, const char* traceMode)
{
  this->Internal->AnariDebugTraceDir = traceDir;
  this->Internal->AnariDebugTraceMode = traceMode;
}

void vtkAnariPass::SetupAnariDeviceFromLibrary(
  const char* libraryName, const char* deviceName, bool enableDebugDevice)
{
  this->Internal->InitAnari(enableDebugDevice, libraryName, deviceName);
}

// ----------------------------------------------------------------------------
void vtkAnariPass::Render(const vtkRenderState* s)
{
  vtkAnariProfiling startProfiling("vtkAnariPass::Render", vtkAnariProfiling::YELLOW);

  if (!this->Internal->IsInitialized())
  {
    this->Internal->InitAnari();
  }

  vtkRenderer* ren = s->GetRenderer();
  if (ren)
  {
    const bool rebuildSceneGraph =
      !this->SceneGraph || this->SceneGraph->GetAnariDevice() != this->Internal->AnariDevice;
    if (rebuildSceneGraph)
    {
      this->SetSceneGraph(
        vtkAnariRendererNode::SafeDownCast(this->Internal->Factory->CreateNode(ren)));
      this->SceneGraph->SetAnariDevice(
        this->Internal->AnariDevice, this->Internal->AnariExtensions);
    }
  }

  this->CameraPass->Render(s);
}

// ----------------------------------------------------------------------------
vtkAnariPass::vtkAnariPass()
{
  this->Internal = vtkAnariPassInternals::New();
  this->Internal->Parent = this;

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

VTK_ABI_NAMESPACE_END
