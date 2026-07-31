// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPUBufferInternals.h"

//------------------------------------------------------------------------------
bool vtkWebGPUBufferInternals::CheckBufferSize(WGPUDevice device, unsigned long sizeBytes)
{
  WGPULimits supportedDeviceLimits = WGPU_LIMITS_INIT;
  wgpuDeviceGetLimits(device, &supportedDeviceLimits);

  return !(sizeBytes > supportedDeviceLimits.maxStorageBufferBindingSize);
}
