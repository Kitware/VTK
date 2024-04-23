// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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

#include "vtkABINamespace.h"

#include <cmath>
#include <limits>
#include <tuple>
#include <type_traits>

namespace vtkMathUtilities
{
VTK_ABI_NAMESPACE_BEGIN

/**
 * Perform a fuzzy compare of floats/doubles, specify the allowed tolerance
 * NB: this uses an absolute tolerance.
 */
template <class A>
bool FuzzyCompare(A a, A b, A epsilon = std::numeric_limits<A>::epsilon())
{
  return fabs(a - b) < epsilon;
}

/**
 * Performs safe division that catches overflow and underflow.
 */
template <class A>
A SafeDivision(A a, A b)
{
  // Avoid overflow
  if ((b < static_cast<A>(1)) && (a > b * std::numeric_limits<A>::max()))
  {
    return std::numeric_limits<A>::max();
  }

  // Avoid underflow
  if ((a == static_cast<A>(0)) ||
    ((b > static_cast<A>(1)) && (a < b * std::numeric_limits<A>::min())))
  {
    return static_cast<A>(0);
  }

  // safe to do the division
  return (a / b);
}

/**
 * A slightly different fuzzy comparator that checks if two values are
 * "nearly" equal based on Knuth, "The Art of Computer Programming (vol II)"
 * NB: this uses a relative tolerance.
 */
template <class A>
bool NearlyEqual(A a, A b, A tol = std::numeric_limits<A>::epsilon())
{
  A absdiff = fabs(a - b);
  A d1 = vtkMathUtilities::SafeDivision<A>(absdiff, fabs(a));
  A d2 = vtkMathUtilities::SafeDivision<A>(absdiff, fabs(b));

  return ((d1 <= tol) || (d2 <= tol));
}

/**
 * Update an existing min - max range with a new prospective value.  If the
 * value is non NaN then the appropriate range comparisons are made and
 * updated, otherwise the original min - max values are set.
 *
 * Examples:
 *
 *   No change:
 *   UpdateRange(-100, 100, 20) -> (-100, 100)
 *
 *   Update min:
 *   UpdateRange(-100, 100, -200) -> (-200, 100)
 *
 *   Update max:
 *   UpdateRange(-100, 100, 200) -> (-100, 200)
 *
 *   Input min and max are inverted creating an invalid range so a new range
 *   with the specified value is set:
 *   UpdateRange(100, -100, 20) -> (20, 20)
 *
 *   Input value is NaN so the original range is set
 *   UpdateRange(-100, 100, NaN) -> (-100, 100)
 */
template <class A>
void UpdateRangeImpl(A& min0, A& max0, const A& value)
{
  // need temporaries to handle const/non const ref mismatch
  if (value < min0)
  {
    min0 = value;
    max0 = max0 < value ? value : max0;
  }
  else if (value > max0)
  {
    min0 = min0 > value ? value : min0;
    max0 = value;
  }
}

template <class A> // Non floating point implementation not caring about NaN
void UpdateRange(A& min0, A& max0, const A& value,
  typename std::enable_if<!std::is_floating_point<A>::value>::type* = nullptr)
{
  UpdateRangeImpl<A>(min0, max0, value);
}

template <class A> // Floating point implementation specifically considering NaN
void UpdateRange(A& min0, A& max0, const A& value,
  typename std::enable_if<std::is_floating_point<A>::value>::type* = nullptr)
{
  if (!std::isnan(value))
  {
    UpdateRangeImpl<A>(min0, max0, value);
  }
}

VTK_ABI_NAMESPACE_END
} // End vtkMathUtilities namespace.

#endif // vtkMathUtilities_h
// VTK-HeaderTest-Exclude: vtkMathUtilities.h
