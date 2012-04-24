/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMathUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkMathUtilities - templated utility math functions intended for
// internal use in tests etc.
//
// .SECTION Description
// Provide a set of inline, lightweight templated math utility functions.
// The initial motivation is to provide a few lightweight functions to help in
// testing and internal implementation files.

#ifndef __vtkMathUtilities_h
#define __vtkMathUtilities_h

#include <cmath>
#include <limits>

namespace vtkMathUtilities
{

// Description:
// Perform a fuzzy compare of floats/doubles.
template<class A>
bool FuzzyCompare(A a, A b)
{
  return fabs(a - b) < std::numeric_limits<A>::epsilon();
}

//Description:
// Perform a fuzzy compare of floats/doubles, specify the allowed tolerance
template<class A>
bool FuzzyCompare(A a, A b, A epsilon)
{
  return fabs(a - b) < epsilon;
}

} // End vtkMathUtilities namespace.

#endif // __vtkMathUtilities_h
// VTK-HeaderTest-Exclude: vtkMathUtilities.h
