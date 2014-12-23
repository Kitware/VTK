/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapPythonEnum.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef VTK_WRAP_PYTHON_ENUM_H
#define VTK_WRAP_PYTHON_ENUM_H

#include "vtkParse.h"
#include "vtkParseData.h"
#include "vtkParseHierarchy.h"

/* check whether an enum type will be wrapped */
int vtkWrapPython_IsEnumWrapped(
  HierarchyInfo *hinfo, const char *enumname);

/* write out an enum type wrapped in python */
void vtkWrapPython_GenerateEnumType(
  FILE *fp, const char *classname, EnumInfo *data);

/* generate code that adds an enum type to a python dict */
void vtkWrapPython_AddEnumType(
  FILE *fp, const char *indent, const char *dictvar, const char *objvar,
  const char *scope, EnumInfo *cls);

/* generate code that adds all public enum types to a python dict */
void vtkWrapPython_AddPublicEnumTypes(
  FILE *fp, const char *indent, const char *dictvar, const char *objvar,
  NamespaceInfo *data);

#endif /* VTK_WRAP_PYTHON_ENUM_H */
