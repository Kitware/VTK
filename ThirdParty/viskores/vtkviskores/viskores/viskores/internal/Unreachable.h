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

#ifndef viskores_Unreachable_h
#define viskores_Unreachable_h

/// VISKORES_UNREACHABLE is similar to VTK_ASSUME, with the significant difference
/// that it is not conditional. Control should never reach a path containing
/// a VISKORES_UNREACHABLE statement under any circumstances.
///
/// If assertions are enabled (e.g. neither NDEBUG nor VISKORES_NO_ASSERT is
/// defined), the following steps are taken:
/// 1. Print an error message containing the macro argument and location of the
///    VISKORES_UNREACHABLE call.
/// 2. Abort the kernel (if CUDA) or process.
///
/// This allows bad code paths to be identified during development and
/// debugging.
///
/// If assertions are disabled and the compiler has some sort of 'unreachable'
/// intrinsic used to provide optimization hints, the intrinsic is used to
/// notify the compiler that this is a dead code path.
///
#define VISKORES_UNREACHABLE(msg)      \
  VISKORES_SWALLOW_SEMICOLON_PRE_BLOCK \
  {                                    \
    VISKORES_UNREACHABLE_IMPL();       \
    VISKORES_UNREACHABLE_PRINT(msg);   \
    VISKORES_UNREACHABLE_ABORT();      \
  }                                    \
  VISKORES_SWALLOW_SEMICOLON_POST_BLOCK

// VISKORES_UNREACHABLE_IMPL is compiler-specific:
#if defined(VISKORES_CUDA_DEVICE_PASS)

#define VISKORES_UNREACHABLE_IMPL() (void)0 /* no-op, no known intrinsic */

#if defined(NDEBUG) || defined(VISKORES_NO_ASSERT)

#define VISKORES_UNREACHABLE_PRINT(msg) (void)0 /* no-op */
#define VISKORES_UNREACHABLE_ABORT() (void)0    /* no-op */

#else // NDEBUG || VISKORES_NO_ASSERT

#define VISKORES_UNREACHABLE_PRINT(msg) \
  printf("Unreachable location reached: %s\nLocation: %s:%d\n", msg, __FILE__, __LINE__)
#define VISKORES_UNREACHABLE_ABORT() \
  asm("trap;") /* Triggers kernel exit with CUDA error 73: Illegal inst */

#endif // NDEBUG || VISKORES_NO_ASSERT

#else // !CUDA


#if defined(NDEBUG) || defined(VISKORES_NO_ASSERT)

#define VISKORES_UNREACHABLE_PRINT(msg) (void)0 /* no-op */
#define VISKORES_UNREACHABLE_ABORT() (void)0    /* no-op */

#if defined(VISKORES_MSVC)
#define VISKORES_UNREACHABLE_IMPL() __assume(false)
#elif defined(VISKORES_ICC) && !defined(__GNUC__)
#define VISKORES_UNREACHABLE_IMPL() __assume(false)
#elif (defined(VISKORES_GCC) || defined(VISKORES_ICC)) && \
  (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
// Added in 4.5.0:
#define VISKORES_UNREACHABLE_IMPL() __builtin_unreachable()
#elif defined(VISKORES_CLANG)
#define VISKORES_UNREACHABLE_IMPL() __builtin_unreachable()
#else
#define VISKORES_UNREACHABLE_IMPL() (void)0 /* no-op */
#endif

#else // NDEBUG || VISKORES_NO_ASSERT

#define VISKORES_UNREACHABLE_IMPL() (void)0
#define VISKORES_UNREACHABLE_PRINT(msg)                        \
  std::cerr << "Unreachable location reached: " << msg << "\n" \
            << "Location: " << __FILE__ << ":" << __LINE__ << "\n"
#define VISKORES_UNREACHABLE_ABORT() abort()

#endif // NDEBUG && !VISKORES_NO_ASSERT

#endif // !CUDA

#endif //viskores_Unreachable_h
