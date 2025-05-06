// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUConfiguration.h"
#include "Private/vtkWebGPUBufferInternals.h"
#include "Private/vtkWebGPUConfigurationInternals.h"
#include "Private/vtkWebGPUTextureInternals.h"
#include "vtkObjectFactory.h"
#include "vtkWebGPUHelpers.h"
#include "vtkWebGPURenderWindow.h"
#include "vtksys/SystemInformation.hxx"
#include "vtksys/SystemTools.hxx"

#include <sstream>
#include <webgpu/webgpu_cpp.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

VTK_ABI_NAMESPACE_BEGIN

namespace
{
// https://pcisig.com/membership/member-companies
const std::uint32_t AMD_PCI_VENDOR_ID = 0x1002;
const std::uint32_t APPLE_PCI_VENDOR_ID = 0x106b;
const std::uint32_t ARM_PCI_VENDOR_ID = 0x13b5;
const std::uint32_t BROADCOM_PCI_VENDOR_ID = 0x14e4; // Used on low power devices like Raspberry-Pi
const std::uint32_t INTEL_PCI_VENDOR_ID = 0x8086;
const std::uint32_t MESA_PCI_VENDOR_ID = 0x10005;
const std::uint32_t MICROSOFT_PCI_VENDOR_ID = 0x1414; // used in Microsoft WSL
const std::uint32_t NVIDIA_PCI_VENDOR_ID = 0x10de;
const std::uint32_t SAMSUNG_PCI_VENDOR_ID = 0x144d;

template <typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>& operator<<(
  std::basic_ostream<CharT, Traits>& o, wgpu::StringView value)
{
  o << std::string_view(value);
  return o;
}

ostream& operator<<(ostream& os, const vtkWebGPUConfiguration::BackendType& backend)
{
  switch (backend)
  {
    case vtkWebGPUConfiguration::BackendType::Null:
      os << "Null";
      break;
    case vtkWebGPUConfiguration::BackendType::WebGPU:
      os << "WebGPU";
      break;
    case vtkWebGPUConfiguration::BackendType::D3D11:
      os << "D3D11";
      break;
    case vtkWebGPUConfiguration::BackendType::D3D12:
      os << "D3D12";
      break;
    case vtkWebGPUConfiguration::BackendType::Metal:
      os << "Metal";
      break;
    case vtkWebGPUConfiguration::BackendType::Vulkan:
      os << "Vulkan";
      break;
    case vtkWebGPUConfiguration::BackendType::OpenGL:
      os << "OpenGL";
      break;
    case vtkWebGPUConfiguration::BackendType::OpenGLES:
      os << "OpenGLES";
      break;
    case vtkWebGPUConfiguration::BackendType::Undefined:
    default:
      os << "Undefined";
      break;
  }
  return os;
}

ostream& operator<<(ostream& os, const wgpu::BackendType& backend)
{
  switch (backend)
  {
    case wgpu::BackendType::Null:
      os << "Null";
      break;
    case wgpu::BackendType::WebGPU:
      os << "WebGPU";
      break;
    case wgpu::BackendType::D3D11:
      os << "D3D11";
      break;
    case wgpu::BackendType::D3D12:
      os << "D3D12";
      break;
    case wgpu::BackendType::Metal:
      os << "Metal";
      break;
    case wgpu::BackendType::Vulkan:
      os << "Vulkan";
      break;
    case wgpu::BackendType::OpenGL:
      os << "OpenGL";
      break;
    case wgpu::BackendType::OpenGLES:
      os << "OpenGLES";
      break;
    case wgpu::BackendType::Undefined:
    default:
      os << "Undefined";
      break;
  }
  return os;
}

ostream& operator<<(ostream& os, const vtkWebGPUConfiguration::PowerPreferenceType& power)
{
  switch (power)
  {
    case vtkWebGPUConfiguration::PowerPreferenceType::HighPerformance:
      os << "HighPerformance";
      break;
    case vtkWebGPUConfiguration::PowerPreferenceType::LowPower:
      os << "LowPower";
      break;
    case vtkWebGPUConfiguration::PowerPreferenceType::Undefined:
    default:
      os << "Undefined";
      break;
  }
  return os;
}

ostream& operator<<(ostream& os, const wgpu::AdapterType& type)
{
  switch (type)
  {
    case wgpu::AdapterType::DiscreteGPU:
      os << "discrete GPU";
      break;
    case wgpu::AdapterType::IntegratedGPU:
      os << "integrated GPU";
      break;
    case wgpu::AdapterType::CPU:
      os << "CPU";
      break;
    case wgpu::AdapterType::Unknown:
      os << "unknown";
      break;
  }
  return os;
}

std::string AsHex(uint32_t val)
{
  std::stringstream hex;
  hex << "0x" << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << val;
  return hex.str();
}

std::string FormatNumber(uint64_t num)
{
  auto s = std::to_string(num);
  std::stringstream ret;
  auto remainder = s.length() % 3;
  ret << s.substr(0, remainder);
  for (size_t i = remainder; i < s.length(); i += 3)
  {
    if (i > 0)
    {
      ret << ",";
    }
    ret << s.substr(i, 3);
  }
  return ret.str();
}

void PrintLimits(ostream& os, vtkIndent indent, const wgpu::Limits& limits)
{
  os << indent << "maxTextureDimension1D: " << FormatNumber(limits.maxTextureDimension1D) << '\n';
  os << indent << "maxTextureDimension2D: " << FormatNumber(limits.maxTextureDimension2D) << '\n';
  os << indent << "maxTextureDimension3D: " << FormatNumber(limits.maxTextureDimension3D) << '\n';
  os << indent << "maxTextureArrayLayers: " << FormatNumber(limits.maxTextureArrayLayers) << '\n';
  os << indent << "maxBindGroups: " << FormatNumber(limits.maxBindGroups) << '\n';
  os << indent
     << "maxBindGroupsPlusVertexBuffers: " << FormatNumber(limits.maxBindGroupsPlusVertexBuffers)
     << '\n';
  os << indent << "maxBindingsPerBindGroup: " << FormatNumber(limits.maxBindingsPerBindGroup)
     << '\n';
  os << indent << "maxDynamicUniformBuffersPerPipelineLayout: "
     << FormatNumber(limits.maxDynamicUniformBuffersPerPipelineLayout) << '\n';
  os << indent << "maxDynamicStorageBuffersPerPipelineLayout: "
     << FormatNumber(limits.maxDynamicStorageBuffersPerPipelineLayout) << '\n';
  os << indent << "maxSampledTexturesPerShaderStage: "
     << FormatNumber(limits.maxSampledTexturesPerShaderStage) << '\n';
  os << indent << "maxSamplersPerShaderStage: " << FormatNumber(limits.maxSamplersPerShaderStage)
     << '\n';
  os << indent
     << "maxStorageBuffersPerShaderStage: " << FormatNumber(limits.maxStorageBuffersPerShaderStage)
     << '\n';
  os << indent << "maxStorageTexturesPerShaderStage: "
     << FormatNumber(limits.maxStorageTexturesPerShaderStage) << '\n';
  os << indent
     << "maxUniformBuffersPerShaderStage: " << FormatNumber(limits.maxUniformBuffersPerShaderStage)
     << '\n';
  os << indent
     << "maxUniformBufferBindingSize: " << FormatNumber(limits.maxUniformBufferBindingSize) << '\n';
  os << indent
     << "maxStorageBufferBindingSize: " << FormatNumber(limits.maxStorageBufferBindingSize) << '\n';
  os << indent
     << "minUniformBufferOffsetAlignment: " << FormatNumber(limits.minUniformBufferOffsetAlignment)
     << '\n';
  os << indent
     << "minStorageBufferOffsetAlignment: " << FormatNumber(limits.minStorageBufferOffsetAlignment)
     << '\n';
  os << indent << "maxVertexBuffers: " << FormatNumber(limits.maxVertexBuffers) << '\n';
  os << indent << "maxBufferSize: " << FormatNumber(limits.maxBufferSize) << '\n';
  os << indent << "maxVertexAttributes: " << FormatNumber(limits.maxVertexAttributes) << '\n';
  os << indent << "maxVertexBufferArrayStride: " << FormatNumber(limits.maxVertexBufferArrayStride)
     << '\n';
  os << indent
     << "maxInterStageShaderVariables: " << FormatNumber(limits.maxInterStageShaderVariables)
     << '\n';
  os << indent << "maxColorAttachments: " << FormatNumber(limits.maxColorAttachments) << '\n';
  os << indent << "maxColorAttachmentBytesPerSample: "
     << FormatNumber(limits.maxColorAttachmentBytesPerSample) << '\n';
  os << indent
     << "maxComputeWorkgroupStorageSize: " << FormatNumber(limits.maxComputeWorkgroupStorageSize)
     << '\n';
  os << indent << "maxComputeInvocationsPerWorkgroup: "
     << FormatNumber(limits.maxComputeInvocationsPerWorkgroup) << '\n';
  os << indent << "maxComputeWorkgroupSizeX: " << FormatNumber(limits.maxComputeWorkgroupSizeX)
     << '\n';
  os << indent << "maxComputeWorkgroupSizeY: " << FormatNumber(limits.maxComputeWorkgroupSizeY)
     << '\n';
  os << indent << "maxComputeWorkgroupSizeZ: " << FormatNumber(limits.maxComputeWorkgroupSizeZ)
     << '\n';
  os << indent << "maxComputeWorkgroupsPerDimension: "
     << FormatNumber(limits.maxComputeWorkgroupsPerDimension) << '\n';
}

void PrintAdapterInfo(ostream& os, vtkIndent indent, const wgpu::Adapter& adapter)
{
  wgpu::AdapterInfo info{};
#if VTK_USE_DAWN_WEBGPU
  wgpu::DawnAdapterPropertiesPowerPreference power_props{};
  info.nextInChain = &power_props;
#endif
  adapter.GetInfo(&info);
  os << indent << "VendorID: " << AsHex(info.vendorID) << '\n';
  os << indent << "Vendor: " << info.vendor << '\n';
  os << indent << "Architecture: " << info.architecture << '\n';
  os << indent << "DeviceID: " << AsHex(info.deviceID) << '\n';
  os << indent << "Name: " << info.device << '\n';
  os << indent << "Driver description: " << info.description << '\n';
  os << indent << "Adapter Type: " << info.adapterType << '\n';
  os << indent << "Backend Type: " << info.backendType << '\n';
  os << indent << "Power: ";
#if VTK_USE_DAWN_WEBGPU
  switch (power_props.powerPreference)
  {
    case wgpu::PowerPreference::LowPower:
      os << "low power\n";
      break;
    case wgpu::PowerPreference::HighPerformance:
      os << "high performance\n";
      break;
    case wgpu::PowerPreference::Undefined:
      os << "<undefined>\n";
      break;
  }
#else
  os << "Unknown\n";
#endif
}

void PrintAdapterFeatures(ostream& os, vtkIndent indent, const wgpu::Adapter& adapter)
{
  wgpu::SupportedFeatures supportedFeatures = {};
  adapter.GetFeatures(&supportedFeatures);
  os << indent << "Features\n";
  os << indent << "========\n";
  for (std::size_t i = 0; i < supportedFeatures.featureCount; ++i)
  {
    const auto feature = supportedFeatures.features[i];
#if VTK_USE_DAWN_WEBGPU
    auto info = dawn::native::GetFeatureInfo(feature);
    os << indent << "   * " << info->name << '\n';
    os << indent << info->description << '\n';
    os << indent << "      " << info->url << '\n';
#elif defined(__EMSCRIPTEN__)
    // Look up the list of feature strings in `WebGPU.FeatureName`
    const auto featureIdx = static_cast<std::underlying_type<wgpu::FeatureName>::type>(feature);
    // clang-format off
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"
    char *featureNameCStr = (char*)EM_ASM_PTR({
      let jsString = WebGPU.FeatureName[$0];
      if (jsString === undefined) {
        jsString = "undefined";
      }
      return stringToNewUTF8(jsString);
    }, featureIdx);
#pragma clang diagnostic pop
    // clang-format on
    os << indent << indent << featureNameCStr << '\n';
    free(featureNameCStr);
#endif
  }
}

void PrintAdapterLimits(ostream& os, vtkIndent indent, const wgpu::Adapter& adapter)
{
  wgpu::Limits adapterLimits;
  if (adapter.GetLimits(&adapterLimits))
  {
    os << indent << '\n';
    os << indent << "Adapter Limits\n";
    os << indent << "==============\n";
    PrintLimits(os, indent.GetNextIndent(), adapterLimits);
  }
}

void PrintAdapter(ostream& os, vtkIndent indent, const wgpu::Adapter& adapter)
{
  os << indent << "Adapter\n";
  os << indent << "=======\n";
  PrintAdapterInfo(os, indent.GetNextIndent(), adapter);
  PrintAdapterFeatures(os, indent.GetNextIndent(), adapter);
  PrintAdapterLimits(os, indent.GetNextIndent(), adapter);
}

#if VTK_USE_DAWN_WEBGPU
/**
 * Implement Dawn's MemoryDump interface.
 */
class DawnMemoryDump : public dawn::native::MemoryDump
{
public:
  void AddScalar(const char* name, const char* key, const char* units, uint64_t value) override
  {
    if (key == MemoryDump::kNameSize && units == MemoryDump::kUnitsBytes)
    {
      TotalSize += value;
    }
    else if (key == MemoryDump::kNameObjectCount && units == MemoryDump::kUnitsObjects)
    {
      TotalObjects += value;
    }
    auto it = this->WebGPUObjects.find(name);
    if (it == this->WebGPUObjects.end())
    {
      MemoryInformation info;
      info.Size = value;
      this->WebGPUObjects[name] = info;
    }
    else
    {
      it->second.Size = value;
    }
  }

