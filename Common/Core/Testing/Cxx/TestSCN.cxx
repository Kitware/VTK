// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// A simple test to test VTK::scnlib is working as expected.

#include "vtk_scn.h"
// clang-format off
#include VTK_SCN(scn/scan.h)
#include VTK_SCN(scn/istream.h)
// clang-format on

#include <cassert>
#include <iostream>

int TestSCN(int, char*[])
{
  // Reading a std::string will read until the first whitespace character
  if (auto result = scn::scan<std::string>("Hello world!", "{}"))
  {
    // Will output "Hello":
    assert(result->value() == "Hello");
    // Access the read value with result->value()
    std::cout << result->value() << std::endl;
  }
  else
  {
    std::cout << "Couldn't parse a word: " << result.error().msg() << std::endl;
  }
  return 0;
}
