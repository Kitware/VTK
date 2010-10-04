/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapText.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * vtkWrap provides useful functions for generating wrapping code.
*/

#ifndef VTK_WRAP_TEXT_H
#define VTK_WRAP_TEXT_H

#include "vtkParse.h"
#include "vtkParseHierarchy.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Quote a string for inclusion in a C string literal.  The "maxlen"
 * should be set to a value between 32 and 2047.  Values over 2047
 * will result in string literals too long for some compilers.  If
 * the string is truncated, a "..." will be appended.
 */
const char *vtkWrapText_QuoteString(const char *comment, size_t maxlen);

/**
 * Format a doxygen comment for plain text, and word-wrap at
 * the specified width.  A 70-char width is recommended.
 */
const char *vtkWrapText_FormatComment(const char *comment, size_t width);

/**
 * Format a method signature by applying word-wrap at the specified
 * width and taking special care not to split any literals or names.
 * A width of 70 chars is recommended.
 */
const char *vtkWrapText_FormatSignature(
  const char *signature, size_t width, size_t maxlen);

/**
 * Produce a python signature for a method, for use in documentation.
 */
const char *vtkWrapText_PythonSignature(FunctionInfo *currentFunction);

#ifdef __cplusplus
}
#endif

#endif
