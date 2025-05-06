// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUCommandEncoderDebugGroup.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkWebGPUCommandEncoderDebugGroup::vtkWebGPUCommandEncoderDebugGroup(
  const wgpu::RenderPassEncoder& passEncoder, const char* groupLabel)
  : PassEncoder(&passEncoder)
{
#if !defined(NDEBUG) && !defined(__EMSCRIPTEN__)
  this->PassEncoder->PushDebugGroup(groupLabel);
#else
  (void)this->PassEncoder;
  (void)groupLabel;
#endif
}

//------------------------------------------------------------------------------
vtkWebGPUCommandEncoderDebugGroup::vtkWebGPUCommandEncoderDebugGroup(
  const wgpu::RenderBundleEncoder& bundleEncoder, const char* groupLabel)
  : BundleEncoder(&bundleEncoder)
{
#if !defined(NDEBUG) && !defined(__EMSCRIPTEN__)
  this->BundleEncoder->PushDebugGroup(groupLabel);
#else
  (void)this->BundleEncoder;
  (void)groupLabel;
#endif
}

//------------------------------------------------------------------------------
vtkWebGPUCommandEncoderDebugGroup::vtkWebGPUCommandEncoderDebugGroup(
  const wgpu::CommandEncoder& commandEncoder, const char* groupLabel)
  : CommandEncoder(&commandEncoder)
{
#if !defined(NDEBUG) && !defined(__EMSCRIPTEN__)
  this->CommandEncoder->PushDebugGroup(groupLabel);
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
    this->PassEncoder->PopDebugGroup();
  }
  if (this->BundleEncoder)
  {
    this->BundleEncoder->PopDebugGroup();
  }
  if (this->CommandEncoder)
  {
    this->CommandEncoder->PopDebugGroup();
  }
}
#else
vtkWebGPUCommandEncoderDebugGroup::~vtkWebGPUCommandEncoderDebugGroup() = default;
#endif

VTK_ABI_NAMESPACE_END
