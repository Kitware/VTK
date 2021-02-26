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
#include "vtkIossReader.h"
#include "vtkObject.h"

int TestIossFilePatternMatching(int, char*[])
{
  return vtkIossReader::DoTestFilePatternMatching() ? EXIT_SUCCESS : EXIT_FAILURE;
}
