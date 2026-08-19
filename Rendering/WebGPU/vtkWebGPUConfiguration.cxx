// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUConfiguration.h"
#include "Private/vtkWebGPUBufferInternals.h"
#include "Private/vtkWebGPUConfigurationInternals.h"
#include "Private/vtkWebGPUHelpersPrivate.h"
#include "Private/vtkWebGPUImplExtensions.h"
#include "Private/vtkWebGPUProcLoader.h"
#include "Private/vtkWebGPUTextureInternals.h"

#include "vtkObjectFactory.h"
#include "vtkStringFormatter.h"
#include "vtkWebGPUHelpers.h"
#include "vtkWebGPURenderWindow.h"

#include "vtksys/SystemInformation.hxx"
#include "vtksys/SystemTools.hxx"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <sstream>

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
  std::basic_ostream<CharT, Traits>& o, WGPUStringView value)
{
  o << vtkWebGPUStringViewToStdString(value);
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

ostream& operator<<(ostream& os, const WGPUBackendType& backend)
{
  switch (backend)
  {
    case WGPUBackendType_Null:
      os << "Null";
      break;
    case WGPUBackendType_WebGPU:
      os << "WebGPU";
      break;
    case WGPUBackendType_D3D11:
      os << "D3D11";
      break;
    case WGPUBackendType_D3D12:
      os << "D3D12";
      break;
    case WGPUBackendType_Metal:
      os << "Metal";
      break;
    case WGPUBackendType_Vulkan:
      os << "Vulkan";
      break;
    case WGPUBackendType_OpenGL:
      os << "OpenGL";
      break;
    case WGPUBackendType_OpenGLES:
      os << "OpenGLES";
      break;
    case WGPUBackendType_Undefined:
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

ostream& operator<<(ostream& os, const WGPUAdapterType& type)
{
  switch (type)
  {
    case WGPUAdapterType_DiscreteGPU:
      os << "discrete GPU";
      break;
    case WGPUAdapterType_IntegratedGPU:
      os << "integrated GPU";
      break;
    case WGPUAdapterType_CPU:
      os << "CPU";
      break;
    case WGPUAdapterType_Unknown:
    default:
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
  auto s = vtk::to_string(num);
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

void PrintLimits(ostream& os, vtkIndent indent, const WGPULimits& limits)
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

void PrintAdapterInfo(ostream& os, vtkIndent indent, WGPUAdapter adapter)
{
  WGPUAdapterInfo info = WGPU_ADAPTER_INFO_INIT;
#if VTK_USE_DAWN_WEBGPU
  VTKWGPUDawnAdapterPropertiesPowerPreference power_props =
    VTK_WGPU_DAWN_ADAPTER_PROPERTIES_POWER_PREFERENCE_INIT;
  info.nextInChain = &power_props.chain;
#endif
  wgpuAdapterGetInfo(adapter, &info);
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
    case WGPUPowerPreference_LowPower:
      os << "low power\n";
      break;
    case WGPUPowerPreference_HighPerformance:
      os << "high performance\n";
      break;
    case WGPUPowerPreference_Undefined:
    default:
      os << "<undefined>\n";
      break;
  }
#else
  os << "Unknown\n";
#endif
  wgpuAdapterInfoFreeMembers(info);
}

void PrintAdapterFeatures(ostream& os, vtkIndent indent, WGPUAdapter adapter)
{
  WGPUSupportedFeatures supportedFeatures = WGPU_SUPPORTED_FEATURES_INIT;
  wgpuAdapterGetFeatures(adapter, &supportedFeatures);
  os << indent << "Features\n";
  os << indent << "========\n";
  for (std::size_t i = 0; i < supportedFeatures.featureCount; ++i)
  {
    const auto feature = supportedFeatures.features[i];
#if defined(__EMSCRIPTEN__)
    // Look up the list of feature strings in `WebGPU.FeatureName`
    const auto featureIdx = static_cast<std::underlying_type<WGPUFeatureName>::type>(feature);
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
#else
    // Deliberately implementation agnostic: dawn::native::GetFeatureInfo() would
    // give names and descriptions here, but it is a Dawn library symbol and this
    // module must not be bound to one implementation. The enum values are
    // defined by webgpu.h, so they are meaningful for any runtime.
    os << indent << "   * Feature (0x" << std::hex << static_cast<uint32_t>(feature) << std::dec
       << ")\n";
#endif
  }
  wgpuSupportedFeaturesFreeMembers(supportedFeatures);
}

void PrintAdapterLimits(ostream& os, vtkIndent indent, WGPUAdapter adapter)
{
  WGPULimits adapterLimits = WGPU_LIMITS_INIT;
  if (wgpuAdapterGetLimits(adapter, &adapterLimits) == WGPUStatus_Success)
  {
    os << indent << '\n';
    os << indent << "Adapter Limits\n";
    os << indent << "==============\n";
    PrintLimits(os, indent.GetNextIndent(), adapterLimits);
  }
}

void PrintAdapter(ostream& os, vtkIndent indent, WGPUAdapter adapter)
{
  os << indent << "Adapter\n";
  os << indent << "=======\n";
  PrintAdapterInfo(os, indent.GetNextIndent(), adapter);
  PrintAdapterFeatures(os, indent.GetNextIndent(), adapter);
  PrintAdapterLimits(os, indent.GetNextIndent(), adapter);
}

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
  os << "Instance: " << vtkWebGPUConfigurationInternals::Instance << '\n';
  os << "Adapter: " << this->Internals->Adapter << '\n';
  os << "Device: " << this->Internals->Device << '\n';
  this->Superclass::PrintSelf(os, indent.GetNextIndent());
}

//------------------------------------------------------------------------------
void vtkWebGPUConfiguration::SetDefaultTimeout(double t)
{
  vtkWebGPUConfigurationInternals::DefaultTimeout = t;
}

//------------------------------------------------------------------------------
WGPUAdapter vtkWebGPUConfiguration::GetAdapter()
{
  return this->Internals->Adapter;
}

//------------------------------------------------------------------------------
WGPUDevice vtkWebGPUConfiguration::GetDevice()
{
  return this->Internals->Device;
}

//------------------------------------------------------------------------------
WGPUInstance vtkWebGPUConfiguration::GetInstance()
{
  return vtkWebGPUConfigurationInternals::Instance;
}

//------------------------------------------------------------------------------
bool vtkWebGPUConfiguration::Initialize()
{
  vtkDebugMacro(<< __func__);

  // Ensure the WebGPU implementation is loaded at runtime (Option B proc table).
  // This must happen before any WebGPU function calls.
  vtkWebGPUProcLoader* procLoader = vtkWebGPUProcLoader::GetInstance();
  if (!procLoader || !procLoader->IsLoaded())
  {
    vtkErrorMacro(<< "Failed to load WebGPU implementation library. "
                  << (procLoader ? procLoader->GetError() : "Unknown error"));
    return false;
  }

  auto& internals = (*this->Internals);
  if (internals.DeviceReady)
  {
    vtkDebugMacro(<< "Device is already initialized.");
    return true;
  }
  vtkWebGPUConfigurationInternals::AddInstanceRef();
  this->InstanceRefHeld = true;

  WGPURequestAdapterOptions adapterOptions = WGPU_REQUEST_ADAPTER_OPTIONS_INIT;
  adapterOptions.backendType = internals.ToWGPUBackendType(this->Backend);
  adapterOptions.powerPreference = internals.ToWGPUPowerPreferenceType(this->PowerPreference);

  std::uint64_t timeoutNS = UINT64_MAX;
  internals.Timedout = false;
  WGPURequestAdapterCallbackInfo adapterCallbackInfo = {};
  adapterCallbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
  adapterCallbackInfo.callback = [](WGPURequestAdapterStatus status, WGPUAdapter adapter,
                                   WGPUStringView message, void* userdata1, void* /*userdata2*/)
  {
    auto* internalsData = static_cast<vtkWebGPUConfigurationInternals*>(userdata1);
    if (status != WGPURequestAdapterStatus_Success)
    {
      vtkGenericWarningMacro(
        "Failed to get an adapter:" << vtkWebGPUStringViewToStdString(message));
      return;
    }
    // The callback owns the adapter handle; store it and release it later.
    internalsData->Adapter = adapter;
  };
  adapterCallbackInfo.userdata1 = this->Internals.get();
  WGPUFutureWaitInfo adapterWaitInfo = {};
  adapterWaitInfo.future = wgpuInstanceRequestAdapter(
    vtkWebGPUConfigurationInternals::Instance, &adapterOptions, adapterCallbackInfo);
  auto waitStatus =
    wgpuInstanceWaitAny(vtkWebGPUConfigurationInternals::Instance, 1, &adapterWaitInfo, timeoutNS);
  if (waitStatus == WGPUWaitStatus_TimedOut)
  {
    vtkWarningMacro(<< "Request adapter timed out!");
    return internals.DeviceReady;
  }
  internals.Timedout = false;

  // Create device descriptor with callbacks and toggles
  WGPUDeviceDescriptor deviceDescriptor = WGPU_DEVICE_DESCRIPTOR_INIT;
  deviceDescriptor.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
  deviceDescriptor.deviceLostCallbackInfo.callback =
    [](WGPUDevice const*, WGPUDeviceLostReason reason, WGPUStringView message, void*, void*)
  {
    const char* reasonName = "";
    switch (reason)
    {
      case WGPUDeviceLostReason_Unknown:
        reasonName = "Unknown";
        break;
      case WGPUDeviceLostReason_Destroyed:
        reasonName = "Destroyed";
        break;
      case WGPUDeviceLostReason_CallbackCancelled:
        reasonName = "CallbackCancelled";
        break;
      case WGPUDeviceLostReason_FailedCreation:
        reasonName = "FailedCreation";
        break;
      default:
        break;
    }
    vtkLog(INFO, << "Device lost, reason=" << reasonName << ". "
                 << vtkWebGPUStringViewToStdString(message));
  };
  deviceDescriptor.uncapturedErrorCallbackInfo.callback =
    [](WGPUDevice const*, WGPUErrorType type, WGPUStringView message, void*, void*)
  {
    const char* errorTypeName = "";
    switch (type)
    {
      case WGPUErrorType_Validation:
        errorTypeName = "Validation";
        break;
      case WGPUErrorType_OutOfMemory:
        errorTypeName = "Out of memory";
        break;
      case WGPUErrorType_Unknown:
        errorTypeName = "Unknown";
        break;
      case WGPUErrorType_Internal:
        errorTypeName = "Internal";
        break;
      default:
        break;
    }
    vtkGenericWarningMacro(<< errorTypeName
                           << " error: " << vtkWebGPUStringViewToStdString(message));
  };

  // Populating limits of the device
  internals.PopulateRequiredLimits(internals.Adapter);
  deviceDescriptor.requiredLimits = &internals.RequiredLimits;

  // Populating required features of the device
  internals.PopulateRequiredFeatures();
  deviceDescriptor.requiredFeatureCount = internals.RequiredFeatures.size();
  deviceDescriptor.requiredFeatures = internals.RequiredFeatures.data();

  // Synchronously create the device
  internals.Timedout = false;
  WGPURequestDeviceCallbackInfo deviceCallbackInfo = {};
  deviceCallbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
  deviceCallbackInfo.callback = [](WGPURequestDeviceStatus status, WGPUDevice device,
                                  WGPUStringView message, void* userdata1, void* /*userdata2*/)
  {
    auto* internalsData = static_cast<vtkWebGPUConfigurationInternals*>(userdata1);
    if (status != WGPURequestDeviceStatus_Success)
    {
      vtkGenericWarningMacro("Failed to get a device:" << vtkWebGPUStringViewToStdString(message));
      return;
    }
    // The callback owns the device handle; store it and release it later.
    internalsData->Device = device;
  };
  deviceCallbackInfo.userdata1 = this->Internals.get();
  WGPUFutureWaitInfo deviceWaitInfo = {};
  deviceWaitInfo.future =
    wgpuAdapterRequestDevice(internals.Adapter, &deviceDescriptor, deviceCallbackInfo);
  waitStatus =
    wgpuInstanceWaitAny(vtkWebGPUConfigurationInternals::Instance, 1, &deviceWaitInfo, UINT64_MAX);
  if (waitStatus == WGPUWaitStatus_TimedOut)
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
void vtkWebGPUConfiguration::FinalizeDevice()
{
  auto& internals = (*this->Internals);
  if (!internals.DeviceReady)
  {
    return;
  }
  if (internals.Device != nullptr)
  {
    wgpuDeviceRelease(internals.Device);
    internals.Device = nullptr;
  }
  if (internals.Adapter != nullptr)
  {
    wgpuAdapterRelease(internals.Adapter);
    internals.Adapter = nullptr;
  }
  internals.DeviceReady = false;
  // Deliberately does NOT call ReleaseInstanceRef() so the Vulkan instance
  // stays alive (keeping GLX libraries loaded) until Finalize() is called.
}

//------------------------------------------------------------------------------
void vtkWebGPUConfiguration::Finalize()
{
  this->FinalizeDevice();
  if (this->InstanceRefHeld)
  {
    this->InstanceRefHeld = false;
    vtkWebGPUConfigurationInternals::ReleaseInstanceRef();
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUConfiguration::DeferBufferRelease(WGPUBuffer buffer)
{
  if (buffer != nullptr)
  {
    this->Internals->BuffersPendingRelease.push_back(buffer);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUConfiguration::ProcessEvents()
{
#if defined(__EMSCRIPTEN__)
  wgpuInstanceProcessEvents(vtkWebGPUConfigurationInternals::Instance);
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
  wgpuInstanceProcessEvents(vtkWebGPUConfigurationInternals::Instance);
#endif
  // Safe to run now: every callback the implementation dispatched above has
  // returned, so it no longer holds a lock on any of these buffers.
  auto& pending = this->Internals->BuffersPendingRelease;
  for (WGPUBuffer buffer : pending)
  {
    wgpuBufferRelease(buffer);
  }
  pending.clear();
}

//------------------------------------------------------------------------------
vtkWebGPUConfiguration::BackendType vtkWebGPUConfiguration::GetBackendInUse()
{
  auto& internals = (*this->Internals);
  if (!internals.DeviceReady)
  {
    return vtkWebGPUConfiguration::BackendType::Undefined;
  }
  WGPUAdapterInfo info = WGPU_ADAPTER_INFO_INIT;
  wgpuAdapterGetInfo(internals.Adapter, &info);
  const auto backend = internals.FromWGPUBackendType(info.backendType);
  wgpuAdapterInfoFreeMembers(info);
  return backend;
}

//------------------------------------------------------------------------------
std::string vtkWebGPUConfiguration::GetBackendInUseAsString()
{
  auto& internals = (*this->Internals);
  if (internals.DeviceReady)
  {
    WGPUAdapterInfo info = WGPU_ADAPTER_INFO_INIT;
    wgpuAdapterGetInfo(internals.Adapter, &info);
    const WGPUBackendType backendType = info.backendType;
    wgpuAdapterInfoFreeMembers(info);
    switch (backendType)
    {
      case WGPUBackendType_Null:
        return "Null";
      case WGPUBackendType_WebGPU:
        return "WebGPU";
      case WGPUBackendType_D3D11:
        return "D3D11";
      case WGPUBackendType_D3D12:
        return "D3D12";
      case WGPUBackendType_Metal:
        return "Metal";
      case WGPUBackendType_Vulkan:
        return "Vulkan";
      case WGPUBackendType_OpenGL:
        return "OpenGL";
      case WGPUBackendType_OpenGLES:
        return "OpenGL ES";
      case WGPUBackendType_Undefined:
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
    WGPUAdapterInfo info = WGPU_ADAPTER_INFO_INIT;
    wgpuAdapterGetInfo(this->Internals->Adapter, &info);
    const auto vendorID = info.vendorID;
    wgpuAdapterInfoFreeMembers(info);
    return vendorID;
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
    WGPUAdapterInfo info = WGPU_ADAPTER_INFO_INIT;
    wgpuAdapterGetInfo(this->Internals->Adapter, &info);
    const auto deviceID = info.deviceID;
    wgpuAdapterInfoFreeMembers(info);
    return deviceID;
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
WGPUBuffer vtkWebGPUConfiguration::CreateBuffer(std::uint64_t sizeBytes, WGPUBufferUsage usage,
  bool mappedAtCreation /*=false*/, const char* label /*=nullptr*/)
{
  auto& internals = (*this->Internals);
  if (!internals.DeviceReady)
  {
    vtkWarningMacro(<< "Cannot create buffer because device is not ready.");
    return nullptr;
  }
  WGPUBufferDescriptor bufferDescriptor{};
  bufferDescriptor.label = WGPUStringView{ label == nullptr ? "(nolabel)" : label, WGPU_STRLEN };
  bufferDescriptor.size = sizeBytes;
  bufferDescriptor.usage = usage;
  bufferDescriptor.mappedAtCreation = mappedAtCreation;

  return this->CreateBuffer(bufferDescriptor);
}

//------------------------------------------------------------------------------
WGPUBuffer vtkWebGPUConfiguration::CreateBuffer(const WGPUBufferDescriptor& bufferDescriptor)
{
  auto& internals = (*this->Internals);
  if (!internals.DeviceReady)
  {
    vtkWarningMacro(<< "Cannot create buffer because device is not ready.");
    return nullptr;
  }
  const auto label = vtkWebGPUStringViewToStdString(bufferDescriptor.label);
  if (!vtkWebGPUBufferInternals::CheckBufferSize(internals.Device, bufferDescriptor.size))
  {
    WGPULimits supportedDeviceLimits = WGPU_LIMITS_INIT;
    wgpuDeviceGetLimits(internals.Device, &supportedDeviceLimits);
    vtkLog(ERROR,
      "The current WebGPU Device cannot create buffers larger than: "
        << supportedDeviceLimits.maxStorageBufferBindingSize << " bytes but the buffer with label "
        << label << " is " << bufferDescriptor.size << " bytes big.");

    return nullptr;
  }
  vtkVLog(this->GetGPUMemoryLogVerbosity(),
    "Create buffer {label: \"" << label << "\", size: " << bufferDescriptor.size << "}");
  return wgpuDeviceCreateBuffer(internals.Device, &bufferDescriptor);
}

//------------------------------------------------------------------------------
void vtkWebGPUConfiguration::WriteBuffer(WGPUBuffer buffer, std::uint64_t offset, const void* data,
  std::size_t sizeBytes, const char* description /*= nullptr*/)
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
  WGPUQueue queue = wgpuDeviceGetQueue(internals.Device);
  wgpuQueueWriteBuffer(queue, buffer, offset, data, sizeBytes);
  wgpuQueueRelease(queue);
}

//------------------------------------------------------------------------------
WGPUTexture vtkWebGPUConfiguration::CreateTexture(WGPUExtent3D extents,
  WGPUTextureDimension dimension, WGPUTextureFormat format, WGPUTextureUsage usage,
  int mipLevelCount, const char* label /*=nullptr*/)
{
  WGPUTextureDescriptor textureDescriptor{};
  textureDescriptor.dimension = dimension;
  textureDescriptor.format = format;
  textureDescriptor.size = extents;
  textureDescriptor.mipLevelCount = mipLevelCount;
  textureDescriptor.nextInChain = nullptr;
  textureDescriptor.sampleCount = 1;
  textureDescriptor.usage = usage;
  textureDescriptor.viewFormatCount = 0;
  textureDescriptor.viewFormats = nullptr;
  textureDescriptor.label = WGPUStringView{ label, WGPU_STRLEN };
  return this->CreateTexture(textureDescriptor);
}

//------------------------------------------------------------------------------
WGPUTexture vtkWebGPUConfiguration::CreateTexture(const WGPUTextureDescriptor& textureDescriptor)
{
  auto& internals = (*this->Internals);
  if (!internals.DeviceReady)
  {
    vtkWarningMacro(<< "Cannot create texture because device is not ready.");
    return nullptr;
  }
  const auto label = vtkWebGPUStringViewToStdString(textureDescriptor.label);
  vtkVLog(this->GetGPUMemoryLogVerbosity(),
    "Create texture {label: \"" << label << "\", size: [" << textureDescriptor.size.width << ','
                                << textureDescriptor.size.height << ','
                                << textureDescriptor.size.depthOrArrayLayers << "]}");
  return wgpuDeviceCreateTexture(internals.Device, &textureDescriptor);
}

//------------------------------------------------------------------------------
WGPUTextureView vtkWebGPUConfiguration::CreateView(WGPUTexture texture,
  WGPUTextureViewDimension dimension, WGPUTextureAspect aspect, WGPUTextureFormat format,
  int baseMipLevel, int mipLevelCount, const char* label /*=nullptr*/)
{
  // Creating a "full" view of the texture
  WGPUTextureViewDescriptor textureViewDescriptor{};
  textureViewDescriptor.arrayLayerCount = 1;
  textureViewDescriptor.aspect = aspect;
  textureViewDescriptor.baseArrayLayer = 0;
  textureViewDescriptor.baseMipLevel = baseMipLevel;
  textureViewDescriptor.dimension = dimension;
  textureViewDescriptor.format = format;
  textureViewDescriptor.label = WGPUStringView{ label, WGPU_STRLEN };
  textureViewDescriptor.mipLevelCount = mipLevelCount;
  textureViewDescriptor.nextInChain = nullptr;

  return this->CreateView(texture, textureViewDescriptor);
}

//------------------------------------------------------------------------------
WGPUTextureView vtkWebGPUConfiguration::CreateView(
  WGPUTexture texture, const WGPUTextureViewDescriptor& viewDescriptor)
{
  auto& internals = (*this->Internals);
  if (!internals.DeviceReady)
  {
    vtkWarningMacro(<< "Cannot create texture because device is not ready.");
    return nullptr;
  }
  return wgpuTextureCreateView(texture, &viewDescriptor);
}

//------------------------------------------------------------------------------
void vtkWebGPUConfiguration::WriteTexture(WGPUTexture texture, uint32_t bytesPerRow,
  uint32_t sizeBytes, const void* data, uint32_t srcOffset /*=0*/,
  WGPUOrigin3D dstOffset /*={0, 0, 0}*/, uint32_t dstMipLevel /*= 0*/,
  const char* description /*= nullptr*/)
{
  auto& internals = (*this->Internals);
  if (!internals.DeviceReady)
  {
    vtkWarningMacro(<< "Cannot write data into texture because device is not ready.");
    return;
  }
  const auto copyTexture =
    vtkWebGPUTextureInternals::GetTexelCopyTextureInfo(texture, dstOffset, dstMipLevel);

  const auto textureDataLayout =
    vtkWebGPUTextureInternals::GetDataLayout(texture, bytesPerRow, srcOffset);

  // Compute the number of layers to copy from the data size rather than the full texture depth.
  // This ensures individual array layer writes (e.g. cube map faces) copy only 1 layer.
  const uint32_t rowsPerImage = wgpuTextureGetHeight(texture);
  const uint32_t layerSizeBytes = bytesPerRow * rowsPerImage;
  const uint32_t depthOrArrayLayers =
    layerSizeBytes > 0 ? std::max(1u, sizeBytes / layerSizeBytes) : 1;
  WGPUExtent3D textureExtents{ wgpuTextureGetWidth(texture), wgpuTextureGetHeight(texture),
    depthOrArrayLayers };
  vtkVLog(this->GetGPUMemoryLogVerbosity(),
    "Write texture {description: \"" << (description ? description : "null")
                                     << "\", size: " << sizeBytes << "}");
  WGPUQueue queue = wgpuDeviceGetQueue(internals.Device);
  wgpuQueueWriteTexture(queue, &copyTexture, data, sizeBytes, &textureDataLayout, &textureExtents);
  wgpuQueueRelease(queue);
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
  // dawn::native::DumpMemoryStatistics requires a dawn::native::MemoryDump* subclass,
  // but deriving from that type is incompatible with VTK's -fvisibility=hidden build
  // because libwebgpu_dawn.so does not export the typeinfo for MemoryDump.
  vtkVLog(this->GetGPUMemoryLogVerbosity(),
    "Cannot determine memory statistics for allocated webgpu objects in this webgpu "
    "implementation");
}
VTK_ABI_NAMESPACE_END
