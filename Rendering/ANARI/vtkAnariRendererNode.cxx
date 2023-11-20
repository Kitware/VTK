// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif

#include "vtkAnariRendererNode.h"
#include "vtkAnariActorNode.h"
#include "vtkAnariCameraNode.h"
#include "vtkAnariLightNode.h"
#include "vtkAnariProfiling.h"
#include "vtkAnariVolumeNode.h"

#include "vtkAbstractVolumeMapper.h"
#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkCollectionIterator.h"
#include "vtkColorTransferFunction.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationStringKey.h"
#include "vtkLight.h"
#include "vtkLogger.h"
#include "vtkMapper.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTexture.h"
#include "vtkTransform.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>

#include <anari/anari_cpp/ext/std.h>

VTK_ABI_NAMESPACE_BEGIN

using uvec2 = anari::std_types::uvec2;
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
    : Subtype()
    , Denoise(false)
    , SamplesPerPixel(-1)
    , AmbientSamples(-1)
    , LightFalloff(-1.f)
    , AmbientIntensity(-1.f)
    , MaxDepth(0)
    , DebugMethod()
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

struct SurfaceState
{
  SurfaceState()
    : changed(false)
    , used(false)
    , Surfaces()
  {
  }

  bool changed;
  bool used;
  std::vector<anari::Surface> Surfaces;
};

struct VolumeState
{
  VolumeState()
    : changed(false)
    , used(false)
    , Volumes()
  {
  }

  bool changed;
  bool used;
  std::vector<anari::Volume> Volumes;
};

struct LightState
{
  LightState()
    : changed(false)
    , used(false)
    , Lights()
  {
  }

  bool changed;
  bool used;
  std::vector<anari::Light> Lights;
};

struct CameraState
{
  CameraState()
    : changed(false)
    , Camera(nullptr)
  {
  }

  bool changed;
  anari::Camera Camera;
};
}

class vtkAnariRendererNodeInternals
{
public:
  vtkAnariRendererNodeInternals(vtkAnariRendererNode*);
  ~vtkAnariRendererNodeInternals();

  //@{
  void AddCamera(anari::Camera, const bool);
  anari_vtk::CameraState GetCameraState();
  //@}

  //@{
  /**
   * Methods to add, get, and clear ANARI lights.
   */
  void AddLight(anari::Light, const bool);
  anari_vtk::LightState GetLightState();
  void ClearLights();
  //@}

  //@{
  /**
   * Methods to add, get, and clear ANARI surfaces.
   */
  void AddSurfaces(const std::vector<anari::Surface>&, const bool);
  anari_vtk::SurfaceState GetSurfaceState();
  void ClearSurfaces();
  //@}

  //@{
  /**
   * Methods to add, get, and clear ANARI volumes.
   */
  void AddVolume(anari::Volume, const bool);
  anari_vtk::VolumeState GetVolumeState();
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

  vtkAnariRendererNode* Owner;

  int ColorBufferTex;
  int DepthBufferTex;

  std::unique_ptr<u_char[]> ColorBuffer;
  std::unique_ptr<float[]> DepthBuffer;

  int ImageX;
  int ImageY;

  std::string LibraryName;
  std::string LibrarySubtype;
  bool CompositeOnGL;
  bool IsUSD;
  bool InitFlag;

  anari_vtk::RendererParameters RendererParams;

  anari::Library AnariLibrary;
  anari::Library DebugAnariLibrary;
  anari::Device AnariDevice;
  anari::Extensions AnariExtensions;
  anari::Renderer AnariRenderer;
  anari::World AnariWorld;
  anari::Instance AnariInstance;
  anari::Group AnariGroup;
  anari::Frame AnariFrame;

  anari_vtk::CameraState AnariCameraState;
  anari_vtk::SurfaceState AnariSurfaceState;
  anari_vtk::VolumeState AnariVolumeState;
  anari_vtk::LightState AnariLightState;
};

vtkAnariRendererNodeInternals::vtkAnariRendererNodeInternals(vtkAnariRendererNode* owner)
  : Owner(owner)
  , ColorBufferTex(0)
  , DepthBufferTex(0)
  , ColorBuffer(nullptr)
  , DepthBuffer(nullptr)
  , ImageX(0)
  , ImageY(0)
  , LibraryName()
  , LibrarySubtype()
  , CompositeOnGL(false)
  , IsUSD(false)
  , InitFlag(false)
  , RendererParams()
  , AnariLibrary(nullptr)
  , DebugAnariLibrary(nullptr)
  , AnariDevice(nullptr)
  , AnariExtensions()
  , AnariRenderer(nullptr)
  , AnariWorld(nullptr)
  , AnariInstance(nullptr)
  , AnariGroup(nullptr)
  , AnariFrame(nullptr)
  , AnariCameraState()
  , AnariSurfaceState()
  , AnariVolumeState()
  , AnariLightState()
{
}

