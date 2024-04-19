// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2011 David Gobbi
// SPDX-License-Identifier: BSD-3-Clause
/**
 * This file contains utilities doing name mangling
 */

#ifndef vtkParseMangle_h
#define vtkParseMangle_h

#include "vtkWrappingToolsModule.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * Generate a mangled name for a type, use gcc ia64 ABI.
   * The result is placed in new_name, which must be large enough
   * to accept the result.
   */
  VTKWRAPPINGTOOLS_EXPORT
  size_t vtkParse_MangledTypeName(const char* name, char* new_name);

  /**
   * Generate a mangled name for a literal.  Only handles decimal
   * integer literals.  It guesses type from suffix "u", "ul",
   * "ull", "l", "ll" so only certain types are supported.
   */
  VTKWRAPPINGTOOLS_EXPORT
  size_t vtkParse_MangledLiteral(const char* name, char* new_name);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
/* VTK-HeaderTest-Exclude: vtkParseMangle.h */
