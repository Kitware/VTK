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
#ifndef viskores_BinaryPredicates_h
#define viskores_BinaryPredicates_h

#include <viskores/internal/ExportMacros.h>

namespace viskores
{

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns True if and only if \c x is not equal to \c y.
/// @note: Requires that types T and U are comparable with !=.
struct NotEqual
{
  template <typename T, typename U>
  VISKORES_EXEC_CONT bool operator()(const T& x, const U& y) const
  {
    return x != y;
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns True if and only if \c x is equal to \c y.
/// @note: Requires that types T and U are comparable with !=.
struct Equal
{
  template <typename T, typename U>
  VISKORES_EXEC_CONT bool operator()(const T& x, const U& y) const
  {
    return x == y;
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns True if and only if \c x is less than \c y.
/// @note: Requires that types T and U are comparable with <.
struct SortLess
{
  template <typename T, typename U>
  VISKORES_EXEC_CONT bool operator()(const T& x, const U& y) const
  {
    return x < y;
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns True if and only if \c x is greater than \c y.
/// @note: Requires that types T and U are comparable via operator<(U, T).

struct SortGreater
{
  template <typename T, typename U>
  VISKORES_EXEC_CONT bool operator()(const T& x, const U& y) const
  {
    return y < x;
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns True if and only if \c x and \c y are True.
/// @note: Requires that types T and U are comparable with &&.

struct LogicalAnd
{
  template <typename T, typename U>
  VISKORES_EXEC_CONT bool operator()(const T& x, const U& y) const
  {
    return x && y;
  }
};

/// Binary Predicate that takes two arguments argument \c x, and \c y and
/// returns True if and only if \c x or \c y is True.
/// @note: Requires that types T and U are comparable with ||.
struct LogicalOr
{
  template <typename T, typename U>
  VISKORES_EXEC_CONT bool operator()(const T& x, const U& y) const
  {
    return x || y;
  }
};

} // namespace viskores

#endif //viskores_BinaryPredicates_h
