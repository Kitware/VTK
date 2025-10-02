// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   VTK_ASSUME
 * @brief   Provide compiler hints for non-obvious conditions.
 */

#ifndef vtkAssume_h
#define vtkAssume_h

#include "vtkCompiler.h"

#include <cassert>

/**
 * VTK_ASSUME instructs the compiler that a certain non-obvious condition will
 * *always* be true. Beware that if cond is false at runtime, the results are
 * unpredictable (and likely catastrophic). A runtime assertion is added so
 * that debugging builds may easily catch violations of the condition.

 * A useful application of this macro is when a vtkGenericDataArray subclass has
 * a known number of components at compile time. Adding, for example,
 * VTK_ASSUME(array->GetNumberOfComponents() == 3); allows the compiler to
 * provide faster access through the GetTypedComponent method, as the fixed data
 * stride in AOS arrays allows advanced optimization of the accesses.

 * A more detailed description of this class and related tools can be found
 * [here](https://docs.vtk.org/en/latest/design_documents/array_dispatch.html).
 */
#define VTK_ASSUME(cond)                                                                           \
  do                                                                                               \
  {                                                                                                \
    const bool c = cond;                                                                           \
    assert("Bad assumption in VTK_ASSUME: " #cond&& c);                                            \
    VTK_ASSUME_IMPL(c);                                                                            \
    (void)c;      /* Prevents unused var warnings */                                               \
  } while (false) /* do-while prevents extra semicolon warnings */

#define VTK_ASSUME_NO_ASSERT(cond)                                                                 \
  do                                                                                               \
  {                                                                                                \
    const bool c = cond;                                                                           \
    VTK_ASSUME_IMPL(c);                                                                            \
    (void)c;      /* Prevents unused var warnings */                                               \
  } while (false) /* do-while prevents extra semicolon warnings */

#ifdef __has_builtin
#define VTK_HAS_BUILTIN(x) __has_builtin(x)
#else
#define VTK_HAS_BUILTIN(x) 0
#endif

// VTK_ASSUME_IMPL is compiler-specific:
#if defined(VTK_COMPILER_MSVC) || defined(VTK_COMPILER_ICC)
#define VTK_ASSUME_IMPL(cond) __assume(cond)
#elif VTK_HAS_BUILTIN(__builtin_assume) || defined(VTK_COMPILER_CLANG)
#define VTK_ASSUME_IMPL(cond) __builtin_assume(cond)
#elif defined(VTK_COMPILER_GCC) && VTK_COMPILER_GCC_VERSION >= 130000
#define VTK_ASSUME_IMPL(cond) __attribute__((__assume__(cond)))
#elif VTK_HAS_BUILTIN(__builtin_unreachable) || defined(VTK_COMPILER_GCC)
#define VTK_ASSUME_IMPL(cond)                                                                      \
  if (!(cond))                                                                                     \
  __builtin_unreachable()
#else
#define VTK_ASSUME_IMPL(cond)                                                                      \
  do                                                                                               \
  {                                                                                                \
  } while (false) /* no-op */
#endif

// VTK_EXPECT & VTK_LIKELY & VTK_UNLIKELY
#if VTK_HAS_BUILTIN(__builtin_expect) || defined(VTK_COMPILER_GCC) || defined(VTK_COMPILER_CLANG)
#define VTK_EXPECT(cond, expected) __builtin_expect(cond, expected)
#define VTK_LIKELY(cond) VTK_EXPECT(!!(cond), 1)
#define VTK_UNLIKELY(cond) VTK_EXPECT(!!(cond), 0)
#else
#define VTK_EXPECT(cond, expected) (cond)
#define VTK_LIKELY(cond) (cond)
#define VTK_UNLIKELY(cond) (cond)
#endif

#endif // vtkAssume_h
// VTK-HeaderTest-Exclude: vtkAssume.h