vtkAnariRendererNodeInternals::~vtkAnariRendererNodeInternals()
{
  if (this->AnariDevice != nullptr)
  {
    for (auto surface : this->AnariSurfaceState.Surfaces)
    {
      anari::release(this->AnariDevice, surface);
    }

    for (auto volume : this->AnariVolumeState.Volumes)
    {
      anari::release(this->AnariDevice, volume);
    }

    for (auto light : this->AnariLightState.Lights)
    {
      anari::release(this->AnariDevice, light);
    }

    if (this->AnariGroup != nullptr)
    {
      anari::release(this->AnariDevice, this->AnariGroup);
    }

    if (this->AnariInstance != nullptr)
    {
      anari::release(this->AnariDevice, this->AnariInstance);
    }

    if (this->AnariWorld != nullptr)
    {
      anari::release(this->AnariDevice, this->AnariWorld);
    }

    if (this->AnariRenderer != nullptr)
    {
      anari::release(this->AnariDevice, this->AnariRenderer);
    }

    if (this->AnariFrame != nullptr)
    {
      anari::release(this->AnariDevice, this->AnariFrame);
    }

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
  vtkDebugWithObjectMacro(this->Owner, << "VTK Library name: " << libraryName);

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
void vtkAnariRendererNodeInternals::AddCamera(anari::Camera camera, const bool changed)
{
  this->AnariCameraState.Camera = camera;
  this->AnariCameraState.changed = changed;
}

//----------------------------------------------------------------------------
anari_vtk::CameraState vtkAnariRendererNodeInternals::GetCameraState()
{
  return this->AnariCameraState;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNodeInternals::AddLight(anari::Light light, const bool changed)
{
  if (light != nullptr)
  {
    if (this->AnariLightState.used)
    {
      this->ClearLights();
      this->AnariLightState.used = false;
    }

    this->AnariLightState.Lights.emplace_back(light);
  }

  if (changed)
  {
    this->AnariLightState.changed = true;
  }
}

//----------------------------------------------------------------------------
anari_vtk::LightState vtkAnariRendererNodeInternals::GetLightState()
{
  return this->AnariLightState;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNodeInternals::ClearLights()
{
  if (!this->AnariLightState.Lights.empty())
  {
    this->AnariLightState.Lights.clear();
    std::vector<anari::Light>().swap(this->AnariLightState.Lights);
    this->AnariLightState.changed = true;
  }
}

//----------------------------------------------------------------------------
void vtkAnariRendererNodeInternals::AddSurfaces(
  const std::vector<anari::Surface>& surfaces, const bool changed)
{
  if (!surfaces.empty())
  {
    if (this->AnariSurfaceState.used)
    {
      this->ClearSurfaces();
      this->AnariSurfaceState.used = false;
    }

    for (auto surface : surfaces)
    {
      this->AnariSurfaceState.Surfaces.emplace_back(surface);
    }
  }

  if (changed)
  {
    this->AnariSurfaceState.changed = true;
  }
}

//----------------------------------------------------------------------------
anari_vtk::SurfaceState vtkAnariRendererNodeInternals::GetSurfaceState()
{
  return this->AnariSurfaceState;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNodeInternals::ClearSurfaces()
{
  if (!this->AnariSurfaceState.Surfaces.empty())
  {
    this->AnariSurfaceState.Surfaces.clear();
    std::vector<anari::Surface>().swap(this->AnariSurfaceState.Surfaces);
    this->AnariSurfaceState.changed = true;
  }
}

//----------------------------------------------------------------------------
void vtkAnariRendererNodeInternals::AddVolume(anari::Volume volume, const bool changed)
{
  if (volume != nullptr)
  {
    if (this->AnariVolumeState.used)
    {
      this->ClearVolumes();
      this->AnariVolumeState.used = false;
    }

    this->AnariVolumeState.Volumes.emplace_back(volume);
  }

  if (changed)
  {
    this->AnariVolumeState.changed = true;
  }
}

//----------------------------------------------------------------------------
anari_vtk::VolumeState vtkAnariRendererNodeInternals::GetVolumeState()
{
  return this->AnariVolumeState;
}

//----------------------------------------------------------------------------
void vtkAnariRendererNodeInternals::ClearVolumes()
{
  if (!this->AnariVolumeState.Volumes.empty())
  {
    this->AnariVolumeState.Volumes.clear();
    std::vector<anari::Volume>().swap(this->AnariVolumeState.Volumes);
    this->AnariVolumeState.changed = true;
  }
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
  const char* const* list = (const char* const*)anariGetObjectInfo(
    this->AnariDevice, ANARI_DEVICE, deviceSubtype, "extension", ANARI_STRING_LIST);
  // const char* const* list = (const char* const*)anariGetDeviceFeatures(library, deviceSubtype);

  if (list)
  {
    memset(&this->AnariExtensions, 0, sizeof(anari::Extensions));

    for (const auto* i = list; *i != NULL; ++i)
    {
      std::string feature = *i;
      vtkDebugWithObjectMacro(
        this->Owner, << "[" << deviceName << ":" << deviceSubtype << "] Feature => " << feature);

      if (feature == "ANARI_KHR_INSTANCE_TRANSFORM")
      {
        this->AnariExtensions.ANARI_KHR_INSTANCE_TRANSFORM = 1;
      }
      else if (feature == "ANARI_KHR_CAMERA_OMNIDIRECTIONAL")
      {
        this->AnariExtensions.ANARI_KHR_CAMERA_OMNIDIRECTIONAL = 1;
      }
      else if (feature == "ANARI_KHR_CAMERA_ORTHOGRAPHIC")
      {
        this->AnariExtensions.ANARI_KHR_CAMERA_ORTHOGRAPHIC = 1;
      }
      else if (feature == "ANARI_KHR_CAMERA_PERSPECTIVE")
      {
        this->AnariExtensions.ANARI_KHR_CAMERA_PERSPECTIVE = 1;
      }
      else if (feature == "ANARI_KHR_CAMERA_STEREO")
      {
        this->AnariExtensions.ANARI_KHR_CAMERA_STEREO = 1;
      }
      else if (feature == "ANARI_KHR_GEOMETRY_CONE")
      {
        this->AnariExtensions.ANARI_KHR_GEOMETRY_CONE = 1;
      }
      else if (feature == "ANARI_KHR_GEOMETRY_CURVE")
      {
        this->AnariExtensions.ANARI_KHR_GEOMETRY_CURVE = 1;
      }
      else if (feature == "ANARI_KHR_GEOMETRY_CYLINDER")
      {
        this->AnariExtensions.ANARI_KHR_GEOMETRY_CYLINDER = 1;
      }
      else if (feature == "ANARI_KHR_GEOMETRY_QUAD")
      {
        this->AnariExtensions.ANARI_KHR_GEOMETRY_QUAD = 1;
      }
      else if (feature == "ANARI_KHR_GEOMETRY_QUAD_MOTION_DEFORMATION")
      {
        this->AnariExtensions.ANARI_KHR_GEOMETRY_QUAD_MOTION_DEFORMATION = 1;
      }
      else if (feature == "ANARI_KHR_GEOMETRY_SPHERE")
      {
        this->AnariExtensions.ANARI_KHR_GEOMETRY_SPHERE = 1;
      }
      else if (feature == "ANARI_KHR_GEOMETRY_TRIANGLE")
      {
        this->AnariExtensions.ANARI_KHR_GEOMETRY_TRIANGLE = 1;
      }
      else if (feature == "ANARI_KHR_GEOMETRY_TRIANGLE_MOTION_DEFORMATION")
      {
        this->AnariExtensions.ANARI_KHR_GEOMETRY_TRIANGLE_MOTION_DEFORMATION = 1;
      }
      else if (feature == "ANARI_KHR_LIGHT_DIRECTIONAL")
      {
        this->AnariExtensions.ANARI_KHR_LIGHT_DIRECTIONAL = 1;
      }
      else if (feature == "ANARI_KHR_LIGHT_POINT")
      {
        this->AnariExtensions.ANARI_KHR_LIGHT_POINT = 1;
      }
      else if (feature == "ANARI_KHR_LIGHT_SPOT")
      {
        this->AnariExtensions.ANARI_KHR_LIGHT_SPOT = 1;
      }
      else if (feature == "ANARI_KHR_MATERIAL_MATTE")
      {
        this->AnariExtensions.ANARI_KHR_MATERIAL_MATTE = 1;
      }
      else if (feature == "ANARI_KHR_MATERIAL_PHYSICALLY_BASED")
      {
        this->AnariExtensions.ANARI_KHR_MATERIAL_PHYSICALLY_BASED = 1;
      }
      else if (feature == "ANARI_KHR_SAMPLER_IMAGE1D")
      {
        this->AnariExtensions.ANARI_KHR_SAMPLER_IMAGE1D = 1;
      }
      else if (feature == "ANARI_KHR_SAMPLER_IMAGE2D")
      {
        this->AnariExtensions.ANARI_KHR_SAMPLER_IMAGE2D = 1;
      }
      else if (feature == "ANARI_KHR_SAMPLER_IMAGE3D")
      {
        this->AnariExtensions.ANARI_KHR_SAMPLER_IMAGE3D = 1;
      }
      else if (feature == "ANARI_KHR_SAMPLER_PRIMITIVE")
      {
        this->AnariExtensions.ANARI_KHR_SAMPLER_PRIMITIVE = 1;
      }
      else if (feature == "ANARI_KHR_SAMPLER_TRANSFORM")
      {
        this->AnariExtensions.ANARI_KHR_SAMPLER_TRANSFORM = 1;
      }
      else if (feature == "ANARI_KHR_SPATIAL_FIELD_STRUCTURED_REGULAR")
      {
        this->AnariExtensions.ANARI_KHR_SPATIAL_FIELD_STRUCTURED_REGULAR = 1;
      }
      else if (feature == "ANARI_KHR_VOLUME_TRANSFER_FUNCTION1D" ||
        feature == "ANARI_KHR_VOLUME_SCIVIS")
      {
        this->AnariExtensions.ANARI_KHR_VOLUME_TRANSFER_FUNCTION1D = 1;
      }
      else if (feature == "ANARI_KHR_LIGHT_RING")
      {
        this->AnariExtensions.ANARI_KHR_LIGHT_RING = 1;
      }
      else if (feature == "ANARI_KHR_LIGHT_QUAD")
      {
        this->AnariExtensions.ANARI_KHR_LIGHT_QUAD = 1;
      }
      else if (feature == "ANARI_KHR_LIGHT_HDRI")
      {
        this->AnariExtensions.ANARI_KHR_LIGHT_HDRI = 1;
      }
      else if (feature == "ANARI_KHR_CAMERA_SHUTTER")
      {
        this->AnariExtensions.ANARI_KHR_CAMERA_SHUTTER = 1;
      }
      else if (feature == "ANARI_KHR_INSTANCE_MOTION_SCALE_ROTATION_TRANSLATION")
      {
        this->AnariExtensions.ANARI_KHR_INSTANCE_MOTION_SCALE_ROTATION_TRANSLATION = 1;
      }
      else if (feature == "ANARI_KHR_AREA_LIGHTS")
      {
        this->AnariExtensions.ANARI_KHR_AREA_LIGHTS = 1;
      }
      else if (feature == "ANARI_KHR_INSTANCE_MOTION_TRANSFORM")
      {
        this->AnariExtensions.ANARI_KHR_INSTANCE_MOTION_TRANSFORM = 1;
      }
      else if (feature == "ANARI_KHR_CAMERA_DEPTH_OF_FIELD")
      {
        this->AnariExtensions.ANARI_KHR_CAMERA_DEPTH_OF_FIELD = 1;
      }
      else if (feature == "ANARI_KHR_ARRAY1D_REGION")
      {
        this->AnariExtensions.ANARI_KHR_ARRAY1D_REGION = 1;
      }
      else if (feature == "ANARI_KHR_RENDERER_AMBIENT_LIGHT")
      {
        this->AnariExtensions.ANARI_KHR_RENDERER_AMBIENT_LIGHT = 1;
      }
      else if (feature == "ANARI_KHR_RENDERER_BACKGROUND_COLOR")
      {
        this->AnariExtensions.ANARI_KHR_RENDERER_BACKGROUND_COLOR = 1;
      }
      else if (feature == "ANARI_KHR_RENDERER_BACKGROUND_IMAGE")
      {
        this->AnariExtensions.ANARI_KHR_RENDERER_BACKGROUND_IMAGE = 1;
      }
      else if (feature == "ANARI_EXP_VOLUME_SAMPLE_RATE")
      {
        this->AnariExtensions.ANARI_EXP_VOLUME_SAMPLE_RATE = 1;
      }
      else if (feature == "ANARI_KHR_CAMERA_MOTION_TRANSFORMATION")
      {
        this->AnariExtensions.ANARI_KHR_CAMERA_MOTION_TRANSFORMATION = 1;
      }
      else if (feature == "ANARI_KHR_DEVICE_SYNCHRONIZATION")
      {
        this->AnariExtensions.ANARI_KHR_DEVICE_SYNCHRONIZATION = 1;
      }
      else if (feature == "ANARI_KHR_FRAME_CHANNEL_ALBEDO")
      {
        this->AnariExtensions.ANARI_KHR_FRAME_CHANNEL_ALBEDO = 1;
      }
      else if (feature == "ANARI_KHR_FRAME_CHANNEL_INSTANCE_ID")
      {
        this->AnariExtensions.ANARI_KHR_FRAME_CHANNEL_INSTANCE_ID = 1;
      }
      else if (feature == "ANARI_KHR_FRAME_CHANNEL_NORMAL")
      {
        this->AnariExtensions.ANARI_KHR_FRAME_CHANNEL_NORMAL = 1;
      }
      else if (feature == "ANARI_KHR_FRAME_CHANNEL_OBJECT_ID")
      {
        this->AnariExtensions.ANARI_KHR_FRAME_CHANNEL_OBJECT_ID = 1;
      }
      else if (feature == "ANARI_KHR_FRAME_CHANNEL_PRIMITIVE_ID")
      {
        this->AnariExtensions.ANARI_KHR_FRAME_CHANNEL_PRIMITIVE_ID = 1;
      }
      else if (feature == "ANARI_KHR_FRAME_COMPLETION_CALLBACK")
      {
        this->AnariExtensions.ANARI_KHR_FRAME_COMPLETION_CALLBACK = 1;
      }
    }
  }

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
}

//----------------------------------------------------------------------------
vtkAnariRendererNode::~vtkAnariRendererNode()
{
  delete this->Internal;
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
    "environment";
  }

  vtkInformation* info = renderer->GetInformation();

  if (info && info->Has(vtkAnariRendererNode::LIBRARY_NAME()))
  {
    return info->Get(vtkAnariRendererNode::LIBRARY_NAME());
  }

  return "environment";
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
    "debug";
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
void vtkAnariRendererNode::AddCamera(anari::Camera camera, const bool changed)
{
  this->Internal->AddCamera(camera, changed);
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::AddLight(anari::Light light, const bool changed)
{
  this->Internal->AddLight(light, changed);
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::AddSurfaces(
  const std::vector<anari::Surface>& surfaces, const bool changed)
{
  this->Internal->AddSurfaces(surfaces, changed);
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::AddVolume(anari::Volume volume, const bool changed)
{
  this->Internal->AddVolume(volume, changed);
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  anari::Library anariLibrary = this->Internal->AnariLibrary;

  if (anariLibrary != nullptr)
  {
    const char* libName = vtkAnariRendererNode::GetLibraryName(this->GetRenderer());

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

//----------------------------------------------------------------------------
void vtkAnariRendererNode::Traverse(int operation)
{

  vtkRenderer* renderer = vtkRenderer::SafeDownCast(this->GetRenderable());
  if (!renderer)
  {
    return;
  }

  // do not override other passes
  if (operation != vtkViewNode::operation_type::render)
  {
    this->Superclass::Traverse(operation);
    return;
  }

  if (!this->Internal->InitFlag)
  {
    this->Internal->InitFlag = this->Internal->InitAnari();
  }

  if (this->Internal->InitFlag)
  {
    this->Apply(operation, true);

    auto const& nodes = this->GetChildren();

    // ANARI camera
    for (auto node : nodes)
    {
      vtkAnariCameraNode* child = vtkAnariCameraNode::SafeDownCast(node);
      if (child)
      {
        child->Traverse(operation);
        break;
      }
    }

    // Lights
    for (auto node : nodes)
    {
      vtkAnariLightNode* child = vtkAnariLightNode::SafeDownCast(node);
      if (child)
      {
        child->Traverse(operation);
      }
    }

    // Surfaces
    for (auto node : nodes)
    {
      vtkAnariActorNode* child = vtkAnariActorNode::SafeDownCast(node);
      if (child)
      {
        child->Traverse(operation);
      }
    }

    // Volumes
    for (auto node : nodes)
    {
      vtkAnariVolumeNode* child = vtkAnariVolumeNode::SafeDownCast(node);
      if (child)
      {
        child->Traverse(operation);
      }
    }

    this->Apply(operation, false);
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
  auto anariDeviceExtensions = this->GetAnariDeviceExtensions();

  if (!ren)
  {
    return;
  }

  if (prepass)
  {
    auto anariDevice = this->GetAnariDevice();

    if (anariDevice == nullptr)
    {
      return;
    }

    // Frame
    // The frame contains all the objects necessary to render and holds the
    // resulting rendered 2D image.
    //----------------------------------------------------------------------------
    if (this->Internal->AnariFrame == nullptr)
    {
      this->Internal->AnariFrame = anari::newObject<anari::Frame>(anariDevice);

      ANARIDataType format = ANARI_UFIXED8_VEC4;
      anari::setParameter(
        anariDevice, this->Internal->AnariFrame, "channel.color", ANARI_DATA_TYPE, &format);
      ANARIDataType depthFormat = ANARI_FLOAT32;
      anari::setParameter(
        anariDevice, this->Internal->AnariFrame, "channel.depth", ANARI_DATA_TYPE, &depthFormat);

      anari::commitParameters(anariDevice, this->Internal->AnariFrame);
    }

    auto anariFrame = this->Internal->AnariFrame;

    // Renderer
    //----------------------------------------------------------------------------
    auto currentRendererSubtype = this->Internal->RendererParams.Subtype;
    this->Internal->CompositeOnGL = (this->GetCompositeOnGL(ren) != 0);
    const char* rendererSubtype = vtkAnariRendererNode::GetRendererSubtype(this->GetRenderer());

    if (currentRendererSubtype != rendererSubtype)
    {
      this->Internal->RendererParams.Subtype = rendererSubtype;

      if (this->Internal->AnariRenderer != nullptr)
      {
        anari::release(anariDevice, this->Internal->AnariRenderer);
      }

      this->Internal->AnariRenderer =
        anari::newObject<anari::Renderer>(anariDevice, rendererSubtype);

      anari::setParameter(anariDevice, anariFrame, "renderer", this->Internal->AnariRenderer);
      anari::commitParameters(anariDevice, anariFrame);
    }

    auto anariRenderer = this->Internal->AnariRenderer;

    // TODO: have this as a renderer parameter
    // bool useAccumulation = this->GetUseAccumulation(ren) > 0 ? true : false;
    // if(anariDeviceExtensions.ANARI_KHR_FRAME_ACCUMULATION &&
    //    this->Internal->RendererParams.Accumulation != useAccumulation)
    // {
    //   this->Internal->RendererParams.Accumulation = useAccumulation;
    //   anari::setParameter(anariDevice, this->Internal->AnariFrame, "accumulation",
    //   useAccumulation); anari::commitParameters(anariDevice, anariRenderer);
    // }

    bool useDenoiser = this->GetUseDenoiser(ren) > 0 ? true : false;
    if (this->Internal->RendererParams.Denoise != useDenoiser)
    {
      anari::setParameter(anariDevice, anariRenderer, "denoise", useDenoiser);
      this->Internal->RendererParams.Denoise = useDenoiser;
      anari::commitParameters(anariDevice, anariRenderer);
    }

    auto spp = this->GetSamplesPerPixel(ren);
    if (this->Internal->RendererParams.SamplesPerPixel != spp)
    {
      anari::setParameter(anariDevice, anariRenderer, "pixelSamples", spp);
      this->Internal->RendererParams.SamplesPerPixel = spp;
      anari::commitParameters(anariDevice, anariRenderer);
    }

    auto aoSamples = this->GetAmbientSamples(ren);
    if (this->Internal->RendererParams.AmbientSamples != aoSamples)
    {
      anari::setParameter(anariDevice, anariRenderer, "ambientSamples", aoSamples);
      this->Internal->RendererParams.AmbientSamples = aoSamples;
      anari::commitParameters(anariDevice, anariRenderer);
    }

    float lightFalloff = static_cast<float>(this->GetLightFalloff(ren));
    if (std::abs(this->Internal->RendererParams.LightFalloff - lightFalloff) > 0.0001f)
    {
      anari::setParameter(anariDevice, anariRenderer, "lightFalloff", lightFalloff);
      this->Internal->RendererParams.LightFalloff = lightFalloff;
      anari::commitParameters(anariDevice, anariRenderer);
    }

    float ambientIntensity = static_cast<float>(this->GetAmbientIntensity(ren));
    if (std::abs(this->Internal->RendererParams.AmbientIntensity - ambientIntensity) > 0.0001f)
    {
      anari::setParameter(anariDevice, anariRenderer, "ambientRadiance", ambientIntensity);
      this->Internal->RendererParams.AmbientIntensity = ambientIntensity;
      anari::commitParameters(anariDevice, anariRenderer);
    }

    auto maxDepth = this->GetMaxDepth(ren);
    if (this->Internal->RendererParams.MaxDepth != maxDepth)
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
  else
  {
    auto anariDevice = this->GetAnariDevice();
    auto anariFrame = this->Internal->AnariFrame;

    if (anariDevice == nullptr || anariFrame == nullptr)
    {
      return;
    }

    // World
    // The world to be populated with renderable objects
    //----------------------------------------------------------------------------
    bool isNewWorld = false;

    if (this->Internal->AnariWorld == nullptr)
    {
      this->Internal->AnariWorld = anari::newObject<anari::World>(anariDevice);
      anari::setParameter(
        anariDevice, this->Internal->AnariWorld, "name", ANARI_STRING, "vtk_world");
      isNewWorld = true;
      anari::commitParameters(anariDevice, this->Internal->AnariWorld);

      anari::setParameter(anariDevice, anariFrame, "world", this->Internal->AnariWorld);
      anari::commitParameters(anariDevice, anariFrame);
    }

    auto anariWorld = this->Internal->AnariWorld;

    uvec2 frameSize = { static_cast<uint>(this->Size[0]), static_cast<uint>(this->Size[1]) };
    int totalSize = this->Size[0] * this->Size[1];

    if (this->Internal->ImageX != frameSize[0] || this->Internal->ImageY != frameSize[1])
    {
      this->Internal->ImageX = frameSize[0];
      this->Internal->ImageY = frameSize[1];

      this->Internal->ColorBuffer.reset(new u_char[totalSize * 4]);
      this->Internal->DepthBuffer.reset(new float[totalSize]);

      anari::setParameter(anariDevice, anariFrame, "size", frameSize);
      anari::commitParameters(anariDevice, anariFrame);
    }

    // Geometry
    //----------------------------------------------------------------------------
    auto surfaceState = this->Internal->GetSurfaceState();
    auto volumeState = this->Internal->GetVolumeState();

    if (surfaceState.changed || volumeState.changed)
    {
      if (this->Internal->AnariInstance == nullptr)
      {
        this->Internal->AnariInstance = anari::newObject<anari::Instance>(anariDevice, "transform");
        anari::setParameter(
          anariDevice, this->Internal->AnariInstance, "name", ANARI_STRING, "vtk_instance");
        anari::commitParameters(anariDevice, this->Internal->AnariInstance);

        auto instanceArray1D = anari::newArray1D(anariDevice, &this->Internal->AnariInstance, 1);
        anari::setAndReleaseParameter(anariDevice, anariWorld, "instance", instanceArray1D);
        anari::commitParameters(anariDevice, anariWorld);
      }

      auto anariInstance = this->Internal->AnariInstance;

      if (this->Internal->AnariGroup == nullptr)
      {
        this->Internal->AnariGroup = anari::newObject<anari::Group>(anariDevice);
        anari::setParameter(
          anariDevice, this->Internal->AnariGroup, "name", ANARI_STRING, "vtk_group");
        anari::commitParameters(anariDevice, this->Internal->AnariGroup);

        anari::setParameter(anariDevice, anariInstance, "group", this->Internal->AnariGroup);
        anari::commitParameters(anariDevice, anariInstance);
      }

      auto anariGroup = this->Internal->AnariGroup;

      if (surfaceState.changed)
      {
        this->Internal->AnariSurfaceState.changed = false;

        if (!surfaceState.Surfaces.empty())
        {
          for (size_t i = 0; i < surfaceState.Surfaces.size(); i++)
          {
            std::string surfaceName("vtk_surface_");
            surfaceName.append(std::to_string(i));

            anari::setParameter(
              anariDevice, surfaceState.Surfaces[i], "name", ANARI_STRING, surfaceName.c_str());
            anari::commitParameters(anariDevice, surfaceState.Surfaces[i]);
          }

          auto surfaceArray1D = anari::newArray1D(
            anariDevice, surfaceState.Surfaces.data(), surfaceState.Surfaces.size());
          anari::setAndReleaseParameter(anariDevice, anariGroup, "surface", surfaceArray1D);
          anari::commitParameters(anariDevice, anariGroup);
        }
        else
        {
          anari::unsetParameter(anariDevice, anariGroup, "surface");
          anari::commitParameters(anariDevice, anariGroup);
        }
      }

      if (volumeState.changed)
      {
        this->Internal->AnariVolumeState.changed = false;

        if (!volumeState.Volumes.empty())
        {
          for (size_t i = 0; i < volumeState.Volumes.size(); i++)
          {
            std::string volumeName("vtk_volume_");
            volumeName.append(std::to_string(i));

            anari::setParameter(
              anariDevice, volumeState.Volumes[i], "name", ANARI_STRING, volumeName.c_str());
            anari::commitParameters(anariDevice, volumeState.Volumes[i]);
          }

          auto volumeArray1D =
            anari::newArray1D(anariDevice, volumeState.Volumes.data(), volumeState.Volumes.size());
          anari::setAndReleaseParameter(anariDevice, anariGroup, "volume", volumeArray1D);
          anari::commitParameters(anariDevice, anariGroup);
        }
        else
        {
          anari::unsetParameter(anariDevice, anariGroup, "volume");
          anari::commitParameters(anariDevice, anariGroup);
        }
      }
    }
    else if (isNewWorld) // TODO: Should just be able to render background color??
    {
      memset(this->Internal->ColorBuffer.get(), 255, totalSize * 4);
      memset(this->Internal->DepthBuffer.get(), 1, totalSize * sizeof(float));
      return;
    }

    this->Internal->AnariSurfaceState.used = true;
    this->Internal->AnariVolumeState.used = true;

    // Lights
    //----------------------------------------------------------------------------
    auto lightState = this->Internal->GetLightState();

    if (lightState.changed)
    {
      this->Internal->AnariLightState.changed = false;

      if (!lightState.Lights.empty())
      {
        for (size_t i = 0; i < lightState.Lights.size(); i++)
        {
          std::string lightName("vtk_light_");
          lightName.append(std::to_string(i));

          anari::setParameter(
            anariDevice, lightState.Lights[i], "name", ANARI_STRING, lightName.c_str());
          anari::commitParameters(anariDevice, lightState.Lights[i]);
        }

        auto lightArray1D =
          anari::newArray1D(anariDevice, lightState.Lights.data(), lightState.Lights.size());
        anari::setAndReleaseParameter(anariDevice, anariWorld, "light", lightArray1D);
        anari::commitParameters(anariDevice, anariWorld);
      }
      else
      {
        vtkWarningMacro(<< "No lights set on world.");
        anari::unsetParameter(anariDevice, anariWorld, "light");
        anari::commitParameters(anariDevice, anariWorld);
      }
    }

    this->Internal->AnariLightState.used = true;

    // Camera
    //----------------------------------------------------------------------------
    auto cameraState = this->Internal->GetCameraState();

    if (cameraState.changed)
    {
      this->Internal->AnariCameraState.changed = false;

      if (cameraState.Camera != nullptr)
      {
        anari::setAndReleaseParameter(anariDevice, anariFrame, "camera", cameraState.Camera);
        anari::commitParameters(anariDevice, anariFrame);
      }
      else
      {
        anari::unsetParameter(anariDevice, anariFrame, "camera");
        anari::commitParameters(anariDevice, anariFrame);
      }
    }

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
      vtkWarningMacro(<< "[ANARI::" << this->Internal->LibraryName
                      << "] World bounds not returned");
    }

    // Render frame
    int accumulationCount = this->GetAccumulationCount(ren);
    for (int i = 0; i < accumulationCount; i++)
    {
      anari::render(anariDevice, anariFrame);
      anari::wait(anariDevice, anariFrame);
    }

    if (!this->Internal->IsUSD)
    {
      float duration = 0.0f;
      anari::getProperty(anariDevice, anariFrame, "duration", duration, ANARI_NO_WAIT);

      float durationInMS = duration * 1000.0f;
      vtkDebugMacro(<< "Rendered frame in " << durationInMS << " ms");

      // Color buffer
      auto renderedFrame = anari::map<uint32_t>(anariDevice, anariFrame, "channel.color");

      if (renderedFrame.data != nullptr)
      {
        int retTotalSize = renderedFrame.width * renderedFrame.height;
        totalSize = (retTotalSize < totalSize) ? retTotalSize : totalSize;
        memcpy(this->Internal->ColorBuffer.get(), renderedFrame.data, totalSize * 4);
      }
      else
      {
        vtkWarningMacro(<< "Color buffer is null");
        memset(this->Internal->ColorBuffer.get(), 255, totalSize * 4);
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

        const float* depthBuffer = mappedDepthBuffer.data;  // s
        float* zBuffer = this->Internal->DepthBuffer.get(); // d

        for (int i = 0; i < totalSize; i++)
        {
          // *d = (*s < clipMin ? 1.0 : (*s - clipMin) * clipDiv);
          zBuffer[i] = (depthBuffer[i] < clipMin ? 1.0f : (depthBuffer[i] - clipMin) * clipDiv);
        }
      }
      else
      {
        vtkWarningMacro(<< "Depth buffer is null");
        memset(this->Internal->DepthBuffer.get(), 0, totalSize * sizeof(float));
      }

      anari::unmap(anariDevice, anariFrame, "channel.depth");
    }
    else
    {
      memset(this->Internal->ColorBuffer.get(), 255, totalSize * 4);
      memset(this->Internal->DepthBuffer.get(), 1, totalSize * sizeof(float));
    }
  }
}

//----------------------------------------------------------------------------
void vtkAnariRendererNode::WriteLayer(
  unsigned char* buffer, float* Z, int buffx, int buffy, int layer)
{
  vtkAnariProfiling startProfiling("vtkAnariRendererNode::WriteLayer", vtkAnariProfiling::BLUE);
  unsigned char* colorBuffer = this->Internal->ColorBuffer.get();
  float* zBuffer = this->Internal->DepthBuffer.get();

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
  return this->Internal->ColorBuffer.get();
}

//------------------------------------------------------------------------------
const float* vtkAnariRendererNode::GetZBuffer()
{
  return this->Internal->DepthBuffer.get();
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

VTK_ABI_NAMESPACE_END
