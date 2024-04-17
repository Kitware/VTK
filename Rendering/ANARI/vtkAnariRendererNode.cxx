// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif

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

#include <anari/anari_cpp/ext/std.h>

VTK_ABI_NAMESPACE_BEGIN

using uvec2 = anari::std_types::uvec2;
using ivec2 = anari::std_types::ivec2;
using vec4 = anari::std_types::vec4;

vtkInformationKeyMacro(vtkAnariRendererNode, SAMPLES_PER_PIXEL, Integer);
vtkInformationKeyMacro(vtkAnariRendererNode, AMBIENT_SAMPLES, Integer);
vtkInformationKeyMacro(vtkAnariRendererNode, COMPOSITE_ON_GL, Integer);
vtkInformationKeyMacro(vtkAnariRendererNode, LIBRARY_NAME, String);
vtkInformationKeyMacro(vtkAnariRendererNode, DEVICE_SUBTYPE, String);
vtkInformationKeyMacro(vtkAnariRendererNode, DEBUG_LIBRARY_NAME, String);
vtkInformationKeyMacro(vtkAnariRendererNode, DEBUG_DEVICE_SUBTYPE, String);
vtkInformationKeyMacro(vtkAnariRendererNode, DEBUG_DEVICE_DIRECTORY, String);
vtkInformationKeyMacro(vtkAnariRendererNode, DEBUG_DEVICE_TRACE_MODE, String);
vtkInformationKeyMacro(vtkAnariRendererNode, USE_DEBUG_DEVICE, Integer);
vtkInformationKeyMacro(vtkAnariRendererNode, RENDERER_SUBTYPE, String);
vtkInformationKeyMacro(vtkAnariRendererNode, ACCUMULATION_COUNT, Integer);
vtkInformationKeyMacro(vtkAnariRendererNode, USE_DENOISER, Integer);
vtkInformationKeyMacro(vtkAnariRendererNode, LIGHT_FALLOFF, Double);
vtkInformationKeyMacro(vtkAnariRendererNode, AMBIENT_INTENSITY, Double);
vtkInformationKeyMacro(vtkAnariRendererNode, MAX_DEPTH, Integer);
vtkInformationKeyMacro(vtkAnariRendererNode, R_VALUE, Double);
vtkInformationKeyMacro(vtkAnariRendererNode, DEBUG_METHOD, String);
vtkInformationKeyMacro(vtkAnariRendererNode, USD_DIRECTORY, String);
vtkInformationKeyMacro(vtkAnariRendererNode, USD_COMMIT, Integer);
vtkInformationKeyMacro(vtkAnariRendererNode, USD_OUTPUT_BINARY, Integer);
vtkInformationKeyMacro(vtkAnariRendererNode, USD_OUTPUT_MATERIAL, Integer);
vtkInformationKeyMacro(vtkAnariRendererNode, USD_OUTPUT_PREVIEW, Integer);
vtkInformationKeyMacro(vtkAnariRendererNode, USD_OUTPUT_MDL, Integer);
vtkInformationKeyMacro(vtkAnariRendererNode, USD_OUTPUT_MDLCOLORS, Integer);
vtkInformationKeyMacro(vtkAnariRendererNode, USD_OUTPUT_DISPLAYCOLORS, Integer);
vtkInformationKeyMacro(vtkAnariRendererNode, AMBIENT_COLOR, DoubleVector);

namespace anari_vtk
{
struct RendererParameters
{
  RendererParameters()
    : Denoise(false)
    , SamplesPerPixel(-1)
    , AmbientSamples(-1)
    , LightFalloff(-1.f)
    , AmbientIntensity(-1.f)
    , MaxDepth(0)
  {
  }

  std::string Subtype;
  bool Denoise;
  int SamplesPerPixel;
  int AmbientSamples;
  float LightFalloff;
  float AmbientIntensity;
  int MaxDepth;
  std::string DebugMethod;
};

using SurfaceState = std::vector<anari::Surface>;
using VolumeState = std::vector<anari::Volume>;
using LightState = std::vector<anari::Light>;
}

class vtkAnariRendererNodeInternals
{
public:
  vtkAnariRendererNodeInternals(vtkAnariRendererNode*);
  ~vtkAnariRendererNodeInternals();

  //@{
  void SetCamera(anari::Camera);
  //@}

  //@{
  /**
   * Methods to add, get, and clear ANARI lights.
   */
  void AddLight(anari::Light);
  const anari_vtk::LightState& GetLightState();
  void ClearLights();
  //@}

  //@{
  /**
   * Methods to add, get, and clear ANARI surfaces.
   */
  void AddSurfaces(const std::vector<anari::Surface>&);
  const anari_vtk::SurfaceState& GetSurfaceState();
  void ClearSurfaces();
  //@}

  //@{
  /**
   * Methods to add, get, and clear ANARI volumes.
   */
  void AddVolume(anari::Volume);
  const anari_vtk::VolumeState& GetVolumeState();
  void ClearVolumes();
  //@}

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
   * Load the ANARI library and initialize the ANARI back-end device.
   * @return true if ANARI was successfully initialized, false otherwise.
   */
  bool InitAnari();

