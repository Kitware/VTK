// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This tests vtkReflectionFilter

#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkReflectionFilter.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

#include <iostream>

#define AssertMacro(b, cell)                                                                       \
  do                                                                                               \
  {                                                                                                \
    if (!(b))                                                                                      \
    {                                                                                              \
      std::cerr << "Failed to reflect " << cell << " on line " << __LINE__ << std::endl;           \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

int TestReflectionFilter(int, char*[])
{
  vtkSmartPointer<vtkUnstructuredGrid> pyramid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  {
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    points->InsertNextPoint(-1, -1, -1);
    points->InsertNextPoint(1, -1, -1);
    points->InsertNextPoint(1, 1, -1);
    points->InsertNextPoint(-1, 1, -1);
    points->InsertNextPoint(0, 0, 1);
    pyramid->SetPoints(points);

    vtkPointData* pd = pyramid->GetPointData();
    vtkNew<vtkDoubleArray> array;
    array->SetNumberOfComponents(3);
    double tuple[3] = { 1, 1, 13 };
    array->InsertNextTuple(tuple);
    array->InsertNextTuple(tuple);
    array->InsertNextTuple(tuple);
    array->InsertNextTuple(tuple);
    pd->AddArray(array);

    vtkNew<vtkDoubleArray> tensor;
    tensor->SetNumberOfComponents(9);
    double tensorTuple[9] = { 1, 1, 7, 1, 1, 1, 1, 1, 1 };
    tensor->InsertNextTuple(tensorTuple);
    tensor->InsertNextTuple(tensorTuple);
    tensor->InsertNextTuple(tensorTuple);
    tensor->InsertNextTuple(tensorTuple);
    pd->SetTensors(tensor);

    vtkCellData* cd = pyramid->GetCellData();
    vtkNew<vtkDoubleArray> symTensor;
    symTensor->SetNumberOfComponents(6);
    double symTensorTuple[6] = { 1, 1, 1, 1, 17, 1 };
    symTensor->InsertNextTuple(symTensorTuple);
    symTensor->InsertNextTuple(symTensorTuple);
    symTensor->InsertNextTuple(symTensorTuple);
    symTensor->InsertNextTuple(symTensorTuple);
    cd->AddArray(symTensor);
  }

  vtkNew<vtkIdList> verts;
  verts->InsertNextId(0);
  verts->InsertNextId(1);
  verts->InsertNextId(2);
  verts->InsertNextId(3);
  verts->InsertNextId(4);

  pyramid->InsertNextCell(VTK_PYRAMID, verts);

  for (int i = 0; i < 2; i++)
  {
    vtkSmartPointer<vtkReflectionFilter> reflectionFilter =
      vtkSmartPointer<vtkReflectionFilter>::New();
    reflectionFilter->SetInputData(pyramid);
    if (i == 0)
    {
      reflectionFilter->CopyInputOff();
      reflectionFilter->FlipAllInputArraysOff();
    }
    else
    {
      reflectionFilter->CopyInputOn();
      reflectionFilter->FlipAllInputArraysOn();
    }
    reflectionFilter->SetPlaneToZMin();
    reflectionFilter->Update();
    vtkUnstructuredGrid* pyramid1 =
      vtkUnstructuredGrid::SafeDownCast(reflectionFilter->GetOutput());
    vtkNew<vtkIdList> cellIds;
    if (i == 0)
    {
      AssertMacro(pyramid1->GetNumberOfCells() == 1, "pyramid");
      AssertMacro(pyramid1->GetPointData()->GetTensors()->GetComponent(0, 2) == -7, "pyramid");
    }
    else
    {
      AssertMacro(pyramid1->GetNumberOfCells() == 2, "pyramid");
      AssertMacro(pyramid1->GetPointData()->GetArray(0)->GetComponent(5, 2) == -13, "pyramid");
      AssertMacro(pyramid1->GetCellData()->GetArray(0)->GetComponent(1, 4) == -17, "pyramid");
      AssertMacro(pyramid1->GetPointData()->GetTensors()->GetComponent(5, 2) == -7, "pyramid");
    }
    pyramid1->GetCellPoints(i, cellIds);
    int apex = cellIds->GetId(4);
    int offset = i == 0 ? 0 : 5;
    AssertMacro(apex == 4 + offset, "pyramid");
    for (int j = 0; j < 4; j++)
    {
      int next = cellIds->GetId((j + 1) % 4);
      int nextExpected = (cellIds->GetId(j) - offset + 3) % 4 + offset;
      AssertMacro(next == nextExpected, "pyramid");
    }
  }

  // Testing reflection of a quad and its triangulation
  vtkNew<vtkUnstructuredGrid> quad;
  vtkNew<vtkPoints> quadPoints;
  quadPoints->InsertNextPoint(0.0, 0.0, 0.0);
  quadPoints->InsertNextPoint(1.0, 0.0, 0.0);
  quadPoints->InsertNextPoint(0.0, 1.0, 0.0);
  quadPoints->InsertNextPoint(1.0, 1.0, 0.0);
  quad->SetPoints(quadPoints);

  vtkNew<vtkIdList> quadVerts;
  quadVerts->InsertNextId(0);
  quadVerts->InsertNextId(1);
  quadVerts->InsertNextId(2);
  quadVerts->InsertNextId(3);

  quad->InsertNextCell(VTK_QUAD, quadVerts);

  vtkNew<vtkReflectionFilter> quadReflectionFilter;
  quadReflectionFilter->SetInputData(quad);
  quadReflectionFilter->CopyInputOff();
  quadReflectionFilter->FlipAllInputArraysOn();
  quadReflectionFilter->SetPlaneToXMin();
  quadReflectionFilter->Update();

  vtkUnstructuredGrid* reflectedQuad =
    vtkUnstructuredGrid::SafeDownCast(quadReflectionFilter->GetOutput());

  // Verify positions of reflected vertices
  vtkPoints* reflectedQuadPts = reflectedQuad->GetPoints();
  double pt[3] = { 0.0, 0.0, 0.0 };
  reflectedQuadPts->GetPoint(0, pt);
  AssertMacro(pt[0] == 0.0 && pt[1] == 0.0 && pt[2] == 0.0, "quad");
  reflectedQuadPts->GetPoint(1, pt);
  AssertMacro(pt[0] == -1.0 && pt[1] == 0.0 && pt[2] == 0.0, "quad");
  reflectedQuadPts->GetPoint(2, pt);
  AssertMacro(pt[0] == 0.0 && pt[1] == 1.0 && pt[2] == 0.0, "quad");
  reflectedQuadPts->GetPoint(3, pt);
  AssertMacro(pt[0] == -1.0 && pt[1] == 1.0 && pt[2] == 0.0, "quad");

  // Verify cell points IDs
  vtkNew<vtkIdList> cellPtIds;
  reflectedQuad->GetCellPoints(0, cellPtIds);
  AssertMacro(cellPtIds->GetId(0) == 0 && cellPtIds->GetId(1) == 3 && cellPtIds->GetId(2) == 2 &&
      cellPtIds->GetId(3) == 1,
    "quad");

  // Test reflection of a triangle strip with even number of triangles
  vtkNew<vtkUnstructuredGrid> strip;
  vtkNew<vtkPoints> stripPoints;
  stripPoints->InsertNextPoint(0.0, 0.0, -0.25);
  stripPoints->InsertNextPoint(-1.0, 0.0, -0.25);
  stripPoints->InsertNextPoint(-1.0, 0.0, -1.0);
  stripPoints->InsertNextPoint(0.0, -0.5, -0.25);
  stripPoints->InsertNextPoint(-1.0, -0.5, -0.25);
  stripPoints->InsertNextPoint(-1.0, -0.5, -1.0);
  strip->SetPoints(stripPoints);

  vtkNew<vtkIdList> stripVerts;
  stripVerts->InsertNextId(3);
  stripVerts->InsertNextId(0);
  stripVerts->InsertNextId(4);
  stripVerts->InsertNextId(1);
  stripVerts->InsertNextId(5);
  stripVerts->InsertNextId(2);

  strip->InsertNextCell(VTK_TRIANGLE_STRIP, stripVerts);

  vtkNew<vtkReflectionFilter> stripReflectionFilter;
  stripReflectionFilter->SetInputData(strip);
  stripReflectionFilter->CopyInputOn();
  stripReflectionFilter->FlipAllInputArraysOn();
  stripReflectionFilter->SetPlaneToXMin();
  stripReflectionFilter->Update();

  vtkUnstructuredGrid* reflectedStrip =
    vtkUnstructuredGrid::SafeDownCast(stripReflectionFilter->GetOutput());

  AssertMacro(reflectedStrip->GetNumberOfPoints() == 12, "strip number of points");
  vtkPoints* reflectedStripPts = reflectedStrip->GetPoints();
  reflectedStripPts->GetPoint(0, pt);
  AssertMacro(pt[0] == 0 && pt[1] == 0 && pt[2] == -0.25, "strip point mismatch");
  reflectedStripPts->GetPoint(1, pt);
  AssertMacro(pt[0] == -1.0 && pt[1] == 0 && pt[2] == -0.25, "strip point mismatch");
  reflectedStripPts->GetPoint(2, pt);
  AssertMacro(pt[0] == -1.0 && pt[1] == 0 && pt[2] == -1.0, "strip point mismatch");
  reflectedStripPts->GetPoint(3, pt);
  AssertMacro(pt[0] == 0 && pt[1] == -0.5 && pt[2] == -0.25, "strip point mismatch");
  reflectedStripPts->GetPoint(4, pt);
  AssertMacro(pt[0] == -1.0 && pt[1] == -0.5 && pt[2] == -0.25, "strip point mismatch");
  reflectedStripPts->GetPoint(5, pt);
  AssertMacro(pt[0] == -1.0 && pt[1] == -0.5 && pt[2] == -1.0, "strip point mismatch");
  reflectedStripPts->GetPoint(6, pt);
  AssertMacro(pt[0] == -2.0 && pt[1] == 0 && pt[2] == -0.25, "strip point mismatch");
  reflectedStripPts->GetPoint(7, pt);
  AssertMacro(pt[0] == -1.0 && pt[1] == 0 && pt[2] == -0.25, "strip point mismatch");
  reflectedStripPts->GetPoint(8, pt);
  AssertMacro(pt[0] == -1.0 && pt[1] == 0 && pt[2] == -1.0, "strip point mismatch");
  reflectedStripPts->GetPoint(9, pt);
  AssertMacro(pt[0] == -2.0 && pt[1] == -0.5 && pt[2] == -0.25, "strip point mismatch");
  reflectedStripPts->GetPoint(10, pt);
  AssertMacro(pt[0] == -1.0 && pt[1] == -0.5 && pt[2] == -0.25, "strip point mismatch");
  reflectedStripPts->GetPoint(11, pt);
  AssertMacro(pt[0] == -1.0 && pt[1] == -0.5 && pt[2] == -1.0, "strip point mismatch");

  // There should be two strips
  AssertMacro(reflectedStrip->GetNumberOfCells() == 2, "Expected 2 triangle strips");

  // Check second strip
  cellPtIds->Initialize();
  reflectedStrip->GetCellPoints(1, cellPtIds);

  AssertMacro(cellPtIds->GetNumberOfIds() == 7, "Expected 7 ids for triangle strip");
  std::array<vtkIdType, 7> stripExpectedIds = { { 9, 10, 6, 10, 7, 11, 8 } };
  for (vtkIdType i = 0; i < cellPtIds->GetNumberOfIds(); ++i)
  {
    AssertMacro(cellPtIds->GetId(i) == stripExpectedIds[i], "Cell point id mismatch");
  }

  return EXIT_SUCCESS;
}
