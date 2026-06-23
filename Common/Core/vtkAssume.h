// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @file vtkAssume.h
 * @brief Compiler hint macros for assumptions and branch prediction.
 *
 * This header provides four families of macros:
 *
 * - `VTK_ASSUME(cond)` — tells the compiler `cond` is always true, with a
 *   debug-mode `assert` to catch violations at runtime.
 * - `VTK_ASSUME_NO_ASSERT(cond)` — same optimizer hint, but skips the assert.
 * - `VTK_EXPECT(cond, expected)` / `VTK_LIKELY(cond)` / `VTK_UNLIKELY(cond)` —
 *   branch-prediction hints that do **not** invoke undefined behaviour when wrong.
 *
 * A more detailed description and related tools can be found
 * [here](https://docs.vtk.org/en/latest/design_documents/ArrayDispatch.html).
 */

#ifndef vtkAssume_h
#define vtkAssume_h

#include "vtkCompiler.h"

#include <cassert>

/**
 * @def VTK_ASSUME(cond)
 * @brief Assert a condition to the compiler as always true, aborting in debug builds if violated.
 *
 * Instructs the compiler that `cond` will *always* evaluate to true at this
 * point in the program, enabling additional optimizations that would otherwise
 * be impossible.  In debug builds (`NDEBUG` not defined) a standard `assert`
 * fires if the condition is false, making violations easy to catch during
 * development.
 *
 * **Warning:** if `cond` is false in a release build the behaviour is
 * undefined. Only use this macro for invariants you are completely certain
 * about.
 *
 * **Common use-case — fixed component count on a typed array:**
 * @code{.cpp}
 * // Tell the compiler the array always has exactly 3 components so it can
 * // unroll the inner loop and use contiguous-stride memory access patterns.
 * VTK_ASSUME(array->GetNumberOfComponents() == 3);
 * for (vtkIdType tupleIdx = 0; tupleIdx < nTuples; ++tupleIdx)
 * {
 *   double x = array->GetTypedComponent(tupleIdx, 0);
 *   double y = array->GetTypedComponent(tupleIdx, 1);
 *   double z = array->GetTypedComponent(tupleIdx, 2);
 *   // ...
 * }
 * @endcode
 *
 * **Use inside vtkArrayDispatch workers:**
 * @code{.cpp}
 * struct MyWorker
 * {
 *   template <typename ArrayT>
 *   void operator()(ArrayT* array)
 *   {
 *     // The dispatch already guarantees 3 components; tell the compiler too.
 *     VTK_ASSUME(array->GetNumberOfComponents() == 3);
 *     // ... optimized component access ...
 *   }
 * };
 * @endcode
 *
 * @param cond A boolean expression that must always be true at this point.
 */
#define VTK_ASSUME(cond)                                                                           \
  do                                                                                               \
  {                                                                                                \
    const bool c = cond;                                                                           \
    assert("Bad assumption in VTK_ASSUME: " #cond&& c);                                            \
    VTK_ASSUME_IMPL(c);                                                                            \
    (void)c; /* Prevents unused var warnings */                                                    \
  } while (false) /* do-while prevents extra semicolon warnings */

/**
 * @def VTK_ASSUME_NO_ASSERT(cond)
 * @brief Same optimizer hint as VTK_ASSUME but without the debug-mode assertion.
 *
 * Passes `cond` to the compiler as an always-true assumption for optimization
 * purposes while deliberately omitting the `assert`.  Use this variant when
 * the condition is provably true by construction and the assertion would be
 * noise, or in performance-critical inner loops where even the assert overhead
 * in debug builds is undesirable.
 *
 * Like `VTK_ASSUME`, a false condition in a release build is undefined
 * behaviour.
 *
 * @code{.cpp}
 * // nComps is passed in from the caller which already validated it.
 * // Skip the redundant assert here.
 * VTK_ASSUME_NO_ASSERT(nComps > 0);
 * for (int c = 0; c < nComps; ++c) { ... }
 * @endcode
 *
 * @param cond A boolean expression that must always be true at this point.
 */
#define VTK_ASSUME_NO_ASSERT(cond)                                                                 \
  do                                                                                               \
  {                                                                                                \
    const bool c = cond;                                                                           \
    VTK_ASSUME_IMPL(c);                                                                            \
    (void)c; /* Prevents unused var warnings */                                                    \
  } while (false) /* do-while prevents extra semicolon warnings */

#ifdef __has_builtin
#define VTK_HAS_BUILTIN(x) __has_builtin(x)
#else
#define VTK_HAS_BUILTIN(x) 0
#endif

// VTK_ASSUME_IMPL is compiler-specific (not part of the public API):
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

/**
 * @def VTK_EXPECT(cond, expected)
 * @brief Low-level branch-prediction hint wrapping `__builtin_expect`.
 *
 * Tells the CPU/compiler that `cond` is expected to equal `expected` (0 or 1).
 * Unlike `VTK_ASSUME`, a misprediction is safe — it only affects performance,
 * not correctness.  Prefer the higher-level `VTK_LIKELY` / `VTK_UNLIKELY`
 * wrappers in most situations.
 *
 * @code{.cpp}
 * if (VTK_EXPECT(ptr != nullptr, 1)) // equivalent to VTK_LIKELY(ptr != nullptr)
 * {
 *   ptr->DoWork();
 * }
 * @endcode
 *
 * @param cond     The condition to evaluate.
 * @param expected The value (0 or 1) the compiler should assume `cond` takes.
 */

/**
 * @def VTK_LIKELY(cond)
 * @brief Hint that `cond` is expected to be true most of the time.
 *
 * Use this to improve branch-prediction on hot code paths where `cond` is
 * almost always true.  Has no effect on correctness.
 *
 * @code{.cpp}
 * if (VTK_LIKELY(array != nullptr))
 * {
 *   // Hot path — array is almost always valid.
 *   ProcessArray(array);
 * }
 * else
 * {
 *   // Cold path — rare error case.
 *   vtkErrorMacro("Array is null.");
 * }
 * @endcode
 *
 * @param cond The boolean expression expected to be true.
 */

/**
 * @def VTK_UNLIKELY(cond)
 * @brief Hint that `cond` is expected to be false most of the time.
 *
 * Use this on error-handling branches or rarely-taken code paths.  Has no
 * effect on correctness.
 *
 * @code{.cpp}
 * if (VTK_UNLIKELY(index < 0 || index >= size))
 * {
 *   vtkErrorMacro("Index " << index << " out of range [0, " << size << ").");
 *   return;
 * }
 * // Normal path continues here.
 * @endcode
 *
 * @param cond The boolean expression expected to be false.
 */
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
