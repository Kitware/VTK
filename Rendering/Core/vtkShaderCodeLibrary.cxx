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


#include <map>
#include <string>
#include <vtksys/SystemTools.hxx>

class vtkShaderCodeLibrary::vtkInternal
{
public:
  std::map<std::string, std::string> Codes;
  const char* GetShaderCode(const char* name)
    {
    std::map<std::string, std::string>::iterator iter;
    iter = this->Codes.find(name);
    if (iter != this->Codes.end())
      {
      return iter->second.c_str();
      }
    return NULL;
    }
};

vtkShaderCodeLibrary::vtkInternal* vtkShaderCodeLibrary::Internal = 0;;

vtkShaderCodeLibrary::vtkInternalCleanup vtkShaderCodeLibrary::Cleanup;
vtkShaderCodeLibrary::vtkInternalCleanup::~vtkInternalCleanup()
{
  delete vtkShaderCodeLibrary::Internal;
  vtkShaderCodeLibrary::Internal = 0;
}


vtkStandardNewMacro(vtkShaderCodeLibrary);
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

  if (vtkShaderCodeLibrary::Internal)
    {
    const char* code = vtkShaderCodeLibrary::Internal->GetShaderCode(name);
    if (code)
      {
      return vtksys::SystemTools::DuplicateString(code);
      }
    }

  // CMake sets VTK_SHADER_CODE_LIBRARY_CHUNK to be the
  // chunk of code that does name comparisons and
  // call appropriate method from the vtk*ShaderLibrary.
  vtkShaderCodeLibraryMacro(name)
  return 0;
}

//-----------------------------------------------------------------------------
const char** vtkShaderCodeLibrary::GetListOfShaderCodeNames()
{
  return ::ListOfShaderNames;
}

//-----------------------------------------------------------------------------
void vtkShaderCodeLibrary::RegisterShaderCode(const char* name, const char* code)
{
  if (name && code)
    {
    if (!vtkShaderCodeLibrary::Internal)
      {
      vtkShaderCodeLibrary::Internal = new vtkShaderCodeLibrary::vtkInternal();
      }
    vtkShaderCodeLibrary::Internal->Codes[name] = code;
    }
}

//-----------------------------------------------------------------------------
void vtkShaderCodeLibrary::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
