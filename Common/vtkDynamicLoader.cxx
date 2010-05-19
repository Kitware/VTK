/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDynamicLoader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDynamicLoader.h"

#include "vtkDebugLeaks.h"


//-----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkDynamicLoader);

//-----------------------------------------------------------------------------
vtkDynamicLoader* vtkDynamicLoader::New()
{
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass("vtkDynamicLoader");
#endif
  return new vtkDynamicLoader;
}


// ----------------------------------------------------------------------------
vtkLibHandle vtkDynamicLoader::OpenLibrary(const char* libname )
{
  return vtksys::DynamicLoader::OpenLibrary(libname);
}

// ----------------------------------------------------------------------------
int vtkDynamicLoader::CloseLibrary(vtkLibHandle lib)
{
  return vtksys::DynamicLoader::CloseLibrary(lib);
}

// ----------------------------------------------------------------------------
//vtkSymbolPointer
void*
vtkDynamicLoader::GetSymbolAddress(vtkLibHandle lib, const char* sym)
{
  return (void *)(vtksys::DynamicLoader::GetSymbolAddress(lib, sym));
}

// ----------------------------------------------------------------------------
const char* vtkDynamicLoader::LibPrefix()
{
  return vtksys::DynamicLoader::LibPrefix();
}

// ----------------------------------------------------------------------------
const char* vtkDynamicLoader::LibExtension()
{
  return vtksys::DynamicLoader::LibExtension();
}

// ----------------------------------------------------------------------------
const char* vtkDynamicLoader::LastError()
{
  return vtksys::DynamicLoader::LastError();
}

