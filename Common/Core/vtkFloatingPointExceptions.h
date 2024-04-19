// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkFloatingPointExceptions
 * @brief   Deal with floating-point exceptions
 *
 * Right now it is really basic and it only provides a function to enable
 * floating point exceptions on some compilers.
 * Note that Borland C++ has floating-point exceptions by default, not
 * Visual studio nor gcc. It is mainly use to optionally enable floating
 * point exceptions in the C++ tests.
 */

#ifndef vtkFloatingPointExceptions_h
#define vtkFloatingPointExceptions_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkSystemIncludes.h"   // For VTKCOMMONCORE_EXPORT

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkFloatingPointExceptions
{
public:
  /**
   * Enable floating point exceptions.
   */
  static void Enable();

  /**
   * Disable floating point exceptions.
   */
  static void Disable();

private:
  vtkFloatingPointExceptions() = delete;
  vtkFloatingPointExceptions(const vtkFloatingPointExceptions&) = delete;
  void operator=(const vtkFloatingPointExceptions&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkFloatingPointExceptions.h
