/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapJavaScriptClass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkWrapJavaScriptClass_h
#define vtkWrapJavaScriptClass_h

#include "vtkParseData.h"
#include "vtkParseHierarchy.h"

#include <stdio.h>

/* Wrap one class, returns zero if not wrappable */
int vtkWrapJavaScript_WrapOneClass(FILE* fp, const char* module, const char* classname,
  ClassInfo* data, FileInfo* file_info, HierarchyInfo* hinfo, int is_vtkobject);

/* Get the true superclass and, if the superclass is in a different module,
   then also provide the name of the module.  The "supermodule" will be set
   to NULL if the superclass is in the same module as the class. */
const char* vtkWrapJavaScript_GetSuperClass(
  ClassInfo* data, HierarchyInfo* hinfo, const char** supermodule);

/* generate the class docstring and write it to "fp" */
void vtkWrapJavaScript_ClassDoc(
  FILE* fp, FileInfo* file_info, ClassInfo* data, HierarchyInfo* hinfo, int is_vtkobject);

#endif /* vtkWrapJavaScriptClass_h */
/* VTK-HeaderTest-Exclude: vtkWrapJavaScriptClass.h */