  /**
   * ANARI status callback used as the default value for the statusCallback
   * parameter on devices created from the returned library object.
   */
  static void StatusCallback(const void* userData, anari::Device device, anari::Object source,
    anari::DataType sourceType, anari::StatusSeverity severity, anari::StatusCode code,
    const char* details);

  vtkAnariRendererNode* Owner{ nullptr };

  int ColorBufferTex{ 0 };
  int DepthBufferTex{ 0 };

  std::vector<u_char> ColorBuffer;
  std::vector<float> DepthBuffer;

  int ImageX;
  int ImageY;

  std::string LibraryName;
  std::string LibrarySubtype;
  bool CompositeOnGL{ false };
  bool IsUSD{ false };
  bool InitFlag{ false };

  anari_vtk::RendererParameters RendererParams;

  anari::Library AnariLibrary{ nullptr };
  anari::Library DebugAnariLibrary{ nullptr };
  anari::Device AnariDevice{ nullptr };
  anari::Renderer AnariRenderer{ nullptr };
  anari::World AnariWorld{ nullptr };
  anari::Instance AnariInstance{ nullptr };
  anari::Group AnariGroup{ nullptr };
  anari::Frame AnariFrame{ nullptr };

  anari::Extensions AnariExtensions{};

  anari_vtk::SurfaceState AnariSurfaceState;
  anari_vtk::VolumeState AnariVolumeState;
  anari_vtk::LightState AnariLightState;
};

vtkAnariRendererNodeInternals::vtkAnariRendererNodeInternals(vtkAnariRendererNode* owner)
  : Owner(owner)
{
}

vtkAnariRendererNodeInternals::~vtkAnariRendererNodeInternals()
{
  if (this->AnariDevice != nullptr)
  {
    anari::release(this->AnariDevice, this->AnariGroup);
    anari::release(this->AnariDevice, this->AnariInstance);
    anari::release(this->AnariDevice, this->AnariWorld);
    anari::release(this->AnariDevice, this->AnariRenderer);
    anari::release(this->AnariDevice, this->AnariFrame);
    anari::release(this->AnariDevice, this->AnariDevice);
  }

  if (this->AnariLibrary != nullptr)
  {
    anari::unloadLibrary(this->AnariLibrary);
  }

  if (this->DebugAnariLibrary != nullptr)
  {
    anari::unloadLibrary(this->DebugAnariLibrary);
  }
}

