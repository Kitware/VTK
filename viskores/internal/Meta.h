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
#ifndef viskores_internal_Meta_h
#define viskores_internal_Meta_h

// This header file contains templates that are helpful with template metaprogramming.

// Perhaps one day these structures can be exposed in the public interface, but the
// interface is a little wonky.

#include <type_traits>

namespace viskores
{
namespace internal
{
namespace meta
{

/// A simple `struct` that holds a type without having to actually make the type object.
template <typename T>
struct Type
{
  using type = T;
};

namespace detail
{

template <typename T1, typename T2>
struct AndImpl : std::integral_constant<bool, T1::value && T2::value>
{
};

template <typename T1, typename T2>
struct OrImpl : std::integral_constant<bool, T1::value || T2::value>
{
};

template <typename T>
struct NotImpl : std::integral_constant<bool, !T::value>
{
};

} // namespace detail

/// Expects two types, both with a `value` constant static value (like a `std::integral_constant`).
/// Resolves to a `std::integral_constant<bool, B>` where B is `T1::value && T2::value`.
template <typename T1, typename T2>
using And = typename detail::AndImpl<T1, T2>::type;

/// Expects two types, both with a `value` constant static value (like a `std::integral_constant`).
/// Resolves to a `std::integral_constant<bool, B>` where B is `T1::value || T2::value`.
template <typename T1, typename T2>
using Or = typename detail::OrImpl<T1, T2>::type;

/// Expects a type with a `value` constant static value (like a std::integral_constant`).
/// Resolves to a `std::integral_constant<bool, B>` where B is `!T::value`.
template <typename T>
using Not = typename detail::NotImpl<T>::type;

/// A single argument template that becomes its argument. Useful for passing an identity to
/// transformations.
template <typename T>
using Identity = T;

}
}
} // namespace viskores::internal::meta

#endif //viskores_internal_Meta_h
