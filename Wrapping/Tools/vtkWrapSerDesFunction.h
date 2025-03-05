// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWrapSerDesFunction_h
#define vtkWrapSerDesFunction_h

#include "vtkParseData.h"
#include "vtkParseHierarchy.h"

#include <stdio.h>

void vtkWrapSerDes_Functions(FILE* fp, ClassInfo* classInfo, const HierarchyInfo* hinfo);

#endif
