// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellArray.h"
#include "vtkDelaunay2D.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

int TestDelaunay2DConstrained(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Define initial set of points to triangulate
  vtkNew<vtkPoints> points;
  points->InsertNextPoint(185, 3849, 0);
  points->InsertNextPoint(425, 4113, 0);
  points->InsertNextPoint(250, 3750, 0);
  points->InsertNextPoint(250, 4000, 0);
  points->InsertNextPoint(500, 3750, 0);
  points->InsertNextPoint(500, 4000, 0);

  vtkNew<vtkPolyData> startingPts;
  startingPts->SetPoints(points);

  // Define edge constraint
  vtkNew<vtkCellArray> edges;
  vtkIdType edgePoints[2] = { 0, 1 };
  edges->InsertNextCell(2, edgePoints);

  vtkNew<vtkPolyData> constraint;
  constraint->SetPoints(points);
  constraint->SetLines(edges);

  // Apply constrained Delaunay triangulation
  vtkNew<vtkDelaunay2D> delaunay;
  delaunay->SetInputData(startingPts);
  delaunay->SetSourceData(constraint);
  delaunay->SetTolerance(0.1);
  delaunay->Update();

  // Check triangulation
  vtkPolyData* output = delaunay->GetOutput();
  vtkIdType numFaces = output->GetNumberOfCells();

  if (numFaces != 4)
  {
    std::cout << "Expected 4 triangles but got " << numFaces << std::endl;
    return EXIT_FAILURE;
  }

  int expected[4][3] = { { 0, 3, 1 }, { 4, 5, 1 }, { 0, 4, 1 }, { 0, 2, 4 } };
  int face[3] = { 0, 0, 0 };

  for (int i = 0; i < numFaces; ++i)
  {
    vtkCell* cell = output->GetCell(i);
    face[0] = cell->GetPointId(0);
    face[1] = cell->GetPointId(1);
    face[2] = cell->GetPointId(2);

    if (face[0] != expected[i][0] || face[1] != expected[i][1] || face[2] != expected[i][2])
    {
      std::cout << "For triangle " << i << ", expected point IDs (" << expected[i][0] << ", "
                << expected[i][1] << ", " << expected[i][2] << ") but got (" << face[0] << ", "
                << face[1] << ", " << face[2] << ")." << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
