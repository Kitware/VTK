// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Check that vtkStaticCleanUnstructuredGrid does not modify the polyhedron
// faces of its input, and that re-executing it gives the same output.

#include "vtkCellArray.h"
#include "vtkCellType.h"
#include "vtkDataArray.h"
#include "vtkIdList.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkStaticCleanUnstructuredGrid.h"
#include "vtkUnstructuredGrid.h"

#include <iostream>
#include <vector>

namespace
{
std::vector<vtkIdType> GetFaceIds(vtkUnstructuredGrid* grid)
{
  vtkDataArray* conn = grid->GetPolyhedronFaces()->GetConnectivityArray();
  std::vector<vtkIdType> ids(conn->GetNumberOfTuples());
  for (vtkIdType i = 0; i < conn->GetNumberOfTuples(); ++i)
  {
    ids[i] = static_cast<vtkIdType>(conn->GetTuple1(i));
  }
  return ids;
}
}

int TestStaticCleanUnstructuredGridPolyhedra(int, char*[])
{
  // A hexahedron and a polyhedron cube sharing the face x=1. The polyhedron
  // uses duplicated copies of the 4 shared points, and point 0 is unused, so
  // the point map shifts every id.
  const double coords[17][3] = { { 9, 9, 9 }, { 1, 0, 0 }, { 2, 0, 0 }, { 2, 1, 0 }, { 1, 1, 0 },
    { 1, 0, 1 }, { 2, 0, 1 }, { 2, 1, 1 }, { 1, 1, 1 }, { 0, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 },
    { 0, 1, 1 }, { 1, 0, 0 }, { 1, 1, 0 }, { 1, 0, 1 }, { 1, 1, 1 } };
  vtkNew<vtkPoints> points;
  for (const auto& c : coords)
  {
    points->InsertNextPoint(c);
  }
  vtkNew<vtkUnstructuredGrid> grid;
  grid->SetPoints(points);

  const vtkIdType hexa[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
  grid->InsertNextCell(VTK_HEXAHEDRON, 8, hexa);

  const vtkIdType faces[6][4] = { { 9, 10, 14, 13 }, { 11, 15, 16, 12 }, { 9, 13, 15, 11 },
    { 13, 14, 16, 15 }, { 14, 10, 12, 16 }, { 10, 9, 11, 12 } };
  vtkNew<vtkIdList> stream;
  stream->InsertNextId(6);
  for (const auto& f : faces)
  {
    stream->InsertNextId(4);
    for (vtkIdType id : f)
    {
      stream->InsertNextId(id);
    }
  }
  grid->InsertNextCell(VTK_POLYHEDRON, stream);

  const std::vector<vtkIdType> inputBefore = GetFaceIds(grid);

  vtkNew<vtkStaticCleanUnstructuredGrid> clean;
  clean->SetInputData(grid);
  clean->ToleranceIsAbsoluteOn();
  clean->SetAbsoluteTolerance(0.0);
  clean->RemoveUnusedPointsOn();
  clean->Update();
  const std::vector<vtkIdType> firstOutput = GetFaceIds(clean->GetOutput());

  if (GetFaceIds(grid) != inputBefore)
  {
    std::cerr << "ERROR: the filter modified the polyhedron faces of its input.\n";
    return EXIT_FAILURE;
  }
  if (clean->GetOutput()->GetNumberOfPoints() != 12)
  {
    std::cerr << "ERROR: expected 12 output points, got " << clean->GetOutput()->GetNumberOfPoints()
              << "\n";
    return EXIT_FAILURE;
  }

  clean->Modified();
  clean->Update();
  if (GetFaceIds(clean->GetOutput()) != firstOutput)
  {
    std::cerr << "ERROR: re-executing the filter changed its output.\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
