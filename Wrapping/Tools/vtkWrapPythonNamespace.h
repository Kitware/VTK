/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapPythonNamespace.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkWrapPythonNamespace_h
#define vtkWrapPythonNamespace_h

#include "vtkParse.h"
#include "vtkParseData.h"
#include "vtkParseHierarchy.h"

/* Wrap one class, returns zero if not wrappable */
int vtkWrapPython_WrapNamespace(
  FILE *fp, const char *module, NamespaceInfo *data);

#endif /* vtkWrapPythonNamespace_h */
