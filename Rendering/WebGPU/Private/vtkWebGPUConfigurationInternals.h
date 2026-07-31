// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUConfigurationInternals_h
#define vtkWebGPUConfigurationInternals_h

#include "vtkWebGPUConfiguration.h"
#include "vtk_wgpu.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class vtkWebGPUConfigurationInternals
{
public:
  ~vtkWebGPUConfigurationInternals();

  WGPUAdapter Adapter = nullptr;
  WGPUDevice Device = nullptr;
  bool DeviceReady = false;
  bool Timedout = false;

  // in milliseconds
  static double DefaultTimeout;
  // We only keep one webgpu Instance around.
  static WGPUInstance Instance;
  // Helps clean up the instance after it is no longer needed.
  static std::size_t InstanceCount;

  WGPULimits RequiredLimits = WGPU_LIMITS_INIT;
  std::vector<WGPUFeatureName> RequiredFeatures;

  static void AddInstanceRef();

  static void ReleaseInstanceRef();

  static WGPUBackendType ToWGPUBackendType(vtkWebGPUConfiguration::BackendType backend);

  static vtkWebGPUConfiguration::BackendType FromWGPUBackendType(WGPUBackendType backend);

  static WGPUPowerPreference ToWGPUPowerPreferenceType(
    vtkWebGPUConfiguration::PowerPreferenceType powerPreference);

  /**
   * Stores the required limits needed for querying the device in the RequiredLimits attribute of
   * this ConfigurationInternals
   */
  void PopulateRequiredLimits(WGPUAdapter adapter);

  /**
   * Stores the required features for querying the device in the RequiredFeatures vector of this
   * ConfigurationInternals
   */
  void PopulateRequiredFeatures();
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPUConfigurationInternals.h
