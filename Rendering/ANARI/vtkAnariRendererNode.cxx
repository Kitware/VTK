// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif

#include <anari/anari_cpp/ext/std.h>
#define ANARI_EXTENSION_UTILITY_IMPL
#include <anari/frontend/anari_extension_utility.h>

#include "vtkAnariRendererNode.h"

#include "vtkAbstractVolumeMapper.h"
#include "vtkAnariActorNode.h"
#include "vtkAnariCameraNode.h"
#include "vtkAnariLightNode.h"
#include "vtkAnariProfiling.h"
#include "vtkAnariVolumeNode.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationKey.h"
#include "vtkInformationStringKey.h"
#include "vtkLight.h"
#include "vtkLogger.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTexture.h"

#include <cmath>

VTK_ABI_NAMESPACE_BEGIN

using namespace anari::std_types;

vtkInformationKeyMacro(vtkAnariRendererNode, COMPOSITE_ON_GL, Integer);
vtkInformationKeyMacro(vtkAnariRendererNode, RENDERER_SUBTYPE, String);
vtkInformationKeyMacro(vtkAnariRendererNode, ACCUMULATION_COUNT, Integer);

struct RendererChangeCallback : vtkCommand
{
  vtkTypeMacro(RendererChangeCallback, vtkCommand);

  static RendererChangeCallback* New() { return new RendererChangeCallback; }

  void Execute(
    vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(eventId), void* vtkNotUsed(callData))
  {
    vtkAnariRendererNode::InvalidateRendererParameters();
  }
};

struct vtkAnariRendererNodeInternals
{
  vtkAnariRendererNodeInternals(vtkAnariRendererNode*);
  ~vtkAnariRendererNodeInternals();

  /**
   * @brief Populate the current ANARI back-end device features.
   * @param library the ANARI library
   * @param deviceName  the ANARI back-end device name
   * @param deviceSubtype the ANARI back-end device subtype name
   * @return true if this device implements a minimum set of features required to
   *         render VTK datasets, false otherwise.
   */
  bool SetAnariDeviceFeatures(
    anari::Library library, const char* deviceName, const char* deviceSubtype);

  /**
   * Set the USD back-end related ANARI parameters
   */
  void SetUSDDeviceParameters();

  /**
   * Set a parameter on the underlying anari::Renderer
   */
  template <typename T>
  void SetRendererParameter(const char* p, const T& v);

  vtkAnariRendererNode* Owner{ nullptr };

  std::vector<u_char> ColorBuffer;
  std::vector<float> DepthBuffer;

  int ImageX;
  int ImageY;

  bool CompositeOnGL{ false };

  std::string RendererSubtype;

  anari::Device AnariDevice{ nullptr };
  anari::Renderer AnariRenderer{ nullptr };
  anari::World AnariWorld{ nullptr };
  anari::Frame AnariFrame{ nullptr };

  anari::Extensions AnariExtensions{};

  std::vector<anari::Surface> AnariSurfaces;
  std::vector<anari::Volume> AnariVolumes;
  std::vector<anari::Light> AnariLights;
};

vtkAnariRendererNodeInternals::vtkAnariRendererNodeInternals(vtkAnariRendererNode* owner)
  : Owner(owner)
{
}

vtkAnariRendererNodeInternals::~vtkAnariRendererNodeInternals()
{
  if (this->AnariDevice != nullptr)
  {
    anari::release(this->AnariDevice, this->AnariWorld);
    anari::release(this->AnariDevice, this->AnariRenderer);
    anari::release(this->AnariDevice, this->AnariFrame);
    anari::release(this->AnariDevice, this->AnariDevice);
  }
}

