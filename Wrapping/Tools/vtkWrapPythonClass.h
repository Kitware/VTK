/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapPythonClass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef VTK_WRAP_PYTHON_CLASS_H
#define VTK_WRAP_PYTHON_CLASS_H

#include "vtkParse.h"
#include "vtkParseData.h"
#include "vtkParseHierarchy.h"

/* Wrap one class, returns zero if not wrappable */
int vtkWrapPython_WrapOneClass(
  FILE *fp, const char *classname, ClassInfo *data,
  FileInfo *file_info, HierarchyInfo *hinfo, int is_vtkobject);

/* get the true superclass */
const char *vtkWrapPython_GetSuperClass(
  ClassInfo *data, HierarchyInfo *hinfo);

/* check whether the superclass of the specified class is wrapped */
int vtkWrapPython_HasWrappedSuperClass(
  HierarchyInfo *hinfo, const char *classname, int *is_external);

/* generate the class docstring and write it to "fp" */
void vtkWrapPython_ClassDoc(
  FILE *fp, FileInfo *file_info, ClassInfo *data, HierarchyInfo *hinfo,
  int is_vtkobject);

#endif /* VTK_WRAP_PYTHON_CLASS_H */
