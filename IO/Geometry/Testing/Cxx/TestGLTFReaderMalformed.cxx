// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkGLTFReader.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"

//------------------------------------------------------------------------------
int TestGLTFReaderMalformed(int argc, char* argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/malformed.glb");

  // Just not segfaulting is considered a success
  vtkNew<vtkGLTFReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  return EXIT_SUCCESS;
}
