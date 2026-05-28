// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test verifies that we can load a set of ANARI materials specification
// from disk and use them.

#include "vtkTestUtilities.h"

#include "vtkANARIMaterialLibrary.h"
#include "vtkActor.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"

#include <fstream>
#include <iostream>
#include <set>
#include <string>

int TestAnariMaterialLibrary(int argc, char* argv[])
{
  vtkSmartPointer<vtkANARIMaterialLibrary> lib = vtkSmartPointer<vtkANARIMaterialLibrary>::New();

  // Try MTL file first (simpler format)
  const char* mtlFile =
    "/Users/sankhesh.jhaveri/Projects/vtk/vtk/Rendering/ANARI/Testing/Data/anari_mats.mtl";
  std::cout << "Open " << mtlFile << std::endl;
  lib->ReadFile(mtlFile);
  std::cout << "Parsed MTL file OK, now check for expected contents." << std::endl;
  std::set<std::string> mats = lib->GetMaterialNames();

  std::cout << "Materials are:" << std::endl;
  std::set<std::string>::iterator it = mats.begin();
  while (it != mats.end())
  {
    std::cout << *it << std::endl;
    ++it;
  }

  // Test mat1 from MTL file
  if (mats.find("mat1") == mats.end())
  {
    std::cerr << "Problem, could not find expected material named mat1 from MTL." << std::endl;
    return VTK_ERROR;
  }
  std::cout << "Found mat1 material from MTL." << std::endl;

  if (lib->LookupImplName("mat1") != "obj")
  {
    std::cerr << "Problem, expected mat1 to be of type obj." << std::endl;
    return VTK_ERROR;
  }
  std::cout << "mat1 is the correct type." << std::endl;

  if (mats.find("mat2") == mats.end())
  {
    std::cerr << "Problem, could not find expected material named mat2." << std::endl;
    return VTK_ERROR;
  }
  std::cout << "Found mat2 material." << std::endl;

  if (mats.find("mat3") == mats.end())
  {
    std::cerr << "Problem, could not find expected material named mat3." << std::endl;
    return VTK_ERROR;
  }
  if (lib->LookupImplName("mat3") != "metal")
  {
    std::cerr << "Problem, expected mat3 to be implemented by the metal material." << std::endl;
    return VTK_ERROR;
  }
  std::cout << "mat3 is the right type." << std::endl;

  std::cout << "We're all clear kid." << std::endl;

  // serialize and deserialize
  std::cout << "Serialize" << std::endl;
  const char* buf = lib->WriteBuffer();

  std::cout << "Deserialize" << std::endl;
  lib->ReadBuffer(buf);

  return 0;
}
