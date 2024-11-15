// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDoubleArray.h"
#include "vtkLogger.h"
#include "vtkPointData.h"
#include "vtkTableBasedClipDataSet.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridReader.h"

#include <vtkTestUtilities.h>
#include <vtkXMLMultiBlockDataReader.h>

int TestChangingScalars(int argc, char* argv[])
{
  char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/can.vtu");
  vtkNew<vtkXMLUnstructuredGridReader> reader;
  reader->SetFileName(fileName);
  reader->Update();
  delete[] fileName;

  vtkNew<vtkTableBasedClipDataSet> clip;
  clip->SetInputConnection(reader->GetOutputPort());
  clip->SetValue(0);
  clip->SetInputArrayToProcess(0, 0, 0, 0, "ACCL");
  clip->Update();

  vtkNew<vtkTableBasedClipDataSet> clip2;
  clip2->SetInputConnection(clip->GetOutputPort());
  clip2->SetValue(0);
  clip2->SetInputArrayToProcess(0, 0, 0, 0, "DISPL");
  clip2->Update();
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(clip2->GetOutputDataObject(0));

  int nArrays = output->GetPointData()->GetNumberOfArrays();
  if (nArrays != 3)
  {
    vtkLogF(ERROR, "Invalid number of arrays, expected 3, got %d", nArrays);
    return EXIT_FAILURE;
  }

  if (strcmp(output->GetPointData()->GetScalars()->GetName(), "DISPL") != 0)
  {
    vtkLogF(ERROR, "Missing data array.");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int TestTableBasedClipDataSet(int argc, char* argv[])
{
  return TestChangingScalars(argc, argv);
}
