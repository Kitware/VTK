// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkWebGPUConfiguration.h"

#include "vtk_wgpu.h"

#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <string>

namespace
{
bool CheckLimit(const char* name, std::uint64_t adapter, std::uint64_t device, bool minimum = false)
{
  const bool restricted = minimum ? device > adapter : device < adapter;
  vtkLog(INFO, << "GPUINFO " << name << ": adapter=" << adapter << " device=" << device);
  if (restricted)
  {
    vtkLog(ERROR, << "GPUINFO Device limit is more restrictive than adapter limit: " << name);
  }
  return !restricted;
}
}

/**
 * Report the adapter used by the WebGPU suite and verify device-limit negotiation.
 * Software adapters are supported unless VTK_WEBGPU_REQUIRE_HARDWARE=1 is set.
 */
int TestReportWebGPUCapabilities(int, char*[])
{
  if (vtkLogger::GetCurrentVerbosityCutoff() < vtkLogger::VERBOSITY_INFO)
  {
    vtkLogger::SetStderrVerbosity(vtkLogger::VERBOSITY_INFO);
  }
  vtkLog(INFO, << "CTEST_FULL_OUTPUT");

  vtkNew<vtkWebGPUConfiguration> config;
  if (!config->Initialize())
  {
    return EXIT_FAILURE;
  }
  const auto capabilities = config->ReportCapabilities();
  if (capabilities == vtkWebGPUConfiguration::DeviceNotReadyMessage())
  {
    return EXIT_FAILURE;
  }
  std::istringstream report(capabilities);
  for (std::string line; std::getline(report, line);)
  {
    vtkLog(INFO, << "GPUINFO " << line);
  }

  WGPUAdapter adapter = config->GetAdapter();
  WGPUDevice device = config->GetDevice();
  WGPUAdapterInfo info = WGPU_ADAPTER_INFO_INIT;
  if (wgpuAdapterGetInfo(adapter, &info) != WGPUStatus_Success)
  {
    vtkLog(ERROR, << "GPUINFO Failed to query adapter identity");
    return EXIT_FAILURE;
  }
  const bool software =
    info.adapterType == WGPUAdapterType_CPU || info.backendType == WGPUBackendType_Null;
  const bool hardware = !software &&
    (info.adapterType == WGPUAdapterType_DiscreteGPU ||
      info.adapterType == WGPUAdapterType_IntegratedGPU);
  wgpuAdapterInfoFreeMembers(info);
  vtkLog(
    INFO, << "GPUINFO VERDICT: "
          << (hardware ? "HARDWARE_ADAPTER" : (software ? "SOFTWARE_ADAPTER" : "UNKNOWN_ADAPTER")));
  const char* requireHardware = std::getenv("VTK_WEBGPU_REQUIRE_HARDWARE");
  if (requireHardware && requireHardware[0] == '1' && !hardware)
  {
    vtkLog(ERROR, << "GPUINFO VTK_WEBGPU_REQUIRE_HARDWARE=1 but no hardware adapter was found");
    return EXIT_FAILURE;
  }

  WGPUSupportedFeatures features = WGPU_SUPPORTED_FEATURES_INIT;
  wgpuAdapterGetFeatures(adapter, &features);
  for (std::size_t i = 0; i < features.featureCount; ++i)
  {
    vtkLog(INFO, << "GPUINFO Feature " << static_cast<std::uint32_t>(features.features[i])
                 << " enabled=" << wgpuDeviceHasFeature(device, features.features[i]));
  }
  wgpuSupportedFeaturesFreeMembers(features);

  WGPULimits adapterLimits = WGPU_LIMITS_INIT;
  WGPULimits deviceLimits = WGPU_LIMITS_INIT;
  if (wgpuAdapterGetLimits(adapter, &adapterLimits) != WGPUStatus_Success ||
    wgpuDeviceGetLimits(device, &deviceLimits) != WGPUStatus_Success)
  {
    vtkLog(ERROR, << "GPUINFO Failed to query adapter or device limits");
    return EXIT_FAILURE;
  }

  bool limitsMatch = true;
#define VTK_CHECK_LIMIT(field)                                                                     \
  limitsMatch &= CheckLimit(#field, adapterLimits.field, deviceLimits.field)
  VTK_CHECK_LIMIT(maxTextureDimension1D);
  VTK_CHECK_LIMIT(maxTextureDimension2D);
  VTK_CHECK_LIMIT(maxTextureDimension3D);
  VTK_CHECK_LIMIT(maxTextureArrayLayers);
  VTK_CHECK_LIMIT(maxBindGroups);
  VTK_CHECK_LIMIT(maxBindGroupsPlusVertexBuffers);
  VTK_CHECK_LIMIT(maxBindingsPerBindGroup);
  VTK_CHECK_LIMIT(maxDynamicUniformBuffersPerPipelineLayout);
  VTK_CHECK_LIMIT(maxDynamicStorageBuffersPerPipelineLayout);
  VTK_CHECK_LIMIT(maxSampledTexturesPerShaderStage);
  VTK_CHECK_LIMIT(maxSamplersPerShaderStage);
  VTK_CHECK_LIMIT(maxStorageBuffersPerShaderStage);
  VTK_CHECK_LIMIT(maxStorageTexturesPerShaderStage);
  VTK_CHECK_LIMIT(maxUniformBuffersPerShaderStage);
  VTK_CHECK_LIMIT(maxUniformBufferBindingSize);
  VTK_CHECK_LIMIT(maxStorageBufferBindingSize);
  limitsMatch &=
    CheckLimit("minUniformBufferOffsetAlignment", adapterLimits.minUniformBufferOffsetAlignment,
      deviceLimits.minUniformBufferOffsetAlignment, true);
  limitsMatch &=
    CheckLimit("minStorageBufferOffsetAlignment", adapterLimits.minStorageBufferOffsetAlignment,
      deviceLimits.minStorageBufferOffsetAlignment, true);
  VTK_CHECK_LIMIT(maxVertexBuffers);
  VTK_CHECK_LIMIT(maxBufferSize);
  VTK_CHECK_LIMIT(maxVertexAttributes);
  VTK_CHECK_LIMIT(maxVertexBufferArrayStride);
  VTK_CHECK_LIMIT(maxInterStageShaderVariables);
  VTK_CHECK_LIMIT(maxColorAttachments);
  VTK_CHECK_LIMIT(maxColorAttachmentBytesPerSample);
  VTK_CHECK_LIMIT(maxComputeWorkgroupStorageSize);
  VTK_CHECK_LIMIT(maxComputeInvocationsPerWorkgroup);
  VTK_CHECK_LIMIT(maxComputeWorkgroupSizeX);
  VTK_CHECK_LIMIT(maxComputeWorkgroupSizeY);
  VTK_CHECK_LIMIT(maxComputeWorkgroupSizeZ);
  VTK_CHECK_LIMIT(maxComputeWorkgroupsPerDimension);
#undef VTK_CHECK_LIMIT
  // maxImmediateSize requires an optional feature that VTK does not request.
  return limitsMatch ? EXIT_SUCCESS : EXIT_FAILURE;
}
