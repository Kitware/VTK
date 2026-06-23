//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_cont_ArrayIsMonotonic_h
#define viskores_cont_ArrayIsMonotonic_h

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/viskores_cont_export.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace cont
{

/// \brief Determines if the values in the array are monotonically increasing.
///
/// An array is monotonically increasing if for every pair of indices i and j
/// such that i < j, the value at index i is less than or equal to the
/// value at index j.
/// This means that the values never decrease as you move through the
/// array from start to end.
/// If the array has less than two values, it is considered
/// to be monotonically increasing.
/// This function only works for scalar arrays (arrays with a single component per value).
/// An exception is thrown if the input is not a scalar array.
///
VISKORES_CONT_EXPORT bool ArrayIsMonotonicIncreasing(
  const viskores::cont::UnknownArrayHandle& input);

/// \brief Determines if the values in the array are monotonically decreasing.
///
/// An array is monotonically decreasing if for every pair of indices i and j
/// such that i < j, the value at index i is greater than or equal to the
/// value at index j.
/// This means that the values never increase as you move through the
/// array from start to end.
/// If the array has less than two values, it is considered
/// to be monotonically decreasing.
/// This function only works for scalar arrays (arrays with a single component per value).
/// An exception is thrown if the input is not a scalar array.
///
VISKORES_CONT_EXPORT bool ArrayIsMonotonicDecreasing(
  const viskores::cont::UnknownArrayHandle& input);

}
} // namespace viskores::cont

#endif //viskores_cont_ArrayIsMonotonic_h