  void AddString(const char* name, const char* key, const std::string& value) override
  {
    auto it = this->WebGPUObjects.find(name);
    if (it == this->WebGPUObjects.end())
    {
      MemoryInformation info;
      info.Properties[key] = value;
      this->WebGPUObjects[name] = info;
    }
    else
    {
      it->second.Properties[key] = value;
    }
  }

  uint64_t GetTotalSize() const { return TotalSize; }
  uint64_t GetTotalNumberOfObjects() const { return TotalObjects; }

  void PrintSelf(ostream& os, vtkIndent indent)
  {
    os << indent << "TotalSize: " << this->TotalSize << '\n';
    os << indent << "TotalObjects: " << this->TotalObjects << '\n';
    for (auto& object : this->WebGPUObjects)
    {
      os << indent << indent << "-Name: " << object.first << '\n';
      os << indent << indent << "  Size: " << object.second.Size << '\n';
      for (auto& property : object.second.Properties)
      {
        os << indent << indent << "  " << property.first << "=" << property.second << '\n';
      }
    }
  }

  struct MemoryInformation
  {
    std::uint64_t Size;
    std::map<std::string, std::string> Properties;
  };

private:
  uint64_t TotalSize = 0;
  uint64_t TotalObjects = 0;

  std::unordered_map<std::string, MemoryInformation> WebGPUObjects;
};
#endif

} // end anon namespace

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPUConfiguration);

