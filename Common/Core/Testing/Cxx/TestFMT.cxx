// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// A simple test to test VTK::fmt is working as expected.

#include "vtkLogger.h"
#include "vtk_fmt.h"

#include <vector>

// clang-format off
#include VTK_FMT(fmt/base.h)
#include VTK_FMT(fmt/ranges.h)
// clang-format on

int TestFMT(int, char*[])
{
  fmt::print("Hello, {}!\n", "World");

  std::vector<int> v = { 1, 2, 3 };
  fmt::print("vector: {}\n", v);
  return EXIT_SUCCESS;
}
