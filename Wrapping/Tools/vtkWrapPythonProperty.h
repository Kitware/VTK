// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWrapPythonProperty_h
#define vtkWrapPythonProperty_h

#include "vtkParseData.h"
#include "vtkParseHierarchy.h"
#include "vtkParseProperties.h"

#include <stdio.h>

/* print out all properties in the getset table. */
void vtkWrapPython_GenerateProperties(FILE* fp, const char* classname, ClassInfo* data,
  const HierarchyInfo* hinfo, ClassProperties* properties, int is_vtkobject);

#endif /* vtkWrapPythonProperty_h */
/* VTK-HeaderTest-Exclude: vtkWrapPythonProperty.h */
