/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestFMT.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// A simple test to test VTK::fmt is working as expected.

#include "vtkLogger.h"
#include "vtk_fmt.h"

#include <vector>

// clang-format off
#include VTK_FMT(fmt/core.h)
#include VTK_FMT(fmt/ranges.h)
// clang-format on

int TestFMT(int, char*[])
{
  fmt::print("Hello, {}!\n", "World");

  std::vector<int> v = { 1, 2, 3 };
  fmt::print("vector: {}\n", v);
  return EXIT_SUCCESS;
}
