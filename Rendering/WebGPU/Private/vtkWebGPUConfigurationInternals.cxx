// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPUConfigurationInternals.h"

VTK_ABI_NAMESPACE_BEGIN

double vtkWebGPUConfigurationInternals::DefaultTimeout = 60000;

wgpu::Instance vtkWebGPUConfigurationInternals::Instance = nullptr;

std::size_t vtkWebGPUConfigurationInternals::InstanceCount = 0;

//------------------------------------------------------------------------------
void vtkWebGPUConfigurationInternals::AddInstanceRef()
{
  if (InstanceCount == 0)
  {
    Instance = wgpu::CreateInstance();
  }
  ++InstanceCount;
}

//------------------------------------------------------------------------------
void vtkWebGPUConfigurationInternals::ReleaseInstanceRef()
{
  if (InstanceCount > 0)
  {
    --InstanceCount;
  }
  if (InstanceCount == 0)
  {
    Instance = nullptr;
  }
}

//------------------------------------------------------------------------------
wgpu::BackendType vtkWebGPUConfigurationInternals::ToWGPUBackendType(
  vtkWebGPUConfiguration::BackendType backend)
{
  switch (backend)
  {
    case vtkWebGPUConfiguration::BackendType::Null:
      return wgpu::BackendType::Null;
    case vtkWebGPUConfiguration::BackendType::WebGPU:
      return wgpu::BackendType::WebGPU;
    case vtkWebGPUConfiguration::BackendType::D3D11:
      return wgpu::BackendType::D3D11;
    case vtkWebGPUConfiguration::BackendType::D3D12:
      return wgpu::BackendType::D3D12;
    case vtkWebGPUConfiguration::BackendType::Metal:
      return wgpu::BackendType::Metal;
    case vtkWebGPUConfiguration::BackendType::Vulkan:
      return wgpu::BackendType::Vulkan;
    case vtkWebGPUConfiguration::BackendType::OpenGL:
      return wgpu::BackendType::OpenGL;
    case vtkWebGPUConfiguration::BackendType::OpenGLES:
      return wgpu::BackendType::OpenGLES;
    case vtkWebGPUConfiguration::BackendType::Undefined:
    default:
      return wgpu::BackendType::Undefined;
  }
}

//------------------------------------------------------------------------------
vtkWebGPUConfiguration::BackendType vtkWebGPUConfigurationInternals::FromWGPUBackendType(
  wgpu::BackendType backend)
{
  switch (backend)
  {
    case wgpu::BackendType::Null:
      return vtkWebGPUConfiguration::BackendType::Null;
    case wgpu::BackendType::WebGPU:
      return vtkWebGPUConfiguration::BackendType::WebGPU;
    case wgpu::BackendType::D3D11:
      return vtkWebGPUConfiguration::BackendType::D3D11;
    case wgpu::BackendType::D3D12:
      return vtkWebGPUConfiguration::BackendType::D3D12;
    case wgpu::BackendType::Metal:
      return vtkWebGPUConfiguration::BackendType::Metal;
    case wgpu::BackendType::Vulkan:
      return vtkWebGPUConfiguration::BackendType::Vulkan;
    case wgpu::BackendType::OpenGL:
      return vtkWebGPUConfiguration::BackendType::OpenGL;
    case wgpu::BackendType::OpenGLES:
      return vtkWebGPUConfiguration::BackendType::OpenGLES;
    case wgpu::BackendType::Undefined:
    default:
      return vtkWebGPUConfiguration::BackendType::Undefined;
  }
}

