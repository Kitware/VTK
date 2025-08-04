// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Constants and functions used for custom attributes in the parser.
 */

#ifndef vtkParseAttributes_h
#define vtkParseAttributes_h

#include "vtkWrappingToolsModule.h"

/**
 * The following attributes are available as bitflags that can be
 * stored in the Attributes field of the ValueInfo struct.
 */

#define VTK_PARSE_NEWINSTANCE 0x00000001
#define VTK_PARSE_ZEROCOPY 0x00000002
#define VTK_PARSE_FILEPATH 0x00000004
#define VTK_PARSE_WRAPEXCLUDE 0x00000010
#define VTK_PARSE_DEPRECATED 0x0000020
#define VTK_PARSE_UNBLOCKTHREADS 0x00000100

/**
 * Return values for attribute handling functions
 */
typedef enum parse_attribute_return_t_
{
  VTK_ATTRIB_HANDLER_SKIPPED, /* attribute was not handled */
  VTK_ATTRIB_HANDLER_HANDLED, /* attribute was successfully handled */
  VTK_ATTRIB_HANDLER_NO_ARGS, /* attribute did not use arguments */
  VTK_ATTRIB_HANDLER_ERRORED, /* attribute handling had an error */
} parse_attribute_return_t;

/* Forward declarations needed by the function definitions */

struct PreprocessInfo_;
struct FunctionInfo_;
struct ValueInfo_;
struct ClassInfo_;

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * Handle class and struct attributes
   */
  VTKWRAPPINGTOOLS_EXPORT
  parse_attribute_return_t vtkParse_ClassAttribute(
    struct ClassInfo_* cls, const char* attr, struct PreprocessInfo_* preprocessor);

  /**
   * Handle function attributes that appear before the function.
   */
  VTKWRAPPINGTOOLS_EXPORT
  parse_attribute_return_t vtkParse_FunctionAttribute(
    struct FunctionInfo_* func, const char* attr, struct PreprocessInfo_* preprocessor);

  /**
   * Handle attributes that appear after the function:
   */
  VTKWRAPPINGTOOLS_EXPORT
  parse_attribute_return_t vtkParse_AfterFunctionAttribute(
    struct FunctionInfo_* func, const char* attr, struct PreprocessInfo_* preprocessor);

  /**
   * Handle attributes for parameters, return values, variables, etc./
   */
  VTKWRAPPINGTOOLS_EXPORT
  parse_attribute_return_t vtkParse_ValueAttribute(
    struct ValueInfo_* val, const char* attr, struct PreprocessInfo_* preprocessor);

  /**
   * Handle attributes that occur after enum constant name
   */
  VTKWRAPPINGTOOLS_EXPORT
  parse_attribute_return_t vtkParse_AfterValueAttribute(
    struct ValueInfo_* val, const char* attr, struct PreprocessInfo_* preprocessor);

  /**
   * Get the last attribute processing error
   */
  VTKWRAPPINGTOOLS_EXPORT
  const char* vtkParse_GetAttributeError(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
/* VTK-HeaderTest-Exclude: vtkParseAttributes.h */
