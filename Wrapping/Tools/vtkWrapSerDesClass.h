// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWrapSerDesClass_h
#define vtkWrapSerDesClass_h

#include "vtkParseHierarchy.h"

#include <stdio.h>

/* Export registrar functions for this class */
void vtkWrapSerDes_ExportClassRegistrars(FILE* fp, const char* name);

void vtkWrapSerDes_Class(FILE* fp, const HierarchyInfo* hinfo, ClassInfo* classInfo);

const char* vtkWrapSerDes_GetSuperClass(
  const ClassInfo* data, const HierarchyInfo* hinfo, const char** supermodule);

#endif
