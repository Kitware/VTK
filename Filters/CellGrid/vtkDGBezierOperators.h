// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef Filters_CellGrid_vtkDGBezierOperators_h
#define Filters_CellGrid_vtkDGBezierOperators_h

#include "vtkCellAttribute.h" // For CellTypeInfo.
#include "vtkDGOperatorEntry.h"
#include "vtkFiltersCellGridModule.h" // For export macro.

namespace vtk
{
namespace basis
{
/// A function space spanned by Bernstein-Bezier basis functions.
///
/// These span the same polynomials as the traditional Lagrange shape functions
/// of the same order, but their coefficients are control values rather than
/// values the function takes on: except at the corners of a cell, a
/// Bernstein-Bezier function does not interpolate its degrees of freedom.
///
/// What it offers in exchange is that the basis functions are non-negative and
/// sum to one, so a cell's values lie within the convex hull of its control
/// values. That bound lets algorithms such as isocontouring discard cells, or
/// regions of cells, without evaluating the function there.
namespace bezier
{
VTK_ABI_NAMESPACE_BEGIN

/// Register basis-function operators for the "Bezier" function space with vtkDGCell.
bool VTKFILTERSCELLGRID_EXPORT RegisterOperators();

VTK_ABI_NAMESPACE_END
} // namespace bezier
} // namespace basis
} // namespace vtk

#endif // Filters_CellGrid_vtkDGBezierOperators_h
