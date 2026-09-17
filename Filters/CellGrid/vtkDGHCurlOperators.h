// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkDGHCurlOperators_h
#define vtkDGHCurlOperators_h

#include "vtkFiltersCellGridModule.h" // For export macro.

namespace vtk
{
namespace basis
{
/// A function space for basis functions defined on edge-sides of a cell.
namespace hcurl
{
VTK_ABI_NAMESPACE_BEGIN

/// Register basis-function operators for the "HCURL" function space with vtkDGCell.
bool VTKFILTERSCELLGRID_EXPORT RegisterOperators();

VTK_ABI_NAMESPACE_END
} // namespace hcurl
} // namespace basis
} // namespace vtk

#endif // vtkDGHCurlOperators_h
