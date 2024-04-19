// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCountVertices.h"

#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"

int TestCountVerticesMode(bool useImplicitArray)
{
  vtkNew<vtkUnstructuredGrid> data;
  vtkNew<vtkPoints> points;
  vtkNew<vtkIdList> cell;
  vtkNew<vtkCountVertices> filter;
  filter->SetUseImplicitArray(useImplicitArray);

  // Need 12 points to test all cell types:
  for (int i = 0; i < 12; ++i)
  {
    points->InsertNextPoint(0., 0., 0.);
  }
  data->SetPoints(points);

  // Insert the following cell types and verify the number of verts computed
  // by the filter:
  // VTK_VERTEX = 1
  // VTK_LINE = 2
  // VTK_TRIANGLE = 3
  // VTK_TETRA = 4
  // VTK_PYRAMID = 5
  // VTK_WEDGE = 6
  // VTK_VOXEL = 8
  // VTK_HEXAHEDRON = 8
  // VTK_PENTAGONAL_PRISM = 10
  // VTK_HEXAGONAL_PRISM = 12

  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(VTK_VERTEX, cell);

  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(VTK_LINE, cell);

  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(VTK_TRIANGLE, cell);

  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(VTK_TETRA, cell);

  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(VTK_PYRAMID, cell);

  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(VTK_WEDGE, cell);

  cell->InsertNextId(cell->GetNumberOfIds());
  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(VTK_VOXEL, cell);
  data->InsertNextCell(VTK_HEXAHEDRON, cell);

  cell->InsertNextId(cell->GetNumberOfIds());
  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(VTK_PENTAGONAL_PRISM, cell);

  cell->InsertNextId(cell->GetNumberOfIds());
  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(VTK_HEXAGONAL_PRISM, cell);

  filter->SetInputData(data);
  filter->Update();

  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(filter->GetOutput());
  if (!output)
  {
    std::cerr << "No output data!\n";
    return EXIT_FAILURE;
  }

  vtkDataArray* verts = output->GetCellData()->GetArray(filter->GetOutputArrayName());
  if (!verts)
  {
    std::cerr << "No output array!\n";
    return EXIT_FAILURE;
  }

  if (verts->GetNumberOfComponents() != 1)
  {
    std::cerr << "Invalid number of components in output array: " << verts->GetNumberOfComponents()
              << "\n";
    return EXIT_FAILURE;
  }

  if (verts->GetNumberOfTuples() != 10)
  {
    std::cerr << "Invalid number of components in output array: " << verts->GetNumberOfTuples()
              << "\n";
    return EXIT_FAILURE;
  }

#define TEST_VERTICES(idx, expected)                                                               \
  do                                                                                               \
  {                                                                                                \
    vtkIdType numVerts = static_cast<vtkIdType>(verts->GetTuple1(idx));                            \
    if (numVerts != (expected))                                                                    \
    {                                                                                              \
      std::cerr << "Expected cell @idx=" << (idx) << " to have " << (expected)                     \
                << " vertices, but found " << numVerts << "\n";                                    \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

  int idx = 0;
  // VTK_VERTEX = 1
  TEST_VERTICES(idx++, 1);
  // VTK_LINE = 2
  TEST_VERTICES(idx++, 2);
  // VTK_TRIANGLE = 3
  TEST_VERTICES(idx++, 3);
  // VTK_TETRA = 4
  TEST_VERTICES(idx++, 4);
  // VTK_PYRAMID = 5
  TEST_VERTICES(idx++, 5);
  // VTK_WEDGE = 6
  TEST_VERTICES(idx++, 6);
  // VTK_VOXEL = 8
  TEST_VERTICES(idx++, 8);
  // VTK_HEXAHEDRON = 8
  TEST_VERTICES(idx++, 8);
  // VTK_PENTAGONAL_PRISM = 10
  TEST_VERTICES(idx++, 10);
  // VTK_HEXAGONAL_PRISM = 12
  TEST_VERTICES(idx++, 12);

#undef TEST_VERTICES

  return EXIT_SUCCESS;
}

int TestCountVertices(int, char*[])
{
  int ret = EXIT_SUCCESS;
  ret |= ::TestCountVerticesMode(false);
  ret |= ::TestCountVerticesMode(true);
  return ret;
}
