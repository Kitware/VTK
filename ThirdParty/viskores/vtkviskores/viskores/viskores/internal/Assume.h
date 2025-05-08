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
#ifndef viskores_internal_Assume_h
#define viskores_internal_Assume_h

#include <viskores/Assert.h>

// Description:
// VISKORES_ASSUME instructs the compiler that a certain non-obvious condition will
// *always* be true. Beware that if cond is false at runtime, the results are
// unpredictable (and likely catastrophic). A runtime assertion is added so
// that debugging builds may easily catch violations of the condition.
//
// A useful application of this macro is when a method is passed in a
// viskores::Vec that is uninitialized and conditional fills the viskores::Vec
// based on other runtime information such as cell type. This allows you to
// assert that only valid cell types will be used, producing more efficient
// code.
//
#define VISKORES_ASSUME(cond)                                         \
  VISKORES_SWALLOW_SEMICOLON_PRE_BLOCK                                \
  {                                                                   \
    const bool c = cond;                                              \
    VISKORES_ASSERT("Bad assumption in VISKORES_ASSUME: " #cond&& c); \
    VISKORES_ASSUME_IMPL(c);                                          \
    (void)c; /* Prevents unused var warnings */                       \
  }                                                                   \
  VISKORES_SWALLOW_SEMICOLON_POST_BLOCK

// VISKORES_ASSUME_IMPL is compiler-specific:
#if defined(VISKORES_CUDA_DEVICE_PASS)
//For all versions of CUDA this is a no-op while we look
//for a CUDA asm snippet that replicates this kind of behavior
#define VISKORES_ASSUME_IMPL(cond) (void)0 /* no-op */
#else

#if defined(VISKORES_MSVC)
#define VISKORES_ASSUME_IMPL(cond) __assume(cond)
#elif defined(VISKORES_ICC) && !defined(__GNUC__)
#define VISKORES_ASSUME_IMPL(cond) __assume(cond)
#elif (defined(VISKORES_GCC) || defined(VISKORES_ICC)) && \
  (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
// Added in 4.5.0:
#define VISKORES_ASSUME_IMPL(cond) \
  if (!(cond))                     \
  __builtin_unreachable()
#elif defined(VISKORES_CLANG)
#define VISKORES_ASSUME_IMPL(cond) \
  if (!(cond))                     \
  __builtin_unreachable()
#else
#define VISKORES_ASSUME_IMPL(cond) (void)0 /* no-op */
#endif

#endif

#endif // viskores_internal_Assume_h
