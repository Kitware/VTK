// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWrapPythonClass_h
#define vtkWrapPythonClass_h

#include "vtkParse.h"
#include "vtkParseData.h"
#include "vtkParseHierarchy.h"

/* Wrap one class, returns zero if not wrappable */
int vtkWrapPython_WrapOneClass(FILE* fp, const char* module, const char* classname, ClassInfo* data,
  FileInfo* file_info, HierarchyInfo* hinfo, int is_vtkobject);

/* Get the true superclass and, if the superclass is in a different module,
   then also provide the name of the module.  The "supermodule" will be set
   to NULL if the superclass is in the same module as the class. */
const char* vtkWrapPython_GetSuperClass(
  ClassInfo* data, HierarchyInfo* hinfo, const char** supermodule);

/* generate the class docstring and write it to "fp" */
void vtkWrapPython_ClassDoc(
  FILE* fp, FileInfo* file_info, ClassInfo* data, HierarchyInfo* hinfo, int is_vtkobject);

#endif /* vtkWrapPythonClass_h */
/* VTK-HeaderTest-Exclude: vtkWrapPythonClass.h */
