/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIossFilePatternMatching.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * Test vtkIossReader::GetRelatedFiles(...).
 */
#include "vtkIossFilesScanner.h"
#include "vtkObject.h"

bool Verify(const std::set<std::string>& original, const std::vector<std::string>& dir_listing,
  const std::set<std::string>& expected)
{
  return (vtkIossFilesScanner::GetRelatedFiles(original, dir_listing) == expected);
}

int TestIossFilePatternMatching(int, char*[])
{
  if (Verify({ "mysimoutput.e-s.000" },
        { "mysimoutput.e-s.000", "mysimoutput.e-s.001", "mysimoutput.e-s.002" },
        { "mysimoutput.e-s.000", "mysimoutput.e-s.001", "mysimoutput.e-s.002" }))
  {
    return EXIT_SUCCESS;
  }

  if (Verify({ "/tmp/mysimoutput.e-s.000" },
        { "mysimoutput.e-s.000", "mysimoutput.e-s.001", "mysimoutput.e-s.002" },
        { "/tmp/mysimoutput.e-s.000", "/tmp/mysimoutput.e-s.001", "/tmp/mysimoutput.e-s.002" }))
  {
    return EXIT_SUCCESS;
  }

  if (Verify({ "C:\\Directory\\mysimoutput.e-s.000" },
        { "mysimoutput.e-s.000", "mysimoutput.e-s.001", "mysimoutput.e-s.002" },
        { "C:/Directory/mysimoutput.e-s.000", "C:/Directory/mysimoutput.e-s.001",
          "C:/Directory/mysimoutput.e-s.002" }))
  {
    return EXIT_SUCCESS;
  }

  if (Verify({ "/tmp space/mysimoutput.e-s.000" },
        { "mysimoutput.e-s.000", "mysimoutput.e-s.001", "mysimoutput.e-s.002" },
        { "/tmp space/mysimoutput.e-s.000", "/tmp space/mysimoutput.e-s.001",
          "/tmp space/mysimoutput.e-s.002" }))
  {
    return EXIT_SUCCESS;
  }

  if (Verify({ "C:\\Directory space\\mysimoutput.e-s.000" },
        { "mysimoutput.e-s.000", "mysimoutput.e-s.001", "mysimoutput.e-s.002" },
        { "C:/Directory space/mysimoutput.e-s.000", "C:/Directory space/mysimoutput.e-s.001",
          "C:/Directory space/mysimoutput.e-s.002" }))
  {
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}