//----------------------------------------------------------------------------
bool vtkAnariRendererNodeInternals::InitAnari()
{
  vtkAnariProfiling startProfiling(
    "vtkAnariRendererNodeInternals::InitAnari", vtkAnariProfiling::AQUA);
  bool retVal = true;

  const char* libraryName = vtkAnariRendererNode::GetLibraryName(this->Owner->GetRenderer());
  vtkDebugWithObjectMacro(
    this->Owner, << "VTK Library name: " << ((libraryName != nullptr) ? libraryName : "nullptr"));

  if (libraryName != nullptr)
  {
    this->LibraryName = libraryName;
    this->AnariLibrary =
      anari::loadLibrary(libraryName, vtkAnariRendererNodeInternals::StatusCallback);

    const char* librarySubtype = vtkAnariRendererNode::GetDeviceSubtype(this->Owner->GetRenderer());
    vtkDebugWithObjectMacro(this->Owner, << "VTK Library subtype: " << librarySubtype);
    this->LibrarySubtype = librarySubtype;

    const int useDebugDevice = vtkAnariRendererNode::GetUseDebugDevice(this->Owner->GetRenderer());

    if (useDebugDevice)
    {
      const char* debugLibraryName =
        vtkAnariRendererNode::GetDebugLibraryName(this->Owner->GetRenderer());
      vtkDebugWithObjectMacro(this->Owner, << "VTK Debug Library name: " << debugLibraryName);

      this->DebugAnariLibrary =
        anari::loadLibrary(debugLibraryName, vtkAnariRendererNodeInternals::StatusCallback);
      const char* debugLibrarySubtype =
        vtkAnariRendererNode::GetDebugDeviceSubtype(this->Owner->GetRenderer());
      this->AnariDevice = anariNewDevice(this->DebugAnariLibrary, debugLibrarySubtype);
    }
    else
    {
      this->AnariDevice = anariNewDevice(this->AnariLibrary, librarySubtype);
    }

    if (this->AnariDevice)
    {
      anari::Device nestedDevice = nullptr;

      if ((this->DebugAnariLibrary != nullptr) && useDebugDevice)
      {
        nestedDevice = anariNewDevice(this->AnariLibrary, librarySubtype);
        anari::setParameter(
          this->AnariDevice, this->AnariDevice, "wrappedDevice", ANARI_DEVICE, &nestedDevice);

        const char* debugDeviceDir =
          vtkAnariRendererNode::GetDebugDeviceDirectory(this->Owner->GetRenderer());
        if (debugDeviceDir != nullptr)
        {
          anari::setParameter(
            this->AnariDevice, this->AnariDevice, "traceDir", ANARI_STRING, debugDeviceDir);
        }

        const char* debugDeviceTraceMode =
          vtkAnariRendererNode::GetDebugDeviceTraceMode(this->Owner->GetRenderer());
        if (debugDeviceTraceMode != nullptr)
        {
          anari::setParameter(
            this->AnariDevice, this->AnariDevice, "traceMode", ANARI_STRING, debugDeviceTraceMode);
        }
      }

      this->IsUSD = false;

      if (this->LibraryName.find("usd") != std::string::npos)
      {
        this->IsUSD = true;
        this->SetUSDDeviceParameters();
      }

      anari::commitParameters(this->AnariDevice, this->AnariDevice);

      if (nestedDevice != nullptr)
      {
        anari::release(nestedDevice, nestedDevice);
      }

      // Populate the current back-end device features
      bool meetsMinReqs =
        this->SetAnariDeviceFeatures(this->AnariLibrary, libraryName, librarySubtype);

      if (meetsMinReqs)
      {
        vtkDebugWithObjectMacro(this->Owner,
          << "[ANARI::" << libraryName << "] Loaded " << librarySubtype << " device.\n");
      }
      else
      {
        vtkDebugWithObjectMacro(
          this->Owner, << "[ANARI::" << libraryName << "] Loaded " << librarySubtype
                       << " device doesn't have the minimum required features.\n");
      }
    }
    else
    {
      vtkErrorWithObjectMacro(this->Owner,
        << "[ANARI::" << libraryName << "] Could not load " << librarySubtype << " device.\n");
      this->LibraryName.clear();

      if (this->AnariLibrary != nullptr)
      {
        anari::unloadLibrary(this->AnariLibrary);
      }

      retVal = false;
    }
  }
  else
  {
    vtkErrorWithObjectMacro(this->Owner, << "[ANARI] Library name not set (nullptr).");
    retVal = false;
  }

  return retVal;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNodeInternals::SetCamera(anari::Camera camera)
{
  if (this->AnariDevice && this->AnariFrame)
  {
    anari::setParameter(this->AnariDevice, this->AnariFrame, "camera", camera);
    anari::commitParameters(this->AnariDevice, this->AnariFrame);
  }
}

//----------------------------------------------------------------------------
void vtkAnariRendererNodeInternals::AddLight(anari::Light light)
{
  if (light != nullptr)
  {
    this->AnariLightState.emplace_back(light);
  }
}

//----------------------------------------------------------------------------
const anari_vtk::LightState& vtkAnariRendererNodeInternals::GetLightState()
{
  return this->AnariLightState;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNodeInternals::ClearLights()
{
  this->AnariLightState.clear();
}

//----------------------------------------------------------------------------
void vtkAnariRendererNodeInternals::AddSurfaces(const std::vector<anari::Surface>& surfaces)
{
  for (auto surface : surfaces)
  {
    this->AnariSurfaceState.emplace_back(surface);
  }
}

//----------------------------------------------------------------------------
const anari_vtk::SurfaceState& vtkAnariRendererNodeInternals::GetSurfaceState()
{
  return this->AnariSurfaceState;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNodeInternals::ClearSurfaces()
{
  this->AnariSurfaceState.clear();
}

//----------------------------------------------------------------------------
void vtkAnariRendererNodeInternals::AddVolume(anari::Volume volume)
{
  if (volume != nullptr)
  {
    this->AnariVolumeState.emplace_back(volume);
  }
}

//----------------------------------------------------------------------------
const anari_vtk::VolumeState& vtkAnariRendererNodeInternals::GetVolumeState()
{
  return this->AnariVolumeState;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNodeInternals::ClearVolumes()
{
  this->AnariVolumeState.clear();
}

//----------------------------------------------------------------------------
void vtkAnariRendererNodeInternals::StatusCallback(const void* userData, anari::Device device,
  anari::Object source, anari::DataType sourceType, anari::StatusSeverity severity,
  anari::StatusCode code, const char* message)
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

//----------------------------------------------------------------------------
void vtkAnariRendererNodeInternals::SetUSDDeviceParameters()
{
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
}

//------------------------------------------------------------------------------
bool vtkAnariRendererNodeInternals::SetAnariDeviceFeatures(
  anari::Library library, const char* deviceName, const char* deviceSubtype)
{
  bool useableDevice = false;
  const char* const* list = (const char* const*)anariGetDeviceExtensions(library, deviceSubtype);
  if (list)
  {
    memset(&this->AnariExtensions, 0, sizeof(anari::Extensions));

    for (const auto* i = list; *i != NULL; ++i)
    {
      std::string feature = *i;
      vtkDebugWithObjectMacro(
        this->Owner, << "[" << deviceName << ":" << deviceSubtype << "] Feature => " << feature);
    }
  }

  anariGetDeviceExtensionStruct(&this->AnariExtensions, library, deviceName);

  if (this->IsUSD)
  {
    useableDevice = true;
    this->AnariExtensions.ANARI_KHR_GEOMETRY_CYLINDER = 1;
    this->AnariExtensions.ANARI_KHR_GEOMETRY_SPHERE = 1;
    this->AnariExtensions.ANARI_KHR_GEOMETRY_TRIANGLE = 1;
  }
  else
  {
    if ((this->AnariExtensions.ANARI_KHR_GEOMETRY_CYLINDER ||
          this->AnariExtensions.ANARI_KHR_GEOMETRY_CURVE) &&
      this->AnariExtensions.ANARI_KHR_GEOMETRY_SPHERE &&
      this->AnariExtensions.ANARI_KHR_GEOMETRY_TRIANGLE &&
      this->AnariExtensions.ANARI_KHR_INSTANCE_TRANSFORM)
    {
      useableDevice = true;
    }
  }

  return useableDevice;
}

//============================================================================

vtkStandardNewMacro(vtkAnariRendererNode);

//----------------------------------------------------------------------------
vtkAnariRendererNode::vtkAnariRendererNode()
  : SphereCount(0)
  , CylinderCount(0)
  , CurveCount(0)
  , TriangleCount(0)
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
void vtkAnariRendererNode::InitAnariFrame()
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
}

//----------------------------------------------------------------------------
bool vtkAnariRendererNode::InitAnariRenderer(vtkRenderer* ren)
{
  auto anariDevice = this->GetAnariDevice();
  auto anariFrame = this->Internal->AnariFrame;

  auto currentRendererSubtype = this->Internal->RendererParams.Subtype;
  this->Internal->CompositeOnGL = (this->GetCompositeOnGL(ren) != 0);
  const char* rendererSubtype = vtkAnariRendererNode::GetRendererSubtype(this->GetRenderer());
  bool isNewRenderer = false;

  if (currentRendererSubtype != rendererSubtype)
  {
    isNewRenderer = true;
    this->Internal->RendererParams.Subtype = rendererSubtype;

    anari::release(anariDevice, this->Internal->AnariRenderer);

    this->Internal->AnariRenderer = anari::newObject<anari::Renderer>(anariDevice, rendererSubtype);

    anari::setParameter(anariDevice, anariFrame, "renderer", this->Internal->AnariRenderer);
    anari::commitParameters(anariDevice, anariFrame);
  }

  return isNewRenderer;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetupAnariRendererParameters(vtkRenderer* ren, bool isNewRenderer)
{
  auto anariDevice = this->GetAnariDevice();
  auto anariRenderer = this->Internal->AnariRenderer;

  // TODO: have this as a renderer parameter
  // bool useAccumulation = this->GetUseAccumulation(ren) > 0 ? true : false;
  // auto anariDeviceExtensions = this->GetAnariDeviceExtensions();
  // if(anariDeviceExtensions.ANARI_KHR_FRAME_ACCUMULATION &&
  //    this->Internal->RendererParams.Accumulation != useAccumulation)
  // {
  //   this->Internal->RendererParams.Accumulation = useAccumulation;
  //   anari::setParameter(anariDevice, this->Internal->AnariFrame, "accumulation",
  //   useAccumulation); anari::commitParameters(anariDevice, anariRenderer);
  // }

  bool useDenoiser = this->GetUseDenoiser(ren) > 0 ? true : false;
  if (isNewRenderer || this->Internal->RendererParams.Denoise != useDenoiser)
  {
    anari::setParameter(anariDevice, anariRenderer, "denoise", useDenoiser);
    this->Internal->RendererParams.Denoise = useDenoiser;
    anari::commitParameters(anariDevice, anariRenderer);
  }

  auto spp = this->GetSamplesPerPixel(ren);
  if (isNewRenderer || this->Internal->RendererParams.SamplesPerPixel != spp)
  {
    anari::setParameter(anariDevice, anariRenderer, "pixelSamples", spp);
    this->Internal->RendererParams.SamplesPerPixel = spp;
    anari::commitParameters(anariDevice, anariRenderer);
  }

  auto aoSamples = this->GetAmbientSamples(ren);
  if (isNewRenderer || this->Internal->RendererParams.AmbientSamples != aoSamples)
  {
    anari::setParameter(anariDevice, anariRenderer, "ambientSamples", aoSamples);
    this->Internal->RendererParams.AmbientSamples = aoSamples;
    anari::commitParameters(anariDevice, anariRenderer);
  }

  float lightFalloff = static_cast<float>(this->GetLightFalloff(ren));
  if (isNewRenderer ||
    std::abs(this->Internal->RendererParams.LightFalloff - lightFalloff) > 0.0001f)
  {
    anari::setParameter(anariDevice, anariRenderer, "lightFalloff", lightFalloff);
    this->Internal->RendererParams.LightFalloff = lightFalloff;
    anari::commitParameters(anariDevice, anariRenderer);
  }

  float ambientIntensity = static_cast<float>(this->GetAmbientIntensity(ren));
  if (isNewRenderer ||
    std::abs(this->Internal->RendererParams.AmbientIntensity - ambientIntensity) > 0.0001f)
  {
    anari::setParameter(anariDevice, anariRenderer, "ambientRadiance", ambientIntensity);
    this->Internal->RendererParams.AmbientIntensity = ambientIntensity;
    anari::commitParameters(anariDevice, anariRenderer);
  }

  auto maxDepth = this->GetMaxDepth(ren);
  if (isNewRenderer || this->Internal->RendererParams.MaxDepth != maxDepth)
  {
    anari::setParameter(anariDevice, anariRenderer, "maxDepth", maxDepth);
    this->Internal->RendererParams.MaxDepth = maxDepth;
    anari::commitParameters(anariDevice, anariRenderer);
  }

  // Debug
  auto debugMethod = this->GetDebugMethod(ren);
  if (debugMethod != nullptr)
  {
    if (this->Internal->RendererParams.DebugMethod != debugMethod)
    {
      this->Internal->RendererParams.DebugMethod = debugMethod;
      anari::setParameter(anariDevice, anariRenderer, "method", debugMethod);
      anari::commitParameters(anariDevice, anariRenderer);
    }
  }

  double* ambientColor = this->GetAmbientColor(ren);
  if (ambientColor != nullptr)
  {
    float ambientColorf[3] = { static_cast<float>(ambientColor[0]),
      static_cast<float>(ambientColor[1]), static_cast<float>(ambientColor[2]) };
    anari::setParameter(anariDevice, anariRenderer, "ambientColor", ambientColorf);
    anari::commitParameters(anariDevice, anariRenderer);
  }

  double* bg = ren->GetBackground();
  double bgAlpha = ren->GetBackgroundAlpha();

  if (!ren->GetGradientBackground())
  {
    float bgColor[4] = { static_cast<float>(bg[0]), static_cast<float>(bg[1]),
      static_cast<float>(bg[2]), static_cast<float>(bgAlpha) };

    anari::setParameter(anariDevice, anariRenderer, "background", bgColor);
    anari::commitParameters(anariDevice, anariRenderer);
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
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::InitAnariWorld()
{
  if (this->Internal->AnariWorld != nullptr)
  {
    return;
  }

  auto anariDevice = this->GetAnariDevice();

  auto anariGroup = anari::newObject<anari::Group>(anariDevice);
  this->Internal->AnariGroup = anariGroup;
  anari::setParameter(anariDevice, anariGroup, "name", ANARI_STRING, "vtk_group");
  anari::commitParameters(anariDevice, anariGroup);

  auto anariInstance = anari::newObject<anari::Instance>(anariDevice, "transform");
  this->Internal->AnariInstance = anariInstance;
  anari::setParameter(anariDevice, anariInstance, "name", ANARI_STRING, "vtk_instance");
  anari::setParameter(anariDevice, anariInstance, "group", anariGroup);
  anari::commitParameters(anariDevice, anariInstance);

  auto anariWorld = anari::newObject<anari::World>(anariDevice);
  this->Internal->AnariWorld = anariWorld;
  anari::setParameter(anariDevice, anariWorld, "name", "vtk_world");
  anari::setParameterArray1D(anariDevice, anariWorld, "instance", &anariInstance, 1);
  anari::commitParameters(anariDevice, anariWorld);

  auto anariFrame = this->Internal->AnariFrame;
  anari::setParameter(anariDevice, anariFrame, "world", this->Internal->AnariWorld);
  anari::commitParameters(anariDevice, anariFrame);
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::UpdateAnariFrameSize()
{
  const uvec2 frameSize = { static_cast<uint>(this->Size[0]), static_cast<uint>(this->Size[1]) };
  if (this->Internal->ImageX == frameSize[0] && this->Internal->ImageY == frameSize[1])
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
  auto lightState = this->Internal->GetLightState();
  auto anariWorld = this->Internal->AnariWorld;

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
  auto surfaceState = this->Internal->GetSurfaceState();
  auto anariGroup = this->Internal->AnariGroup;

  if (!surfaceState.empty())
  {
    for (size_t i = 0; i < surfaceState.size(); i++)
    {
      std::string surfaceName = "vtk_surface_" + std::to_string(i);
      anari::setParameter(anariDevice, surfaceState[i], "name", surfaceName.c_str());
      anari::commitParameters(anariDevice, surfaceState[i]);
    }

    anari::setParameterArray1D(
      anariDevice, anariGroup, "surface", surfaceState.data(), surfaceState.size());
  }
  else
  {
    anari::unsetParameter(anariDevice, anariGroup, "surface");
  }

  anari::commitParameters(anariDevice, anariGroup);
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::UpdateAnariVolumes()
{
  auto anariDevice = this->GetAnariDevice();
  auto volumeState = this->Internal->GetVolumeState();
  auto anariGroup = this->Internal->AnariGroup;

  if (!volumeState.empty())
  {
    for (size_t i = 0; i < volumeState.size(); i++)
    {
      std::string volumeName = "vtk_volume_" + std::to_string(i);
      anari::setParameter(anariDevice, volumeState[i], "name", volumeName.c_str());
      anari::commitParameters(anariDevice, volumeState[i]);
    }

    anari::setParameterArray1D(
      anariDevice, anariGroup, "volume", volumeState.data(), volumeState.size());
  }
  else
  {
    anari::unsetParameter(anariDevice, anariGroup, "volume");
  }

  anari::commitParameters(anariDevice, anariGroup);
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
    vtkDebugMacro(<< "[ANARI::" << this->Internal->LibraryName << "] World Bounds: "
                  << "{" << worldBounds[0] << ", " << worldBounds[1] << ", " << worldBounds[2]
                  << "}, "
                  << "{" << worldBounds[3] << ", " << worldBounds[4] << ", " << worldBounds[5]
                  << "}");
  }
  else
  {
    vtkWarningMacro(<< "[ANARI::" << this->Internal->LibraryName << "] World bounds not returned");
  }
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::CopyAnariFrameBufferData()
{
  int totalSize = this->Size[0] * this->Size[1];
  if (this->Internal->IsUSD)
  {
    memset(this->Internal->ColorBuffer.data(), 255, totalSize * 4);
    memset(this->Internal->DepthBuffer.data(), 1, totalSize * sizeof(float));
    return;
  }

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
    int totalSize = this->Size[0] * this->Size[1];
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
void vtkAnariRendererNode::SetUseDenoiser(int value, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::USE_DENOISER(), value);
}

//----------------------------------------------------------------------------
int vtkAnariRendererNode::GetUseDenoiser(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return 0;
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::USE_DENOISER()))
  {
    return (info->Get(vtkAnariRendererNode::USE_DENOISER()));
  }

  return 0;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetSamplesPerPixel(int value, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::SAMPLES_PER_PIXEL(), value);
}

//----------------------------------------------------------------------------
int vtkAnariRendererNode::GetSamplesPerPixel(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return 1;
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::SAMPLES_PER_PIXEL()))
  {
    return (info->Get(vtkAnariRendererNode::SAMPLES_PER_PIXEL()));
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetLibraryName(const char* name, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::LIBRARY_NAME(), name);
}

//----------------------------------------------------------------------------
const char* vtkAnariRendererNode::GetLibraryName(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return nullptr;
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::LIBRARY_NAME()))
  {
    return info->Get(vtkAnariRendererNode::LIBRARY_NAME());
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetDeviceSubtype(const char* name, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::DEVICE_SUBTYPE(), name);
}

//----------------------------------------------------------------------------
const char* vtkAnariRendererNode::GetDeviceSubtype(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return "default";
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::DEVICE_SUBTYPE()))
  {
    return info->Get(vtkAnariRendererNode::DEVICE_SUBTYPE());
  }

  return "default";
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetDebugLibraryName(const char* name, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::DEBUG_LIBRARY_NAME(), name);
}

//----------------------------------------------------------------------------
const char* vtkAnariRendererNode::GetDebugLibraryName(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return "debug";
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::DEBUG_LIBRARY_NAME()))
  {
    return info->Get(vtkAnariRendererNode::DEBUG_LIBRARY_NAME());
  }

  return "debug";
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetDebugDeviceSubtype(const char* name, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::DEBUG_DEVICE_SUBTYPE(), name);
}

