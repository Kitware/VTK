/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapPythonMethodDef.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef VTK_WRAP_PYTHON_METHODDEF_H
#define VTK_WRAP_PYTHON_METHODDEF_H

#include "vtkParse.h"
#include "vtkParseData.h"
#include "vtkParseHierarchy.h"

/* check whether a method is wrappable */
int vtkWrapPython_MethodCheck(
  ClassInfo *data, FunctionInfo *currentFunction, HierarchyInfo *hinfo);

/* print out all methods and the method table */
void vtkWrapPython_GenerateMethods(
  FILE *fp, const char *classname, ClassInfo *data,
  FileInfo *finfo, HierarchyInfo *hinfo,
  int is_vtkobject, int do_constructors);

#endif /* VTK_WRAP_PYTHON_METHODDEF_H */
