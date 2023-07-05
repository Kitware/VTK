// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkClipDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkQuadraticTetra.h"
#include "vtkUnstructuredGrid.h"

#include <cstdlib>
#include <numeric>

namespace
{
vtkSmartPointer<vtkQuadraticTetra> MakeReferenceTet();
vtkSmartPointer<vtkUnstructuredGrid> MakeDummyGrid();
}

int TestHOStableClip(int, char*[])
{
  vtkNew<vtkClipDataSet> clip;
  clip->SetInputDataObject(0, ::MakeDummyGrid());
  clip->SetValue(0.7);
  clip->SetInsideOut(true);
  clip->Update();
  auto out = clip->GetOutput();

  // output should have 1 non-linear tetra, 7 linear tetras and 1 wedge from the decomposition
  return out->GetNumberOfCells() == 9 ? EXIT_SUCCESS : EXIT_FAILURE;
}

namespace
{

vtkSmartPointer<vtkQuadraticTetra> MakeReferenceTet()
{
  vtkNew<vtkDoubleArray> coords;
  coords->SetNumberOfComponents(3);
  coords->SetNumberOfTuples(10);
  coords->SetTuple3(0, 0, 0, 0);
  coords->SetTuple3(1, 1, 0, 0);
  coords->SetTuple3(2, 0, 1, 0);
  coords->SetTuple3(3, 0, 0, 1);
  coords->SetTuple3(4, 0.5, 0, 0);
  coords->SetTuple3(5, 0.5, 0.5, 0);
  coords->SetTuple3(6, 0, 0.5, 0);
  coords->SetTuple3(7, 0, 0, 0.5);
  coords->SetTuple3(8, 0.5, 0, 0.5);
  coords->SetTuple3(9, 0, 0.5, 0.5);

  vtkNew<vtkPoints> points;
  points->SetData(coords);

  vtkNew<vtkQuadraticTetra> tet;
  tet->Initialize(10, points);

  return tet;
}

vtkSmartPointer<vtkUnstructuredGrid> MakeDummyGrid()
{
  vtkNew<vtkUnstructuredGrid> dummyGrid;
  dummyGrid->Initialize();

  auto refTet = MakeReferenceTet();

  vtkNew<vtkDoubleArray> allPoints;
  allPoints->SetNumberOfComponents(3);
  allPoints->SetNumberOfTuples(30);

  std::array<double, 3> coords = { 0 };
  for (vtkIdType iP = 0; iP < 10; ++iP)
  {
    refTet->GetPoints()->GetData()->GetTuple(iP, coords.data());
    allPoints->SetTuple(iP, coords.data());
    coords[0] -= 1;
    allPoints->SetTuple(iP + 10, coords.data());
    coords[0] += 2;
    allPoints->SetTuple(iP + 20, coords.data());
  }

  vtkNew<vtkPoints> points;
  points->SetData(allPoints);
  dummyGrid->SetPoints(points);

  dummyGrid->Allocate(3);
  std::array<vtkIdType, 10> connectivity = { 0 };
  std::iota(connectivity.begin(), connectivity.end(), 0);
  dummyGrid->InsertNextCell(refTet->GetCellType(), 10, connectivity.data());
  std::iota(connectivity.begin(), connectivity.end(), 10);
  dummyGrid->InsertNextCell(refTet->GetCellType(), 10, connectivity.data());
  std::iota(connectivity.begin(), connectivity.end(), 20);
  dummyGrid->InsertNextCell(refTet->GetCellType(), 10, connectivity.data());

  vtkNew<vtkDoubleArray> dummyScalars;
  dummyScalars->SetName("X");
  dummyScalars->SetNumberOfComponents(1);
  dummyScalars->SetNumberOfTuples(30);
  for (vtkIdType iP = 0; iP < 30; ++iP)
  {
    dummyScalars->SetValue(iP,
      dummyGrid->GetPoints()->GetData()->GetComponent(
        iP, 0)); // the field is set to the x coordinate
  }

  dummyGrid->GetPointData()->AddArray(dummyScalars);
  dummyGrid->GetPointData()->SetScalars(dummyGrid->GetPointData()->GetArray("X"));
  return dummyGrid;
}

}
