// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWrapPythonTemplate_h
#define vtkWrapPythonTemplate_h

#include "vtkParse.h"
#include "vtkParseData.h"
#include "vtkParseHierarchy.h"

/* if name has template args, convert to pythonic dict format */
size_t vtkWrapPython_PyTemplateName(const char* name, char* pname);

/* wrap a templated class */
int vtkWrapPython_WrapTemplatedClass(
  FILE* fp, ClassInfo* data, FileInfo* file_info, HierarchyInfo* hinfo);

#endif /* vtkWrapPythonTemplate_h */
/* VTK-HeaderTest-Exclude: vtkWrapPythonTemplate.h */
