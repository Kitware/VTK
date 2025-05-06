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
#ifndef viskores_std_is_trivial_h
#define viskores_std_is_trivial_h

#include <viskores/StaticAssert.h>
#include <viskores/internal/Configure.h>

#include <type_traits>

#if defined(VISKORES_GCC) && !defined(VISKORES_USING_GLIBCXX_4)
#define VISKORES_USE_STD_IS_TRIVIAL
#endif

#ifndef VISKORES_USE_STD_IS_TRIVIAL
namespace viskoresstd
{

// GCC 4.8 and 4.9 standard library does not support std::is_trivially_copyable.
// There is no relyable way to get this information (since it has to come special from
// the compiler). For our purposes, we will report as nothing being trivially copyable,
// which causes us to call the constructors with everything. This should be fine unless
// some other part of the compiler is trying to check for trivial copies (perhaps nvcc
// on top of GCC 4.8).
template <typename>
struct is_trivially_copyable : std::false_type
{
};
// I haven't tried the other forms of is_trivial, but let's just assume they don't
// work as expected.
template <typename...>
struct is_trivially_constructible : std::false_type
{
};
template <typename>
struct is_trivially_destructible : std::false_type
{
};
template <typename>
struct is_trivial : std::false_type
{
};

// A common exception to reporting nothing as trivially copyable is assertions that
// a class is trivially copyable. If we have code that _only_ works with trivially
// copyable classes, we don't want to report nothing as trivially copyably, because
// that will error out for everything. For this case, we define the macro
// `VISKORES_IS_TRIVIALLY_COPYABLE`, which will always pass on compilers that don't
// support is_trivially_copyable, but will do the correct check on compilers that
// do support it.
#define VISKORES_IS_TRIVIALLY_COPYABLE(...) VISKORES_STATIC_ASSERT(true)
#define VISKORES_IS_TRIVIALLY_CONSTRUCTIBLE(...) VISKORES_STATIC_ASSERT(true)
#define VISKORES_IS_TRIVIALLY_DESTRUCTIBLE(...) VISKORES_STATIC_ASSERT(true)
#define VISKORES_IS_TRIVIAL(...) VISKORES_STATIC_ASSERT(true)

} // namespace viskoresstd

#else // NOT VISKORES_USING_GLIBCXX_4
namespace viskoresstd
{

using std::is_trivial;
using std::is_trivially_constructible;
using std::is_trivially_copyable;
using std::is_trivially_destructible;

namespace detail
{

// Note: the reason why we redirect the trivial checks to these classes is so when the checks
// fail it is easier to read from the error messages exactly which type was being checked. If
// you just do a static assert in a macro, the error message is likely to give you either
// macro argument names or unresolved types in the error message, which is unhelpful.

template <typename T>
struct CheckTriviallyCopyable
{
  VISKORES_STATIC_ASSERT_MSG(viskoresstd::is_trivially_copyable<T>::value,
                             "Type must be trivially copyable to be used here.");
  static constexpr bool value = true;
};

template <typename T>
struct CheckTriviallyConstructible
{
  VISKORES_STATIC_ASSERT_MSG(::viskoresstd::is_trivially_constructible<T>::value,
                             "Type must be trivially constructible to be used here.");
  static constexpr bool value = true;
};

template <typename T>
struct CheckTriviallyDestructible
{
  VISKORES_STATIC_ASSERT_MSG(::viskoresstd::is_trivially_destructible<T>::value,
                             "Type must be trivially destructible to be used here.");
  static constexpr bool value = true;
};

template <typename T>
struct CheckTrivial
{
  VISKORES_STATIC_ASSERT_MSG(::viskoresstd::is_trivial<T>::value,
                             "Type must be trivial to be used here.");
  static constexpr bool value = true;
};

} // namespace detail

#define VISKORES_IS_TRIVIALLY_COPYABLE(...) \
  VISKORES_STATIC_ASSERT(::viskoresstd::detail::CheckTriviallyCopyable<__VA_ARGS__>::value)
#define VISKORES_IS_TRIVIALLY_CONSTRUCTIBLE(...) \
  VISKORES_STATIC_ASSERT(::viskoresstd::detail::CheckTriviallyConstructible<__VA_ARGS__>::value)
#define VISKORES_IS_TRIVIALLY_DESTRUCTIBLE(...) \
  VISKORES_STATIC_ASSERT(::viskoresstd::detail::CheckTriviallyDestructible<__VA_ARGS__>::value)
#define VISKORES_IS_TRIVIAL(...) \
  VISKORES_STATIC_ASSERT(::viskoresstd::detail::CheckTrivial<__VA_ARGS__>::value)

} // namespace viskoresstd
#endif

#endif //viskores_std_is_trivial_h
