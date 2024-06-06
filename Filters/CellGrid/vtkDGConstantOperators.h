// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef Filters_CellGrid_vtkDGConstantOperators_h
#define Filters_CellGrid_vtkDGConstantOperators_h

#include "vtkFiltersCellGridModule.h" // For export macro.

namespace vtk
{
/// A namespace for finite-element basis functions.
namespace basis
{
/// A function space for basis functions that are constant over a cell's underlying space.
namespace constant
{
VTK_ABI_NAMESPACE_BEGIN

/// Register basis-function operators for the "constant" function space with vtkDGCell.
bool VTKFILTERSCELLGRID_EXPORT RegisterOperators();

VTK_ABI_NAMESPACE_END
} // namespace constant
} // namespace basis
} // namespace vtk

#endif // Filters_CellGrid_vtkDGConstantOperators_h
