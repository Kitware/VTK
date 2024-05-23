// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2010,2015 David Gobbi
// SPDX-License-Identifier: BSD-3-Clause
/**
 * This file contains structures and methods for finding properties
 * based on the Set and Get functions defined in the ClassInfo struct
 * created by vtkParse
 */

#ifndef VTK_PARSE_PROPERTIES_H
#define VTK_PARSE_PROPERTIES_H

#include "vtkParseData.h"
#include "vtkParseHierarchy.h"
#include "vtkWrappingToolsModule.h"

/**
 * bitfield values to say what methods are available for a property
 *
 * GET       is "type GetValue()" or "type *GetValue()"
 * SET       is "void SetValue(type)" or "type SetValue(type [])"
 * GET_MULTI is "void GetValue(type&, type&, type&)"
 * SET_MULTI is "void SetValue(type, type, type)"
 * GET_IDX   is "type GetValue(int)" or "type *GetValue(int)"
 * SET_IDX   is "void SetValue(int, type)" or "void SetValue(int, type [])"
 * GET_NTH   is "type GetNthValue(int)" or "type *GetNthValue(int)"
 * SET_NTH   is "void SetNthValue(int,type)" or "void SetNthValue(int,type[])"
 * GET_RHS   is "void GetValue(type *)"
 * GET_IDX_RHS is "void GetValue(int, type [])"
 * GET_NTH_RHS is "void GetNthValue(int, type [])"
 * GET_AS_STRING is "const char *GetValueAsString()"
 * SET_VALUE_TO is "void SetValueToEnumVal()"
 * BOOL_ON   is "void ValueOn()"
 * BOOL_OFF  is "void ValueOff()"
 * GET_MIN_VALUE is "type GetVarMinValue()"
 * GET_MAX_VALUE is "type GetVarMaxValue()"
 * GET_NUMBER_OF is "int GetNumberOfValues()"
 * SET_NUMBER_OF is "void SetNumberOfValues(int)"
 * ADD        is "void AddValue(type)"
 * ADD_MULTI  is "void AddValue(type, type, type)"
 * ADD_IDX    is "void AddValue(int, type)"
 * REMOVE     is "void RemoveValue(type)"
 * REMOVE_IDX is "void RemoveValue(int, type)"
 * REMOVE_ALL is "void RemoveAllValues()"
 * ADD_NODISCARD is "int AddValue(type)"
 * REMOVE_NODISCARD is "bool RemoveValue(type)"
 *
 * Note: If more method types are introduced, do not forget to change VTK_METHOD_MAX_MSB_POSITION
 *
 */
#define VTK_METHOD_GET 0x00000001
#define VTK_METHOD_SET 0x00000002
#define VTK_METHOD_GET_MULTI 0x00000004
#define VTK_METHOD_SET_MULTI 0x00000008
#define VTK_METHOD_GET_IDX 0x00000010
#define VTK_METHOD_SET_IDX 0x00000020
#define VTK_METHOD_GET_NTH 0x00000040
#define VTK_METHOD_SET_NTH 0x00000080
#define VTK_METHOD_GET_RHS 0x00000100
#define VTK_METHOD_GET_IDX_RHS 0x00000200
#define VTK_METHOD_GET_NTH_RHS 0x00000400
#define VTK_METHOD_GET_AS_STRING 0x00001000
#define VTK_METHOD_SET_VALUE_TO 0x00002000
#define VTK_METHOD_BOOL_ON 0x00004000
#define VTK_METHOD_BOOL_OFF 0x00008000
#define VTK_METHOD_GET_MIN_VALUE 0x00010000
#define VTK_METHOD_GET_MAX_VALUE 0x00020000
#define VTK_METHOD_GET_NUMBER_OF 0x00040000
#define VTK_METHOD_SET_NUMBER_OF 0x00080000
#define VTK_METHOD_ADD 0x00100000
#define VTK_METHOD_ADD_MULTI 0x00200000
#define VTK_METHOD_ADD_IDX 0x00400000
#define VTK_METHOD_REMOVE 0x01000000
#define VTK_METHOD_REMOVE_IDX 0x04000000
#define VTK_METHOD_REMOVE_ALL 0x08000000
#define VTK_METHOD_ADD_NODISCARD 0x10000000
#define VTK_METHOD_REMOVE_NODISCARD 0x20000000

#define VTK_METHOD_SET_CLAMP (VTK_METHOD_GET_MIN_VALUE | VTK_METHOD_GET_MAX_VALUE)

#define VTK_METHOD_SET_BOOL (VTK_METHOD_BOOL_ON | VTK_METHOD_BOOL_OFF)

// Maximum position of the MSB among all method types.
#define VTK_METHOD_MAX_MSB_POSITION 29

/**
 * A struct that contains all the property information that
 * can be ascertained from the vtkParse info
 */
typedef struct PropertyInfo_
{
  const char* Name;               /* property name */
  unsigned int Type;              /* property type as VTK_PARSE constant */
  int Count;                      /* the count for array-type properties */
  const char* ClassName;          /* VTK object type of the property, or NULL */
  const char** EnumConstantNames; /* the names of int enum values */
  unsigned int PublicMethods;     /* bitfield for public methods */
  unsigned int ProtectedMethods;  /* bitfield for protected methods */
  unsigned int PrivateMethods;    /* bitfield for private methods */
  unsigned int LegacyMethods;     /* bitfield for legacy methods */
  const char* Comment;            /* comment from header file */
  int IsStatic;                   /* if the property is static */
} PropertyInfo;

/**
 * List of methods for accessing/changing properties
 */
typedef struct ClassProperties_
{
  int NumberOfProperties;    /* total number of properties found */
  PropertyInfo** Properties; /* info for each property */
  int NumberOfMethods;       /* number of methods in FunctionInfo */
  unsigned int* MethodTypes; /* discovered type of each method */
  int* MethodHasProperty;    /* method has a property */
  int* MethodProperties;     /* discovered property for each method */
} ClassProperties;

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * Build the ClassProperties struct from a ClassInfo struct
   */
  VTKWRAPPINGTOOLS_EXPORT ClassProperties* vtkParseProperties_Create(
    ClassInfo* data, const HierarchyInfo* hinfo);

  /**
   * Free a ClassProperties struct
   */
  VTKWRAPPINGTOOLS_EXPORT void vtkParseProperties_Free(ClassProperties* properties);

  /**
   * Convert a method bitfield to a string,
   * e.g. VTK_METHOD_GET -> "METHOD_GET"
   */
  VTKWRAPPINGTOOLS_EXPORT const char* vtkParseProperties_MethodTypeAsString(
    unsigned int methodType);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
/* VTK-HeaderTest-Exclude: vtkParseProperties.h */
