// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAnariDeviceManager.h"
#include "vtkAnariProfiling.h"

#include "vtkLogger.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"

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
class vtkAnariDeviceManagerInternals : public vtkObject
{
public:
  static vtkAnariDeviceManagerInternals* New();
  vtkTypeMacro(vtkAnariDeviceManagerInternals, vtkObject);

  vtkAnariDeviceManagerInternals() = default;
  ~vtkAnariDeviceManagerInternals() override = default;

  bool IsInitialized() const;
  bool InitAnari(bool useDebugDevice = false, const char* libraryName = "environment",
    const char* deviceName = "default");
  void CleanupAnariObjects();

  std::string AnariLibraryName;
  std::string AnariDeviceName;
  std::string AnariDebugTraceDir;
  std::string AnariDebugTraceMode;
  bool AnariDebugDeviceEnabled{ false };
  anari::Library AnariLibrary{ nullptr };
  anari::Device AnariDevice{ nullptr };
  anari::Extensions AnariExtensions{};

  vtkAnariDeviceManager::OnNewDeviceCallback NewDeviceCB;
};

// ----------------------------------------------------------------------------
bool vtkAnariDeviceManagerInternals::IsInitialized() const
{
  return this->AnariDevice != nullptr;
}

// ----------------------------------------------------------------------------
bool vtkAnariDeviceManagerInternals::InitAnari(
  bool useDebugDevice, const char* libraryName, const char* deviceName)
{
  vtkAnariProfiling startProfiling(
    "vtkAnariDeviceManagerInternals::InitAnari", vtkAnariProfiling::YELLOW);

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
void vtkAnariDeviceManagerInternals::CleanupAnariObjects()
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
vtkStandardNewMacro(vtkAnariDeviceManagerInternals);

//============================================================================

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAnariDeviceManager);

//----------------------------------------------------------------------------
void vtkAnariDeviceManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
void vtkAnariDeviceManager::SetAnariDebugConfig(const char* traceDir, const char* traceMode)
{
  this->Internal->AnariDebugTraceDir = traceDir;
  this->Internal->AnariDebugTraceMode = traceMode;
}

// ----------------------------------------------------------------------------
bool vtkAnariDeviceManager::SetupAnariDeviceFromLibrary(
  const char* libraryName, const char* deviceName, bool enableDebugDevice)
{
  return this->Internal->InitAnari(enableDebugDevice, libraryName, deviceName);
}

// ----------------------------------------------------------------------------
bool vtkAnariDeviceManager::AnariInitialized() const
{
  return GetAnariDevice() != nullptr;
}

// ----------------------------------------------------------------------------
anari::Device vtkAnariDeviceManager::GetAnariDevice() const
{
  return this->Internal->AnariDevice;
}

// ----------------------------------------------------------------------------
const anari::Extensions& vtkAnariDeviceManager::GetAnariDeviceExtensions() const
{
  return this->Internal->AnariExtensions;
}

// ----------------------------------------------------------------------------
void vtkAnariDeviceManager::SetOnNewDeviceCallback(OnNewDeviceCallback&& cb)
{
  this->Internal->NewDeviceCB = std::move(cb);
}

// ----------------------------------------------------------------------------
vtkAnariDeviceManager::vtkAnariDeviceManager()
{
  this->Internal = vtkAnariDeviceManagerInternals::New();
}

// ----------------------------------------------------------------------------
vtkAnariDeviceManager::~vtkAnariDeviceManager()
{
  this->Internal->Delete();
  this->Internal = nullptr;
}

VTK_ABI_NAMESPACE_END