//----------------------------------------------------------------------------
const char* vtkAnariRendererNode::GetDebugDeviceSubtype(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return "debug";
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::DEBUG_DEVICE_SUBTYPE()))
  {
    return info->Get(vtkAnariRendererNode::DEBUG_DEVICE_SUBTYPE());
  }

  return "debug";
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetDebugDeviceDirectory(const char* name, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::DEBUG_DEVICE_DIRECTORY(), name);
}

//----------------------------------------------------------------------------
const char* vtkAnariRendererNode::GetDebugDeviceDirectory(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return nullptr;
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::DEBUG_DEVICE_DIRECTORY()))
  {
    return info->Get(vtkAnariRendererNode::DEBUG_DEVICE_DIRECTORY());
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetDebugDeviceTraceMode(const char* name, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::DEBUG_DEVICE_TRACE_MODE(), name);
}

//----------------------------------------------------------------------------
const char* vtkAnariRendererNode::GetDebugDeviceTraceMode(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return "code";
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::DEBUG_DEVICE_TRACE_MODE()))
  {
    return info->Get(vtkAnariRendererNode::DEBUG_DEVICE_TRACE_MODE());
  }

  return "code";
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetUseDebugDevice(int value, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::USE_DEBUG_DEVICE(), value);
}

//----------------------------------------------------------------------------
int vtkAnariRendererNode::GetUseDebugDevice(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return 0;
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::USE_DEBUG_DEVICE()))
  {
    return (info->Get(vtkAnariRendererNode::USE_DEBUG_DEVICE()));
  }

  return 0;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetRendererSubtype(const char* name, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::RENDERER_SUBTYPE(), name);
}

