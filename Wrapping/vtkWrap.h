/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrap.h

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

#ifndef VTK_WRAP_H
#define VTK_WRAP_H

#include "vtkParse.h"
#include "vtkParseHierarchy.h"

/**
 * For use with vtkWrap_DeclareVariable.
 */
/*@{*/
#define VTK_WRAP_RETURN  1
#define VTK_WRAP_ARG     2
#define VTK_WRAP_NOSEMI  4
/*@}*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Check for common types.
 */
/*@{*/
int vtkWrap_IsVoid(ValueInfo *val);
int vtkWrap_IsVoidFunction(ValueInfo *val);
int vtkWrap_IsVoidPointer(ValueInfo *val);
int vtkWrap_IsCharPointer(ValueInfo *val);
int vtkWrap_IsVTKObject(ValueInfo *val);
int vtkWrap_IsSpecialObject(ValueInfo *val);
int vtkWrap_IsQtObject(ValueInfo *val);
int vtkWrap_IsQtEnum(ValueInfo *val);
/*@}*/

/**
 * The basic types, all are mutually exclusive.
 * Note that enums are considered to be objects,
 * bool and char are considered to be numeric.
 */
/*@{*/
int vtkWrap_IsObject(ValueInfo *val);
int vtkWrap_IsFunction(ValueInfo *val);
int vtkWrap_IsStream(ValueInfo *val);
int vtkWrap_IsNumeric(ValueInfo *val);
int vtkWrap_IsString(ValueInfo *val);
/*@}*/

/**
 * Subcategories of numeric types.  In this categorization,
 * bool and char are not considered to be integers.
 */
/*@{*/
int vtkWrap_IsBool(ValueInfo *val);
int vtkWrap_IsChar(ValueInfo *val);
int vtkWrap_IsInteger(ValueInfo *val);
int vtkWrap_IsRealNumber(ValueInfo *val);
/*@}*/

/**
 * Arrays and pointers. These are mutually exclusive.
 * IsPointer() does not include pointers to pointers.
 * IsArray() and IsNArray() do not include unsized arrays.
 * Arrays of pointers are not included in any of these.
 */
/*@{*/
int vtkWrap_IsScalar(ValueInfo *val);
int vtkWrap_IsPointer(ValueInfo *val);
int vtkWrap_IsArray(ValueInfo *val);
int vtkWrap_IsNArray(ValueInfo *val);
/*@}*/

/**
 * Properties that can combine with other properties.
 */
/*@{*/
int vtkWrap_IsNonConstRef(ValueInfo *val);
int vtkWrap_IsConstRef(ValueInfo *val);
int vtkWrap_IsRef(ValueInfo *val);
int vtkWrap_IsConst(ValueInfo *val);
/*@}*/

/**
 * Hints.
 * NewInstance objects must be freed by the caller.
 */
/*@{*/
int vtkWrap_IsNewInstance(ValueInfo *val);
/*@}*/


/**
 * Check whether the class is derived from vtkObjectBase.
 * If "hinfo" is NULL, this just checks that the class
 * name starts with "vtk".
 */
int vtkWrap_IsVTKObjectBaseType(
  HierarchyInfo *hinfo, const char *classname);

/**
 * Check if the WRAP_SPECIAL flag is set for the class.
 * If "hinfo" is NULL, it defaults to just checking if
 * the class starts with "vtk" and returns -1 if so.
 */
int vtkWrap_IsSpecialType(
  HierarchyInfo *hinfo, const char *classname);

/**
 * Check if the class is derived from superclass.
 * If "hinfo" is NULL, then only an exact match to the
 * superclass will succeed.
 */
int vtkWrap_IsTypeOf(
  HierarchyInfo *hinfo, const char *classname, const char *superclass);

/**
 * Check whether a class is wrapped.  If "hinfo" is NULL,
 * it just checks that the class starts with "vtk".
 */
int vtkWrap_IsClassWrapped(
  HierarchyInfo *hinfo, const char *classname);

/**
 * Check whether the destructor is public
 */
int vtkWrap_HasPublicDestructor(ClassInfo *data);

/**
 * Check whether the copy constructor is public
 */
int vtkWrap_HasPublicCopyConstructor(ClassInfo *data);

/**
 * Expand all typedef types that are used in function arguments.
 * This should be done before any wrapping is done, to make sure
 * that the wrappers see the real types.
 */
void vtkWrap_ExpandTypedefs(ClassInfo *data, HierarchyInfo *hinfo);

/**
 * Apply any hints about array sizes, e.g. hint that the
 * GetNumberOfComponents() method gives the tuple size.
 */
void vtkWrap_FindCountHints(
  ClassInfo *data, HierarchyInfo *hinfo);

/**
 * Get the size of a fixed-size tuple
 */
int vtkWrap_GetTupleSize(ClassInfo *data, HierarchyInfo *hinfo);

/**
 * Apply any hints about methods that return a new object instance,
 * i.e. factory methods and the like.  Reference counts must be
 * handled differently for such returned objects.
 */
void vtkWrap_FindNewInstanceMethods(
  ClassInfo *data, HierarchyInfo *hinfo);

/**
 * Get the name of a type.  The name will not include "const".
 */
const char *vtkWrap_GetTypeName(ValueInfo *val);

/**
 * True if the method a constructor of the class.
 */
int vtkWrap_IsConstructor(ClassInfo *c, FunctionInfo *f);

/**
 * True if the method a destructor of the class.
 */
int vtkWrap_IsDestructor(ClassInfo *c, FunctionInfo *f);

/**
 * Check if a method is from a SetVector method.
 */
int vtkWrap_IsSetVectorMethod(FunctionInfo *f);

/**
 * Check if a method is from a GetVector method.
 */
int vtkWrap_IsGetVectorMethod(FunctionInfo *f);

/**
 * Count the number of args that are wrapped.
 * This skips the "void *" argument that follows
 * wrapped function pointer arguments.
 */
int vtkWrap_CountWrappedArgs(FunctionInfo *f);

/**
 * Count the number of args that are required.
 * This counts to the last argument that does not
 * have a default value.  Array args are not allowed
 * to have default values.
 */
int vtkWrap_CountRequiredArgs(FunctionInfo *f);

/**
 * Write a variable declaration to a file.
 * Void is automatically ignored, and nothing is written for
 * function pointers
 * Set "idx" to -1 to avoid writing an idx.
 * Set "flags" to VTK_WRAP_RETURN to write a return value,
 * or to VTK_WRAP_ARG to write a temp argument variable.
 * The following rules apply:
 * - if VTK_WRAP_NOSEMI is set, then no semicolon/newline is printed
 * - if VTK_WRAP_RETURN is set, then "&" becomes "*"
 * - if VTK_WRAP_ARG is set, "&" becomes "*" only for object
 *   types, and is removed for all other types.
 * - "const" is removed except for return values with "&" or "*".
 */
void vtkWrap_DeclareVariable(
  FILE *fp, ValueInfo *v, const char *name, int idx, int flags);

/**
 * Write an "int" size variable for arrays, initialized to
 * the array size if the size is greater than zero.
 * For N-dimensional arrays, write a static array of ints.
 */
void vtkWrap_DeclareVariableSize(
  FILE *fp, ValueInfo *v, const char *name, int idx);


#ifdef __cplusplus
}
#endif

#endif
