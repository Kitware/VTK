// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUConfiguration.h"
#include "Private/vtkWebGPUCallbacksInternals.h"
#include "Private/vtkWebGPUConfigurationInternals.h"
#include "vtkObjectFactory.h"
#include "vtkWebGPURenderWindow.h"
#include "vtksys/SystemInformation.hxx"

#include <chrono>
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
     << "maxInterStageShaderComponents: " << FormatNumber(limits.maxInterStageShaderComponents)
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
  auto feature_count = adapter.EnumerateFeatures(nullptr);
  std::vector<wgpu::FeatureName> features(feature_count);
  adapter.EnumerateFeatures(features.data());
  os << indent << "Features\n";
  os << indent << "========\n";
  for (const auto& f : features)
  {
#if VTK_USE_DAWN_WEBGPU
    auto info = dawn::native::GetFeatureInfo(f);
    os << indent << "   * " << info->name << '\n';
    os << indent << info->description << '\n';
    os << indent << "      " << info->url << '\n';
#else
    // Look up the list of feature strings in `WebGPU.FeatureName`
    const auto featureIdx = static_cast<std::underlying_type<wgpu::FeatureName>::type>(f);
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
  wgpu::SupportedLimits adapterLimits;
  if (adapter.GetLimits(&adapterLimits))
  {
    os << indent << '\n';
    os << indent << "Adapter Limits\n";
    os << indent << "==============\n";
    PrintLimits(os, indent.GetNextIndent(), adapterLimits.limits);
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
  // Install adapter request completion callback
  this->AddObserver(vtkWebGPUConfiguration::AdapterRequestCompletedEvent, this,
    &vtkWebGPUConfiguration::AcquireAdapter);
  // Install device request completion callback
  this->AddObserver(vtkWebGPUConfiguration::DeviceRequestCompletedEvent, this,
    &vtkWebGPUConfiguration::AcquireDevice);
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
void vtkWebGPUConfiguration::AcquireAdapter(
  vtkObject* vtkNotUsed(caller), unsigned long event, void* calldata)
{
  vtkDebugMacro(<< __func__);
  if (event != vtkWebGPUConfiguration::AdapterRequestCompletedEvent)
  {
    return;
  }
  auto& internals = (*this->Internals);
  // Fail on purpose if adapter is null.
  if (calldata == nullptr)
  {
    internals.DeviceReady = false;
    this->InvokeEvent(
      vtkWebGPUConfiguration::DeviceRequestCompletedEvent, &(internals.DeviceReady));
  }
  else
  {
    const std::string label = this->GetObjectDescription();
    auto cAdapter = reinterpret_cast<WGPUAdapter>(calldata);
    internals.Adapter = wgpu::Adapter::Acquire(cAdapter);

    wgpu::DeviceDescriptor opts = {};
    opts.label = label.c_str();
    opts.defaultQueue.nextInChain = nullptr;
    opts.defaultQueue.label = label.c_str();
#if defined(__EMSCRIPTEN__)
    // XXX(emwebgpu-update) Remove this ifdef after emscripten's webgpu.h catches up.
    opts.deviceLostCallback = [](WGPUDeviceLostReason reason, char const* message, void* userdata)
    { return vtkWebGPUCallbacksInternals::DeviceLostCallback(nullptr, reason, message, userdata); };
    opts.deviceLostUserdata = nullptr;
#else
    opts.deviceLostCallbackInfo.nextInChain = nullptr;
    opts.deviceLostCallbackInfo.callback = &vtkWebGPUCallbacksInternals::DeviceLostCallback;
    opts.deviceLostCallbackInfo.userdata = nullptr;
    opts.uncapturedErrorCallbackInfo.nextInChain = nullptr;
    opts.uncapturedErrorCallbackInfo.callback =
      &vtkWebGPUCallbacksInternals::UncapturedErrorCallback;
    opts.uncapturedErrorCallbackInfo.userdata = nullptr;
#endif
    // Populating limits of the device
    internals.PopulateRequiredLimits(internals.Adapter);
    opts.requiredLimits = &internals.RequiredLimits;

    // Populating required features of the device
    internals.PopulateRequiredFeatures();

    opts.requiredFeatureCount = internals.RequiredFeatures.size();
    opts.requiredFeatures = internals.RequiredFeatures.data();

    internals.Adapter.RequestDevice(
      &opts, vtkWebGPUConfigurationInternals::OnDeviceRequestCompleted, this);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUConfiguration::AcquireDevice(
  vtkObject* vtkNotUsed(caller), unsigned long event, void* calldata)
{
  vtkDebugMacro(<< __func__);
  if (event != vtkWebGPUConfiguration::DeviceRequestCompletedEvent)
  {
    return;
  }
  auto& internals = (*this->Internals);
  // Fail on purpose if device is null.
  if (calldata == nullptr)
  {
    internals.DeviceReady = false;
  }
  else
  {
    internals.DeviceReady = true;
    auto cDevice = reinterpret_cast<WGPUDevice>(calldata);
    internals.Device = wgpu::Device::Acquire(cDevice);
#ifdef __EMSCRIPTEN__
    // XXX(emwebgpu-update) Remove this ifdef after emscripten's webgpu.h catches up.
    internals.Device.SetUncapturedErrorCallback(
      &vtkWebGPUCallbacksInternals::UncapturedErrorCallback, nullptr);
#endif
  }
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

  wgpu::RequestAdapterOptions options;
  options.backendType = internals.ToWGPUBackendType(this->Backend);
  options.powerPreference = internals.ToWGPUPowerPreferenceType(this->PowerPreference);

#if defined(__EMSCRIPTEN__)
  vtkWebGPUConfigurationInternals::Instance.RequestAdapter(
    &options, vtkWebGPUConfigurationInternals::OnAdapterRequestCompleted, this);
#else
  wgpu::RequestAdapterCallbackInfo adapterCbInfo;
  adapterCbInfo.nextInChain = nullptr;
  adapterCbInfo.callback = vtkWebGPUConfigurationInternals::OnAdapterRequestCompleted;
  adapterCbInfo.mode = wgpu::CallbackMode::AllowProcessEvents;
  adapterCbInfo.userdata = this;
  vtkWebGPUConfigurationInternals::Instance.RequestAdapter(&options, adapterCbInfo);
#endif
  double elapsed = 0;
  while (!internals.DeviceReady)
  {
    const auto start = std::chrono::steady_clock::now();
    vtkDebugMacro(<< "Wait for device initalization ... (" << elapsed << "ms)");
    this->ProcessEvents();
    const auto end = std::chrono::steady_clock::now();
    elapsed += std::chrono::duration<double, std::milli>(end - start).count();
    if (elapsed >= this->Timeout)
    {
      vtkErrorMacro(<< "Request for a WebGPU device timed out!");
      break;
    }
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
  wgpu::AdapterProperties properties = {};
  internals.Adapter.GetProperties(&properties);
  return internals.FromWGPUBackendType(properties.backendType);
}

//------------------------------------------------------------------------------
std::string vtkWebGPUConfiguration::GetBackendInUseAsString()
{
  auto& internals = (*this->Internals);
  if (internals.DeviceReady)
  {
    wgpu::AdapterProperties properties = {};
    internals.Adapter.GetProperties(&properties);
    switch (properties.backendType)
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

VTK_ABI_NAMESPACE_END
