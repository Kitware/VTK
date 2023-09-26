// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test covers degenerate cells in vtkUnstructuredGridGeometryFilter

#include "vtkCellType.h"
#include "vtkGeometryFilter.h"
#include "vtkPointData.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridGeometryFilter.h"

int TestUnstructuredGridGeometryFilterDegenerateCells(int, char*[])
{
  // Create an unstructured grid.
  auto grid = vtkSmartPointer<vtkUnstructuredGrid>::New();

  auto points = vtkSmartPointer<vtkPoints>::New();
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 0, 0);
  points->InsertNextPoint(0, 1, 0);
  points->InsertNextPoint(1, 1, 0);
  points->InsertNextPoint(0.5, 0.5, 0.5);

  vtkIdType linearHexConnectivity0[8] = { 4, 1, 3, 4, 0, 0, 0, 0 };
  vtkIdType linearHexConnectivity1[8] = { 0, 0, 0, 0, 3, 4, 4, 2 };
  grid->SetPoints(points);
  grid->InsertNextCell(VTK_LAGRANGE_HEXAHEDRON, 8, linearHexConnectivity0);
  grid->InsertNextCell(VTK_LAGRANGE_HEXAHEDRON, 8, linearHexConnectivity1);

  // Create the filter
  auto ugridFilter = vtkSmartPointer<vtkUnstructuredGridGeometryFilter>::New();
  ugridFilter->SetInputData(grid);
  ugridFilter->Update();
  {
    vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(ugridFilter->GetOutput());
    if (ugrid->GetNumberOfCells() != 8)
    {
      vtkGenericWarningMacro(
        "If MatchBoundariesIgnoringCellOrder = 1, GetNumberOfCells should be 8 but is "
        << ugrid->GetNumberOfCells());
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
