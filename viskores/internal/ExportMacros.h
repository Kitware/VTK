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
#ifndef viskores_internal__ExportMacros_h
#define viskores_internal__ExportMacros_h

#include <viskores/internal/Configure.h>

/*!
  * Export macros for various parts of the Viskores library.
  */

#ifdef VISKORES_HIP

#include "hip/hip_runtime.h"
#define VISKORES_EXEC __device__ __host__
#define VISKORES_EXEC_CONT __device__ __host__
#define VISKORES_SUPPRESS_EXEC_WARNINGS

#elif defined(VISKORES_CUDA)

#define VISKORES_EXEC __device__ __host__
#define VISKORES_EXEC_CONT __device__ __host__

#ifdef VISKORES_MSVC

#if __CUDAVER__ >= 75000
#define VISKORES_SUPPRESS_EXEC_WARNINGS __pragma(nv_exec_check_disable)
#else
#define VISKORES_SUPPRESS_EXEC_WARNINGS __pragma(hd_warning_disable)
#endif

#else

#if __CUDAVER__ >= 75000
#define VISKORES_SUPPRESS_EXEC_WARNINGS _Pragma("nv_exec_check_disable")
#else
#define VISKORES_SUPPRESS_EXEC_WARNINGS _Pragma("hd_warning_disable")
#endif

#endif

#else // !VISKORES_CUDA

#define VISKORES_EXEC
#define VISKORES_EXEC_CONT
#define VISKORES_SUPPRESS_EXEC_WARNINGS

#endif // !VISKORES_CUDA

#define VISKORES_CONT

// Background:
// Viskores libraries are built with the hidden symbol visibility by default.
// This means that all template classes that are defined in Viskores headers are
// defined with hidden visibility when we are building Viskores. But when those
// headers are included by a third-party which has differing visibility controls
// causing link time warnings ( external vs private ).
//
// Issue:
// Dynamic cast is not just based on the name of the class, but also the
// combined visibility of the class on OSX. When building the hash_code of
// an object the symbol visibility controls of the type are taken into
// consideration ( including symbol vis of template parameters ).
// Therefore if a class has a component with private/hidden vis than it
// can't be passed across library boundaries.
//
//  Example is PolymorphicArrayHandleContainer<T,S> whose visibility affected
//  by the visibility of both T and S.
//
// Solution:
// The solution is fairly simple, but annoying. You need to mark every single
// header only class that is tempgit lated on non value types to be marked as
// always exported ( or never pass fvisibility=hidden ).
//
// TL;DR:
// This markup is used when we want to make sure:
//  - The class can be compiled into multiple libraries and at runtime will
//    resolve to a single type instance
//  - Be a type ( or component of a types signature ) that can be passed between
//    dynamic libraries and requires RTTI support ( dynamic_cast ).
#if defined(VISKORES_MSVC) || defined(VISKORES_CUDA) || defined(VISKORES_DOXYGEN_ONLY)
#define VISKORES_ALWAYS_EXPORT
#define VISKORES_NEVER_EXPORT
#else
#define VISKORES_ALWAYS_EXPORT __attribute__((visibility("default")))
#define VISKORES_NEVER_EXPORT __attribute__((visibility("hidden")))
#endif

// cuda 7.5 doesn't support static const or static constexpr variables
// that exist inside methods or classes, so in those cases we have to use
// just constexpr
#if defined(VISKORES_CUDA_VERSION_MAJOR) && (VISKORES_CUDA_VERSION_MAJOR < 8)
#define VISKORES_STATIC_CONSTEXPR_ARRAY constexpr
// cuda 8-9 doesn't support static constexpr pointers/fixed size arrays
// that exist inside methods or classes, so in those cases we gracefully
// fall back to static const
#elif defined(VISKORES_CUDA_VERSION_MAJOR) && (VISKORES_CUDA_VERSION_MAJOR < 10)
#define VISKORES_STATIC_CONSTEXPR_ARRAY static const
#else
#define VISKORES_STATIC_CONSTEXPR_ARRAY static constexpr
#endif

// Clang will warn about weak vtables (-Wweak-vtables) on exception classes,
// but there's no good way to eliminate them in this case because MSVC (See
// http://stackoverflow.com/questions/24511376). These macros will silence the
// warning for classes defined within them.
#ifdef VISKORES_CLANG
#define VISKORES_SILENCE_WEAK_VTABLE_WARNING_START \
  _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wweak-vtables\"")
#define VISKORES_SILENCE_WEAK_VTABLE_WARNING_END _Pragma("clang diagnostic pop")
#else // VISKORES_CLANG
#define VISKORES_SILENCE_WEAK_VTABLE_WARNING_START
#define VISKORES_SILENCE_WEAK_VTABLE_WARNING_END
#endif // VISKORES_CLANG

/// Simple macro to identify a parameter as unused. This allows you to name a
/// parameter that is not used. There are several instances where you might
/// want to do this. For example, when using a parameter to overload or
/// template a function but do not actually use the parameter. Another example
/// is providing a specialization that does not need that parameter.
#define viskoresNotUsed(parameter_name)

#endif //viskores_internal__ExportMacros_h
