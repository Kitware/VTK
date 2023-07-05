// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPUInternalsBuffer.h"

//------------------------------------------------------------------------------
wgpu::Buffer vtkWebGPUInternalsBuffer::Upload(const wgpu::Device& device, unsigned long offset,
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

wgpu::Buffer vtkWebGPUInternalsBuffer::CreateABuffer(const wgpu::Device& device,
  unsigned long sizeBytes, wgpu::BufferUsage usage, bool mappedAtCreation /*=false*/,
  const char* label /*=nullptr*/)
{
  wgpu::BufferDescriptor descriptor;
  descriptor.label = label == nullptr ? "(nolabel)" : label;
  descriptor.size = sizeBytes;
  descriptor.usage = usage;
  descriptor.mappedAtCreation = mappedAtCreation;

  wgpu::Buffer buffer = device.CreateBuffer(&descriptor);
  return buffer;
}