//------------------------------------------------------------------------------
wgpu::PowerPreference vtkWebGPUConfigurationInternals::ToWGPUPowerPreferenceType(
  vtkWebGPUConfiguration::PowerPreferenceType powerPreference)
{
  switch (powerPreference)
  {
    case vtkWebGPUConfiguration::PowerPreferenceType::LowPower:
      return wgpu::PowerPreference::LowPower;
    case vtkWebGPUConfiguration::PowerPreferenceType::HighPerformance:
      return wgpu::PowerPreference::HighPerformance;
    case vtkWebGPUConfiguration::PowerPreferenceType::Undefined:
    default:
      return wgpu::PowerPreference::Undefined;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUConfigurationInternals::OnAdapterRequestCompleted(
  WGPURequestAdapterStatus status, WGPUAdapter cAdapter, const char* message, void* userdata)
{
  auto* self = reinterpret_cast<vtkWebGPUConfiguration*>(userdata);
  if (self == nullptr)
  {
    vtkErrorWithObjectMacro(nullptr, "OnAdapterRequestCompleted callback received null userdata!");
    return;
  }
  vtkWarningWithObjectMacro(self, << "Adapter request completed");
  switch (status)
  {
    case WGPURequestAdapterStatus_Success:
      self->InvokeEvent(vtkWebGPUConfiguration::AdapterRequestCompletedEvent, cAdapter);
      break;
#ifndef __EMSCRIPTEN__
      // XXX(emwebgpu-update) Remove this ifdef after emscripten's webgpu.h catches up.
    case WGPURequestAdapterStatus_InstanceDropped:
      vtkWarningWithObjectMacro(self, << "Adapter request completed with status InstanceDropped!");
      self->InvokeEvent(vtkWebGPUConfiguration::AdapterRequestCompletedEvent, nullptr);
      break;
#endif
    case WGPURequestAdapterStatus_Unavailable:
      vtkWarningWithObjectMacro(self, << "Adapter request completed with status Unavailable!");
      self->InvokeEvent(vtkWebGPUConfiguration::AdapterRequestCompletedEvent, nullptr);
      break;
    case WGPURequestAdapterStatus_Error:
      vtkErrorWithObjectMacro(self, << "Error occured in wgpu::Instance::RequestAdapter");
      self->InvokeEvent(vtkWebGPUConfiguration::AdapterRequestCompletedEvent, nullptr);
      break;
    case WGPURequestAdapterStatus_Unknown:
    default:
      vtkWarningWithObjectMacro(self, << "Adapter request completed with status Unknown!");
      self->InvokeEvent(vtkWebGPUConfiguration::AdapterRequestCompletedEvent, nullptr);
      break;
  }
  if (message)
  {
    vtkWarningWithObjectMacro(self, << message);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUConfigurationInternals::PopulateRequiredLimits(wgpu::Adapter adapter)
{
  RequiredLimits.nextInChain = nullptr;

  wgpu::SupportedLimits supportedLimits;
  adapter.GetLimits(&supportedLimits);

  RequiredLimits.limits.maxStorageBufferBindingSize =
    supportedLimits.limits.maxStorageBufferBindingSize;
  RequiredLimits.limits.maxBufferSize = supportedLimits.limits.maxBufferSize;
}

//------------------------------------------------------------------------------
void vtkWebGPUConfigurationInternals::PopulateRequiredFeatures()
{
  // Required feature for writing to the BGRA8 framebuffer of the render window from a compute
  // shader (used by the point the cloud renderer which needs to write the point color to the
  // framebuffer of the render window from its compute shader)
  //
  // Only ~50% of devices support this extension according to:
  // http://vulkan.gpuinfo.org/listoptimaltilingformats.php
  // CTRL+F "B8G8R8A8_UNORM"
  RequiredFeatures.push_back(wgpu::FeatureName::BGRA8UnormStorage);
}

//------------------------------------------------------------------------------
void vtkWebGPUConfigurationInternals::OnDeviceRequestCompleted(
  WGPURequestDeviceStatus status, WGPUDevice cDevice, const char* message, void* userdata)
{
  auto* self = reinterpret_cast<vtkWebGPUConfiguration*>(userdata);
  if (self == nullptr)
  {
    vtkErrorWithObjectMacro(nullptr, "OnDeviceRequestCompleted callback received null userdata!");
    return;
  }
  vtkWarningWithObjectMacro(self, << "Device request completed");
  switch (status)
  {
    case WGPURequestDeviceStatus_Success:
      self->InvokeEvent(vtkWebGPUConfiguration::DeviceRequestCompletedEvent, cDevice);
      break;
#ifndef __EMSCRIPTEN__
      // XXX(emwebgpu-update) Remove this ifdef after emscripten's webgpu.h catches up.
    case WGPURequestDeviceStatus_InstanceDropped:
      vtkWarningWithObjectMacro(self, << "Device request completed with status InstanceDropped!");
      self->InvokeEvent(vtkWebGPUConfiguration::DeviceRequestCompletedEvent, cDevice);
      break;
#endif
    case WGPURequestDeviceStatus_Error:
      vtkErrorWithObjectMacro(self, << "Error occured in wgpu::Adapter::RequestDevice");
      self->InvokeEvent(vtkWebGPUConfiguration::DeviceRequestCompletedEvent, cDevice);
      break;
    case WGPURequestDeviceStatus_Unknown:
    default:
      vtkWarningWithObjectMacro(self, << "Device request completed with status Unknown!");
      self->InvokeEvent(vtkWebGPUConfiguration::DeviceRequestCompletedEvent, cDevice);
      break;
  }
  if (message)
  {
    vtkWarningWithObjectMacro(self, << message);
  }
}
VTK_ABI_NAMESPACE_END