//----------------------------------------------------------------------------
const char* vtkAnariRendererNode::GetRendererSubtype(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return "default";
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::RENDERER_SUBTYPE()))
  {
    return (info->Get(vtkAnariRendererNode::RENDERER_SUBTYPE()));
  }

  return "default";
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetAccumulationCount(int value, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::ACCUMULATION_COUNT(), value);
}

//----------------------------------------------------------------------------
int vtkAnariRendererNode::GetAccumulationCount(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return 1;
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::ACCUMULATION_COUNT()))
  {
    return (info->Get(vtkAnariRendererNode::ACCUMULATION_COUNT()));
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetAmbientSamples(int value, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::AMBIENT_SAMPLES(), value);
}

//----------------------------------------------------------------------------
int vtkAnariRendererNode::GetAmbientSamples(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return 0;
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::AMBIENT_SAMPLES()))
  {
    return (info->Get(vtkAnariRendererNode::AMBIENT_SAMPLES()));
  }

  return 0;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetLightFalloff(double value, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::LIGHT_FALLOFF(), value);
}

//----------------------------------------------------------------------------
double vtkAnariRendererNode::GetLightFalloff(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return 1;
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::LIGHT_FALLOFF()))
  {
    return info->Get(vtkAnariRendererNode::LIGHT_FALLOFF());
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetAmbientColor(double* value, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::AMBIENT_COLOR(), value, 3);
}

