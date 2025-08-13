/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapJavaScriptNamespace.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkWrapJavaScriptNamespace.h"
#include "vtkWrapJavaScriptConstant.h"
#include "vtkWrapJavaScriptEnum.h"

// NOLINTBEGIN(bugprone-unsafe-functions)

/* -------------------------------------------------------------------- */
/* Wrap the namespace */
int vtkWrapJavaScript_Namespace(FILE* fp, const char* module, NamespaceInfo* data)
{
  vtkWrapJavaScript_GenerateEnumTypes(fp, module, data->Name, "  ", data);
  vtkWrapJavaScript_GenerateConstants(fp, module, NULL, "  ", data);
  return 1;
}

// NOLINTEND(bugprone-unsafe-functions)
