// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test verifies that we can load a set of materials specification
// from disk and use them.

#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkOSPRayMaterialLibrary.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"

#include <string>

#include <iostream>

int TestPathTracerMaterialLibrary(int argc, char* argv[])
{
  // read an ospray material file
  const char* materialFile =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ospray_mats.json");
  vtkSmartPointer<vtkOSPRayMaterialLibrary> lib = vtkSmartPointer<vtkOSPRayMaterialLibrary>::New();
  std::cout << "Open " << materialFile << std::endl;
  lib->ReadFile(materialFile);
  std::cout << "Parsed file OK, now check for expected contents." << std::endl;
  std::set<std::string> mats = lib->GetMaterialNames();

  std::cout << "Materials are:" << std::endl;
  std::set<std::string>::iterator it = mats.begin();
  while (it != mats.end())
  {
    std::cout << *it << std::endl;
    ++it;
  }
  if (mats.find("Water") == mats.end())
  {
    std::cerr << "Problem, could not find expected material named water." << std::endl;
    return VTK_ERROR;
  }
  std::cout << "Found Water material." << std::endl;
  if (lib->LookupImplName("Water") != "glass")
  {
    std::cerr << "Problem, expected Water to be implemented by the glass material." << std::endl;
    return VTK_ERROR;
  }
  std::cout << "Water is the right type." << std::endl;
  if (lib->GetDoubleShaderVariable("Water", "attenuationColor").size() != 3)
  {
    std::cerr << "Problem, expected Water to have a 3 component variable called attentuationColor."
              << std::endl;
    return VTK_ERROR;
  }
  std::cout << "Water has an expected variable." << std::endl;
  if (lib->GetTexture("Bumpy", "map_bump") == nullptr)
  {
    std::cerr << "Problem, expected Bumpy to have a texture called map_bump." << std::endl;
    return VTK_ERROR;
  }
  std::cout << "Bumpy has a good texture too." << std::endl;
  std::string textureName = lib->GetTextureName("Bumpy", "map_bump");
  if (textureName != "vtk")
  {
    std::cerr << "Problem, expected Bumpy to have a 'map_bump' texture named 'vtk'." << std::endl;
    return VTK_ERROR;
  }
  std::cout << "Bumpy has a good texture name too." << std::endl;
  std::string textureFilename = lib->GetTextureFilename("Bumpy", "map_bump");
  if (textureFilename != vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vtk.png"))
  {
    std::cerr << "Problem, expected Bumpy to have a 'map_bump' texture with filename named vtk.png"
              << std::endl;
    return VTK_ERROR;
  }
  std::cout << "Bumpy has a good texture filename too." << std::endl;
  // read a wavefront mtl file
  const char* materialFile2 =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ospray_mats.mtl");
  std::cout << "Open " << materialFile2 << std::endl;

  lib->ReadFile(materialFile2);
  std::cout << "Parsed file OK, now check for expected contents." << std::endl;

  mats = lib->GetMaterialNames();
  std::cout << "Materials are now:" << std::endl;
  it = mats.begin();
  while (it != mats.end())
  {
    std::cout << *it << std::endl;
    ++it;
  }

  auto ks = lib->GetDoubleShaderVariable("mat1", "Ks");
  if (ks[2] != 0.882353)
  {
    std::cerr << "Problem, could not find expected material mat1 ks component." << std::endl;
    return VTK_ERROR;
  }

  if (mats.find("mat2") == mats.end())
  {
    std::cerr << "Problem, could not find expected material named mat2." << std::endl;
    return VTK_ERROR;
  }
  if (lib->GetDoubleShaderVariable("mat2", "Kd").size() == 0)
  {
    std::cerr << "Problem, expected mat2 to have a variable called Kd." << std::endl;
    return VTK_ERROR;
  }

  lib->RemoveAllShaderVariables("mat2");
  if (lib->GetDoubleShaderVariable("mat2", "Kd").size() > 0)
  {
    std::cerr << "Problem, expected mat2 to have Kd removed." << std::endl;
    return VTK_ERROR;
  }

  std::cout << "mat2 has an expected variable." << std::endl;
  if (lib->GetTexture("mat2", "map_Kd") == nullptr)
  {
    std::cerr << "Problem, expected mat2 to have a texture called map_Kd." << std::endl;
    return VTK_ERROR;
  }
  std::cout << "mat2 has a good texture too." << std::endl;

  textureName = lib->GetTextureName("mat2", "map_Kd");
  if (textureName != "vtk")
  {
    std::cerr << "Problem, expected mat2 to have a texture named 'vtk'." << std::endl;
    return VTK_ERROR;
  }
  std::cout << "mat2 has a good texture name too." << std::endl;
  textureFilename = lib->GetTextureFilename("mat2", "map_Kd");
  if (textureFilename != vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vtk.png"))
  {
    std::cerr << "Problem, expected Bumpy to have a 'map_bump' texture with filename named vtk.png"
              << std::endl;
    return VTK_ERROR;
  }
  std::cout << "mat2 has a good texture filename too." << std::endl;
  lib->RemoveAllTextures("mat2");
  if (lib->GetTexture("mat2", "map_Kd") != nullptr)
  {
    std::cerr << "Problem, expected mat2 to have map_Kd removed." << std::endl;
    return VTK_ERROR;
  }

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