//----------------------------------------------------------------------------
double* vtkAnariRendererNode::GetAmbientColor(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return nullptr;
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::AMBIENT_COLOR()))
  {
    return info->Get(vtkAnariRendererNode::AMBIENT_COLOR());
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetAmbientIntensity(double value, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::AMBIENT_INTENSITY(), value);
}

//----------------------------------------------------------------------------
double vtkAnariRendererNode::GetAmbientIntensity(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return 1;
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::AMBIENT_INTENSITY()))
  {
    return info->Get(vtkAnariRendererNode::AMBIENT_INTENSITY());
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetMaxDepth(int value, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::MAX_DEPTH(), value);
}

//----------------------------------------------------------------------------
int vtkAnariRendererNode::GetMaxDepth(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return 0;
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::MAX_DEPTH()))
  {
    return info->Get(vtkAnariRendererNode::MAX_DEPTH());
  }

  return 0;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetROptionValue(double value, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::R_VALUE(), value);
}

//----------------------------------------------------------------------------
double vtkAnariRendererNode::GetROptionValue(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return 1;
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::R_VALUE()))
  {
    return info->Get(vtkAnariRendererNode::R_VALUE());
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetDebugMethod(const char* name, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::DEBUG_METHOD(), name);
}

//----------------------------------------------------------------------------
const char* vtkAnariRendererNode::GetDebugMethod(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return nullptr;
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::DEBUG_METHOD()))
  {
    return info->Get(vtkAnariRendererNode::DEBUG_METHOD());
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetUsdDirectory(const char* name, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::USD_DIRECTORY(), name);
}

