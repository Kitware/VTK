// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUHelpersPrivate_h
#define vtkWebGPUHelpersPrivate_h

#include "webgpu/webgpu_cpp.h"
#include <string>

// Helper function for converting wgpu::StringView to std::string
// This is used internally by WebGPU implementations
inline std::string vtkWebGPUStringViewToStdString(wgpu::StringView sv)
{
  if (sv.length == wgpu::kStrlen)
  {
    if (sv.IsUndefined())
    {
      return {};
    }
    return { sv.data };
  }
  return { sv.data, sv.length };
}

#endif
