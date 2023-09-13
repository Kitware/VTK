//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
#ifndef token_testing_helpers_h
#define token_testing_helpers_h

#include <array>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

#if !defined(_WIN32) && !defined(WIN32)
#include <unistd.h>
#else
#include <io.h>
#endif

/**\brief A function to generate a filename for testing.
  *
  * mkstemp is cumbersome and tmpnam is deprecated.
  */
inline std::string generateFilename(const std::string& prefix, const std::string& suffix)
{
#if !defined(_WIN32) && !defined(WIN32)
  std::array<char, 7> filenameTemplate{ 'X', 'X', 'X', 'X', 'X', 'X', '\0' };
  close(mkstemp(filenameTemplate.data()));
  std::remove(filenameTemplate.data());
#else
  std::array<char, 11> filenameTemplate{ 's', 'm', 't', 'k', 'X', 'X', 'X', 'X', 'X', 'X', '\0' };
  _mktemp_s(filenameTemplate.data(), 11);
#endif
  std::ostringstream filename;
  filename << prefix << filenameTemplate.data() << suffix;
  return filename.str();
}

/**\brief A function for unit tests that behaves like assert.
  *
  * When \a condition is non-zero, the test passes \a condition's
  * value as its output. Otherwise it throws an exception containing
  * the optional message string \a explanation.
  *
  * While this function behaves like assert, it doesn't get optimized
  * away in release builds.
  * Use this instead of assert to avoid getting "unused variable"
  * warnings in unit tests.
  */
inline int test(int condition, const std::string& explanation = std::string())
{
  if (!condition)
  {
    if (!explanation.empty())
    {
      std::cerr << "## TEST FAILURE ##\n\n  " << explanation << "\n\n## TEST FAILURE ##\n";
    }
    throw explanation;
  }
  return condition;
}

#endif
