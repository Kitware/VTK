// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef Filters_CellGrid_vtkDGHDivOperators_h
#define Filters_CellGrid_vtkDGHDivOperators_h

#include "vtkFiltersCellGridModule.h" // For export macro.

namespace vtk
{
namespace basis
{
/// A function space for basis functions defined on face-sides of cells.
namespace hdiv
{
VTK_ABI_NAMESPACE_BEGIN

/// Register basis-function operators for the "HDIV" function space with vtkDGCell.
bool VTKFILTERSCELLGRID_EXPORT RegisterOperators();

VTK_ABI_NAMESPACE_END
} // namespace hdiv
} // namespace basis
} // namespace vtk

#endif // Filters_CellGrid_vtkDGHDivOperators_h
