// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUCommandEncoderDebugGroup.h"
#include "vtk_wgpu.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkWebGPUCommandEncoderDebugGroup::vtkWebGPUCommandEncoderDebugGroup(
  const WGPURenderPassEncoder& passEncoder, const char* groupLabel)
  : PassEncoder(&passEncoder)
{
#if !defined(NDEBUG) && !defined(__EMSCRIPTEN__)
  wgpuRenderPassEncoderPushDebugGroup(passEncoder, WGPUStringView{ groupLabel, WGPU_STRLEN });
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
  wgpuRenderBundleEncoderPushDebugGroup(bundleEncoder, WGPUStringView{ groupLabel, WGPU_STRLEN });
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
  wgpuCommandEncoderPushDebugGroup(commandEncoder, WGPUStringView{ groupLabel, WGPU_STRLEN });
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
    wgpuRenderPassEncoderPopDebugGroup(*this->PassEncoder);
  }
  if (this->BundleEncoder)
  {
    wgpuRenderBundleEncoderPopDebugGroup(*this->BundleEncoder);
  }
  if (this->CommandEncoder)
  {
    wgpuCommandEncoderPopDebugGroup(*this->CommandEncoder);
  }
}
#else
vtkWebGPUCommandEncoderDebugGroup::~vtkWebGPUCommandEncoderDebugGroup() = default;
#endif

VTK_ABI_NAMESPACE_END
