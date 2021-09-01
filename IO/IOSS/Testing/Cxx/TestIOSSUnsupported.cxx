/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIOSSUnsupported.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkIOSSReader.h>
#include <vtkNew.h>
#include <vtkTestUtilities.h>

static bool TestFile(const std::string& fname)
{
  vtkObject::SetGlobalWarningDisplay(0);
  vtkNew<vtkIOSSReader> reader;
  reader->AddFileName(fname.c_str());
  reader->Update();
  vtkObject::SetGlobalWarningDisplay(1);
  return true;
}

static std::string GetFileName(int argc, char* argv[], const char* fnameC)
{
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, fnameC);
  std::string fname(fileNameC);
  delete[] fileNameC;
  return fname;
}

/**
 * This test open various unsupported files and ensures that the reader raises
 * errors as expected without crashing.
 */
int TestIOSSUnsupported(int argc, char* argv[])
{
  if (TestFile(GetFileName(argc, argv, "Data/Exodus/test-nfaced.exo")))
  {
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}
