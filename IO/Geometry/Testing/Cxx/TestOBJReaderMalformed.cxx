// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDebugLeaks.h"
#include "vtkOBJReader.h"

#include "vtkCellArray.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkTestUtilities.h"

//------------------------------------------------------------------------------
int TestOBJReaderMalformed(int argc, char* argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/malformed.obj");

  // Just not segfaulting is considered a success
  vtkNew<vtkOBJReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  return EXIT_SUCCESS;
}