//----------------------------------------------------------------------------
void vtkAnariRendererNodeInternals::SetUSDDeviceParameters()
{
#if 0
  // Initialize USD Device Parameters
  auto renderer = this->Owner->GetRenderer();

  const char* location = vtkAnariRendererNode::GetUsdDirectory(renderer);
  bool outputBinary = vtkAnariRendererNode::GetUsdOutputBinary(renderer) == 1;
  bool outputMaterial = vtkAnariRendererNode::GetUsdOutputMaterial(renderer) == 1;
  bool outputPreviewSurface = vtkAnariRendererNode::GetUsdOutputPreviewSurface(renderer) == 1;
  bool outputMdl = vtkAnariRendererNode::GetUsdOutputMDL(renderer) == 1;
  bool outputDisplayColors = vtkAnariRendererNode::GetUsdOutputDisplayColors(renderer) == 1;
  bool outputMdlColors = vtkAnariRendererNode::GetUsdOutputMDLColors(renderer) == 1;
  bool writeAtCommit = vtkAnariRendererNode::GetUsdAtCommit(renderer) == 1;

  // Set USD Device Parameters
  if (location != nullptr)
  {
    anari::setParameter(this->AnariDevice, this->AnariDevice, "usd::serialize.location", location);
  }

  anari::setParameter(
    this->AnariDevice, this->AnariDevice, "usd::serialize.outputbinary", outputBinary);
  anari::setParameter(this->AnariDevice, this->AnariDevice, "usd::output.material", outputMaterial);
  anari::setParameter(
    this->AnariDevice, this->AnariDevice, "usd::output.previewsurfaceshader", outputPreviewSurface);
  anari::setParameter(this->AnariDevice, this->AnariDevice, "usd::output.mdlshader", outputMdl);
  anari::setParameter(
    this->AnariDevice, this->AnariDevice, "usd::output.displaycolors", outputDisplayColors);
  anari::setParameter(
    this->AnariDevice, this->AnariDevice, "usd::output.mdlcolors", outputMdlColors);
  anari::setParameter(this->AnariDevice, this->AnariDevice, "usd::writeatcommit", writeAtCommit);
#endif
}

//------------------------------------------------------------------------------
template <typename T>
void vtkAnariRendererNodeInternals::SetRendererParameter(const char* p, const T& v)
{
  anari::setParameter(this->AnariDevice, this->AnariRenderer, p, v);
  anari::commitParameters(this->AnariDevice, this->AnariRenderer);
}

//============================================================================

vtkStandardNewMacro(vtkAnariRendererNode);

vtkTimeStamp vtkAnariRendererNode::AnariRendererModifiedTime;

//----------------------------------------------------------------------------
vtkAnariRendererNode::vtkAnariRendererNode()
{
  this->Internal = new vtkAnariRendererNodeInternals(this);
  InvalidateSceneStructure();
}

