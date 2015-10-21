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

#ifndef vtkWrapPythonTemplate_h
#define vtkWrapPythonTemplate_h

#include "vtkParse.h"
#include "vtkParseData.h"
#include "vtkParseHierarchy.h"

/* if name has template args, convert to pythonic dict format */
size_t vtkWrapPython_PyTemplateName(const char *name, char *pname);

/* wrap a templated class */
int vtkWrapPython_WrapTemplatedClass(
  FILE *fp, ClassInfo *data, FileInfo *file_info, HierarchyInfo *hinfo);

#endif /* vtkWrapPythonTemplate_h */
