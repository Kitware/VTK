// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test covers the option MatchBoundariesIgnoringCellOrder in vtkUnstructuredGridGeometryFilter
// class

#include "vtkCellType.h"
#include "vtkGeometryFilter.h"
#include "vtkHexahedron.h"
#include "vtkPointData.h"
#include "vtkTestUtilities.h"
#include "vtkTriQuadraticHexahedron.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridGeometryFilter.h"

int TestMatchBoundariesIgnoringCellOrder(int, char*[])
{
  // Create an unstructured grid.
  auto grid = vtkSmartPointer<vtkUnstructuredGrid>::New();

  auto points = vtkSmartPointer<vtkPoints>::New();
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 0, 0);
  points->InsertNextPoint(0, 1, 0);
  points->InsertNextPoint(1, 1, 0);
  points->InsertNextPoint(0, 0, 1);
  points->InsertNextPoint(1, 0, 1);
  points->InsertNextPoint(0, 1, 1);
  points->InsertNextPoint(1, 1, 1);
  points->InsertNextPoint(0, 0, 2);
  points->InsertNextPoint(1, 0, 2);
  points->InsertNextPoint(0, 1, 2);
  points->InsertNextPoint(1, 1, 2);
  points->InsertNextPoint(0.5, 0, 0);
  points->InsertNextPoint(0, 0.5, 0);
  points->InsertNextPoint(0.5, 0.5, 0);
  points->InsertNextPoint(1, 0.5, 0);
  points->InsertNextPoint(0.5, 1, 0);
  points->InsertNextPoint(0, 0, 0.5);
  points->InsertNextPoint(0.5, 0, 0.5);
  points->InsertNextPoint(1, 0, 0.5);
  points->InsertNextPoint(0, 0.5, 0.5);
  points->InsertNextPoint(0.5, 0.5, 0.5);
  points->InsertNextPoint(1, 0.5, 0.5);
  points->InsertNextPoint(0, 1, 0.5);
  points->InsertNextPoint(0.5, 1, 0.5);
  points->InsertNextPoint(1, 1, 0.5);
  points->InsertNextPoint(0.5, 0, 1);
  points->InsertNextPoint(0, 0.5, 1);
  points->InsertNextPoint(0.5, 0.5, 1);
  points->InsertNextPoint(1, 0.5, 1);
  points->InsertNextPoint(0.5, 1, 1);

  vtkIdType triQuadHexConnectivity[27] = { 0, 1, 3, 2, 4, 5, 7, 6, 12, 15, 16, 13, 26, 29, 30, 27,
    17, 19, 25, 23, 20, 22, 18, 24, 14, 28, 21 };
  vtkIdType linearHexConnectivity[8] = { 4, 5, 7, 6, 8, 9, 11, 10 };
  grid->SetPoints(points);
  grid->InsertNextCell(VTK_TRIQUADRATIC_HEXAHEDRON, 27, triQuadHexConnectivity);
  grid->InsertNextCell(VTK_HEXAHEDRON, 8, linearHexConnectivity);

  // Create the filter
  auto ugridFilter = vtkSmartPointer<vtkUnstructuredGridGeometryFilter>::New();
  ugridFilter->SetMatchBoundariesIgnoringCellOrder(true);
  ugridFilter->SetInputData(grid);
  ugridFilter->Update();
  {
    vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(ugridFilter->GetOutput());
    if (ugrid->GetNumberOfCells() != 10)
    {
      vtkGenericWarningMacro(
        "If MatchBoundariesIgnoringCellOrder = 1, GetNumberOfCells should be 10 but is "
        << ugrid->GetNumberOfCells());
      return EXIT_FAILURE;
    }
  }

  ugridFilter->SetMatchBoundariesIgnoringCellOrder(false);
  ugridFilter->Update();
  {
    vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(ugridFilter->GetOutput());
    if (ugrid->GetNumberOfCells() != 12)
    {
      vtkGenericWarningMacro(
        "If MatchBoundariesIgnoringCellOrder = 0, GetNumberOfCells should be 12 but is "
        << ugrid->GetNumberOfCells());
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
