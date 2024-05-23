// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2010,2015 David Gobbi
// SPDX-License-Identifier: BSD-3-Clause
/**
 * This file contains methods to categorize method types between Set/Get/{Set,Get}NumberOf, etc.
 */
#ifndef vtkParseMethodType_h
#define vtkParseMethodType_h

#include "vtkWrappingToolsModule.h"

#ifdef __cplusplus
extern "C"
{
#endif

  VTKWRAPPINGTOOLS_EXPORT int vtkParseMethodType_IsSetMethod(const char* name);
  VTKWRAPPINGTOOLS_EXPORT int vtkParseMethodType_IsSetNthMethod(const char* name);
  VTKWRAPPINGTOOLS_EXPORT int vtkParseMethodType_IsSetNumberOfMethod(const char* name);

  VTKWRAPPINGTOOLS_EXPORT int vtkParseMethodType_IsGetMethod(const char* name);
  VTKWRAPPINGTOOLS_EXPORT int vtkParseMethodType_IsGetNthMethod(const char* name);
  VTKWRAPPINGTOOLS_EXPORT int vtkParseMethodType_IsGetNumberOfMethod(const char* name);

  VTKWRAPPINGTOOLS_EXPORT int vtkParseMethodType_IsBooleanMethod(const char* name);
  VTKWRAPPINGTOOLS_EXPORT int vtkParseMethodType_IsEnumeratedMethod(const char* name);
  VTKWRAPPINGTOOLS_EXPORT int vtkParseMethodType_IsAsStringMethod(const char* name);

  VTKWRAPPINGTOOLS_EXPORT int vtkParseMethodType_IsAddMethod(const char* name);
  VTKWRAPPINGTOOLS_EXPORT int vtkParseMethodType_IsRemoveMethod(const char* name);
  VTKWRAPPINGTOOLS_EXPORT int vtkParseMethodType_IsRemoveAllMethod(const char* name);

  VTKWRAPPINGTOOLS_EXPORT int vtkParseMethodType_IsGetMinValueMethod(const char* name);
  VTKWRAPPINGTOOLS_EXPORT int vtkParseMethodType_IsGetMaxValueMethod(const char* name);
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
