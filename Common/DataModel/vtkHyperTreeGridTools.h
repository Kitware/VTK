// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkHyperTreeGridTools_h
#define vtkHyperTreeGridTools_h

#include "vtkABINamespace.h"

namespace vtk
{
namespace hypertreegrid
{
VTK_ABI_NAMESPACE_BEGIN

template <class T>
bool HasTree(const T& e)
{
  return e.GetTree() != nullptr;
}

VTK_ABI_NAMESPACE_END
} // namespace hypertreegrid
} // namespace vtk

#endif // vtHyperTreeGridTools_h
// VTK-HeaderTest-Exclude: vtkHyperTreeGridTools.h
