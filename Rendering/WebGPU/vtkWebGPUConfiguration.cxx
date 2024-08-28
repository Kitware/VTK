// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUConfiguration.h"
#include "Private/vtkWebGPUCallbacksInternals.h"
#include "Private/vtkWebGPUConfigurationInternals.h"
#include "vtkObjectFactory.h"
#include "vtkWebGPURenderWindow.h"
#include "vtksys/SystemInformation.hxx"

#include <chrono>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

VTK_ABI_NAMESPACE_BEGIN

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

namespace
{
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
    opts.deviceLostCallback = [](WGPUDeviceLostReason reason, char const* message, void* userdata) {
      return vtkWebGPUCallbacksInternals::DeviceLostCallback(nullptr, reason, message, userdata);
    };
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

VTK_ABI_NAMESPACE_END
