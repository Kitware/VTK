// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAnariDevice.h"
#include "vtkAnariProfiling.h"

#include "vtkLogger.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"

#include <memory>
#include <sstream>

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
class vtkAnariDeviceInternals : public vtkObject
{
public:
  static vtkAnariDeviceInternals* New();
  vtkTypeMacro(vtkAnariDeviceInternals, vtkObject);

  vtkAnariDeviceInternals() = default;
  ~vtkAnariDeviceInternals() override = default;

  bool IsInitialized() const;
  bool InitAnari(bool useDebugDevice = false, const char* libraryName = "environment",
    const char* deviceName = "default");
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
bool vtkAnariDeviceInternals::IsInitialized() const
{
  return this->AnariDevice != nullptr;
}

// ----------------------------------------------------------------------------
bool vtkAnariDeviceInternals::InitAnari(
  bool useDebugDevice, const char* libraryName, const char* deviceName)
{
  vtkAnariProfiling startProfiling("vtkAnariDeviceInternals::InitAnari", vtkAnariProfiling::YELLOW);

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

  if (this->NewDeviceCB)
  {
    this->NewDeviceCB(this->AnariDevice);
  }

  return true;
}

// ----------------------------------------------------------------------------
void vtkAnariDeviceInternals::CleanupAnariObjects()
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
vtkStandardNewMacro(vtkAnariDeviceInternals);

//============================================================================

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
  return this->Internal->InitAnari(enableDebugDevice, libraryName, deviceName);
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
const anari::Extensions& vtkAnariDevice::GetAnariDeviceExtensions() const
{
  return this->Internal->AnariExtensions;
}

// ----------------------------------------------------------------------------
const char* const* vtkAnariDevice::GetAnariDeviceExtensionStrings() const
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
  this->Internal = vtkAnariDeviceInternals::New();
}

// ----------------------------------------------------------------------------
vtkAnariDevice::~vtkAnariDevice()
{
  this->Internal->Delete();
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

VTK_ABI_NAMESPACE_END
