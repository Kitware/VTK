/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAMReXGridReaderNonZeroOrigin.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
