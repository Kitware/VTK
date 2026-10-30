// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// The triangulated contour of a polyhedron of a hanging-node mesh, whose iso-polygon stalls the
// ear cutting of vtkPolygon, has the triangles and the area of the iso-polygon instead of a hole.

#include "vtkCellData.h"
#include "vtkContourFilter.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkMassProperties.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkTriangleFilter.h"
#include "vtkUnstructuredGrid.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace
{
vtkSmartPointer<vtkPolyData> Contour(vtkUnstructuredGrid* grid, double value, bool triangles)
{
  vtkNew<vtkContourFilter> contour;
  contour->SetInputData(grid);
  contour->SetValue(0, value);
  contour->SetGenerateTriangles(triangles);
  contour->Update();
  return contour->GetOutput();
}

double Area(vtkPolyData* surface)
{
  vtkNew<vtkTriangleFilter> triangles;
  triangles->SetInputData(surface);
  vtkNew<vtkMassProperties> mass;
  mass->SetInputConnection(triangles->GetOutputPort());
  mass->Update();
  return mass->GetSurfaceArea();
}
}

int TestPolyhedronContourStalledEarCut(int, char*[])
{
  // Point coordinates, then the scalar: the distance to (6, 6, 6).
  const double x[19][4] = { { 8, 10, 8, 2 }, { 8, 10, 9, 2.2360679774997898 },
    { 8, 10, 10, 2.8284271247461903 }, { 8, 12, 8, 4 }, { 8, 12, 10, 4.4721359549995796 },
    { 8, 11, 10, 3.6055512754639891 }, { 10, 10, 8, 2.8284271247461903 },
    { 10, 10, 10, 3.4641016151377544 }, { 9, 10, 8, 2.2360679774997898 },
    { 9, 11, 8, 3.1622776601683795 }, { 8, 11, 8, 3 }, { 9, 12, 8, 4.1231056256176606 },
    { 10, 11, 8, 3.6055512754639891 }, { 10, 12, 8, 4.4721359549995796 },
    { 10, 12, 10, 4.8989794855663558 }, { 8, 12, 9, 4.1231056256176606 },
    { 9, 12, 9, 4.2426406871192848 }, { 9, 12, 10, 4.5825756949558398 },
    { 10, 12, 9, 4.5825756949558398 } };
  const std::vector<vtkIdType> faceStream = { 12, 8, 0, 1, 2, 5, 4, 15, 3, 10, 6, 6, 12, 13, 18, 14,
    7, 6, 0, 8, 6, 7, 2, 1, 4, 3, 15, 16, 11, 4, 15, 4, 17, 16, 4, 16, 17, 14, 18, 4, 11, 16, 18,
    13, 4, 0, 10, 9, 8, 4, 10, 3, 11, 9, 4, 9, 11, 13, 12, 4, 8, 9, 12, 6, 6, 2, 7, 14, 17, 4, 5 };

  vtkNew<vtkPoints> points;
  vtkNew<vtkDoubleArray> scalars;
  scalars->SetName("distance");
  for (const auto& p : x)
  {
    points->InsertNextPoint(p);
    scalars->InsertNextValue(p[3]);
  }
  vtkNew<vtkUnstructuredGrid> grid;
  grid->SetPoints(points);
  grid->GetPointData()->SetScalars(scalars);
  vtkNew<vtkIdList> stream;
  for (vtkIdType id : faceStream)
  {
    stream->InsertNextId(id);
  }
  grid->InsertNextCell(VTK_POLYHEDRON, stream);

  const double value = 4.1569219381653051;
  auto polygons = Contour(grid, value, false);
  auto triangles = Contour(grid, value, true);

  vtkIdType expected = 0;
  for (vtkIdType cellId = 0; cellId < polygons->GetNumberOfCells(); ++cellId)
  {
    expected += polygons->GetCell(cellId)->GetNumberOfPoints() - 2;
  }
  int status = EXIT_SUCCESS;
  if (triangles->GetNumberOfCells() != expected)
  {
    std::cerr << triangles->GetNumberOfCells() << " triangles instead of " << expected << std::endl;
    status = EXIT_FAILURE;
  }
  const double polygonArea = Area(polygons);
  const double triangleArea = Area(triangles);
  if (!vtkMathUtilities::FuzzyCompare(triangleArea, polygonArea, 1e-9 * polygonArea))
  {
    std::cerr << "triangulated area " << triangleArea << " instead of " << polygonArea << std::endl;
    status = EXIT_FAILURE;
  }
  return status;
}
