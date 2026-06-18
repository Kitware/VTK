// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUHelpersPrivate_h
#define vtkWebGPUHelpersPrivate_h

#include "webgpu/webgpu_cpp.h"
#include <string>

namespace vtkWebGPUHelpers
{
inline std::string StringViewToStdString(wgpu::StringView sv)
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
} // namespace vtkWebGPUHelpers

#endif
