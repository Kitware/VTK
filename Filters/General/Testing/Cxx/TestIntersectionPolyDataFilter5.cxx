// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkIntersectionPolyDataFilter.h"

#include "vtkCellArray.h"
#include "vtkLogger.h"
#include "vtkPoints.h"
#include "vtkTriangle.h"

#include <array>

namespace
{
void CreateTriangle(const std::array<std::array<float, 3>, 3>& pts, vtkPolyData* trianglePolyData)
{
  vtkNew<vtkPoints> trianglePoints;
  trianglePoints->InsertNextPoint(pts[0][0], pts[0][1], pts[0][2]);
  trianglePoints->InsertNextPoint(pts[1][0], pts[1][1], pts[1][2]);
  trianglePoints->InsertNextPoint(pts[2][0], pts[2][1], pts[2][2]);

  vtkNew<vtkTriangle> triangle;
  triangle->GetPointIds()->SetId(0, 0);
  triangle->GetPointIds()->SetId(1, 1);
  triangle->GetPointIds()->SetId(2, 2);

  vtkNew<vtkCellArray> triangleCells;
  triangleCells->InsertNextCell(triangle);

  trianglePolyData->SetPoints(trianglePoints);
  trianglePolyData->SetPolys(triangleCells);
}
}

// This tests the edge case when 2 points of a triangle, lies in the plane of the other one.
int TestIntersectionPolyDataFilter5(int, char*[])
{
  std::array<std::array<float, 3>, 3> pts1 = { { { -0.25f, 0.5f, -0.5f }, { -0.25f, 0.5f, 0.5f },
    { 0.25f, 0.5f, -0.5f } } };
  std::array<std::array<float, 3>, 3> pts2 = { { { -0.25f, 0.5f, 0.0f }, { -0.25f, 1.5f, 0.0f },
    { 0.25f, 0.5f, 0.0f } } };
  std::array<std::array<float, 3>, 3> pts3 = { { { -0.25f, 0.5f, 0.0f }, { -0.25f, -1.5f, 0.0f },
    { 0.25f, 0.5f, 0.0f } } };

  vtkNew<vtkPolyData> triangle1PolyData;
  vtkNew<vtkPolyData> triangle2PolyData;
  vtkNew<vtkPolyData> triangle3PolyData;
  ::CreateTriangle(pts1, triangle1PolyData);
  ::CreateTriangle(pts2, triangle2PolyData);
  ::CreateTriangle(pts3, triangle3PolyData);

  vtkNew<vtkIntersectionPolyDataFilter> interFilter1;
  interFilter1->SetInputData(0, triangle1PolyData);
  interFilter1->SetInputData(1, triangle2PolyData);
  interFilter1->SplitFirstOutputOff();
  interFilter1->SplitSecondOutputOff();
  interFilter1->Update();
  vtkPolyData* output1 = interFilter1->GetOutput(0);

  if (output1->GetNumberOfCells() != 1)
  {
    vtkLog(ERROR, "Expected 1 cell, but got " << output1->GetNumberOfCells());
    return EXIT_FAILURE;
  }

  vtkNew<vtkIntersectionPolyDataFilter> interFilter2;
  interFilter2->SetInputData(0, triangle1PolyData);
  interFilter2->SetInputData(1, triangle3PolyData);
  interFilter2->SplitFirstOutputOff();
  interFilter2->SplitSecondOutputOff();
  interFilter2->Update();
  vtkPolyData* output2 = interFilter2->GetOutput(0);

  if (output2->GetNumberOfCells() != 0)
  {
    vtkLog(ERROR, "Expected 0 cell, but got " << output2->GetNumberOfCells());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
