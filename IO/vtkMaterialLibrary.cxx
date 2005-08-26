/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMaterialLibrary.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMaterialLibrary.h"

#include "vtkObjectFactory.h"
#include "vtkMaterialLibraryMacro.h"

#ifndef vtkMaterialLibraryMacro
  #define vtkMaterialLibraryMacro(name) \
    vtkGenericWarningMacro("VTK is not built with shading support." \
      "No materials are available.");
#endif

vtkStandardNewMacro(vtkMaterialLibrary);
vtkCxxRevisionMacro(vtkMaterialLibrary, "1.1.2.1");
//-----------------------------------------------------------------------------
vtkMaterialLibrary::vtkMaterialLibrary()
{
}

//-----------------------------------------------------------------------------
vtkMaterialLibrary::~vtkMaterialLibrary()
{
}

//-----------------------------------------------------------------------------
char* vtkMaterialLibrary::GetMaterial(const char* name)
{
  if (!name || !*name)
    {
    return 0;
    }

  // CMake sets VTK_SHADER_CODE_LIBRARY_CHUNK to be the
  // chunk of code that does name comparisons and
  // call appropriate method from the vtk*ShaderLibrary.
  vtkMaterialLibraryMacro(name)
  return 0;
}

//-----------------------------------------------------------------------------
void vtkMaterialLibrary::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