//------------------------------------------------------------------------------
vtkWebGPUConfiguration::vtkWebGPUConfiguration()
  : Internals(new vtkWebGPUConfigurationInternals())
{
  vtksys::SystemInformation info;
  if (info.GetOSIsApple())
  {
    this->Backend = BackendType::Metal;
  }
  else if (info.GetOSIsWindows())
  {
    this->Backend = BackendType::D3D12;
  }
  else
  {
    this->Backend = BackendType::Vulkan;
  }
  this->Timeout = vtkWebGPUConfigurationInternals::DefaultTimeout;
}

//------------------------------------------------------------------------------
vtkWebGPUConfiguration::~vtkWebGPUConfiguration()
{
  this->Finalize();
}

//------------------------------------------------------------------------------
void vtkWebGPUConfiguration::PrintSelf(ostream& os, vtkIndent indent)
{
  os << "Backend: " << this->Backend << '\n';
  os << "BackendInUse: " << this->GetBackendInUseAsString() << '\n';
  os << "PowerPreference: " << this->PowerPreference << '\n';
  os << "DeviceReady: " << (this->Internals->DeviceReady ? "yes\n" : "no\n");
  os << "Timeout:" << this->Timeout << "ms\n";
  os << "Instance: " << vtkWebGPUConfigurationInternals::Instance.Get() << '\n';
  os << "Adapter: " << this->Internals->Adapter.Get() << '\n';
  os << "Device: " << this->Internals->Device.Get() << '\n';
  this->Superclass::PrintSelf(os, indent.GetNextIndent());
}

