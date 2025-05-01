// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUConfigurationInternals_h
#define vtkWebGPUConfigurationInternals_h

#include "vtkWebGPUConfiguration.h"
#include "vtk_wgpu.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkWebGPUConfigurationInternals
{
public:
  wgpu::Adapter Adapter;
  wgpu::Device Device;
  bool DeviceReady = false;
  bool Timedout = false;

  // in milliseconds
  static double DefaultTimeout;
  // We only keep one webgpu Instance around.
  static wgpu::Instance Instance;
  // Helps clean up the instance after it is no longer needed.
  static std::size_t InstanceCount;

  wgpu::Limits RequiredLimits;
  std::vector<wgpu::FeatureName> RequiredFeatures;

  static void AddInstanceRef();

  static void ReleaseInstanceRef();

  static wgpu::BackendType ToWGPUBackendType(vtkWebGPUConfiguration::BackendType backend);

  static vtkWebGPUConfiguration::BackendType FromWGPUBackendType(wgpu::BackendType backend);

  static wgpu::PowerPreference ToWGPUPowerPreferenceType(
    vtkWebGPUConfiguration::PowerPreferenceType powerPreference);

  /**
   * Stores the required limits needed for querying the device in the RequiredLimits attribute of
   * this ConfigurationInternals
   */
  void PopulateRequiredLimits(wgpu::Adapter adapter);

  /**
   * Stores the required features for querying the device in the RequiredFeatures vector of this
   * ConfigurationInternals
   */
  void PopulateRequiredFeatures();
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPUConfigurationInternals.h
