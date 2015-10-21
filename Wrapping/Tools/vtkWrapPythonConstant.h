/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapPythonConstant.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkWrapPythonConstant_h
#define vtkWrapPythonConstant_h

#include "vtkParse.h"
#include "vtkParseData.h"
#include "vtkParseHierarchy.h"

/* generate code that adds a constant value to a python dict */
void vtkWrapPython_AddConstant(
  FILE *fp, const char *indent, const char *dictvar, const char *objvar,
  const char *scope, ValueInfo *val);

/* generate code that adds all public constants in a namespace */
void vtkWrapPython_AddPublicConstants(
  FILE *fp, const char *indent, const char *dictvar, const char *objvar,
  NamespaceInfo *data);

#endif /* vtkWrapPythonConstant_h */
