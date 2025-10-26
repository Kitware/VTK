// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkDataArraySelection.h>
#include <vtkIOSSReader.h>
#include <vtkTestUtilities.h>

static std::string GetFileName(int argc, char* argv[], const std::string& fnameC)
{
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, fnameC.c_str());
  std::string fname(fileNameC);
  delete[] fileNameC;
  return fname;
}

int TestIOSSGroupAlphabeticVectorFieldComponents(int argc, char* argv[])
{
  auto fname = GetFileName(argc, argv, std::string("Data/Exodus/Flow1D.e"));
  vtkNew<vtkIOSSReader> reader;
  reader->AddFileName(fname.c_str());
  reader->SetGroupAlphabeticVectorFieldComponents(false);
  reader->Update();
  auto elementBlocks = reader->GetElementBlockFieldSelection();
  // print all arrays
  const bool correctResult = elementBlocks->ArrayExists("vel") &&
    elementBlocks->ArrayExists("vel_x") && elementBlocks->ArrayExists("vel_y") &&
    elementBlocks->ArrayExists("vel_z");
  return correctResult ? EXIT_SUCCESS : EXIT_FAILURE;
}
