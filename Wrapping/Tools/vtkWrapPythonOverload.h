/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapPythonOverload.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef VTK_WRAP_PYTHON_OVERLOAD_H
#define VTK_WRAP_PYTHON_OVERLOAD_H

#include "vtkParse.h"
#include "vtkParseData.h"
#include "vtkParseHierarchy.h"

/* output the method table for all overloads of a particular method */
void vtkWrapPython_OverloadMethodDef(
  FILE *fp, const char *classname, ClassInfo *data, int *overloadMap,
  FunctionInfo **wrappedFunctions, int numberOfWrappedFunctions, int fnum,
  int numberOfOccurrences, int is_vtkobject, int all_legacy);

/* a master method to choose which overload to call */
void vtkWrapPython_OverloadMasterMethod(
  FILE *fp, const char *classname, int *overloadMap, int maxArgs,
  FunctionInfo **wrappedFunctions, int numberOfWrappedFunctions, int fnum,
  int is_vtkobject, int all_legacy);

/* generate an int array that maps arg counts to overloads */
int *vtkWrapPython_ArgCountToOverloadMap(
  FunctionInfo **wrappedFunctions, int numberOfWrappedFunctions,
  int fnum, int is_vtkobject, int *nmax, int *overlap);

#endif /* VTK_WRAP_PYTHON_OVERLOAD_H */