//------------------------------------------------------------------------------
void vtkWebGPUConfiguration::SetDefaultTimeout(double t)
{
  vtkWebGPUConfigurationInternals::DefaultTimeout = t;
}

//------------------------------------------------------------------------------
wgpu::Adapter vtkWebGPUConfiguration::GetAdapter()
{
  return this->Internals->Adapter;
}

//------------------------------------------------------------------------------
wgpu::Device vtkWebGPUConfiguration::GetDevice()
{
  return this->Internals->Device;
}

//------------------------------------------------------------------------------
wgpu::Instance vtkWebGPUConfiguration::GetInstance()
{
  return vtkWebGPUConfigurationInternals::Instance;
}

//------------------------------------------------------------------------------
bool vtkWebGPUConfiguration::Initialize()
{
  vtkDebugMacro(<< __func__);
  auto& internals = (*this->Internals);
  if (internals.DeviceReady)
  {
    vtkDebugMacro(<< "Device is already initialized.");
    return true;
  }
  vtkWebGPUConfigurationInternals::AddInstanceRef();

  wgpu::RequestAdapterOptions adapterOptions = {};
  adapterOptions.backendType = internals.ToWGPUBackendType(this->Backend);
  adapterOptions.powerPreference = internals.ToWGPUPowerPreferenceType(this->PowerPreference);

  std::uint64_t timeoutNS = UINT64_MAX;
  internals.Timedout = false;
  auto waitStatus = vtkWebGPUConfigurationInternals::Instance.WaitAny(
    vtkWebGPUConfigurationInternals::Instance.RequestAdapter(
      &adapterOptions, wgpu::CallbackMode::WaitAnyOnly,
      [](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, const char* message,
        vtkWebGPUConfigurationInternals* internalsData)
      {
        if (status != wgpu::RequestAdapterStatus::Success)
        {
          vtkGenericWarningMacro("Failed to get an adapter:" << message);
          return;
        }
        internalsData->Adapter = std::move(adapter);
      },
      this->Internals.get()),
    timeoutNS);
  if (waitStatus == wgpu::WaitStatus::TimedOut)
  {
    vtkWarningMacro(<< "Request adapter timed out!");
    return internals.DeviceReady;
  }
  internals.Timedout = false;

  // Create device descriptor with callbacks and toggles
  wgpu::DeviceDescriptor deviceDescriptor = {};
  deviceDescriptor.SetDeviceLostCallback(wgpu::CallbackMode::AllowSpontaneous,
    [](const wgpu::Device&, wgpu::DeviceLostReason reason, wgpu::StringView message)
    {
      const char* reasonName = "";
      switch (reason)
      {
        case wgpu::DeviceLostReason::Unknown:
          reasonName = "Unknown";
          break;
        case wgpu::DeviceLostReason::Destroyed:
          reasonName = "Destroyed";
          break;
        case wgpu::DeviceLostReason::CallbackCancelled:
          reasonName = "CallbackCancelled";
          break;
        case wgpu::DeviceLostReason::FailedCreation:
          reasonName = "FailedCreation";
          break;
        default:
          break;
      }
      vtkLog(INFO, << "Device lost, reason=" << reasonName << ". "
                   << vtkWebGPUHelpers::StringViewToStdString(message));
    });
  deviceDescriptor.SetUncapturedErrorCallback(
    [](const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView message)
    {
      const char* errorTypeName = "";
      switch (type)
      {
        case wgpu::ErrorType::Validation:
          errorTypeName = "Validation";
          break;
        case wgpu::ErrorType::OutOfMemory:
          errorTypeName = "Out of memory";
          break;
        case wgpu::ErrorType::Unknown:
          errorTypeName = "Unknown";
          break;
        case wgpu::ErrorType::Internal:
          errorTypeName = "Internal";
          break;
        default:
          break;
      }
      vtkGenericWarningMacro(<< errorTypeName
                             << " error: " << vtkWebGPUHelpers::StringViewToStdString(message));
    });

  // Populating limits of the device
  internals.PopulateRequiredLimits(internals.Adapter);
  deviceDescriptor.requiredLimits = &internals.RequiredLimits;

  // Populating required features of the device
  internals.PopulateRequiredFeatures();
  deviceDescriptor.requiredFeatureCount = internals.RequiredFeatures.size();
  deviceDescriptor.requiredFeatures = internals.RequiredFeatures.data();

  // Synchronously create the device
  internals.Timedout = false;
  waitStatus = vtkWebGPUConfigurationInternals::Instance.WaitAny(
    internals.Adapter.RequestDevice(
      &deviceDescriptor, wgpu::CallbackMode::WaitAnyOnly,
      [](wgpu::RequestDeviceStatus status, wgpu::Device device, const char* message,
        vtkWebGPUConfigurationInternals* internalsData)
      {
        if (status != wgpu::RequestDeviceStatus::Success)
        {
          vtkGenericWarningMacro("Failed to get a device:" << message);
          return;
        }
        internalsData->Device = std::move(device);
        // internalsData->Queue = internals->Device.GetQueue();
      },
      this->Internals.get()),
    UINT64_MAX);
  if (waitStatus == wgpu::WaitStatus::TimedOut)
  {
    vtkWarningMacro(<< "Request device timed out!");
    return internals.DeviceReady;
  }
  if (internals.Device != nullptr)
  {
    internals.DeviceReady = true;
  }
  return internals.DeviceReady;
}

