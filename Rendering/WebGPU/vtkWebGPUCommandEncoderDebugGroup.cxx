// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUCommandEncoderDebugGroup.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkWebGPUCommandEncoderDebugGroup::vtkWebGPUCommandEncoderDebugGroup(
  const wgpu::RenderPassEncoder& passEncoder, const char* groupLabel)
  : PassEncoder(&passEncoder)
{
#ifndef NDEBUG
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
#ifndef NDEBUG
  this->BundleEncoder->PushDebugGroup(groupLabel);
#else
  (void)this->BundleEncoder;
  (void)groupLabel;
#endif
}

//------------------------------------------------------------------------------
#ifndef NDEBUG
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
}
#else
vtkWebGPUCommandEncoderDebugGroup::~vtkWebGPUCommandEncoderDebugGroup() = default;
#endif

VTK_ABI_NAMESPACE_END
