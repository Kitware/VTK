// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkRenderingCoreEnums_h
#define vtkRenderingCoreEnums_h

#include "vtkABINamespace.h"

// Marker shapes for plotting
VTK_ABI_NAMESPACE_BEGIN
enum
{
  VTK_MARKER_NONE = 0,
  VTK_MARKER_CROSS,
  VTK_MARKER_PLUS,
  VTK_MARKER_SQUARE,
  VTK_MARKER_CIRCLE,
  VTK_MARKER_DIAMOND,

  VTK_MARKER_UNKNOWN // Must be last.
};

VTK_ABI_NAMESPACE_END
#endif // vtkRenderingCoreEnums_h
// VTK-HeaderTest-Exclude: vtkRenderingCoreEnums.h
