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
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkDynamicLoader* vtkDynamicLoader::New()
{
  VTK_STANDARD_NEW_BODY(vtkDynamicLoader);
}

// ----------------------------------------------------------------------------
vtkLibHandle vtkDynamicLoader::OpenLibrary(const char* libname)
{
  return vtksys::DynamicLoader::OpenLibrary(libname);
}

// ----------------------------------------------------------------------------
vtkLibHandle vtkDynamicLoader::OpenLibrary(const char* libname, int flags)
{
  return vtksys::DynamicLoader::OpenLibrary(libname, flags);
}

// ----------------------------------------------------------------------------
int vtkDynamicLoader::CloseLibrary(vtkLibHandle lib)
{
  return vtksys::DynamicLoader::CloseLibrary(lib);
}

// ----------------------------------------------------------------------------
vtkSymbolPointer vtkDynamicLoader::GetSymbolAddress(vtkLibHandle lib, const char* sym)
{
  return vtksys::DynamicLoader::GetSymbolAddress(lib, sym);
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
