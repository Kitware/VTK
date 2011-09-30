/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParseInternal.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
  This is an internal header for vtkParse.y, it contains methods for
  manipulating the data structures that are not meant for general
  use by the wrappers, and that are likely to change over time.
*/

#ifndef VTK_PARSE_PRIVATE_H
#define VTK_PARSE_PRIVATE_H

#include "vtkParse.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializer methods
 */
/*@{*/
void vtkParse_InitFile(FileInfo *file_info);
void vtkParse_InitNamespace(NamespaceInfo *namespace_info);
void vtkParse_InitClass(ClassInfo *cls);
void vtkParse_InitFunction(FunctionInfo *func);
void vtkParse_InitValue(ValueInfo *val);
void vtkParse_InitEnum(EnumInfo *item);
void vtkParse_InitUsing(UsingInfo *item);
void vtkParse_InitTemplateArgs(TemplateArgs *arg);
void vtkParse_InitTemplateArg(TemplateArg *arg);
/*@}*/

/**
 * Copy methods
 */
/*@{*/
void vtkParse_CopyNamespace(NamespaceInfo *data, const NamespaceInfo *orig);
void vtkParse_CopyClass(ClassInfo *data, const ClassInfo *orig);
void vtkParse_CopyFunction(FunctionInfo *data, const FunctionInfo *orig);
void vtkParse_CopyValue(ValueInfo *data, const ValueInfo *orig);
void vtkParse_CopyEnum(EnumInfo *data, const EnumInfo *orig);
void vtkParse_CopyUsing(UsingInfo *data, const UsingInfo *orig);
void vtkParse_CopyTemplateArgs(TemplateArgs *data, const TemplateArgs *orig);
void vtkParse_CopyTemplateArg(TemplateArg *data, const TemplateArg *orig);
/*@}*/

/**
 * Free methods
 */
/*@{*/
void vtkParse_FreeFile(FileInfo *file_info);
void vtkParse_FreeNamespace(NamespaceInfo *namespace_info);
void vtkParse_FreeClass(ClassInfo *cls);
void vtkParse_FreeFunction(FunctionInfo *func);
void vtkParse_FreeValue(ValueInfo *val);
void vtkParse_FreeEnum(EnumInfo *item);
void vtkParse_FreeUsing(UsingInfo *item);
void vtkParse_FreeTemplateArgs(TemplateArgs *arg);
void vtkParse_FreeTemplateArg(TemplateArg *arg);
/*@}*/

/**
 * Make a persistent copy of a string for use with AddStringToArray:
 * At most 'n' chars will be copied, and the string will be terminated.
 * If a null pointer is provided, then a null pointer will be returned.
 */
const char *vtkParse_DuplicateString(const char *cp, size_t n);

/**
 * Add a string to an array of strings, grow array as necessary.
 */
void vtkParse_AddStringToArray(
  const char ***valueArray, int *count, const char *value);

/**
 * Expand the Item array for classes and namespaces.
 */
void vtkParse_AddItemToArray(
  ItemInfo **valueArray, int *count, parse_item_t type, int idx);


/**
 * Add various items to the structs.
 */
/*@{*/
void vtkParse_AddClassToClass(ClassInfo *info, ClassInfo *item);
void vtkParse_AddFunctionToClass(ClassInfo *info, FunctionInfo *item);
void vtkParse_AddEnumToClass(ClassInfo *info, EnumInfo *item);
void vtkParse_AddConstantToClass(ClassInfo *info, ValueInfo *item);
void vtkParse_AddVariableToClass(ClassInfo *info, ValueInfo *item);
void vtkParse_AddTypedefToClass(ClassInfo *info, ValueInfo *item);
void vtkParse_AddUsingToClass(ClassInfo *info, UsingInfo *item);
void vtkParse_AddNamespaceToNamespace(NamespaceInfo *info,NamespaceInfo *item);
void vtkParse_AddClassToNamespace(NamespaceInfo *info, ClassInfo *item);
void vtkParse_AddFunctionToNamespace(NamespaceInfo *info, FunctionInfo *item);
void vtkParse_AddEnumToNamespace(NamespaceInfo *info, EnumInfo *item);
void vtkParse_AddConstantToNamespace(NamespaceInfo *info, ValueInfo *item);
void vtkParse_AddVariableToNamespace(NamespaceInfo *info, ValueInfo *item);
void vtkParse_AddTypedefToNamespace(NamespaceInfo *info, ValueInfo *item);
void vtkParse_AddUsingToNamespace(NamespaceInfo *info, UsingInfo *item);
void vtkParse_AddArgumentToFunction(FunctionInfo *info, ValueInfo *item);
void vtkParse_AddArgumentToTemplate(TemplateArgs *info, TemplateArg *item);
/*@}*/

/**
 * Add default constructors if they do not already exist
 */
void vtkParse_AddDefaultConstructors(ClassInfo *data);

/**
 * Simple utility for mapping VTK types to VTK_PARSE types.
 */
unsigned int vtkParse_MapType(int vtktype);

/**
 * Ignore BTX/ETX markers
 */
void vtkParse_SetIgnoreBTX(int option);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