//------------------------------------------------------------------------------
void vtkWebGPUConfiguration::Finalize()
{
  auto& internals = (*this->Internals);
  if (!internals.DeviceReady)
  {
    return;
  }
  internals.Adapter = nullptr;
  internals.Device = nullptr;
  internals.DeviceReady = false;
  vtkWebGPUConfigurationInternals::ReleaseInstanceRef();
}

//------------------------------------------------------------------------------
void vtkWebGPUConfiguration::ProcessEvents()
{
#if defined(__EMSCRIPTEN__)
  vtkWebGPUConfigurationInternals::Instance.ProcessEvents();
  if (emscripten_has_asyncify())
  {
    // gives a chance for webgpu callback code to execute
    emscripten_sleep(1);
  }
  else
  {
    vtkErrorMacro(<< "This build of VTK cannot run asynchronous javascript code synchronously."
                     "Please compile VTK with ASYNCIFY or JSPI.");
  }
#else
  vtkWebGPUConfigurationInternals::Instance.ProcessEvents();
#endif
}

//------------------------------------------------------------------------------
vtkWebGPUConfiguration::BackendType vtkWebGPUConfiguration::GetBackendInUse()
{
  auto& internals = (*this->Internals);
  if (!internals.DeviceReady)
  {
    return vtkWebGPUConfiguration::BackendType::Undefined;
  }
  wgpu::AdapterInfo info{};
  internals.Adapter.GetInfo(&info);
  return internals.FromWGPUBackendType(info.backendType);
}

