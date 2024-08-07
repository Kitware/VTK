// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPUConfigurationInternals.h"
#include "Private/vtkWebGPUCallbacksInternals.h"

VTK_ABI_NAMESPACE_BEGIN

double vtkWebGPUConfigurationInternals::DefaultTimeout = 1000;

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
  auto* bridge = reinterpret_cast<vtkWebGPUConfigurationInternals::CallbackBridge*>(userdata);
  if (bridge == nullptr)
  {
    vtkErrorWithObjectMacro(nullptr, "OnAdapterRequestCompleted callback received null userdata!");
    return;
  }
  vtkDebugWithObjectMacro(bridge->VTKDevice, << "Adapter request completed");
  std::string label = "no label";
  if (bridge->VTKDevice)
  {
    label = bridge->VTKDevice->GetObjectDescription();
  }
  switch (status)
  {
    case WGPURequestAdapterStatus_Success:
    {
      bridge->Self->Adapter = wgpu::Adapter::Acquire(cAdapter);
      wgpu::DeviceDescriptor opts = {};
      opts.label = label.c_str();
      opts.defaultQueue.nextInChain = nullptr;
      opts.defaultQueue.label = label.c_str();
      opts.deviceLostCallbackInfo.nextInChain = nullptr;
      opts.deviceLostCallbackInfo.callback = &vtkWebGPUCallbacksInternals::DeviceLostCallback;
      opts.deviceLostCallbackInfo.userdata = nullptr;
      opts.uncapturedErrorCallbackInfo.nextInChain = nullptr;
      opts.uncapturedErrorCallbackInfo.callback =
        &vtkWebGPUCallbacksInternals::UncapturedErrorCallback;
      opts.uncapturedErrorCallbackInfo.userdata = nullptr;
      ///@{ TODO: Populate feature requests
      // ...
      ///@}
      ///@{ TODO: Populate limit requests
      // ...
      ///@}
      bridge->Self->Adapter.RequestDevice(&opts, OnDeviceRequestCompleted, bridge);
      break;
    }
    case WGPURequestAdapterStatus_InstanceDropped:
      break;
    case WGPURequestAdapterStatus_Unavailable:
      break;
    case WGPURequestAdapterStatus_Error:
      vtkErrorWithObjectMacro(
        bridge->VTKDevice, << "Error occured in wgpu::Instance::RequestAdapter");
      break;
    case WGPURequestAdapterStatus_Unknown:
      break;
    default:
      break;
  }
  if (message)
  {
    vtkWarningWithObjectMacro(bridge->VTKDevice, << message);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUConfigurationInternals::OnDeviceRequestCompleted(
  WGPURequestDeviceStatus status, WGPUDevice cDevice, const char* message, void* userdata)
{
  auto* bridge = reinterpret_cast<CallbackBridge*>(userdata);
  if (bridge == nullptr)
  {
    vtkErrorWithObjectMacro(nullptr, "OnAdapterRequestCompleted callback received null userdata!");
    return;
  }
  vtkDebugWithObjectMacro(bridge->VTKDevice, << "Device request completed");
  switch (status)
  {
    case WGPURequestDeviceStatus_Success:
      bridge->Self->Device = wgpu::Device::Acquire(cDevice);
      bridge->Self->DeviceReady = true;
      break;
    case WGPURequestDeviceStatus_InstanceDropped:
      break;
    case WGPURequestDeviceStatus_Error:
      vtkErrorWithObjectMacro(
        bridge->VTKDevice, << "Error occured in wgpu::Adapter::RequestDevice");
      break;
    case WGPURequestDeviceStatus_Unknown:
      break;
    default:
      break;
  }
  if (message)
  {
    vtkWarningWithObjectMacro(bridge->VTKDevice, << message);
  }
}
VTK_ABI_NAMESPACE_END
