/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShaderCodeLibrary.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkShaderCodeLibrary.h"

#include "vtkObjectFactory.h"
#include "vtkShaderCodeLibraryMacro.h"

#ifndef vtkShaderCodeLibraryMacro
  #define vtkShaderCodeLibraryMacro(name) \
    vtkGenericWarningMacro("VTK is not built with shading support." \
      "No shaders are available.");
#endif

vtkStandardNewMacro(vtkShaderCodeLibrary);
vtkCxxRevisionMacro(vtkShaderCodeLibrary, "1.2");
//-----------------------------------------------------------------------------
vtkShaderCodeLibrary::vtkShaderCodeLibrary()
{
}

//-----------------------------------------------------------------------------
vtkShaderCodeLibrary::~vtkShaderCodeLibrary()
{
}

//-----------------------------------------------------------------------------
char* vtkShaderCodeLibrary::GetShaderCode(const char* name)
{
  if (!name || !*name)
    {
    return 0;
    }

  // CMake sets VTK_SHADER_CODE_LIBRARY_CHUNK to be the
  // chunk of code that does name comparisons and
  // call appropriate method from the vtk*ShaderLibrary.
  vtkShaderCodeLibraryMacro(name)
  return 0;
}

//-----------------------------------------------------------------------------
void vtkShaderCodeLibrary::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
