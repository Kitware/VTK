/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssume.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   VTK_ASSUME
 * @brief   Provide compiler hints for non-obvious conditions.
*/

#ifndef vtkAssume_h
#define vtkAssume_h

#include "vtkConfigure.h"

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
 * \ref VTK-7-1-ArrayDispatch "here".
 */
#define VTK_ASSUME(cond) \
  do { \
  const bool c = cond; \
  assert("Bad assumption in VTK_ASSUME: " #cond && c); \
  VTK_ASSUME_IMPL(c); \
  (void)c; /* Prevents unused var warnings */ \
  } while (false) /* do-while prevents extra semicolon warnings */

// VTK_ASSUME_IMPL is compiler-specific:
#if defined(VTK_COMPILER_MSVC) || defined(VTK_COMPILER_ICC)
# define VTK_ASSUME_IMPL(cond) __assume(cond)
#elif defined(VTK_COMPILER_GCC) && VTK_COMPILER_GCC_VERSION >= 40500
// Added in 4.5.0:
# define VTK_ASSUME_IMPL(cond) if (!(cond)) __builtin_unreachable()
#elif defined(VTK_COMPILER_CLANG)
# define VTK_ASSUME_IMPL(cond) if (!(cond)) __builtin_unreachable()
#else
# define VTK_ASSUME_IMPL(cond) do {} while (false) /* no-op */
#endif

#endif // vtkAssume_h
// VTK-HeaderTest-Exclude: vtkAssume.h
