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

#ifndef VTK_WRAP_PYTHON_CONSTANT_H
#define VTK_WRAP_PYTHON_CONSTANT_H

#include "vtkParse.h"
#include "vtkParseData.h"
#include "vtkParseHierarchy.h"

/* Generate code that add a constant value to a python dict */
void vtkWrapPython_AddConstant(
  FILE *fp, const char *indent, const char *dictvar, const char *objvar,
  ValueInfo *val);

#endif /* VTK_WRAP_PYTHON_CONSTANT_H */
