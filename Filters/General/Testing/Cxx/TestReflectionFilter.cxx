/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestReflectionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
  if (!(b))                                                                                        \
  {                                                                                                \
    std::cerr << "Failed to reflect " << cell << std::endl;                                        \
    return EXIT_FAILURE;                                                                           \
  }

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

  pyramid->InsertNextCell(VTK_PYRAMID, verts.GetPointer());

  for (int i = 0; i < 2; i++)
  {
    vtkSmartPointer<vtkReflectionFilter> reflectionFilter =
      vtkSmartPointer<vtkReflectionFilter>::New();
    reflectionFilter->SetInputData(pyramid.GetPointer());
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
    pyramid1->GetCellPoints(i, cellIds.GetPointer());
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

  quad->InsertNextCell(VTK_QUAD, quadVerts.GetPointer());

  vtkNew<vtkReflectionFilter> quadReflectionFilter;
  quadReflectionFilter->SetInputData(quad.GetPointer());
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

  return EXIT_SUCCESS;
}
