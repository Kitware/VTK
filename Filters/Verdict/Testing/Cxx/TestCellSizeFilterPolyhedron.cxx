// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// vtkCellSizeFilter on polyhedra: a thin cell, which the Delaunay triangulation
// of vtkPolyhedron::TriangulateIds() used to miss, and a non-convex cell, whose
// triangulation covers its convex hull.

#include "vtkCellData.h"
#include "vtkCellSizeFilter.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"

#include <iostream>
#include <vector>

namespace
{
void InsertPolyhedron(vtkUnstructuredGrid* grid, const std::vector<std::vector<vtkIdType>>& faces)
{
  vtkNew<vtkIdList> stream;
  stream->InsertNextId(static_cast<vtkIdType>(faces.size()));
  for (const auto& face : faces)
  {
    stream->InsertNextId(static_cast<vtkIdType>(face.size()));
    for (vtkIdType id : face)
    {
      stream->InsertNextId(id);
    }
  }
  grid->InsertNextCell(VTK_POLYHEDRON, stream);
}
}

int TestCellSizeFilterPolyhedron(int, char*[])
{
  vtkNew<vtkPoints> pts;
  pts->SetDataTypeToDouble();
  vtkNew<vtkUnstructuredGrid> grid;
  grid->SetPoints(pts);
  grid->Allocate(2);

  // An L-shaped prism: three unit squares extruded by 1, so a volume of 3.
  const double footprint[6][2] = { { 0, 0 }, { 2, 0 }, { 2, 1 }, { 1, 1 }, { 1, 2 }, { 0, 2 } };
  for (double z : { 0.0, 1.0 })
  {
    for (const auto& xy : footprint)
    {
      pts->InsertNextPoint(xy[0], xy[1], z);
    }
  }
  std::vector<std::vector<vtkIdType>> lFaces = { { 5, 4, 3, 2, 1, 0 }, { 6, 7, 8, 9, 10, 11 } };
  for (vtkIdType i = 0; i < 6; ++i)
  {
    const vtkIdType j = (i + 1) % 6;
    lFaces.push_back({ i, j, j + 6, i + 6 });
  }
  InsertPolyhedron(grid, lFaces);

  // A thin tetrahedron, of volume 1e-3 * 1/6, flattened along z.
  const vtkIdType base = pts->GetNumberOfPoints();
  pts->InsertNextPoint(10, 0, 0);
  pts->InsertNextPoint(11, 0, 0);
  pts->InsertNextPoint(10, 1, 0);
  pts->InsertNextPoint(10.3, 0.3, 1e-3);
  InsertPolyhedron(grid,
    { { base, base + 2, base + 1 }, { base, base + 1, base + 3 }, { base + 1, base + 2, base + 3 },
      { base, base + 3, base + 2 } });

  vtkNew<vtkCellSizeFilter> filter;
  filter->SetInputData(grid);
  filter->Update();
  vtkDataArray* volume =
    vtkDataSet::SafeDownCast(filter->GetOutput())->GetCellData()->GetArray("Volume");

  const double expected[2] = { 3.0, 1e-3 / 6.0 };
  const char* names[2] = { "L-shaped prism", "thin tetrahedron" };
  int status = EXIT_SUCCESS;
  for (int c = 0; c < 2; ++c)
  {
    const double value = volume->GetComponent(c, 0);
    if (!vtkMathUtilities::FuzzyCompare(value, expected[c], 1e-9 * expected[c]))
    {
      std::cerr << names[c] << ": volume " << value << ", expected " << expected[c] << std::endl;
      status = EXIT_FAILURE;
    }
  }
  return status;
}
