/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapJavaScriptNamespace.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkWrapJavaScriptNamespace_h
#define vtkWrapJavaScriptNamespace_h

#include "vtkParseData.h"

#include <stdio.h>

/* Wrap a namespace, returns zero if not wrappable */
int vtkWrapJavaScript_Namespace(FILE* fp, const char* module, NamespaceInfo* data);

#endif /* vtkWrapJavaScriptNamespace_h */
// VTK-HeaderTest-Exclude: vtkWrapJavaScriptNamespace.h