//----------------------------------------------------------------------------
const char* vtkAnariRendererNode::GetUsdDirectory(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return nullptr;
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::USD_DIRECTORY()))
  {
    return (info->Get(vtkAnariRendererNode::USD_DIRECTORY()));
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetUsdAtCommit(int value, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::USD_COMMIT(), value);
}

//----------------------------------------------------------------------------
int vtkAnariRendererNode::GetUsdAtCommit(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return 0;
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::USD_COMMIT()))
  {
    return (info->Get(vtkAnariRendererNode::USD_COMMIT()));
  }

  return 0;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetUsdOutputBinary(int value, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::USD_OUTPUT_BINARY(), value);
}

//----------------------------------------------------------------------------
int vtkAnariRendererNode::GetUsdOutputBinary(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return 1;
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::USD_OUTPUT_BINARY()))
  {
    return (info->Get(vtkAnariRendererNode::USD_OUTPUT_BINARY()));
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetUsdOutputMaterial(int value, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::USD_OUTPUT_MATERIAL(), value);
}

//----------------------------------------------------------------------------
int vtkAnariRendererNode::GetUsdOutputMaterial(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return 1;
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::USD_OUTPUT_MATERIAL()))
  {
    return (info->Get(vtkAnariRendererNode::USD_OUTPUT_MATERIAL()));
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetUsdOutputPreviewSurface(int value, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::USD_OUTPUT_PREVIEW(), value);
}

//----------------------------------------------------------------------------
int vtkAnariRendererNode::GetUsdOutputPreviewSurface(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return 1;
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::USD_OUTPUT_PREVIEW()))
  {
    return (info->Get(vtkAnariRendererNode::USD_OUTPUT_PREVIEW()));
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetUsdOutputMDL(int value, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::USD_OUTPUT_MDL(), value);
}

