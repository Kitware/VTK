/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapJavaScriptConstant.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkWrapJavaScriptConstant_h
#define vtkWrapJavaScriptConstant_h

#include "vtkParseData.h"

#include <stdio.h>

/* generate code that adds all public constants in a namespace */
void vtkWrapJavaScript_GenerateConstants(
  FILE* fp, const char* module, const char* basename, const char* indent, NamespaceInfo* data);

/* generate code that adds a constant value to the binding file */
void vtkWrapJavaScript_AddConstant(FILE* fp, const char* indent, ValueInfo* val);

#endif /* vtkWrapJavaScriptConstant_h */
// VTK-HeaderTest-Exclude: vtkWrapJavaScriptConstant.h
