// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * vtkWrap provides useful functions for generating wrapping code.
 */

#ifndef vtkWrapText_h
#define vtkWrapText_h

#include "vtkParse.h"
#include "vtkParseHierarchy.h"
#include "vtkWrappingToolsModule.h"

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * Quote a string for inclusion in a C string literal.  The "maxlen"
   * should be set to a value between 32 and 2047.  Values over 2047
   * will result in string literals too long for some compilers.  If
   * the string is truncated, a "..." will be appended.
   */
  VTKWRAPPINGTOOLS_EXPORT const char* vtkWrapText_QuoteString(const char* comment, size_t maxlen);

  /**
   * Format a doxygen comment for plain text, and word-wrap at
   * the specified width.  A 70-char width is recommended.
   */
  VTKWRAPPINGTOOLS_EXPORT const char* vtkWrapText_FormatComment(const char* comment, size_t width);

  /**
   * Format a method signature by applying word-wrap at the specified
   * width and taking special care not to split any literals or names.
   * A width of 70 chars is recommended.
   */
  VTKWRAPPINGTOOLS_EXPORT const char* vtkWrapText_FormatSignature(
    const char* signature, size_t width, size_t maxlen);

  /**
   * Produce a python signature for a method, for use in documentation.
   */
  VTKWRAPPINGTOOLS_EXPORT const char* vtkWrapText_PythonSignature(FunctionInfo* currentFunction);

  /**
   * Convert a C++ identifier into an identifier that can be used from Python.
   * The "::" namespace separators are converted to ".", and template args
   * are mangled and prefix with "T" according to the ia64 ABI. The output
   * parameter "pname" must be large enough to accept the result.  If it is
   * as long as the input name, that is sufficient. */
  VTKWRAPPINGTOOLS_EXPORT void vtkWrapText_PythonName(const char* name, char* pname);

  /**
   * Check if a name is a reserved keyword in Python.
   */
  VTKWRAPPINGTOOLS_EXPORT int vtkWrapText_IsPythonKeyword(const char* name);

#ifdef __cplusplus
}
#endif

#endif
/* VTK-HeaderTest-Exclude: vtkWrapText.h */
