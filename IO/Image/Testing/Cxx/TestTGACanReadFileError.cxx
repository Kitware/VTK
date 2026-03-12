// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkNew.h"
#include "vtkTGAReader.h"

#include <iostream>

int TestTGACanReadFileError(int argc, char* argv[])
{
  if (argc <= 1)
  {
    std::cout << "Usage: " << argv[0] << " <non-tga file>" << std::endl;
    return EXIT_FAILURE;
  }

  std::string filename = argv[1];
  vtkNew<vtkTGAReader> tgaReader;

  // Verify that a non TGA file is correctly rejected
  if (tgaReader->CanReadFile(filename.c_str()))
  {
    std::cerr << "CanReadFile should have failed for " << filename << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
