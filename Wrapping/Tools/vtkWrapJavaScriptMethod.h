/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapJavaScriptMethod.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkWrapJavaScriptMethod_h
#define vtkWrapJavaScriptMethod_h

#include "vtkParseData.h"
#include "vtkParseHierarchy.h"

#include <stdio.h>

/* print out the code for one method, including all of its overloads */
void vtkWrapJavaScript_GenerateOneMethod(FILE* fp, const char* classname, ClassInfo* data,
  FunctionInfo* wrappedFunctions[], int numberOfWrappedFunctions,
  FunctionInfo* unwrappableFunctions[], int numberOfUnwrappableFunctions, int fnum,
  int is_vtkobject, const char* indent);

/* check whether a method is wrappable */
int vtkWrapJavaScript_MethodCheck(
  ClassInfo* data, FunctionInfo* currentFunction, HierarchyInfo* hinfo);

/* print out all methods */
void vtkWrapJavaScript_GenerateMethods(FILE* fp, const char* classname, ClassInfo* data,
  FileInfo* finfo, HierarchyInfo* hinfo, const char* indent);

#endif /* vtkWrapJavaScriptMethod_h */
// VTK-HeaderTest-Exclude: vtkWrapJavaScriptMethod.h
