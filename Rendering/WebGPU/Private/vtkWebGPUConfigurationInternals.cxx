// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPUConfigurationInternals.h"

VTK_ABI_NAMESPACE_BEGIN

double vtkWebGPUConfigurationInternals::DefaultTimeout = 60000;

WGPUInstance vtkWebGPUConfigurationInternals::Instance = nullptr;

std::size_t vtkWebGPUConfigurationInternals::InstanceCount = 0;

//------------------------------------------------------------------------------
vtkWebGPUConfigurationInternals::~vtkWebGPUConfigurationInternals()
{
  if (this->Device != nullptr)
  {
    wgpuDeviceRelease(this->Device);
    this->Device = nullptr;
  }
  if (this->Adapter != nullptr)
  {
    wgpuAdapterRelease(this->Adapter);
    this->Adapter = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUConfigurationInternals::AddInstanceRef()
{
  if (InstanceCount == 0)
  {
    // Create the instance to allow WaitAny.
    WGPUInstanceDescriptor instanceDescriptor = WGPU_INSTANCE_DESCRIPTOR_INIT;
    std::vector<WGPUInstanceFeatureName> features = {
      WGPUInstanceFeatureName_TimedWaitAny,
    };
    instanceDescriptor.requiredFeatures = features.data();
    instanceDescriptor.requiredFeatureCount = features.size();
    Instance = wgpuCreateInstance(&instanceDescriptor);
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
  if (InstanceCount == 0 && Instance != nullptr)
  {
    wgpuInstanceRelease(Instance);
    Instance = nullptr;
  }
}

//------------------------------------------------------------------------------
WGPUBackendType vtkWebGPUConfigurationInternals::ToWGPUBackendType(
  vtkWebGPUConfiguration::BackendType backend)
{
  switch (backend)
  {
    case vtkWebGPUConfiguration::BackendType::Null:
      return WGPUBackendType_Null;
    case vtkWebGPUConfiguration::BackendType::WebGPU:
      return WGPUBackendType_WebGPU;
    case vtkWebGPUConfiguration::BackendType::D3D11:
      return WGPUBackendType_D3D11;
    case vtkWebGPUConfiguration::BackendType::D3D12:
      return WGPUBackendType_D3D12;
    case vtkWebGPUConfiguration::BackendType::Metal:
      return WGPUBackendType_Metal;
    case vtkWebGPUConfiguration::BackendType::Vulkan:
      return WGPUBackendType_Vulkan;
    case vtkWebGPUConfiguration::BackendType::OpenGL:
      return WGPUBackendType_OpenGL;
    case vtkWebGPUConfiguration::BackendType::OpenGLES:
      return WGPUBackendType_OpenGLES;
    case vtkWebGPUConfiguration::BackendType::Undefined:
    default:
      return WGPUBackendType_Undefined;
  }
}

//------------------------------------------------------------------------------
vtkWebGPUConfiguration::BackendType vtkWebGPUConfigurationInternals::FromWGPUBackendType(
  WGPUBackendType backend)
{
  switch (backend)
  {
    case WGPUBackendType_Null:
      return vtkWebGPUConfiguration::BackendType::Null;
    case WGPUBackendType_WebGPU:
      return vtkWebGPUConfiguration::BackendType::WebGPU;
    case WGPUBackendType_D3D11:
      return vtkWebGPUConfiguration::BackendType::D3D11;
    case WGPUBackendType_D3D12:
      return vtkWebGPUConfiguration::BackendType::D3D12;
    case WGPUBackendType_Metal:
      return vtkWebGPUConfiguration::BackendType::Metal;
    case WGPUBackendType_Vulkan:
      return vtkWebGPUConfiguration::BackendType::Vulkan;
    case WGPUBackendType_OpenGL:
      return vtkWebGPUConfiguration::BackendType::OpenGL;
    case WGPUBackendType_OpenGLES:
      return vtkWebGPUConfiguration::BackendType::OpenGLES;
    case WGPUBackendType_Undefined:
    default:
      return vtkWebGPUConfiguration::BackendType::Undefined;
  }
}

//------------------------------------------------------------------------------
WGPUPowerPreference vtkWebGPUConfigurationInternals::ToWGPUPowerPreferenceType(
  vtkWebGPUConfiguration::PowerPreferenceType powerPreference)
{
  switch (powerPreference)
  {
    case vtkWebGPUConfiguration::PowerPreferenceType::LowPower:
      return WGPUPowerPreference_LowPower;
    case vtkWebGPUConfiguration::PowerPreferenceType::HighPerformance:
      return WGPUPowerPreference_HighPerformance;
    case vtkWebGPUConfiguration::PowerPreferenceType::Undefined:
    default:
      return WGPUPowerPreference_Undefined;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUConfigurationInternals::PopulateRequiredLimits(WGPUAdapter adapter)
{
  RequiredLimits.nextInChain = nullptr;

  WGPULimits supportedLimits = WGPU_LIMITS_INIT;
  wgpuAdapterGetLimits(adapter, &supportedLimits);

  RequiredLimits.maxStorageBufferBindingSize = supportedLimits.maxStorageBufferBindingSize;
  RequiredLimits.maxBufferSize = supportedLimits.maxBufferSize;
  RequiredLimits.maxStorageBuffersPerShaderStage = supportedLimits.maxStorageBuffersPerShaderStage;
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
  RequiredFeatures.push_back(WGPUFeatureName_BGRA8UnormStorage);
  // Required for bilinear filtering of float32 textures (e.g. HDR environment maps used by
  // skybox rendering).
  RequiredFeatures.push_back(WGPUFeatureName_Float32Filterable);
}
VTK_ABI_NAMESPACE_END
