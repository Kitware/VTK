/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapPythonTemplate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef VTK_WRAP_PYTHON_TEMPLATE_H
#define VTK_WRAP_PYTHON_TEMPLATE_H

#include "vtkParse.h"
#include "vtkParseData.h"
#include "vtkParseHierarchy.h"

/* if name has template args, convert to pythonic dict format */
size_t vtkWrapPython_PyTemplateName(const char *name, char *pname);

/* wrap a templated class */
int vtkWrapPython_WrapTemplatedClass(
  FILE *fp, ClassInfo *data, FileInfo *file_info, HierarchyInfo *hinfo);

/* if name has template args, mangle and prefix with "T" */
void vtkWrapPython_PythonicName(const char *name, char *pname);

#endif /* VTK_WRAP_PYTHON_TEMPLATE_H */