//----------------------------------------------------------------------------
vtkAnariRendererNode::~vtkAnariRendererNode()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::InitAnariFrame(vtkRenderer* ren)
{
  if (this->Internal->AnariFrame != nullptr)
  {
    return;
  }

  auto anariDevice = this->GetAnariDevice();
  this->Internal->AnariFrame = anari::newObject<anari::Frame>(anariDevice);
  anari::setParameter(anariDevice, this->Internal->AnariFrame, "channel.color", ANARI_UFIXED8_VEC4);
  anari::setParameter(anariDevice, this->Internal->AnariFrame, "channel.depth", ANARI_FLOAT32);
  anari::commitParameters(anariDevice, this->Internal->AnariFrame);

  if (!ren->HasObserver(vtkCommand::ModifiedEvent))
  {
    vtkNew<RendererChangeCallback> cc;
    ren->AddObserver(vtkCommand::ModifiedEvent, cc);
    cc->Execute(nullptr, vtkCommand::ModifiedEvent, nullptr);
  }
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::InitAnariRenderer(vtkRenderer* ren)
{
  auto anariDevice = this->GetAnariDevice();
  auto anariFrame = this->Internal->AnariFrame;

  auto currentRendererSubtype = this->Internal->RendererSubtype;
  this->Internal->CompositeOnGL = (this->GetCompositeOnGL(ren) != 0);
  const char* rendererSubtype = vtkAnariRendererNode::GetRendererSubtype(this->GetRenderer());

  if (currentRendererSubtype != rendererSubtype)
  {
    this->Internal->RendererSubtype = rendererSubtype;

    anari::release(anariDevice, this->Internal->AnariRenderer);

    this->Internal->AnariRenderer = anari::newObject<anari::Renderer>(anariDevice, rendererSubtype);

    anari::setParameter(anariDevice, anariFrame, "renderer", this->Internal->AnariRenderer);
    anari::commitParameters(anariDevice, anariFrame);

    this->AnariRendererModifiedTime.Modified();
  }
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetupAnariRendererParameters(vtkRenderer* ren)
{
  if (this->AnariRendererModifiedTime <= this->AnariRendererUpdatedTime)
  {
    return;
  }

  auto anariDevice = this->GetAnariDevice();
  auto anariRenderer = this->Internal->AnariRenderer;

#if 0
  anari::setParameter(anariDevice, anariRenderer, "denoise", bool(this->GetUseDenoiser(ren)));
  anari::setParameter<int>(
    anariDevice, anariRenderer, "pixelSamples", this->GetSamplesPerPixel(ren));
  anari::setParameter<int>(
    anariDevice, anariRenderer, "ambientSamples", this->GetAmbientSamples(ren));
  anari::setParameter<float>(
    anariDevice, anariRenderer, "lightFalloff", this->GetLightFalloff(ren));
  anari::setParameter<float>(
    anariDevice, anariRenderer, "ambientRadiance", this->GetAmbientIntensity(ren));
  anari::setParameter<int>(anariDevice, anariRenderer, "maxDepth", this->GetMaxDepth(ren));
  double* ambientColor = this->GetAmbientColor(ren);
  if (ambientColor)
  {
    float ambientColorf[3] = { static_cast<float>(ambientColor[0]),
      static_cast<float>(ambientColor[1]), static_cast<float>(ambientColor[2]) };
    anari::setParameter(anariDevice, anariRenderer, "ambientColor", ambientColorf);
  }

  // Debug method
  anari::setParameter(anariDevice, anariRenderer, "method", this->GetDebugMethod(ren));
#endif

  double* bg = ren->GetBackground();
  double bgAlpha = ren->GetBackgroundAlpha();

  if (!ren->GetGradientBackground())
  {
    float bgColor[4] = { static_cast<float>(bg[0]), static_cast<float>(bg[1]),
      static_cast<float>(bg[2]), static_cast<float>(bgAlpha) };

    anari::setParameter(anariDevice, anariRenderer, "background", bgColor);
  }
  else
  {
    double* topbg = ren->GetBackground2();
    constexpr int IMAGE_SIZE = 128;

    vtkNew<vtkColorTransferFunction> colorTF;
    colorTF->AddRGBPoint(0, bg[0], bg[1], bg[2]);
    colorTF->AddRGBPoint(IMAGE_SIZE, topbg[0], topbg[1], topbg[2]);

    auto gradientArray = anari::newArray2D(anariDevice, ANARI_FLOAT32_VEC4, 1, IMAGE_SIZE + 1);
    auto* gradientColors = anari::map<vec4>(anariDevice, gradientArray);

    for (int i = 0; i <= IMAGE_SIZE; i++)
    {
      double* color = colorTF->GetColor(i);

      gradientColors[i] = vec4{ static_cast<float>(color[0]), static_cast<float>(color[1]),
        static_cast<float>(color[2]), static_cast<float>(bgAlpha) };
    }

    anari::unmap(anariDevice, gradientArray);
    anari::setAndReleaseParameter(anariDevice, anariRenderer, "background", gradientArray);
    anari::commitParameters(anariDevice, anariRenderer);
  }

  anari::commitParameters(anariDevice, anariRenderer);

  this->AnariRendererUpdatedTime = this->AnariRendererModifiedTime;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::InitAnariWorld()
{
  if (this->Internal->AnariWorld != nullptr)
  {
    return;
  }

  auto anariDevice = this->GetAnariDevice();

  auto anariWorld = anari::newObject<anari::World>(anariDevice);
  this->Internal->AnariWorld = anariWorld;
  anari::setParameter(anariDevice, anariWorld, "name", "vtk_world");
  anari::commitParameters(anariDevice, anariWorld);

  auto anariFrame = this->Internal->AnariFrame;
  anari::setParameter(anariDevice, anariFrame, "world", this->Internal->AnariWorld);
  anari::commitParameters(anariDevice, anariFrame);
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::UpdateAnariFrameSize()
{
  const uvec2 frameSize = { static_cast<uint>(this->Size[0]), static_cast<uint>(this->Size[1]) };
  if ((uint)this->Internal->ImageX == frameSize[0] && (uint)this->Internal->ImageY == frameSize[1])
  {
    return;
  }

  this->Internal->ImageX = frameSize[0];
  this->Internal->ImageY = frameSize[1];

  const size_t totalSize = this->Size[0] * this->Size[1];
  this->Internal->ColorBuffer.resize(totalSize * sizeof(float));
  this->Internal->DepthBuffer.resize(totalSize);

  auto anariDevice = this->GetAnariDevice();
  auto anariFrame = this->Internal->AnariFrame;
  anari::setParameter(anariDevice, anariFrame, "size", frameSize);
  anari::commitParameters(anariDevice, anariFrame);
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::UpdateAnariLights()
{
  auto anariDevice = this->GetAnariDevice();
  auto anariWorld = this->Internal->AnariWorld;
  const auto& lightState = this->Internal->AnariLights;

  if (!lightState.empty())
  {
    for (size_t i = 0; i < lightState.size(); i++)
    {
      std::string lightName = "vtk_light_" + std::to_string(i);
      anari::setParameter(anariDevice, lightState[i], "name", lightName.c_str());
      anari::commitParameters(anariDevice, lightState[i]);
    }

    anari::setParameterArray1D(
      anariDevice, anariWorld, "light", lightState.data(), lightState.size());
  }
  else
  {
    vtkWarningMacro(<< "No lights set on world.");
    anari::unsetParameter(anariDevice, anariWorld, "light");
  }

  anari::commitParameters(anariDevice, anariWorld);
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::UpdateAnariSurfaces()
{
  auto anariDevice = this->GetAnariDevice();
  auto anariWorld = this->Internal->AnariWorld;
  const auto& surfaceState = this->Internal->AnariSurfaces;

  if (!surfaceState.empty())
  {
    for (size_t i = 0; i < surfaceState.size(); i++)
    {
      std::string surfaceName = "vtk_surface_" + std::to_string(i);
      anari::setParameter(anariDevice, surfaceState[i], "name", surfaceName.c_str());
      anari::commitParameters(anariDevice, surfaceState[i]);
    }

    anari::setParameterArray1D(
      anariDevice, anariWorld, "surface", surfaceState.data(), surfaceState.size());
  }
  else
  {
    anari::unsetParameter(anariDevice, anariWorld, "surface");
  }

  anari::commitParameters(anariDevice, anariWorld);
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::UpdateAnariVolumes()
{
  auto anariDevice = this->GetAnariDevice();
  auto anariWorld = this->Internal->AnariWorld;
  const auto& volumeState = this->Internal->AnariVolumes;

  if (!volumeState.empty())
  {
    for (size_t i = 0; i < volumeState.size(); i++)
    {
      std::string volumeName = "vtk_volume_" + std::to_string(i);
      anari::setParameter(anariDevice, volumeState[i], "name", volumeName.c_str());
      anari::commitParameters(anariDevice, volumeState[i]);
    }

    anari::setParameterArray1D(
      anariDevice, anariWorld, "volume", volumeState.data(), volumeState.size());
  }
  else
  {
    anari::unsetParameter(anariDevice, anariWorld, "volume");
  }

  anari::commitParameters(anariDevice, anariWorld);
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::DebugOutputWorldBounds()
{
  auto anariDevice = this->GetAnariDevice();
  auto anariWorld = this->Internal->AnariWorld;

  // Get world bounds
  float worldBounds[6];
  if (anariGetProperty(anariDevice, anariWorld, "bounds", ANARI_FLOAT32_BOX3, worldBounds,
        sizeof(worldBounds), ANARI_WAIT))
  {
    vtkDebugMacro(<< "[ANARI] World Bounds: "
                  << "{" << worldBounds[0] << ", " << worldBounds[1] << ", " << worldBounds[2]
                  << "}, "
                  << "{" << worldBounds[3] << ", " << worldBounds[4] << ", " << worldBounds[5]
                  << "}");
  }
  else
  {
    vtkWarningMacro(<< "[ANARI] World bounds not returned");
  }
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::CopyAnariFrameBufferData()
{
  int totalSize = this->Size[0] * this->Size[1];

  auto anariDevice = this->GetAnariDevice();
  auto anariFrame = this->Internal->AnariFrame;

  float duration = 0.0f;
  anari::getProperty(anariDevice, anariFrame, "duration", duration, ANARI_NO_WAIT);

  vtkDebugMacro(<< "Rendered frame in " << duration * 1000.0f << " ms");

  // Color buffer
  auto renderedFrame = anari::map<uint32_t>(anariDevice, anariFrame, "channel.color");

  if (renderedFrame.data != nullptr)
  {
    int retTotalSize = renderedFrame.width * renderedFrame.height;
    totalSize = std::min(retTotalSize, totalSize);
    memcpy(this->Internal->ColorBuffer.data(), renderedFrame.data, totalSize * 4);
  }
  else
  {
    vtkWarningMacro(<< "Color buffer is null");
    memset(this->Internal->ColorBuffer.data(), 255, totalSize * 4);
  }

  anari::unmap(anariDevice, anariFrame, "channel.color");

  // Depth Buffer
  auto mappedDepthBuffer = anari::map<float>(anariDevice, anariFrame, "channel.depth");

  if (mappedDepthBuffer.data != nullptr)
  {
    vtkCamera* cam = vtkRenderer::SafeDownCast(this->Renderable)->GetActiveCamera();
    double* clipValues = cam->GetClippingRange();
    double clipMin = clipValues[0];
    double clipMax = clipValues[1];
    double clipDiv = 1.0 / (clipMax - clipMin);

    const float* depthBuffer = mappedDepthBuffer.data;   // s
    float* zBuffer = this->Internal->DepthBuffer.data(); // d

    for (int i = 0; i < totalSize; i++)
    {
      // *d = (*s < clipMin ? 1.0 : (*s - clipMin) * clipDiv);
      zBuffer[i] = (depthBuffer[i] < clipMin ? 1.0f : (depthBuffer[i] - clipMin) * clipDiv);
    }
  }
  else
  {
    vtkWarningMacro(<< "Depth buffer is null");
    memset(this->Internal->DepthBuffer.data(), 0, totalSize * sizeof(float));
  }

  anari::unmap(anariDevice, anariFrame, "channel.depth");
}

//----------------------------------------------------------------------------
#define RENDERER_NODE_PARAM_SET_DEFINITION(FCN, PARAM, TYPE)                                       \
  void vtkAnariRendererNode::Set##FCN(vtkRenderer* r, TYPE v)                                      \
  {                                                                                                \
    if (!r)                                                                                        \
    {                                                                                              \
      return;                                                                                      \
    }                                                                                              \
                                                                                                   \
    vtkInformation* info = r->GetInformation();                                                    \
    info->Set(vtkAnariRendererNode::PARAM(), v);                                                   \
    vtkAnariRendererNode::AnariRendererModifiedTime.Modified();                                    \
  }

RENDERER_NODE_PARAM_SET_DEFINITION(RendererSubtype, RENDERER_SUBTYPE, const char*)
RENDERER_NODE_PARAM_SET_DEFINITION(AccumulationCount, ACCUMULATION_COUNT, int)
RENDERER_NODE_PARAM_SET_DEFINITION(CompositeOnGL, COMPOSITE_ON_GL, int)

//----------------------------------------------------------------------------
#define RENDERER_NODE_PARAM_GET_DEFINITION(FCN, PARAM, TYPE, DEFAULT_VALUE)                        \
  TYPE vtkAnariRendererNode::Get##FCN(vtkRenderer* r)                                              \
  {                                                                                                \
    if (!r)                                                                                        \
    {                                                                                              \
      return DEFAULT_VALUE;                                                                        \
    }                                                                                              \
                                                                                                   \
    vtkInformation* info = r->GetInformation();                                                    \
                                                                                                   \
    if (info && info->Has(vtkAnariRendererNode::PARAM()))                                          \
    {                                                                                              \
      return info->Get(vtkAnariRendererNode::PARAM());                                             \
    }                                                                                              \
                                                                                                   \
    return DEFAULT_VALUE;                                                                          \
  }

RENDERER_NODE_PARAM_GET_DEFINITION(RendererSubtype, RENDERER_SUBTYPE, const char*, "default")
RENDERER_NODE_PARAM_GET_DEFINITION(AccumulationCount, ACCUMULATION_COUNT, int, 1)
RENDERER_NODE_PARAM_GET_DEFINITION(CompositeOnGL, COMPOSITE_ON_GL, int, 0)

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetAnariParameter(const char* param, bool b)
{
  this->Internal->SetRendererParameter(param, b);
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetAnariParameter(const char* param, int x)
{
  this->Internal->SetRendererParameter(param, x);
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetAnariParameter(const char* param, int x, int y)
{
  this->Internal->SetRendererParameter(param, ivec2{ x, y });
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetAnariParameter(const char* param, int x, int y, int z)
{
  this->Internal->SetRendererParameter(param, ivec3{ x, y, z });
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetAnariParameter(const char* param, int x, int y, int z, int w)
{
  this->Internal->SetRendererParameter(param, ivec4{ x, y, z, w });
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetAnariParameter(const char* param, float x)
{
  this->Internal->SetRendererParameter(param, x);
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetAnariParameter(const char* param, float x, float y)
{
  this->Internal->SetRendererParameter(param, vec2{ x, y });
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetAnariParameter(const char* param, float x, float y, float z)
{
  this->Internal->SetRendererParameter(param, vec3{ x, y, z });
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetAnariParameter(const char* param, float x, float y, float z, float w)
{
  this->Internal->SetRendererParameter(param, vec4{ x, y, z, w });
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetCamera(anari::Camera camera)
{
  auto d = this->Internal->AnariDevice;
  auto f = this->Internal->AnariFrame;
  if (d && f)
  {
    anari::setParameter(d, f, "camera", camera);
    anari::commitParameters(d, f);
  }
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::AddLight(anari::Light light)
{
  if (light != nullptr)
  {
    this->Internal->AnariLights.emplace_back(light);
  }
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::AddSurface(anari::Surface surface)
{
  if (surface != nullptr)
  {
    this->Internal->AnariSurfaces.push_back(surface);
  }
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::AddVolume(anari::Volume volume)
{
  if (volume != nullptr)
  {
    this->Internal->AnariVolumes.emplace_back(volume);
  }
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
#if 0
  anari::Library anariLibrary = this->Internal->AnariLibrary;

  if (anariLibrary != nullptr)
  {
    const char* libName = vtkAnariRendererNode::GetLibraryName(this->GetRenderer());

    if (libName != nullptr)
    {
      // Available devices
      const char** devices = anariGetDeviceSubtypes(anariLibrary);
      os << indent << "[ANARI::" << libName << "] Available devices: \n";

      for (const char** d = devices; *d != NULL; d++)
      {
        os << indent << indent << *d << "\n";
      }

      // Available renderers
      const char** renderers = anariGetObjectSubtypes(this->Internal->AnariDevice, ANARI_RENDERER);
      os << "\n";
      os << indent << "[ANARI::" << libName << "] Available renderers: \n";

      for (const char** r = renderers; *r != NULL; r++)
      {
        os << indent << indent << *r << "\n";
      }
    }
  }
#endif
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::Traverse(int operation)
{
  if (operation == operation_type::render)
  {
    this->Apply(operation, true);
    if (this->AnariSceneConstructedMTime < this->AnariSceneStructureModifiedMTime)
    {
      this->Internal->AnariLights.clear();
      this->Internal->AnariVolumes.clear();
      this->Internal->AnariSurfaces.clear();
      for (auto val : this->Children)
      {
        val->Traverse(operation);
      }
      this->UpdateAnariLights();
      this->UpdateAnariSurfaces();
      this->UpdateAnariVolumes();
      this->AnariSceneConstructedMTime = this->AnariSceneStructureModifiedMTime;
    }
    this->Apply(operation, false);
  }
  else
  {
    this->Superclass::Traverse(operation);
  }
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::Invalidate(bool prepass)
{
  if (prepass)
  {
    this->RenderTime = 0;
  }
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::Build(bool prepass)
{
  vtkAnariProfiling startProfiling("vtkAnariRendererNode::Build", vtkAnariProfiling::BLUE);

  if (prepass)
  {
    vtkRenderer* aren = vtkRenderer::SafeDownCast(this->Renderable);

    // make sure we have a camera
    if (!(aren->IsActiveCameraCreated()))
    {
      aren->ResetCamera();
    }
  }

  this->Superclass::Build(prepass);
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::Render(bool prepass)
{
  vtkAnariProfiling startProfiling("vtkAnariRendererNode::Render", vtkAnariProfiling::BLUE);

  vtkRenderer* ren = this->GetRenderer();
  if (prepass || !ren)
  {
    return;
  }

  this->InitAnariRenderer(ren);
  this->SetupAnariRendererParameters(ren);
  this->UpdateAnariFrameSize();
#if 0
    this->DebugOutputWorldBounds();
#endif

  // Render frame
  auto anariDevice = this->GetAnariDevice();
  auto anariFrame = this->Internal->AnariFrame;
  int accumulationCount = this->GetAccumulationCount(ren);
  for (int i = 0; i < accumulationCount; i++)
  {
    anari::render(anariDevice, anariFrame);
  }

  anari::wait(anariDevice, anariFrame);

  CopyAnariFrameBufferData();
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::WriteLayer(
  unsigned char* buffer, float* Z, int buffx, int buffy, int layer)
{
  vtkAnariProfiling startProfiling("vtkAnariRendererNode::WriteLayer", vtkAnariProfiling::BLUE);
  unsigned char* colorBuffer = this->Internal->ColorBuffer.data();
  float* zBuffer = this->Internal->DepthBuffer.data();

  if (layer == 0)
  {
    for (int j = 0; j < buffy && j < this->Size[1]; j++)
    {
      unsigned char* iptr = colorBuffer + j * this->Size[0] * 4;
      float* zptr = zBuffer + j * this->Size[0];

      unsigned char* optr = buffer + j * buffx * 4;
      float* ozptr = Z + j * buffx;

      for (int i = 0; i < buffx && i < this->Size[0]; i++)
      {
        // copy 4 color channels
        *optr++ = *iptr++;
        *optr++ = *iptr++;
        *optr++ = *iptr++;
        *optr++ = *iptr++;

        // copy depth values
        *ozptr++ = *zptr++;
      }
    }
  }
  else
  {
    for (int j = 0; j < buffy && j < this->Size[1]; j++)
    {
      unsigned char* iptr = colorBuffer + j * this->Size[0] * 4;
      float* zptr = zBuffer + j * this->Size[0];

      unsigned char* optr = buffer + j * buffx * 4;
      float* ozptr = Z + j * buffx;

      for (int i = 0; i < buffx && i < this->Size[0]; i++)
      {
        if (*zptr < 1.0)
        {
          if (this->Internal->CompositeOnGL)
          {
            unsigned char a = (*(iptr + 3));
            float A = static_cast<float>(a) / 255.0f;

            for (int h = 0; h < 3; h++)
            {
              *optr = static_cast<u_char>(((float)*iptr) * (A) + ((float)*optr) * (1 - A));
              optr++;
              iptr++;
            }

            *optr++ = *iptr++;
          }
          else
          {
            *optr++ = *iptr++;
            *optr++ = *iptr++;
            *optr++ = *iptr++;
            *optr++ = *iptr++;
          }

          *ozptr = *zptr;
        }
        else
        {
          optr += 4;
          iptr += 4;
        }

        ozptr++;
        zptr++;
      }
    }
  }
}

//------------------------------------------------------------------------------
vtkRenderer* vtkAnariRendererNode::GetRenderer()
{
  return vtkRenderer::SafeDownCast(this->GetRenderable());
}

//------------------------------------------------------------------------------
anari::Device vtkAnariRendererNode::GetAnariDevice()
{
  return this->Internal->AnariDevice;
}

//------------------------------------------------------------------------------
const anari::Extensions& vtkAnariRendererNode::GetAnariDeviceExtensions()
{
  return this->Internal->AnariExtensions;
}

//------------------------------------------------------------------------------
const unsigned char* vtkAnariRendererNode::GetBuffer()
{
  return this->Internal->ColorBuffer.data();
}

//------------------------------------------------------------------------------
const float* vtkAnariRendererNode::GetZBuffer()
{
  return this->Internal->DepthBuffer.data();
}

//------------------------------------------------------------------------------
void vtkAnariRendererNode::InvalidateSceneStructure()
{
  this->AnariSceneStructureModifiedMTime.Modified();
}

//------------------------------------------------------------------------------
void vtkAnariRendererNode::InvalidateRendererParameters()
{
  vtkAnariRendererNode::AnariRendererModifiedTime.Modified();
}

//------------------------------------------------------------------------------
void vtkAnariRendererNode::SetAnariDevice(anari::Device d, anari::Extensions e)
{
  vtkRenderer* renderer = GetRenderer();
  if (!renderer)
  {
    vtkErrorMacro(<< "Null vtkRenderer in vtkAnariRendererNode::SetAnariDevice()");
    return;
  }

  if (this->Internal->AnariDevice != nullptr)
  {
    vtkErrorMacro(<< "vtkAnariRendererNode::SetAnariDevice() called too many times");
  }

  anari::retain(d, d);
  this->Internal->AnariDevice = d;
  this->Internal->AnariExtensions = e;
  this->InitAnariFrame(renderer);
  this->InitAnariWorld();
}

VTK_ABI_NAMESPACE_END
