/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCountVertices.h"

#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"

int TestCountVertices(int, char*[])
{
  vtkNew<vtkUnstructuredGrid> data;
  vtkNew<vtkPoints> points;
  vtkNew<vtkIdList> cell;
  vtkNew<vtkCountVertices> filter;

  // Need 12 points to test all cell types:
  for (int i = 0; i < 12; ++i)
  {
    points->InsertNextPoint(0., 0., 0.);
  }
  data->SetPoints(points.Get());

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
  data->InsertNextCell(VTK_VERTEX, cell.Get());

  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(VTK_LINE, cell.Get());

  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(VTK_TRIANGLE, cell.Get());

  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(VTK_TETRA, cell.Get());

  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(VTK_PYRAMID, cell.Get());

  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(VTK_WEDGE, cell.Get());

  cell->InsertNextId(cell->GetNumberOfIds());
  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(VTK_VOXEL, cell.Get());
  data->InsertNextCell(VTK_HEXAHEDRON, cell.Get());

  cell->InsertNextId(cell->GetNumberOfIds());
  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(VTK_PENTAGONAL_PRISM, cell.Get());

  cell->InsertNextId(cell->GetNumberOfIds());
  cell->InsertNextId(cell->GetNumberOfIds());
  data->InsertNextCell(VTK_HEXAGONAL_PRISM, cell.Get());

  filter->SetInputData(data.Get());
  filter->Update();

  vtkUnstructuredGrid *output =
      vtkUnstructuredGrid::SafeDownCast(filter->GetOutput());
  if (!output)
  {
    std::cerr << "No output data!\n";
    return EXIT_FAILURE;
  }

  vtkIdTypeArray *verts =
      vtkIdTypeArray::SafeDownCast(
        output->GetCellData()->GetArray(
          filter->GetOutputArrayName()));
  if (!verts)
  {
    std::cerr << "No output array!\n";
    return EXIT_FAILURE;
  }

  if (verts->GetNumberOfComponents() != 1)
  {
    std::cerr << "Invalid number of components in output array: "
              << verts->GetNumberOfComponents() << "\n";
    return EXIT_FAILURE;
  }

  if (verts->GetNumberOfTuples() != 10)
  {
    std::cerr << "Invalid number of components in output array: "
              << verts->GetNumberOfTuples() << "\n";
    return EXIT_FAILURE;
  }

#define TEST_VERTICES(idx, expected) \
  { \
  vtkIdType numVerts = verts->GetTypedComponent(idx, 0); \
  if (numVerts != expected) \
  { \
    std::cerr << "Expected cell @idx=" << idx << " to have " << expected \
              << " vertices, but found " << numVerts<< "\n"; \
    return EXIT_FAILURE; \
  } \
  }

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
