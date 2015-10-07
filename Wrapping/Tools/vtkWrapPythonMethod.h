/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapPythonMethod.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkWrapPythonMethod_h
#define vtkWrapPythonMethod_h

#include "vtkParse.h"
#include "vtkParseData.h"
#include "vtkParseHierarchy.h"

/* print out the code for one method, including all of its overloads */
void vtkWrapPython_GenerateOneMethod(
  FILE *fp, const char *classname, ClassInfo *data, HierarchyInfo *hinfo,
  FunctionInfo *wrappedFunctions[], int numberOfWrappedFunctions, int fnum,
  int is_vtkobject, int do_constructors);

/* declare all variables needed by the wrapper method */
void vtkWrapPython_DeclareVariables(
  FILE *fp, ClassInfo *data, FunctionInfo *theFunc);

/* Write the code to convert an argument with vtkPythonArgs */
void vtkWrapPython_GetSingleArgument(
  FILE *fp, ClassInfo *data, int i, ValueInfo *arg, int call_static);

/* print the code to build python return value from a method */
void vtkWrapPython_ReturnValue(
  FILE *fp, ClassInfo *data, ValueInfo *val, int static_call);

#endif /* vtkWrapPythonMethod_h */
