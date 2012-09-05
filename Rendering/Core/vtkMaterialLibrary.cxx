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
const char** vtkMaterialLibrary::GetListOfMaterialNames()
{
  // defined in vtkMaterialLibraryMacro.h
  return ::ListOfMaterialNames;
}

//-----------------------------------------------------------------------------
unsigned int vtkMaterialLibrary::GetNumberOfMaterials()
{
  const char** names = vtkMaterialLibrary::GetListOfMaterialNames();
  unsigned int cc ;
  for (cc=0; names[cc]; cc++)
    {
    }
  return cc;
}

//-----------------------------------------------------------------------------
void vtkMaterialLibrary::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
