// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * vtkWrap provides useful functions for generating wrapping code.
 */

#ifndef vtkWrap_h
#define vtkWrap_h

#include "vtkParse.h"
#include "vtkParseHierarchy.h"
#include "vtkParseMain.h"
#include "vtkWrappingToolsModule.h"

/**
 * For use with vtkWrap_DeclareVariable.
 */
/*@{*/
#define VTK_WRAP_RETURN 1
#define VTK_WRAP_ARG 2
#define VTK_WRAP_NOSEMI 4
/*@}*/

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * Check for common types.
   * IsPODPointer is for unsized arrays of POD types.
   * IsZeroCopyPointer is for buffers that shouldn't be copied.
   * IsArrayRef is for references to arrays.
   */
  /*@{*/
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsVoid(const ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsVoidFunction(const ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsVoidPointer(const ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsCharPointer(const ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsPODPointer(const ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsZeroCopyPointer(const ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsArrayRef(const ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsStdVector(const ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsStdMap(const ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsVTKObject(const ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsVTKSmartPointer(const ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsSpecialObject(const ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsPythonObject(const ValueInfo* val);
  /*@}*/

  /**
   * The basic types, all are mutually exclusive.
   * Note that enums are considered to be objects,
   * bool and char are considered to be numeric.
   */
  /*@{*/
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsObject(const ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsFunction(const ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsStream(const ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsNumeric(const ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsString(const ValueInfo* val);
  /*@}*/

  /**
   * Subcategories of numeric types.  In this categorization,
   * bool and char are not considered to be integers.
   */
  /*@{*/
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsBool(const ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsChar(const ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsInteger(const ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsRealNumber(const ValueInfo* val);
  /*@}*/

  /**
   * Arrays and pointers. These are mutually exclusive.
   * IsPointer() does not include pointers to pointers.
   * IsArray() and IsNArray() do not include unsized arrays.
   * Arrays of pointers are not included in any of these.
   */
  /*@{*/
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsScalar(const ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsPointer(const ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsArray(const ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsNArray(const ValueInfo* val);
  /*@}*/

  /**
   * Properties that can combine with other properties.
   */
  /*@{*/
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsNonConstRef(const ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsConstRef(const ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsRef(const ValueInfo* val);
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsConst(const ValueInfo* val);
  /*@}*/

  /**
   * Hints.
   * NewInstance objects must be freed by the caller.
   */
  /*@{*/
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsNewInstance(const ValueInfo* val);
  /*@}*/

  /**
   * Check whether the class is derived from vtkObjectBase.
   * If "hinfo" is NULL, this just checks that the class
   * name starts with "vtk".
   */
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsVTKObjectBaseType(
    const HierarchyInfo* hinfo, const char* classname);

  /**
   * Check whether the class is not derived from vtkObjectBase.
   * If "hinfo" is NULL, it defaults to just checking if
   * the class starts with "vtk" and returns -1 if so.
   */
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsSpecialType(
    const HierarchyInfo* hinfo, const char* classname);

  /**
   * Check if the class is derived from superclass.
   * If "hinfo" is NULL, then only an exact match to the
   * superclass will succeed.
   */
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsTypeOf(
    const HierarchyInfo* hinfo, const char* classname, const char* superclass);

  /**
   * Check if the type of the value is an enum member of the class.
   */
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsEnumMember(const ClassInfo* data, const ValueInfo* arg);

  /**
   * Check whether a class is wrapped.  If "hinfo" is NULL,
   * it just checks that the class starts with "vtk".
   */
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsClassWrapped(
    const HierarchyInfo* hinfo, const char* classname);

  /**
   * Check whether the destructor is public
   */
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_HasPublicDestructor(ClassInfo* data);

  /**
   * Check whether the copy constructor is public
   */
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_HasPublicCopyConstructor(ClassInfo* data);

  /**
   * Expand all typedef types that are used in function arguments.
   * This should be done before any wrapping is done, to make sure
   * that the wrappers see the real types.
   */
  VTKWRAPPINGTOOLS_EXPORT void vtkWrap_ExpandTypedefs(
    ClassInfo* data, FileInfo* finfo, const HierarchyInfo* hinfo);

  /**
   * Apply any using declarations that appear in the class.
   * If any using declarations appear in the class that refer to superclass
   * methods, the superclass header file will be parsed and the used methods
   * will be brought into the class.
   */
  VTKWRAPPINGTOOLS_EXPORT void vtkWrap_ApplyUsingDeclarations(
    ClassInfo* data, FileInfo* finfo, const HierarchyInfo* hinfo);

  /**
   * Merge members of all superclasses into the data structure.
   * The superclass header files will be read and parsed.
   */
  VTKWRAPPINGTOOLS_EXPORT void vtkWrap_MergeSuperClasses(
    ClassInfo* data, FileInfo* finfo, const HierarchyInfo* hinfo);

  /**
   * Apply any hints about array sizes, e.g. hint that the
   * GetNumberOfComponents() method gives the tuple size.
   */
  VTKWRAPPINGTOOLS_EXPORT void vtkWrap_FindCountHints(
    ClassInfo* data, FileInfo* finfo, const HierarchyInfo* hinfo);

  /**
   * Get the size of a fixed-size tuple
   */
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_GetTupleSize(
    const ClassInfo* data, const HierarchyInfo* hinfo);

  /**
   * Apply any hints about methods that return a new object instance,
   * i.e. factory methods and the like.  Reference counts must be
   * handled differently for such returned objects.
   */
  VTKWRAPPINGTOOLS_EXPORT void vtkWrap_FindNewInstanceMethods(
    ClassInfo* data, const HierarchyInfo* hinfo);

  /**
   * Apply hints about methods that take file names or other
   * file paths as arguments
   */
  VTKWRAPPINGTOOLS_EXPORT void vtkWrap_FindFilePathMethods(ClassInfo* data);

  /**
   * Get the name of a type.  The name will not include "const".
   */
  VTKWRAPPINGTOOLS_EXPORT const char* vtkWrap_GetTypeName(const ValueInfo* val);

  /**
   * True if the method a constructor of the class.
   */
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsConstructor(const ClassInfo* c, const FunctionInfo* f);

  /**
   * True if the method a destructor of the class.
   */
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsDestructor(const ClassInfo* c, const FunctionInfo* f);

  /**
   * True if the method is inherited from a base class.
   */
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsInheritedMethod(const ClassInfo* c, const FunctionInfo* f);

  /**
   * Check if a method is from a SetVector method.
   */
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsSetVectorMethod(const FunctionInfo* f);

  /**
   * Check if a method is from a GetVector method.
   */
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_IsGetVectorMethod(const FunctionInfo* f);

  /**
   * Count the number of parameters that are wrapped.
   * This skips the "void *" parameter that follows
   * wrapped function pointer parameters.
   */
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_CountWrappedParameters(const FunctionInfo* f);

  /**
   * Count the number of args that are required.
   * This counts to the last argument that does not
   * have a default value.  Array args are not allowed
   * to have default values.
   */
  VTKWRAPPINGTOOLS_EXPORT int vtkWrap_CountRequiredArguments(const FunctionInfo* f);

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
  VTKWRAPPINGTOOLS_EXPORT void vtkWrap_DeclareVariable(
    FILE* fp, const ClassInfo* data, const ValueInfo* val, const char* name, int idx, int flags);

  /**
   * Write an "int" size variable for arrays, initialized to
   * the array size if the size is greater than zero.
   * For N-dimensional arrays, write a static array of ints.
   */
  VTKWRAPPINGTOOLS_EXPORT void vtkWrap_DeclareVariableSize(
    FILE* fp, const ValueInfo* val, const char* name, int idx);

  /**
   * Qualify all the unqualified identifiers in the given expression
   * and print the result to the file.
   */
  VTKWRAPPINGTOOLS_EXPORT void vtkWrap_QualifyExpression(
    FILE* fp, const ClassInfo* data, const char* text);

  /**
   * Makes a superclass name into a valid identifier. Returns NULL if the given
   * name is valid as-is, otherwise returns a malloc'd string.
   */
  VTKWRAPPINGTOOLS_EXPORT char* vtkWrap_SafeSuperclassName(const char* name);

  /**
   * Return the arg from "templated<T>" as a malloc'd string.
   */
  VTKWRAPPINGTOOLS_EXPORT char* vtkWrap_TemplateArg(const char* name);

  /**
   * Emit a warning about nothing being wrapped.
   *
   * Depends on the warning flag being requested.
   */
  VTKWRAPPINGTOOLS_EXPORT void vtkWrap_WarnEmpty(const OptionInfo* options);

#ifdef __cplusplus
}
#endif

#endif
/* VTK-HeaderTest-Exclude: vtkWrap.h */