//------------------------------------------------------------------------------
std::string vtkWebGPUConfiguration::GetBackendInUseAsString()
{
  auto& internals = (*this->Internals);
  if (internals.DeviceReady)
  {
    wgpu::AdapterInfo info{};
    internals.Adapter.GetInfo(&info);
    switch (info.backendType)
    {
      case wgpu::BackendType::Null:
        return "Null";
      case wgpu::BackendType::WebGPU:
        return "WebGPU";
      case wgpu::BackendType::D3D11:
        return "D3D11";
      case wgpu::BackendType::D3D12:
        return "D3D12";
      case wgpu::BackendType::Metal:
        return "Metal";
      case wgpu::BackendType::Vulkan:
        return "Vulkan";
      case wgpu::BackendType::OpenGL:
        return "OpenGL";
      case wgpu::BackendType::OpenGLES:
        return "OpenGL ES";
      case wgpu::BackendType::Undefined:
      default:
        return "Undefined";
    }
  }
  else
  {
    return "Undefined";
  }
}

//------------------------------------------------------------------------------
std::size_t vtkWebGPUConfiguration::Align(std::size_t value, std::size_t alignment)
{
  // This is equivalent to std::ceil(value / (float)alignment) * alignment.
  // This implementation is more efficient because it avoids floating point operations, ceil with
  // the use of bitmasks.
  return
    // This step ensures that any remainder when value is divided by alignment is handled correctly
    // by rounding up to the next multiple of alignment.
    (value + alignment - 1)
    // clear the lower bits (using mask) that are less than the alignment boundary.
    & ~(alignment - 1);
}

//------------------------------------------------------------------------------
std::string vtkWebGPUConfiguration::DeviceNotReadyMessage()
{
  return "Device not ready\n";
}

//------------------------------------------------------------------------------
std::string vtkWebGPUConfiguration::ReportCapabilities()
{
  std::ostringstream os;
  if (this->Internals->DeviceReady)
  {
    ::PrintAdapter(os, vtkIndent(), this->Internals->Adapter);
    return os.str();
  }
  else
  {
    return DeviceNotReadyMessage();
  }
}

//------------------------------------------------------------------------------
std::uint32_t vtkWebGPUConfiguration::GetAdapterVendorID()
{
  if (this->Internals->DeviceReady)
  {
    wgpu::AdapterInfo info{};
    this->Internals->Adapter.GetInfo(&info);
    return info.vendorID;
  }
  else
  {
    return 0;
  }
}

//------------------------------------------------------------------------------
std::uint32_t vtkWebGPUConfiguration::GetAdapterDeviceID()
{
  if (this->Internals->DeviceReady)
  {
    wgpu::AdapterInfo info{};
    this->Internals->Adapter.GetInfo(&info);
    return info.deviceID;
  }
  else
  {
    return 0;
  }
}

//------------------------------------------------------------------------------
bool vtkWebGPUConfiguration::IsAMDGPUInUse()
{
  return this->GetAdapterVendorID() == ::AMD_PCI_VENDOR_ID;
}

//------------------------------------------------------------------------------
bool vtkWebGPUConfiguration::IsAppleGPUInUse()
{
  return this->GetAdapterVendorID() == ::APPLE_PCI_VENDOR_ID;
}

//------------------------------------------------------------------------------
bool vtkWebGPUConfiguration::IsARMGPUInUse()
{
  return this->GetAdapterVendorID() == ::ARM_PCI_VENDOR_ID;
}

//------------------------------------------------------------------------------
bool vtkWebGPUConfiguration::IsBroadcomGPUInUse()
{
  return this->GetAdapterVendorID() == ::BROADCOM_PCI_VENDOR_ID;
}

//------------------------------------------------------------------------------
bool vtkWebGPUConfiguration::IsIntelGPUInUse()
{
  return this->GetAdapterVendorID() == ::INTEL_PCI_VENDOR_ID;
}

//------------------------------------------------------------------------------
bool vtkWebGPUConfiguration::IsMesaGPUInUse()
{
  return this->GetAdapterVendorID() == ::MESA_PCI_VENDOR_ID;
}

//------------------------------------------------------------------------------
bool vtkWebGPUConfiguration::IsMicrosoftGPUInUse()
{
  return this->GetAdapterVendorID() == ::MICROSOFT_PCI_VENDOR_ID;
}

//------------------------------------------------------------------------------
bool vtkWebGPUConfiguration::IsNVIDIAGPUInUse()
{
  return this->GetAdapterVendorID() == ::NVIDIA_PCI_VENDOR_ID;
}

