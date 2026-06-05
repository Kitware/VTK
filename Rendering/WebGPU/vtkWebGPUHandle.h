// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) VTK Project Contributors
// See VTK Copyright.txt or https://www.kitware.com/Copyright.htm for details.

#ifndef vtkWebGPUHandle_h
#define vtkWebGPUHandle_h

#include "vtkRenderingWebGPUModule.h"

// Forward declarations - users of VTK don't need to know about Dawn/WebGPU internals
// These opaque pointer types wrap the actual wgpu:: C++ types
namespace vtkWebGPU
{
// Opaque handle types - defined in implementation, not exposed in headers
struct DeviceImpl;
struct AdapterImpl;
struct InstanceImpl;
struct BufferImpl;
struct TextureImpl;
struct TextureViewImpl;
struct RenderPipelineImpl;
struct ComputePipelineImpl;
struct BindGroupImpl;
struct BindGroupLayoutImpl;
struct CommandEncoderImpl;
struct RenderPassEncoderImpl;
struct RenderBundleEncoderImpl;

// Type-safe handle wrapper for opaque pointers
template <typename T>
class Handle
{
public:
  Handle()
    : impl(nullptr)
  {
  }
  Handle(T* ptr)
    : impl(ptr)
  {
  }

  T* Get() const { return impl; }
  operator bool() const { return impl != nullptr; }

private:
  T* impl;
};

// Common handle types
using DeviceHandle = Handle<DeviceImpl>;
using AdapterHandle = Handle<AdapterImpl>;
using InstanceHandle = Handle<InstanceImpl>;
using BufferHandle = Handle<BufferImpl>;
using TextureHandle = Handle<TextureImpl>;
using TextureViewHandle = Handle<TextureViewImpl>;
using RenderPipelineHandle = Handle<RenderPipelineImpl>;
using ComputePipelineHandle = Handle<ComputePipelineImpl>;
using BindGroupHandle = Handle<BindGroupImpl>;
using BindGroupLayoutHandle = Handle<BindGroupLayoutImpl>;
using CommandEncoderHandle = Handle<CommandEncoderImpl>;
using RenderPassEncoderHandle = Handle<RenderPassEncoderImpl>;
using RenderBundleEncoderHandle = Handle<RenderBundleEncoderImpl>;

} // namespace vtkWebGPU

#endif // vtkWebGPUHandle_h
