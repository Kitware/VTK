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

#ifndef viskores_Assert_h
#define viskores_Assert_h

#include <viskores/internal/Configure.h>

#include <cassert>

// Pick up conditions where we want to turn on/off assert.
#ifndef VISKORES_NO_ASSERT
#if defined(NDEBUG)
#define VISKORES_NO_ASSERT
#elif defined(VISKORES_CUDA_DEVICE_PASS) && defined(VISKORES_NO_ASSERT_CUDA)
#define VISKORES_NO_ASSERT
#elif defined(VISKORES_HIP) && defined(VISKORES_NO_ASSERT_HIP)
#define VISKORES_NO_ASSERT
#endif
#endif // VISKORES_NO_ASSERT

/// \def VISKORES_ASSERT(condition)
///
/// Asserts that \a condition resolves to true.  If \a condition is false,
/// then a diagnostic message is outputted and execution is terminated. The
/// behavior is essentially the same as the POSIX assert macro, but is
/// wrapped for added portability.
///
/// Like the POSIX assert macro, the check will be removed when compiling
/// in non-debug mode (specifically when NDEBUG is defined), so be prepared
/// for the possibility that the condition is never evaluated.
///
/// The VISKORES_NO_ASSERT cmake and preprocessor option allows debugging builds
/// to remove assertions for performance reasons.
#ifndef VISKORES_NO_ASSERT
#define VISKORES_ASSERT(condition) assert(condition)
#define VISKORES_ASSERTS_CHECKED
#else
#define VISKORES_ASSERT(condition) (void)(condition)
#endif

#endif //viskores_Assert_h
