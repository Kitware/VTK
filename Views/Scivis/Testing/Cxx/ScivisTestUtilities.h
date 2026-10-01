// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef ScivisTestUtilities_h
#define ScivisTestUtilities_h

#include <cstdlib>
#include <iostream>

// Fail the enclosing test function, printing msg, when expr is false.  msg may
// be a chain of stream insertions, as in CHECK(n == 3, "got " << n << " bars").
#define CHECK(expr, msg)                                                                           \
  do                                                                                               \
  {                                                                                                \
    if (!(expr))                                                                                   \
    {                                                                                              \
      std::cerr << "FAILED: " << msg << "\n";                                                      \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

#endif