//------------------------------------------------------------------------------
bool vtkWebGPUConfiguration::IsSamsungGPUInUse()
{
  return this->GetAdapterVendorID() == ::SAMSUNG_PCI_VENDOR_ID;
}

//------------------------------------------------------------------------------
wgpu::Buffer vtkWebGPUConfiguration::CreateBuffer(unsigned long sizeBytes, wgpu::BufferUsage usage,
  bool mappedAtCreation /*=false*/, const char* label /*=nullptr*/)
{
  auto& internals = (*this->Internals);
  if (!internals.DeviceReady)
  {
    vtkWarningMacro(<< "Cannot create buffer because device is not ready.");
    return nullptr;
  }
  wgpu::BufferDescriptor bufferDescriptor;
  bufferDescriptor.label = label == nullptr ? "(nolabel)" : label;
  bufferDescriptor.size = sizeBytes;
  bufferDescriptor.usage = usage;
  bufferDescriptor.mappedAtCreation = mappedAtCreation;

  return this->CreateBuffer(bufferDescriptor);
}

//------------------------------------------------------------------------------
wgpu::Buffer vtkWebGPUConfiguration::CreateBuffer(const wgpu::BufferDescriptor& bufferDescriptor)
{
  auto& internals = (*this->Internals);
  if (!internals.DeviceReady)
  {
    vtkWarningMacro(<< "Cannot create buffer because device is not ready.");
    return nullptr;
  }
  const auto label = vtkWebGPUHelpers::StringViewToStdString(bufferDescriptor.label);
  if (!vtkWebGPUBufferInternals::CheckBufferSize(internals.Device, bufferDescriptor.size))
  {
    wgpu::Limits supportedDeviceLimits;
    internals.Device.GetLimits(&supportedDeviceLimits);
    vtkLog(ERROR,
      "The current WebGPU Device cannot create buffers larger than: "
        << supportedDeviceLimits.maxStorageBufferBindingSize << " bytes but the buffer with label "
        << label << " is " << bufferDescriptor.size << " bytes big.");

    return nullptr;
  }
  vtkVLog(this->GetGPUMemoryLogVerbosity(),
    "Create buffer {label: \"" << label << "\", size: " << bufferDescriptor.size << "}");
  wgpu::Buffer buffer = internals.Device.CreateBuffer(&bufferDescriptor);
  return buffer;
}

//------------------------------------------------------------------------------
void vtkWebGPUConfiguration::WriteBuffer(const wgpu::Buffer& buffer, unsigned long offset,
  const void* data, unsigned long sizeBytes, const char* description /*= nullptr*/)
{
  auto& internals = (*this->Internals);
  if (!internals.DeviceReady)
  {
    vtkWarningMacro(<< "Cannot write data into buffer because device is not ready.");
    return;
  }
  vtkVLog(this->GetGPUMemoryLogVerbosity(),
    "Write buffer {description: \"" << (description ? description : "null")
                                    << "\", offset: " << offset << ", size: " << sizeBytes << "}");
  internals.Device.GetQueue().WriteBuffer(buffer, offset, data, sizeBytes);
}

//------------------------------------------------------------------------------
wgpu::Texture vtkWebGPUConfiguration::CreateTexture(wgpu::Extent3D extents,
  wgpu::TextureDimension dimension, wgpu::TextureFormat format, wgpu::TextureUsage usage,
  int mipLevelCount, const char* label /*=nullptr*/)
{
  wgpu::TextureDescriptor textureDescriptor;
  textureDescriptor.dimension = dimension;
  textureDescriptor.format = format;
  textureDescriptor.size = extents;
  textureDescriptor.mipLevelCount = mipLevelCount;
  textureDescriptor.nextInChain = nullptr;
  textureDescriptor.sampleCount = 1;
  textureDescriptor.usage = usage;
  textureDescriptor.viewFormatCount = 0;
  textureDescriptor.viewFormats = nullptr;
  textureDescriptor.label = label;
  return this->CreateTexture(textureDescriptor);
}

//------------------------------------------------------------------------------
wgpu::Texture vtkWebGPUConfiguration::CreateTexture(
  const wgpu::TextureDescriptor& textureDescriptor)
{
  auto& internals = (*this->Internals);
  if (!internals.DeviceReady)
  {
    vtkWarningMacro(<< "Cannot create texture because device is not ready.");
    return nullptr;
  }
  const auto label = vtkWebGPUHelpers::StringViewToStdString(textureDescriptor.label);
  vtkVLog(this->GetGPUMemoryLogVerbosity(),
    "Create texture {label: \"" << label << "\", size: [" << textureDescriptor.size.width << ','
                                << textureDescriptor.size.height << ','
                                << textureDescriptor.size.depthOrArrayLayers << "]}");
  return internals.Device.CreateTexture(&textureDescriptor);
}

