// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUHelpersPrivate_h
#define vtkWebGPUHelpersPrivate_h

// Transitional: vtk_wgpu_impl.h still pulls in webgpu_cpp.h so that the
// wgpu::StringView overload below keeps compiling while call sites are migrated
// to the C API. Once no caller passes a wgpu::StringView, drop that overload and
// include "vtk_wgpu.h" here instead.
#include "vtk_wgpu_impl.h"
#include <string>

// Helper function for converting WGPUStringView to std::string.
// This is used internally by WebGPU implementations.
inline std::string vtkWebGPUStringViewToStdString(WGPUStringView sv)
{
  if (sv.length == WGPU_STRLEN)
  {
    if (sv.data == nullptr)
    {
      return {};
    }
    return { sv.data };
  }
  return { sv.data, sv.length };
}

// Transitional overload for the C++ wrapper's StringView. Remove once all call
// sites pass a C-API WGPUStringView.
inline std::string vtkWebGPUStringViewToStdString(wgpu::StringView sv)
{
  return vtkWebGPUStringViewToStdString(WGPUStringView{ sv.data, sv.length });
}

// Helper function for constructing a WGPUStringView that borrows a std::string's
// storage. The returned view is only valid while the string is alive.
inline WGPUStringView vtkWebGPUMakeStringView(const std::string& str)
{
  return WGPUStringView{ str.data(), str.size() };
}

#endif
