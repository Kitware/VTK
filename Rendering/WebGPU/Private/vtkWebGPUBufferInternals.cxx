// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPUBufferInternals.h"

//------------------------------------------------------------------------------
bool vtkWebGPUBufferInternals::CheckBufferSize(const wgpu::Device& device, unsigned long sizeBytes)
{
  wgpu::Limits supportedDeviceLimits;
  device.GetLimits(&supportedDeviceLimits);

  return !(sizeBytes > supportedDeviceLimits.maxStorageBufferBindingSize);
}
