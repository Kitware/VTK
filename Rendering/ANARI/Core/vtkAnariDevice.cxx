// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// As anari/anari_cpp.hpp already includes anari_extension_utility.h internally, this needs to be
// included before vtkAnariDevice.h, otherwise #pragma once will discard the inclusion.
#define ANARI_EXTENSION_UTILITY_IMPL
#include <anari/frontend/anari_extension_utility.h>

#include "vtkAnariDevice.h"

#include "vtkAnariProfiling.h"

#include "vtkLogger.h"
#include "vtkObjectFactory.h"

#include <anari/anari_cpp/ext/std.h>

VTK_ABI_NAMESPACE_BEGIN

using namespace anari::std_types;
using vec2_d = std::array<double, 2>;
using vec3_d = std::array<double, 3>;
using vec4_d = std::array<double, 4>;

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
class vtkAnariDeviceInternals
{
public:
  vtkAnariDeviceInternals() = default;
  ~vtkAnariDeviceInternals();

  bool IsInitialized() const;
  bool InitAnari(const char* libraryName = "environment", const char* deviceName = "default",
    bool useDebugDevice = false);
  void CleanupAnariObjects();

  template <typename T>
  void SetDeviceParameter(const char* p, const T& v);
  void CommitDeviceParameters();

  std::string AnariLibraryName;
  std::string AnariDeviceName;
  std::string AnariDebugTraceDir;
  std::string AnariDebugTraceMode;
  bool AnariDebugDeviceEnabled{ false };
  anari::Library AnariLibrary{ nullptr };
  anari::Device AnariDevice{ nullptr };
  anari::Extensions AnariExtensions{};

  vtkAnariDevice::OnNewDeviceCallback NewDeviceCB;
};

// ----------------------------------------------------------------------------
vtkAnariDeviceInternals::~vtkAnariDeviceInternals()
{
  this->CleanupAnariObjects();
}

// ----------------------------------------------------------------------------
bool vtkAnariDeviceInternals::IsInitialized() const
{
  return this->AnariDevice != nullptr;
}

// ----------------------------------------------------------------------------
bool vtkAnariDeviceInternals::InitAnari(
  const char* libraryName, const char* deviceName, bool useDebugDevice)
{
  vtkAnariProfiling startProfiling("vtkAnariDeviceInternals::InitAnari", vtkAnariProfiling::YELLOW);

  const bool configIsTheSame = IsInitialized() && libraryName == this->AnariLibraryName &&
    deviceName == this->AnariDeviceName && useDebugDevice == this->AnariDebugDeviceEnabled;
  if (configIsTheSame)
  {
    return true;
  }

  this->CleanupAnariObjects();

  vtkLogF(TRACE, "VTK Anari Library name: %s", libraryName != nullptr ? libraryName : "nullptr");
  vtkLogF(TRACE, "VTK Anari Device type: %s", deviceName);

  this->AnariLibrary = anari::loadLibrary(libraryName, AnariStatusCallback);

  if (!this->AnariLibrary)
  {
    this->CleanupAnariObjects();
    vtkLogF(ERROR,
      "[ANARI::%s] Could not load %s library. Make sure to set ANARI_LIBRARY and expose it to your "
      "LIBRARY_PATH.",
      libraryName, libraryName);
    return false;
  }

  this->AnariDevice = anari::newDevice(this->AnariLibrary, deviceName);
  if (!this->AnariDevice)
  {
    this->CleanupAnariObjects();
    vtkLogF(ERROR, "[ANARI::%s] Could not load %s device", libraryName, deviceName);
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
      vtkLogF(ERROR, "[ANARI::%s] Could not load debug library.", libraryName);
      return false;
    }

    debugDevice = anari::newDevice(debugLibrary, "default");
    if (!debugDevice)
    {
      this->CleanupAnariObjects();
      vtkLogF(ERROR, "[ANARI::%s] Could not load debug device.", libraryName);
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
    vtkLogF(TRACE, "[%s:%s] Feature => %s", libraryName, deviceName, *i);
  }

  anariGetDeviceExtensionStruct(&this->AnariExtensions, this->AnariLibrary, deviceName);

  if ((this->AnariExtensions.ANARI_KHR_GEOMETRY_CYLINDER ||
        this->AnariExtensions.ANARI_KHR_GEOMETRY_CURVE) &&
    this->AnariExtensions.ANARI_KHR_GEOMETRY_SPHERE &&
    this->AnariExtensions.ANARI_KHR_GEOMETRY_TRIANGLE &&
    this->AnariExtensions.ANARI_KHR_INSTANCE_TRANSFORM)
  {
    vtkLogF(TRACE, "[ANARI::%s] Loaded %s device.", libraryName, deviceName);
  }
  else
  {
    vtkLogF(TRACE, "[ANARI::%s] Loaded %s device doesn't have the minimum required feature.",
      libraryName, deviceName);
  }

  this->AnariLibraryName = libraryName;
  this->AnariDeviceName = deviceName;
  this->AnariDebugDeviceEnabled = useDebugDevice;

  if (this->NewDeviceCB)
  {
    this->NewDeviceCB();
  }

  return true;
}

// ----------------------------------------------------------------------------
void vtkAnariDeviceInternals::CleanupAnariObjects()
{
  if (this->AnariDevice)
  {
    anari::release(this->AnariDevice, this->AnariDevice);
  }

  if (this->AnariLibrary)
  {
    anari::unloadLibrary(this->AnariLibrary);
  }

  this->AnariLibraryName = "";
  this->AnariDeviceName = "";
  this->AnariDebugDeviceEnabled = false;
  this->AnariLibrary = nullptr;
  this->AnariDevice = nullptr;
  this->AnariExtensions = {};
}

