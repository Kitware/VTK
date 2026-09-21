// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef Filters_CellGrid_vtkDGHGradOperators_h
#define Filters_CellGrid_vtkDGHGradOperators_h

#include "vtkCellAttribute.h" // For CellTypeInfo.
#include "vtkDGCell.h"        // For shape enum.
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

/// Fetch parametric coordinates where Lagrange basis functions exactly interpolate a value.
///
/// This only returns true when the \a shape, \a basis, and \a nominalOrder match
/// a **fixed-order** basis function. You should prefer using the vtkLagrangePoints
/// calculator as that class calls this method when appropriate but also handles
/// arbitrary-order basis functions.
///
/// This function exists because programmatically generating parametric coordinates
/// for serendipity and other finite elements would be difficult.
bool VTKFILTERSCELLGRID_EXPORT FixedOrderLagrangePoints(vtkDGCell::Shape shape,
  vtkStringToken basis, int nominalOrder, std::vector<std::vector<double>>& points);

/// Fetch parametric coordinates where the "G" (Gauss-point) basis functions interpolate.
///
/// The G bases are nodal on the Gauss points rather than on a uniform lattice, so
/// they need their own point set. Returns true and fills \a points for the shapes
/// whose G basis is nodal; returns false for the tetrahedron and pyramid, whose G
/// bases are modal and so interpolate nowhere (see TestCellGridLagrangePoints).
///
/// Prefer the vtkLagrangePoints calculator, which calls this when appropriate.
bool VTKFILTERSCELLGRID_EXPORT FixedOrderGaussPoints(
  vtkDGCell::Shape shape, int nominalOrder, std::vector<std::vector<double>>& points);

VTK_ABI_NAMESPACE_END
} // namespace hgrad
} // namespace basis
} // namespace vtk

#endif // Filters_CellGrid_vtkDGHGradOperators_h
