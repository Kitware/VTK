/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapJavaScriptEnum.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkWrapJavaScriptEnum_h
#define vtkWrapJavaScriptEnum_h

#include "vtkParseData.h"
#include "vtkParseHierarchy.h"

#include <stdio.h>

/** check whether an enum type will be wrapped */
int vtkWrapJavaScript_IsEnumWrapped(HierarchyInfo* hinfo, const char* enumname);

/** find and mark all enum parameters by setting IsEnum=1 */
void vtkWrapJavaScript_MarkAllEnums(NamespaceInfo* contents, HierarchyInfo* hinfo);

/** write out the enum bindings */
void vtkWrapJavaScript_GenerateEnumTypes(
  FILE* fp, const char* modulename, const char* classname, const char* indent, NamespaceInfo* data);

#endif /* vtkWrapJavaScriptEnum_h */
// VTK-HeaderTest-Exclude: vtkWrapJavaScriptEnum.h
