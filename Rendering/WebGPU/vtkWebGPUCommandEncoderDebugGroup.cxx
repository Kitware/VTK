// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUCommandEncoderDebugGroup.h"
#include "vtk_wgpu_impl.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkWebGPUCommandEncoderDebugGroup::vtkWebGPUCommandEncoderDebugGroup(
  const WGPURenderPassEncoder& passEncoder, const char* groupLabel)
  : PassEncoder(&passEncoder)
{
#if !defined(NDEBUG) && !defined(__EMSCRIPTEN__)
  wgpu::RenderPassEncoder wrappedEncoder(passEncoder);
  wrappedEncoder.PushDebugGroup(groupLabel);
#else
  (void)this->PassEncoder;
  (void)groupLabel;
#endif
}

//------------------------------------------------------------------------------
vtkWebGPUCommandEncoderDebugGroup::vtkWebGPUCommandEncoderDebugGroup(
  const WGPURenderBundleEncoder& bundleEncoder, const char* groupLabel)
  : BundleEncoder(&bundleEncoder)
{
#if !defined(NDEBUG) && !defined(__EMSCRIPTEN__)
  wgpu::RenderBundleEncoder wrappedEncoder(bundleEncoder);
  wrappedEncoder.PushDebugGroup(groupLabel);
#else
  (void)this->BundleEncoder;
  (void)groupLabel;
#endif
}

//------------------------------------------------------------------------------
vtkWebGPUCommandEncoderDebugGroup::vtkWebGPUCommandEncoderDebugGroup(
  const WGPUCommandEncoder& commandEncoder, const char* groupLabel)
  : CommandEncoder(&commandEncoder)
{
#if !defined(NDEBUG) && !defined(__EMSCRIPTEN__)
  wgpu::CommandEncoder wrappedEncoder(commandEncoder);
  wrappedEncoder.PushDebugGroup(groupLabel);
#else
  (void)this->CommandEncoder;
  (void)groupLabel;
#endif
}

//------------------------------------------------------------------------------
#if !defined(NDEBUG) && !defined(__EMSCRIPTEN__)
vtkWebGPUCommandEncoderDebugGroup::~vtkWebGPUCommandEncoderDebugGroup()
{
  if (this->PassEncoder)
  {
    wgpu::RenderPassEncoder wrappedPassEncoder(*this->PassEncoder);
    wrappedPassEncoder.PopDebugGroup();
  }
  if (this->BundleEncoder)
  {
    wgpu::RenderBundleEncoder wrappedBundleEncoder(*this->BundleEncoder);
    wrappedBundleEncoder.PopDebugGroup();
  }
  if (this->CommandEncoder)
  {
    wgpu::CommandEncoder wrappedCommandEncoder(*this->CommandEncoder);
    wrappedCommandEncoder.PopDebugGroup();
  }
}
#else
vtkWebGPUCommandEncoderDebugGroup::~vtkWebGPUCommandEncoderDebugGroup() = default;
#endif

VTK_ABI_NAMESPACE_END
