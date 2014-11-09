/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapPythonType.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef VTK_WRAP_PYTHON_TYPE_H
#define VTK_WRAP_PYTHON_TYPE_H

#include "vtkParse.h"
#include "vtkParseData.h"
#include "vtkParseHierarchy.h"

/* check whether a non-vtkObjectBase class is wrappable */
int vtkWrapPython_IsSpecialTypeWrappable(ClassInfo *data);

/* write out a python type object */
void vtkWrapPython_GenerateSpecialType(
  FILE *fp, const char *classname, ClassInfo *data,
  FileInfo *finfo, HierarchyInfo *hinfo);

#endif /* VTK_WRAP_PYTHON_TYPE_H */
