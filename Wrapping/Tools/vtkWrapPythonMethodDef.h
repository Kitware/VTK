// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWrapPythonMethodDef_h
#define vtkWrapPythonMethodDef_h

#include "vtkParse.h"
#include "vtkParseData.h"
#include "vtkParseHierarchy.h"

/* check whether a method is wrappable */
int vtkWrapPython_MethodCheck(
  const ClassInfo* data, const FunctionInfo* currentFunction, const HierarchyInfo* hinfo);

/* print out all methods and the method table */
void vtkWrapPython_GenerateMethods(FILE* fp, const char* classname, ClassInfo* data,
  FileInfo* finfo, const HierarchyInfo* hinfo, int is_vtkobject, int do_constructors);

#endif /* vtkWrapPythonMethodDef_h */
/* VTK-HeaderTest-Exclude: vtkWrapPythonMethodDef.h */
