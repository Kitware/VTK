// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWrapSerDesFunction_h
#define vtkWrapSerDesFunction_h

#include "vtkParseData.h"
#include "vtkParseHierarchy.h"

#include <stdio.h>

/* Define function void Invoke_ClassName_FuncName(..) for all methods in class*/
void vtkWrapSerDes_DefineFunctions(FILE* fp, ClassInfo* classInfo, const HierarchyInfo* hinfo);

/* Generate code that calls Invoke_ClassName_FuncName() */
void vtkWrapSerDes_CallFunctions(FILE* fp, ClassInfo* classInfo, const HierarchyInfo* hinfo);

#endif