//------------------------------------------------------------------------------
wgpu::TextureView vtkWebGPUConfiguration::CreateView(wgpu::Texture texture,
  wgpu::TextureViewDimension dimension, wgpu::TextureAspect aspect, wgpu::TextureFormat format,
  int baseMipLevel, int mipLevelCount, const char* label /*=nullptr*/)
{
  // Creating a "full" view of the texture
  wgpu::TextureViewDescriptor textureViewDescriptor;
  textureViewDescriptor.arrayLayerCount = 1;
  textureViewDescriptor.aspect = aspect;
  textureViewDescriptor.baseArrayLayer = 0;
  textureViewDescriptor.baseMipLevel = baseMipLevel;
  textureViewDescriptor.dimension = dimension;
  textureViewDescriptor.format = format;
  textureViewDescriptor.label = label;
  textureViewDescriptor.mipLevelCount = mipLevelCount;
  textureViewDescriptor.nextInChain = nullptr;

  return this->CreateView(texture, textureViewDescriptor);
}

//------------------------------------------------------------------------------
wgpu::TextureView vtkWebGPUConfiguration::CreateView(
  wgpu::Texture texture, const wgpu::TextureViewDescriptor& viewDescriptor)
{
  auto& internals = (*this->Internals);
  if (!internals.DeviceReady)
  {
    vtkWarningMacro(<< "Cannot create texture because device is not ready.");
    return nullptr;
  }
  return texture.CreateView(&viewDescriptor);
}

//------------------------------------------------------------------------------
void vtkWebGPUConfiguration::WriteTexture(wgpu::Texture texture, uint32_t bytesPerRow,
  uint32_t sizeBytes, const void* data, const char* description /*= nullptr*/)
{
  auto& internals = (*this->Internals);
  if (!internals.DeviceReady)
  {
    vtkWarningMacro(<< "Cannot write data into texture because device is not ready.");
    return;
  }
  const auto copyTexture = vtkWebGPUTextureInternals::GetTexelCopyTextureInfo(texture);

  const auto textureDataLayout = vtkWebGPUTextureInternals::GetDataLayout(texture, bytesPerRow);

  wgpu::Extent3D textureExtents = { texture.GetWidth(), texture.GetHeight(),
    texture.GetDepthOrArrayLayers() };
  vtkVLog(this->GetGPUMemoryLogVerbosity(),
    "Write texture {description: \"" << (description ? description : "null")
                                     << "\", size: " << sizeBytes << "}");
  internals.Device.GetQueue().WriteTexture(
    &copyTexture, data, sizeBytes, &textureDataLayout, &textureExtents);
}

//------------------------------------------------------------------------------
void vtkWebGPUConfiguration::SetGPUMemoryLogVerbosity(vtkLogger::Verbosity verbosity)
{
  this->GPUMemoryLogVerbosity = verbosity;
}

//------------------------------------------------------------------------------
vtkLogger::Verbosity vtkWebGPUConfiguration::GetGPUMemoryLogVerbosity()
{
  if (this->GPUMemoryLogVerbosity == vtkLogger::VERBOSITY_INVALID)
  {
    this->GPUMemoryLogVerbosity = vtkLogger::VERBOSITY_TRACE;
    // Find an environment variable that specifies logger verbosity
    const char* verbosityKey = "VTK_WEBGPU_MEMORY_LOG_VERBOSITY";
    if (vtksys::SystemTools::HasEnv(verbosityKey))
    {
      const char* verbosityCStr = vtksys::SystemTools::GetEnv(verbosityKey);
      const auto verbosity = vtkLogger::ConvertToVerbosity(verbosityCStr);
      if (verbosity > vtkLogger::VERBOSITY_INVALID)
      {
        this->GPUMemoryLogVerbosity = verbosity;
      }
    }
  }
  return this->GPUMemoryLogVerbosity;
}

void vtkWebGPUConfiguration::DumpMemoryStatistics()
{
#if VTK_USE_DAWN_WEBGPU
  auto* memoryDump = new DawnMemoryDump();
  dawn::native::DumpMemoryStatistics(this->GetDevice().Get(), memoryDump);
  std::ostringstream os;
  memoryDump->PrintSelf(os, vtkIndent());
  vtkVLog(this->GetGPUMemoryLogVerbosity(), << os.str());
  delete memoryDump;
#else
  // Cannot do anything here because we don't know if the textures/buffers
  // created through `this->CreateTexture` or `this->CreateBuffer` are still alive.
  vtkVLog(this->GetGPUMemoryLogVerbosity(),
    "Cannot determine memory statistics for allocated webgpu objects in this webgpu "
    "implementation");
#endif
}
VTK_ABI_NAMESPACE_END
