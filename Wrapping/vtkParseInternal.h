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
void vtkParse_InitTemplateArgs(TemplateArgs *arg);
void vtkParse_InitTemplateArg(TemplateArg *arg);
/*@}*/

/**
 * Add a string to an array of strings, grow array as necessary.
 */
void vtkParse_AddStringToArray(
  const char ***valueArray, int *count, const char *value);


/**
 * Expand the Item array for classes and namespaces.
 */
void vtkParse_AddItemToArray(
  ItemInfo **valueArray, int *count, parse_item_t type, int index);


/**
 * Add various items to the structs.
 */
/*@{*/
void vtkParse_AddClassToClass(ClassInfo *info, ClassInfo *item);
void vtkParse_AddFunctionToClass(ClassInfo *info, FunctionInfo *item);
void vtkParse_AddEnumToClass(ClassInfo *info, EnumInfo *item);
void vtkParse_AddUnionToClass(ClassInfo *info, UnionInfo *item);
void vtkParse_AddConstantToClass(ClassInfo *info, ValueInfo *item);
void vtkParse_AddVariableToClass(ClassInfo *info, ValueInfo *item);
void vtkParse_AddTypedefToClass(ClassInfo *info, ValueInfo *item);
void vtkParse_AddNamespaceToNamespace(NamespaceInfo *info,NamespaceInfo *item);
void vtkParse_AddClassToNamespace(NamespaceInfo *info, ClassInfo *item);
void vtkParse_AddFunctionToNamespace(NamespaceInfo *info, FunctionInfo *item);
void vtkParse_AddEnumToNamespace(NamespaceInfo *info, EnumInfo *item);
void vtkParse_AddUnionToNamespace(NamespaceInfo *info, UnionInfo *item);
void vtkParse_AddConstantToNamespace(NamespaceInfo *info, ValueInfo *item);
void vtkParse_AddVariableToNamespace(NamespaceInfo *info, ValueInfo *item);
void vtkParse_AddTypedefToNamespace(NamespaceInfo *info, ValueInfo *item);
void vtkParse_AddArgumentToFunction(FunctionInfo *info, ValueInfo *item);
void vtkParse_AddArgumentToTemplate(TemplateArgs *info, TemplateArg *item);
/*@}*/

/**
 * Ignore BTX/ETX markers
 */
void vtkParse_SetIgnoreBTX(int option);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
