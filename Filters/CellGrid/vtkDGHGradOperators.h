// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef Filters_CellGrid_vtkDGHGradOperators_h
#define Filters_CellGrid_vtkDGHGradOperators_h

#include "vtkCellAttribute.h" // For CellTypeInfo.
#include "vtkDGOperatorEntry.h"
#include "vtkFiltersCellGridModule.h" // For export macro.

namespace vtk
{
namespace basis
{
/// A function space for basis functions defined on corner vertices of cells.
///
/// This function space is analogous to the traditional Lagrange shape functions.
namespace hgrad
{
VTK_ABI_NAMESPACE_BEGIN

/// Register basis-function operators for the "HGRAD" function space with vtkDGCell.
bool VTKFILTERSCELLGRID_EXPORT RegisterOperators();

VTK_ABI_NAMESPACE_END
} // namespace hgrad
} // namespace basis
} // namespace vtk

#endif // Filters_CellGrid_vtkDGHGradOperators_h