//------------------------------------------------------------------------------
template <typename T>
void vtkAnariDeviceInternals::SetDeviceParameter(const char* p, const T& v)
{
  if (!this->AnariDevice)
  {
    return;
  }

  anari::setParameter(this->AnariDevice, this->AnariDevice, p, v);
}

//------------------------------------------------------------------------------
void vtkAnariDeviceInternals::CommitDeviceParameters()
{
  anari::commitParameters(this->AnariDevice, this->AnariDevice);
}

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAnariDevice);

//----------------------------------------------------------------------------
void vtkAnariDevice::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
void vtkAnariDevice::SetAnariDebugConfig(const char* traceDir, const char* traceMode)
{
  this->Internal->AnariDebugTraceDir = traceDir;
  this->Internal->AnariDebugTraceMode = traceMode;
}

// ----------------------------------------------------------------------------
bool vtkAnariDevice::SetupAnariDeviceFromLibrary(
  const char* libraryName, const char* deviceName, bool enableDebugDevice)
{
  return this->Internal->InitAnari(libraryName, deviceName, enableDebugDevice);
}

// ----------------------------------------------------------------------------
bool vtkAnariDevice::AnariInitialized() const
{
  return this->GetHandle() != nullptr;
}

// ----------------------------------------------------------------------------
anari::Device vtkAnariDevice::GetHandle() const
{
  return this->Internal->AnariDevice;
}

// ----------------------------------------------------------------------------
const anari::Extensions& vtkAnariDevice::GetExtensions() const
{
  return this->Internal->AnariExtensions;
}

// ----------------------------------------------------------------------------
const char* const* vtkAnariDevice::GetExtensionStrings() const
{
  return (const char* const*)anariGetDeviceExtensions(
    this->Internal->AnariLibrary, this->Internal->AnariDeviceName.c_str());
}

// ----------------------------------------------------------------------------
void vtkAnariDevice::SetOnNewDeviceCallback(OnNewDeviceCallback&& cb)
{
  this->Internal->NewDeviceCB = std::move(cb);
}

//----------------------------------------------------------------------------
void vtkAnariDevice::SetParameterc(const char* param, char* c)
{
  this->Internal->SetDeviceParameter(param, c);
}

//----------------------------------------------------------------------------
void vtkAnariDevice::SetParameterb(const char* param, bool b)
{
  this->Internal->SetDeviceParameter(param, b);
}

//----------------------------------------------------------------------------
void vtkAnariDevice::SetParameteri(const char* param, int x)
{
  this->Internal->SetDeviceParameter(param, x);
}

//----------------------------------------------------------------------------
void vtkAnariDevice::SetParameter2i(const char* param, int x, int y)
{
  this->Internal->SetDeviceParameter(param, ivec2{ x, y });
}

//----------------------------------------------------------------------------
void vtkAnariDevice::SetParameter3i(const char* param, int x, int y, int z)
{
  this->Internal->SetDeviceParameter(param, ivec3{ x, y, z });
}

//----------------------------------------------------------------------------
void vtkAnariDevice::SetParameter4i(const char* param, int x, int y, int z, int w)
{
  this->Internal->SetDeviceParameter(param, ivec4{ x, y, z, w });
}

//----------------------------------------------------------------------------
void vtkAnariDevice::SetParameterf(const char* param, float x)
{
  this->Internal->SetDeviceParameter(param, x);
}

//----------------------------------------------------------------------------
void vtkAnariDevice::SetParameter2f(const char* param, float x, float y)
{
  this->Internal->SetDeviceParameter(param, vec2{ x, y });
}

//----------------------------------------------------------------------------
void vtkAnariDevice::SetParameter3f(const char* param, float x, float y, float z)
{
  this->Internal->SetDeviceParameter(param, vec3{ x, y, z });
}

//----------------------------------------------------------------------------
void vtkAnariDevice::SetParameter4f(const char* param, float x, float y, float z, float w)
{
  this->Internal->SetDeviceParameter(param, vec4{ x, y, z, w });
}

//----------------------------------------------------------------------------
void vtkAnariDevice::SetParameterd(const char* param, double x)
{
  this->Internal->SetDeviceParameter(param, x);
}

//----------------------------------------------------------------------------
void vtkAnariDevice::CommitParameters()
{
  this->Internal->CommitDeviceParameters();
}

// ----------------------------------------------------------------------------
vtkAnariDevice::vtkAnariDevice()
{
  this->Internal = new vtkAnariDeviceInternals();
}

// ----------------------------------------------------------------------------
vtkAnariDevice::~vtkAnariDevice()
{
  delete this->Internal;
  this->Internal = nullptr;
}

// ----------------------------------------------------------------------------
std::string& vtkAnariDevice::GetAnariLibraryName() const
{
  return this->Internal->AnariLibraryName;
}

// ----------------------------------------------------------------------------
std::string& vtkAnariDevice::GetAnariDeviceName() const
{
  return this->Internal->AnariDeviceName;
}

// ----------------------------------------------------------------------------
std::vector<std::string> vtkAnariDevice::GetAnariRendererSubTypes() const
{
  std::vector<std::string> subtypes;
  if (auto d = this->GetHandle())
  {
    const char** rSubTypes = anariGetObjectSubtypes(d, ANARI_RENDERER);
    for (int i = 0; rSubTypes[i]; ++i)
    {
      subtypes.emplace_back(rSubTypes[i]);
    }
  }
  return subtypes;
}

VTK_ABI_NAMESPACE_END
