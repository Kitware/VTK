// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWrapPythonType_h
#define vtkWrapPythonType_h

#include "vtkParse.h"
#include "vtkParseData.h"
#include "vtkParseHierarchy.h"

/* check whether a non-vtkObjectBase class is wrappable */
int vtkWrapPython_IsSpecialTypeWrappable(const ClassInfo* data);

/* write out a python type object */
void vtkWrapPython_GenerateSpecialType(FILE* fp, const char* module, const char* classname,
  ClassInfo* data, FileInfo* finfo, const HierarchyInfo* hinfo);

#endif /* vtkWrapPythonType_h */
/* VTK-HeaderTest-Exclude: vtkWrapPythonType.h */
