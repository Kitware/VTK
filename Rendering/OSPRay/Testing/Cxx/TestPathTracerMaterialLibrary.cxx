/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPathTracerMaterials.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that we can load a set of materials specification
// from disk and use them.

#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkOSPRayMaterialLibrary.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"

int TestPathTracerMaterialLibrary(int argc, char* argv[])
{
  const char* materialFile = vtkTestUtilities::ExpandDataFileName(
                            argc, argv, "Data/ospray_mats.json");
  vtkSmartPointer<vtkOSPRayMaterialLibrary> lib =
    vtkSmartPointer<vtkOSPRayMaterialLibrary>::New();
  cout << "Open " << materialFile << endl;
  lib->ReadFile(materialFile);
  cout << "Parsed file OK, now check for expected contents." << endl;
  std::set<std::string> mats = lib->GetMaterialNames();
  if (mats.find("Water") == mats.end())
  {
    cerr << "Problem, could not find expected material named water." << endl;
    return VTK_ERROR;
  }
  cout << "Found Water material." << endl;
  if (lib->LookupImplName("Water") != "Glass")
  {
    cerr << "Problem, expected Water's to be implemented by the Glass material." << endl;
    return VTK_ERROR;
  }
  cout << "Water is the right type." << endl;
  if (lib->GetDoubleShaderVariable("Water","attenuationColor").size() != 3)
  {
    cerr << "Problem, expected Water's to have a 3 component variable called attentuationColor." << endl;
    return VTK_ERROR;
  }
  cout << "Water has an expected variable." << endl;
  if (lib->GetTexture("Bumpy","map_bump") == nullptr)
  {
    cerr << "Problem, expected Bumpy to have a texture called map_bump." << endl;
    return VTK_ERROR;
  }
  cout << "We read in a texture too." << endl;

  cout << "We're all clear kid." << endl;


  cout << "Serialize" << endl;
  const char *buf = lib->WriteBuffer();

  cout << "Deserialize" << endl;
  lib->ReadBuffer(buf);

  return 0;
}
