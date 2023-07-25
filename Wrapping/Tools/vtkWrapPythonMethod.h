// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWrapPythonMethod_h
#define vtkWrapPythonMethod_h

#include "vtkParse.h"
#include "vtkParseData.h"
#include "vtkParseHierarchy.h"

/* print out the code for one method, including all of its overloads */
void vtkWrapPython_GenerateOneMethod(FILE* fp, const char* classname, ClassInfo* data,
  FileInfo* finfo, HierarchyInfo* hinfo, FunctionInfo* wrappedFunctions[],
  int numberOfWrappedFunctions, int fnum, int is_vtkobject, int do_constructors);

/* declare all variables needed by the wrapper method */
void vtkWrapPython_DeclareVariables(FILE* fp, ClassInfo* data, FunctionInfo* theFunc);

/* Write the code to convert an argument with vtkPythonArgs */
void vtkWrapPython_GetSingleArgument(
  FILE* fp, ClassInfo* data, int i, ValueInfo* arg, int static_call);

/* print the code to build python return value from a method */
void vtkWrapPython_ReturnValue(FILE* fp, ClassInfo* data, ValueInfo* val, int static_call);

/* print the code that generates a DeprecationWarning */
void vtkWrapPython_DeprecationWarning(
  FILE* fp, const char* what, const char* name, const char* reason, const char* version);

#endif /* vtkWrapPythonMethod_h */
/* VTK-HeaderTest-Exclude: vtkWrapPythonMethod.h */
