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
#ifndef viskores_UnaryPredicates_h
#define viskores_UnaryPredicates_h

#include <viskores/TypeTraits.h>
#include <viskores/internal/ExportMacros.h>

namespace viskores
{

/// Predicate that takes a single argument \c x, and returns
/// True if it is the identity of the Type \p T.
struct IsZeroInitialized
{
  template <typename T>
  VISKORES_EXEC_CONT bool operator()(const T& x) const
  {
    return (x == viskores::TypeTraits<T>::ZeroInitialization());
  }
};

/// Predicate that takes a single argument \c x, and returns
/// True if it isn't the identity of the Type \p T.
struct NotZeroInitialized
{
  template <typename T>
  VISKORES_EXEC_CONT bool operator()(const T& x) const
  {
    return (x != viskores::TypeTraits<T>::ZeroInitialization());
  }
};

/// Predicate that takes a single argument \c x, and returns
/// True if and only if \c x is \c false.
/// Note: Requires Type \p T to be convertible to \c bool or implement the
/// ! operator.
struct LogicalNot
{
  template <typename T>
  VISKORES_EXEC_CONT bool operator()(const T& x) const
  {
    return !x;
  }
};

} // namespace viskores

#endif //viskores_UnaryPredicates_h