//----------------------------------------------------------------------------
int vtkAnariRendererNode::GetUsdOutputMDL(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return 1;
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::USD_OUTPUT_MDL()))
  {
    return (info->Get(vtkAnariRendererNode::USD_OUTPUT_MDL()));
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetUsdOutputMDLColors(int value, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::USD_OUTPUT_MDLCOLORS(), value);
}

//----------------------------------------------------------------------------
int vtkAnariRendererNode::GetUsdOutputMDLColors(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return 1;
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::USD_OUTPUT_MDLCOLORS()))
  {
    return (info->Get(vtkAnariRendererNode::USD_OUTPUT_MDLCOLORS()));
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetUsdOutputDisplayColors(int value, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::USD_OUTPUT_DISPLAYCOLORS(), value);
}

//----------------------------------------------------------------------------
int vtkAnariRendererNode::GetUsdOutputDisplayColors(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return 1;
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::USD_OUTPUT_DISPLAYCOLORS()))
  {
    return (info->Get(vtkAnariRendererNode::USD_OUTPUT_DISPLAYCOLORS()));
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetCompositeOnGL(int value, vtkRenderer* renderer)
{
  if (!renderer)
  {
    return;
  }
  vtkInformation* info = renderer->GetInformation();
  info->Set(vtkAnariRendererNode::COMPOSITE_ON_GL(), value);
}

//----------------------------------------------------------------------------
int vtkAnariRendererNode::GetCompositeOnGL(vtkRenderer* renderer)
{
  if (!renderer)
  {
    return 0;
  }
  vtkInformation* info = renderer->GetInformation();
  if (info && info->Has(vtkAnariRendererNode::COMPOSITE_ON_GL()))
  {
    return (info->Get(vtkAnariRendererNode::COMPOSITE_ON_GL()));
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::SetCamera(anari::Camera camera)
{
  this->Internal->SetCamera(camera);
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::AddLight(anari::Light light)
{
  this->Internal->AddLight(light);
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::AddSurfaces(const std::vector<anari::Surface>& surfaces)
{
  this->Internal->AddSurfaces(surfaces);
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::AddVolume(anari::Volume volume)
{
  this->Internal->AddVolume(volume);
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
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
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::Traverse(int operation)
{

  vtkRenderer* renderer = vtkRenderer::SafeDownCast(this->GetRenderable());
  if (!renderer)
  {
    return;
  }

  if (!this->Internal->InitFlag)
  {
    this->Internal->InitFlag = this->Internal->InitAnari();
  }

  if (operation == operation_type::render)
  {
    this->Apply(operation, true);
    if (this->AnariSceneConstructedMTime < this->AnariSceneStructureModifiedMTime)
    {
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
  if (!ren)
  {
    return;
  }

  auto anariDevice = this->GetAnariDevice();
  if (anariDevice == nullptr)
  {
    return;
  }

  if (prepass)
  {
    this->InitAnariFrame();
    static bool once = false;
    if (!once)
    {
      bool isNewRenderer = this->InitAnariRenderer(ren);
      this->SetupAnariRendererParameters(ren, isNewRenderer);
      once = true;
    }
    this->InitAnariWorld();
  }
  else
  {
    this->UpdateAnariFrameSize();
#if 0
    this->DebugOutputWorldBounds();
#endif

    // Render frame
    auto anariFrame = this->Internal->AnariFrame;
    int accumulationCount = this->GetAccumulationCount(ren);
    for (int i = 0; i < accumulationCount; i++)
    {
      anari::render(anariDevice, anariFrame);
    }

    anari::wait(anariDevice, anariFrame);

    CopyAnariFrameBufferData();
  }
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
void vtkAnariRendererNode::ResetCounts()
{
  this->SphereCount = 0;
  this->CylinderCount = 0;
  this->CurveCount = 0;
  this->TriangleCount = 0;
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
std::string vtkAnariRendererNode::GetAnariDeviceName()
{
  return this->Internal->LibrarySubtype;
}

//------------------------------------------------------------------------------
anari::Library vtkAnariRendererNode::GetAnariLibrary()
{
  return this->Internal->AnariLibrary;
}

//------------------------------------------------------------------------------
anari::Extensions vtkAnariRendererNode::GetAnariDeviceExtensions()
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
int vtkAnariRendererNode::GetColorBufferTextureGL()
{
  return this->Internal->ColorBufferTex;
}

//------------------------------------------------------------------------------
int vtkAnariRendererNode::GetDepthBufferTextureGL()
{
  return this->Internal->DepthBufferTex;
}

//------------------------------------------------------------------------------
void vtkAnariRendererNode::InvalidateSceneStructure()
{
  this->Internal->ClearLights();
  this->Internal->ClearVolumes();
  this->Internal->ClearSurfaces();
  this->AnariSceneStructureModifiedMTime.Modified();
}

VTK_ABI_NAMESPACE_END
