// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAMReXGridReader.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkOverlappingAMR.h"
#include "vtkTestUtilities.h"

int TestAMReXGridReaderNonZeroOrigin(int argc, char* argv[])
{
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/AMReX/NonZeroOrigin/plt00000");
  vtkNew<vtkAMReXGridReader> reader;
  reader->SetFileName(fname);
  delete[] fname;
  reader->UpdateInformation();
  reader->Update();

  auto amr = reader->GetOutput();
  double origin[3];
  amr->GetOrigin(0, 0, origin);

  if (origin[0] != 0 || origin[1] != 0.001 || origin[2] != 0.001)
  {
    vtkLogF(ERROR, "Failed: incorrect origin.");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
