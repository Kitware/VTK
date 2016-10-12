/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCountFaces.h"

#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"

int TestCountFaces(int, char*[])
{
  vtkNew<vtkUnstructuredGrid> data;
  vtkNew<vtkPoints> points;
  vtkNew<vtkIdList> cell;
  vtkNew<vtkCountFaces> filter;

  // Need 12 points to test all cell types:
  for (int i = 0; i < 12; ++i)
  {
    points->InsertNextPoint(0., 0., 0.);
  }
  data->SetPoints(points.Get());

  // Insert the following cell types and verify the number of faces computed
  // by the filter:
  // VTK_VERTEX = 0
  // VTK_LINE = 0
  // VTK_TRIANGLE = 0
  // VTK_TETRA = 4
  // VTK_PYRAMID = 5
  // VTK_WEDGE = 5
  // VTK_VOXEL = 6
  // VTK_HEXAHEDRON = 6
  // VTK_PENTAGONAL_PRISM = 7
  // VTK_HEXAGONAL_PRISM = 8

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

  vtkIdTypeArray *faces =
      vtkIdTypeArray::SafeDownCast(
        output->GetCellData()->GetArray(
          filter->GetOutputArrayName()));
  if (!faces)
  {
    std::cerr << "No output array!\n";
    return EXIT_FAILURE;
  }

  if (faces->GetNumberOfComponents() != 1)
  {
    std::cerr << "Invalid number of components in output array: "
              << faces->GetNumberOfComponents() << "\n";
    return EXIT_FAILURE;
  }

  if (faces->GetNumberOfTuples() != 10)
  {
    std::cerr << "Invalid number of components in output array: "
              << faces->GetNumberOfTuples() << "\n";
    return EXIT_FAILURE;
  }

#define TEST_FACES(idx, expected) \
  { \
  vtkIdType numFaces = faces->GetTypedComponent(idx, 0); \
  if (numFaces != expected) \
  { \
    std::cerr << "Expected cell @idx=" << idx << " to have " << expected \
              << " faces, but found " << numFaces << "\n"; \
    return EXIT_FAILURE; \
  } \
  }

  int idx = 0;
  // VTK_VERTEX = 0
  TEST_FACES(idx++, 0);
  // VTK_LINE = 0
  TEST_FACES(idx++, 0);
  // VTK_TRIANGLE = 0
  TEST_FACES(idx++, 0);
  // VTK_TETRA = 4
  TEST_FACES(idx++, 4);
  // VTK_PYRAMID = 5
  TEST_FACES(idx++, 5);
  // VTK_WEDGE = 5
  TEST_FACES(idx++, 5);
  // VTK_VOXEL = 6
  TEST_FACES(idx++, 6);
  // VTK_HEXAHEDRON = 6
  TEST_FACES(idx++, 6);
  // VTK_PENTAGONAL_PRISM = 7
  TEST_FACES(idx++, 7);
  // VTK_HEXAGONAL_PRISM = 8
  TEST_FACES(idx++, 8);

#undef TEST_FACES

  return EXIT_SUCCESS;
}
