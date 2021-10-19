/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBlueObeliskData.h"

#include "vtksys/FStream.hxx"

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    return EXIT_FAILURE;
  }
  std::string srcdir = argv[1];

  vtksys::ifstream xml(srcdir + "/elements.xml");
  if (!xml)
  {
    std::cerr << "Error opening file " VTK_BODR_DATA_PATH_BUILD "/elements.xml.\n";
    return EXIT_FAILURE;
  }

  std::cout << "// VTK/Domains/Chemistry/Testing/Cxx/GenerateBlueObeliskHeader.cxx\n";
  if (!vtkBlueObeliskData::GenerateHeaderFromXML(xml, std::cout))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
