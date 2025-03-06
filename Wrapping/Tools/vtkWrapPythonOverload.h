// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWrapPythonOverload_h
#define vtkWrapPythonOverload_h

#include "vtkParse.h"
#include "vtkParseData.h"
#include "vtkParseHierarchy.h"

/* output the method table for all overloads of a particular method */
void vtkWrapPython_OverloadMethodDef(FILE* fp, const char* classname, const ClassInfo* data,
  const int* overloadMap, FunctionInfo** wrappedFunctions, int numberOfWrappedFunctions, int fnum,
  int numberOfOccurrences);

/* a master method to choose which overload to call */
void vtkWrapPython_OverloadMasterMethod(FILE* fp, const char* classname, const int* overloadMap,
  int maxArgs, FunctionInfo** wrappedFunctions, int numberOfWrappedFunctions, int fnum,
  int is_vtkobject);

/* generate an int array that maps arg counts to overloads */
int* vtkWrapPython_ArgCountToOverloadMap(FunctionInfo** wrappedFunctions,
  int numberOfWrappedFunctions, int fnum, int is_vtkobject, int* nmax, int* overlap);

#endif /* vtkWrapPythonOverload_h */
/* VTK-HeaderTest-Exclude: vtkWrapPythonOverload.h */
