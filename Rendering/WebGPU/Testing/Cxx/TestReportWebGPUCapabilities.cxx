// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkNew.h"
#include "vtkWebGPUConfiguration.h"

#include <cstdlib>

/**
 * This test is to give an insight into the adapter that is used to run the rest of the webgpu test
 * suite.
 */
int TestReportWebGPUCapabilities(int, char*[])
{
  vtkNew<vtkWebGPUConfiguration> wgpuConfiguration;
  wgpuConfiguration->Initialize();
  const auto capabilities = wgpuConfiguration->ReportCapabilities();
  std::cout << capabilities << '\n';
  return capabilities == vtkWebGPUConfiguration::DeviceNotReadyMessage() ? EXIT_FAILURE
                                                                         : EXIT_SUCCESS;
}
