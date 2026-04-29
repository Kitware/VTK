// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAMReXGridReader.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkOverlappingAMR.h"
#include "vtkPointData.h"
#include "vtkTestUtilities.h"

#include <iostream>

#define ensure(x, msg)                                                                             \
  do                                                                                               \
  {                                                                                                \
    if (!(x))                                                                                      \
    {                                                                                              \
      std::cerr << "FAILED: " << msg << std::endl;                                                 \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

namespace
{
int ValidateNodalMainFab(vtkOverlappingAMR* mb)
{
  ensure(mb != nullptr, "expecting Overlapping AMR Dataset.");
  ensure(mb->GetNumberOfLevels() == 2, "expecting num-levels == 2");

  // Level 0: nodal box ((0,0)-(64,64)) -> 65x65 nodes (= 64x64 cells).
  vtkImageData* l0 = mb->GetDataSetAsImageData(0, 0);
  ensure(l0 != nullptr, "expecting level 0 block as vtkImageData.");
  int dims0[3];
  l0->GetDimensions(dims0);
  ensure(dims0[0] == 65 && dims0[1] == 65 && dims0[2] == 1,
    "expecting level 0 dims (65,65,1) for nodal main fab");
  ensure(l0->GetNumberOfPoints() == 65 * 65, "expecting 65*65 points on level 0");
  ensure(l0->GetPointData()->GetArray("alpha") != nullptr,
    "expecting nodal main-fab variable 'alpha' on level 0 PointData");
  ensure(
    l0->GetCellData()->GetArray("alpha") == nullptr, "'alpha' must not be attached as cell data");

  // Level 1: nodal box ((48,48)-(80,80)) -> 33x33 nodes.
  vtkImageData* l1 = mb->GetDataSetAsImageData(1, 0);
  ensure(l1 != nullptr, "expecting level 1 block as vtkImageData.");
  int dims1[3];
  l1->GetDimensions(dims1);
  ensure(dims1[0] == 33 && dims1[1] == 33 && dims1[2] == 1,
    "expecting level 1 dims (33,33,1) for nodal main fab");

  return EXIT_SUCCESS;
}
}

int TestAMReXGridReaderNodalMainFab(int argc, char* argv[])
{
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/AMReX/NodalMainFab/plt00000");
  vtkNew<vtkAMReXGridReader> reader;
  reader->SetFileName(fname);
  delete[] fname;

  reader->SetMaxLevel(2);
  reader->UpdateInformation();

  // All three main-fab variables must be exposed as point arrays, none as cell.
  ensure(reader->GetNumberOfPointArrays() == 3, "expected 3 nodal point arrays");
  ensure(reader->GetNumberOfCellArrays() == 0,
    "expected no cell arrays (main fab is nodal, no extra cell multifabs)");

  for (int i = 0; i < reader->GetNumberOfPointArrays(); ++i)
  {
    reader->SetPointArrayStatus(reader->GetPointArrayName(i), 1);
  }

  reader->Update();
  return ValidateNodalMainFab(reader->GetOutput());
}
