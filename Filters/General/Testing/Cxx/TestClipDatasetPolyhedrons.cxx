// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellArray.h"
#include "vtkCellTypeSource.h"
#include "vtkClipDataSet.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"

int TestClipDatasetPolyhedrons(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkCellTypeSource> cellTypeSource;
  cellTypeSource->SetCellOrder(1);
  cellTypeSource->SetCellType(VTK_POLYHEDRON);
  cellTypeSource->SetBlocksDimensions(30, 30, 30);

  vtkNew<vtkPlane> plane;
  plane->SetOrigin(8, 2, 4);
  plane->SetNormal(0.5, 0.5, 0.5);

  vtkNew<vtkClipDataSet> clip;
  clip->SetInputConnection(cellTypeSource->GetOutputPort());
  clip->SetClipFunction(plane);
  clip->Update();

  // Check number of cells / points
  vtkPoints* points = clip->GetOutput()->GetPoints();
  if (int nPoints = points->GetNumberOfPoints(); nPoints != 30465)
  {
    vtkLogF(ERROR, "Number of points: expecting 30465 got %i", nPoints);
    return EXIT_FAILURE;
  }
  vtkCellArray* cells = clip->GetOutput()->GetCells();
  if (int nCells = cells->GetNumberOfCells(); nCells != 26714)
  {
    vtkLogF(ERROR, "Number of points: expecting 26714 got %i", nCells);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
