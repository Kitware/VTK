// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// A simple test to test VTK::scnlib is working as expected.

#include "vtk_scn.h"
// clang-format off
#include VTK_SCN(scn/scan.h)
#include VTK_SCN(scn/istream.h)
// clang-format on

#include <cassert>
#include <cstdio>
#include <fstream>
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
    std::cerr << "Couldn't parse a word: " << result.error().msg() << std::endl;
  }

  // dump hello world to a file
  std::ofstream fout;
  fout.open("test.txt");
  fout << "Hello world";
  fout.close();

  // create std::FILE* from a file
  std::FILE* file = std::fopen("test.txt", "r");
  if (!file)
  {
    std::cerr << "Failed to open file" << std::endl;
    return EXIT_FAILURE;
  }
  if (auto resultIO = scn::scan<std::string, std::string>(file, "{:s} {:s}"))
  {
    auto [hello, world] = resultIO->values();
    std::cout << hello << " " << world << std::endl;
  }
  else
  {
    std::cerr << "Couldn't parse a word: " << resultIO.error().msg() << std::endl;
  }
  // close the file
  std::fclose(file);
  // delete the file
  std::remove("test.txt");

  return EXIT_SUCCESS;
}
