// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) VTK Project Contributors
// See VTK Copyright.txt or https://www.kitware.com/Copyright.htm for details.

#ifndef vtkWebGPUHandleUtils_h
#define vtkWebGPUHandleUtils_h

// This is a PRIVATE header - NOT part of the public API
// It provides conversion between opaque handles and actual wgpu:: types

#include "vtkWebGPUHandle.h"
#include "vtk_wgpu.h"

namespace vtkWebGPU
{
// Internal conversion utilities (implementation file should specialize these)
inline wgpu::Device GetDevice(DeviceHandle handle);
inline wgpu::Adapter GetAdapter(AdapterHandle handle);
inline wgpu::Instance GetInstance(InstanceHandle handle);
inline wgpu::Buffer GetBuffer(BufferHandle handle);
inline wgpu::Texture GetTexture(TextureHandle handle);
inline wgpu::TextureView GetTextureView(TextureViewHandle handle);
inline wgpu::RenderPipeline GetRenderPipeline(RenderPipelineHandle handle);
inline wgpu::CommandEncoder GetCommandEncoder(CommandEncoderHandle handle);
inline wgpu::RenderPassEncoder GetRenderPassEncoder(RenderPassEncoderHandle handle);

// Reverse conversions (wgpu:: to opaque handle)
inline DeviceHandle WrapDevice(wgpu::Device device);
inline AdapterHandle WrapAdapter(wgpu::Adapter adapter);
inline InstanceHandle WrapInstance(wgpu::Instance instance);
inline BufferHandle WrapBuffer(wgpu::Buffer buffer);
inline TextureHandle WrapTexture(wgpu::Texture texture);
inline TextureViewHandle WrapTextureView(wgpu::TextureView view);

} // namespace vtkWebGPU

#endif // vtkWebGPUHandleUtils_h
