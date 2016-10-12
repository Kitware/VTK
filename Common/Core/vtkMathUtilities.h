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

/**
 * @class   vtkMathUtilities
 * @brief   templated utility math functions intended for
 * internal use in tests etc.
 *
 *
 * Provide a set of inline, lightweight templated math utility functions.
 * The initial motivation is to provide a few lightweight functions to help in
 * testing and internal implementation files.
*/

#ifndef vtkMathUtilities_h
#define vtkMathUtilities_h

#include <cmath>
#include <limits>

namespace vtkMathUtilities
{

/**
 * Perform a fuzzy compare of floats/doubles.
 */
template<class A>
bool FuzzyCompare(A a, A b)
{
  return fabs(a - b) < std::numeric_limits<A>::epsilon();
}

/**
 * Perform a fuzzy compare of floats/doubles, specify the allowed tolerance
 */
template<class A>
bool FuzzyCompare(A a, A b, A epsilon)
{
  return fabs(a - b) < epsilon;
}

/**
 * Performs safe division that catches overflow and underflow.
 */
template<class A>
A SafeDivision(A a, A b)
{
  // Avoid overflow
  if( (b < static_cast<A>(1)) && (a > b*std::numeric_limits<A>::max()) )
  {
    return std::numeric_limits<A>::max();
  }

  // Avoid underflow
  if( (a == static_cast<A>(0)) ||
      ((b > static_cast<A>(1)) && (a < b*std::numeric_limits<A>::min())) )
  {
    return static_cast<A>(0);
  }

  // safe to do the division
  return( a/b );
}

//@{
/**
 * A slightly different fuzzy comparator that checks if two values are
 * "nearly" equal based on Knuth, "The Art of Computer Programming (vol II)"
 */
template<class A>
bool NearlyEqual(A a, A b, A tol=std::numeric_limits<A>::epsilon())
{
  A absdiff = fabs(a-b);
  A d1  = vtkMathUtilities::SafeDivision<A>(absdiff,fabs(a));
  A d2  = vtkMathUtilities::SafeDivision<A>(absdiff,fabs(b));
//@}

  if( (d1 <= tol) || (d2 <= tol) )
  {
    return true;
  }
  return false;
}

} // End vtkMathUtilities namespace.

#endif // vtkMathUtilities_h
// VTK-HeaderTest-Exclude: vtkMathUtilities.h
