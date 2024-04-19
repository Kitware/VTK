//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_Deprecated_H_
#define fides_Deprecated_H_

#include <fides/Configure.h>

#define FIDES_EXPAND(expr) expr

#define FIDES_DEPRECATED_MAKE_MESSAGE(...) \
  FIDES_EXPAND(FIDES_DEPRECATED_MAKE_MESSAGE_IMPL(__VA_ARGS__, ""))
#define FIDES_DEPRECATED_MAKE_MESSAGE_IMPL(version, message, ...) \
  message " Deprecated in version " #version "."

/// \def FIDES_DEPRECATED(version, message)
///
/// Classes and methods are marked deprecated using the `FIDES_DEPRECATED`
/// macro. The first argument of `FIDES_DEPRECATED` should be set to the first
/// version in which the feature is deprecated. For example, if the last
/// released version of VTK-m was 1.5, and on the master branch a developer
/// wants to deprecate a class foo, then the `FIDES_DEPRECATED` release version
/// should be given as 1.6, which will be the next minor release of VTK-m. The
/// second argument of `FIDES_DEPRECATED`, which is optional but highly
/// encouraged, is a short message that should clue developers on how to update
/// their code to the new changes. For example, it could point to the
/// replacement class or method for the changed feature.
///

/// \def FIDES_DEPRECATED_SUPPRESS_BEGIN
///
/// Begins a region of code in which warnings about using deprecated code are ignored.
/// Such suppression is usually helpful when implementing other deprecated features.
/// (You would think if one deprecated method used another deprecated method this
/// would not be a warning, but it is.)
///
/// Any use of `FIDES_DEPRECATED_SUPPRESS_BEGIN` must be paired with a
/// `FIDES_DEPRECATED_SUPPRESS_END`, which will re-enable warnings in subsequent code.
///
/// Do not use a semicolon after this macro.
///

/// \def FIDES_DEPRECATED_SUPPRESS_END
///
/// Ends a region of code in which warnings about using deprecated code are ignored.
/// Any use of `FIDES_DEPRECATED_SUPPRESS_BEGIN` must be paired with a
/// `FIDES_DEPRECATED_SUPPRESS_END`.
///
/// Do not use a semicolon after this macro.
///

// Determine whether the [[deprecated]] attribute is supported. Note that we are not
// using other older compiler features such as __attribute__((__deprecated__)) because
// they do not all support all [[deprecated]] uses (such as uses in enums). If
// [[deprecated]] is supported, then FIDES_DEPRECATED_ATTRIBUTE_SUPPORTED will get defined.
#ifndef FIDES_DEPRECATED_ATTRIBUTE_SUPPORTED

#if defined(__NVCC__)
// Currently nvcc has zero support deprecated attributes
#elif __cplusplus >= 201402L && !defined(FIDES_GCC)

// C++14 and better supports [[deprecated]]
// Except in these cases:
//   - nvcc
#define FIDES_DEPRECATED_ATTRIBUTE_SUPPORTED

#elif defined(FIDES_GCC)
// GCC has supported [[deprecated]] since version 5.0, but using it on enum was not
// supported until 6.0. So we have to make a special case to only use it for high
// enough revisions.
#if __GNUC__ >= 6
#define FIDES_DEPRECATED_ATTRIBUTE_SUPPORTED
#endif // Too old GCC

#elif defined(__has_cpp_attribute)

#if __has_cpp_attribute(deprecated)
// Compiler not fully C++14 compliant, but it reports to support [[deprecated]]
#define FIDES_DEPRECATED_ATTRIBUTE_SUPPORTED
#endif // __has_cpp_attribute(deprecated)

#elif defined(FIDES_MSVC) && (_MSC_VER >= 1900)

#define FIDES_DEPRECATED_ATTRIBUTE_SUPPORTED

#endif // no known compiler support for [[deprecated]]

#endif // FIDES_DEPRECATED_ATTRIBUTE_SUPPORTED check

// Determine how to turn deprecated warnings on and off, generally with pragmas. If
// deprecated warnings can be turned off and on, then FIDES_DEPRECATED_SUPPRESS_SUPPORTED
// is defined and the pair FIDES_DEPRECATED_SUPPRESS_BEGIN and FIDES_DEPRECATED_SUPRESS_END
// are defined to the pragmas to disable and restore these warnings. If this support
// cannot be determined, FIDES_DEPRECATED_SUPPRESS_SUPPORTED is _not_ define whereas
// FIDES_DEPRECATED_SUPPRESS_BEGIN and FIDES_DEPRECATED_SUPPRESS_END are defined to be
// empty.
#ifndef FIDES_DEPRECATED_SUPPRESS_SUPPORTED

#if defined(FIDES_GCC) || defined(FIDES_CLANG)

#define FIDES_DEPRECATED_SUPPRESS_SUPPORTED
#define FIDES_DEPRECATED_SUPPRESS_BEGIN \
  _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#define FIDES_DEPRECATED_SUPPRESS_END _Pragma("GCC diagnostic pop")

#elif defined(FIDES_MSVC)

#define FIDES_DEPRECATED_SUPPRESS_SUPPORTED
#define FIDES_DEPRECATED_SUPPRESS_BEGIN __pragma(warning(push)) __pragma(warning(disable : 4996))
#define FIDES_DEPRECATED_SUPPRESS_END __pragma(warning(pop))

#else

//   Other compilers probably have different pragmas for turning warnings off and on.
//   Adding more compilers to this list is fine, but the above probably capture most
//   developers and should be covered on dashboards.
#define FIDES_DEPRECATED_SUPPRESS_BEGIN
#define FIDES_DEPRECATED_SUPPRESS_END

#endif

#endif // FIDES_DEPRECATED_SUPPRESS_SUPPORTED check

#if !defined(FIDES_DEPRECATED_SUPPRESS_BEGIN) || !defined(FIDES_DEPRECATED_SUPPRESS_END)
#error FIDES_DEPRECATED_SUPPRESS macros not properly defined.
#endif

// Only actually use the [[deprecated]] attribute if the compiler supports it AND
// we know how to suppress deprecations when necessary.
#if defined(FIDES_DEPRECATED_ATTRIBUTE_SUPPORTED) && defined(FIDES_DEPRECATED_SUPPRESS_SUPPORTED)
#ifdef FIDES_MSVC
#define FIDES_DEPRECATED(...) [[deprecated(FIDES_DEPRECATED_MAKE_MESSAGE(__VA_ARGS__))]]
#else // !MSVC
// GCC and other compilers support the C++14 attribute [[deprecated]], but there appears to be a
// bug (or other undesirable behavior) where if you mix [[deprecated]] with __attribute__(()) you
// get compile errors. To get around this, use __attribute((deprecated)) where supported.
#define FIDES_DEPRECATED(...) \
  __attribute__((deprecated(FIDES_DEPRECATED_MAKE_MESSAGE(__VA_ARGS__))))
#endif // !MSVC
#else
#define FIDES_DEPRECATED(...)
#endif

#endif
