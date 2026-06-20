// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPolyDataReader.h"
#include "vtkTableBasedClipDataSet.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

int TestClipPolyLine(int argc, char* argv[])
{
  char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/UG_3D_traces.vtk");
  vtkNew<vtkPolyDataReader> reader;
  reader->SetFileName(fileName);
  delete[] fileName;

  vtkNew<vtkPlane> plane;
  plane->SetOrigin(386856.1875, 6524147.5, -908.9945526123047);
  plane->SetNormal(1.0, 0.0, 0.0);

  vtkNew<vtkTableBasedClipDataSet> clip;
  clip->SetInputConnection(reader->GetOutputPort());
  clip->SetClipFunction(plane);
  clip->SetInsideOut(true);
  clip->Update();

  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(clip->GetOutputDataObject(0));
  const int nCells = static_cast<int>(output->GetNumberOfCells());
  constexpr int expectedCells = 10069;
  if (nCells != expectedCells)
  {
    vtkLogF(ERROR, "Expected %d cells, but got %d", expectedCells, nCells);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
