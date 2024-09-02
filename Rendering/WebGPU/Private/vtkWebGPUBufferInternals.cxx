// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPUBufferInternals.h"
#include "vtkLogger.h"

//------------------------------------------------------------------------------
wgpu::Buffer vtkWebGPUBufferInternals::Upload(const wgpu::Device& device, unsigned long offset,
  void* data, unsigned long sizeBytes, wgpu::BufferUsage usage, const char* label /*=nullptr*/)
{
  wgpu::BufferDescriptor descriptor;
  descriptor.label = label == nullptr ? "(nolabel)" : label;
  descriptor.size = sizeBytes;
  descriptor.usage = usage | wgpu::BufferUsage::CopyDst;

  wgpu::Buffer buffer = device.CreateBuffer(&descriptor);
  device.GetQueue().WriteBuffer(buffer, offset, data, sizeBytes);
  return buffer;
}

wgpu::Buffer vtkWebGPUBufferInternals::CreateBuffer(const wgpu::Device& device,
  unsigned long sizeBytes, wgpu::BufferUsage usage, bool mappedAtCreation /*=false*/,
  const char* label /*=nullptr*/)
{
  if (!vtkWebGPUBufferInternals::CheckBufferSize(device, sizeBytes))
  {
    wgpu::SupportedLimits supportedDeviceLimits;
    device.GetLimits(&supportedDeviceLimits);

    vtkLog(ERROR,
      "The current WebGPU Device cannot create buffers larger than: "
        << supportedDeviceLimits.limits.maxStorageBufferBindingSize
        << " bytes but the buffer with label " << (label ? label : "") << " is " << sizeBytes
        << " bytes big.");

    return nullptr;
  }

  wgpu::BufferDescriptor descriptor;
  descriptor.label = label == nullptr ? "(nolabel)" : label;
  descriptor.size = sizeBytes;
  descriptor.usage = usage;
  descriptor.mappedAtCreation = mappedAtCreation;

  wgpu::Buffer buffer = device.CreateBuffer(&descriptor);
  return buffer;
}

//------------------------------------------------------------------------------
bool vtkWebGPUBufferInternals::CheckBufferSize(const wgpu::Device& device, unsigned long sizeBytes)
{
  wgpu::SupportedLimits supportedDeviceLimits;
  device.GetLimits(&supportedDeviceLimits);

  return !(sizeBytes > supportedDeviceLimits.limits.maxStorageBufferBindingSize);
}
