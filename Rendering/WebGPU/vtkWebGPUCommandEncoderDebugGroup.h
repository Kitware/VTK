// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGPUCommandEncoderDebugGroup
 * @brief   Convenient class that inserts annotations around draw commands within a render
 * pass/bundle.
 *
 * Implementation classes can use the `vtkScopedEncoderDebugGroup(encoder, label)`
 * macro to automatically push a debug group in the encoder using a label string.
 * Upon leaving scope, this class will take care of popping the debug group.
 * The label string will appear in professional graphics debugging tools like
 * RenderDoc/NSight/apitrace and is very useful to isolate draw commands. You may use indicators
 * like class names, pointer addresses, line numbers, array names to make sense of the draw commands
 * by mapping their origins to VTK C++ code.
 *
 */

#ifndef vtkWebGPUCommandEncoderDebugGroup_h
#define vtkWebGPUCommandEncoderDebugGroup_h

#include "vtkABINamespace.h"          // for VTK_ABI_NAMESPACE macros
#include "vtkRenderingWebGPUModule.h" // for export macro
#include "vtk_wgpu.h"                 // for wgpu::RenderPassEncoder

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUCommandEncoderDebugGroup
{
public:
  vtkWebGPUCommandEncoderDebugGroup(
    const wgpu::RenderPassEncoder& passEncoder, const char* groupLabel);
  vtkWebGPUCommandEncoderDebugGroup(
    const wgpu::RenderBundleEncoder& passEncoder, const char* groupLabel);
  vtkWebGPUCommandEncoderDebugGroup(
    const wgpu::CommandEncoder& commandEncoder, const char* groupLabel);
  ~vtkWebGPUCommandEncoderDebugGroup();

  vtkWebGPUCommandEncoderDebugGroup() = delete;

  // make this class non-copyable and non-movable.
  vtkWebGPUCommandEncoderDebugGroup(const vtkWebGPUCommandEncoderDebugGroup&) = delete;
  void operator=(const vtkWebGPUCommandEncoderDebugGroup&) = delete;
  vtkWebGPUCommandEncoderDebugGroup(vtkWebGPUCommandEncoderDebugGroup&&) = delete;
  void operator=(vtkWebGPUCommandEncoderDebugGroup&&) = delete;

private:
  const wgpu::RenderPassEncoder* PassEncoder = nullptr;
  const wgpu::RenderBundleEncoder* BundleEncoder = nullptr;
  const wgpu::CommandEncoder* CommandEncoder = nullptr;
};

#define vtkScopedEncoderDebugGroupConcatImpl(s1, s2) s1##s2
#define vtkScopedEncoderDebugGroupConcat(s1, s2) vtkScopedEncoderDebugGroupConcatImpl(s1, s2)
#define vtkScopedEncoderDebugGroupAnonymousVariable(x) vtkScopedEncoderDebugGroupConcat(x, __LINE__)
// Use this macro to annotate a group of commands in an renderpass/bundle encoder.
#define vtkScopedEncoderDebugGroup(encoder, name)                                                  \
  ::vtkWebGPUCommandEncoderDebugGroup vtkScopedEncoderDebugGroupAnonymousVariable(                 \
    encoderDebugGroup)(encoder, name)

VTK_ABI_NAMESPACE_END
#endif
