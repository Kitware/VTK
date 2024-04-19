// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDynamicLoader.h"

#include "vtkDebugLeaks.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkDynamicLoader* vtkDynamicLoader::New()
{
  VTK_STANDARD_NEW_BODY(vtkDynamicLoader);
}

//------------------------------------------------------------------------------
void vtkDynamicLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkLibHandle vtkDynamicLoader::OpenLibrary(const char* libname)
{
  return vtksys::DynamicLoader::OpenLibrary(libname);
}

//------------------------------------------------------------------------------
vtkLibHandle vtkDynamicLoader::OpenLibrary(const char* libname, int flags)
{
  return vtksys::DynamicLoader::OpenLibrary(libname, flags);
}

//------------------------------------------------------------------------------
int vtkDynamicLoader::CloseLibrary(vtkLibHandle lib)
{
  return vtksys::DynamicLoader::CloseLibrary(lib);
}

//------------------------------------------------------------------------------
vtkSymbolPointer vtkDynamicLoader::GetSymbolAddress(vtkLibHandle lib, const char* sym)
{
  return vtksys::DynamicLoader::GetSymbolAddress(lib, sym);
}

//------------------------------------------------------------------------------
const char* vtkDynamicLoader::LibPrefix()
{
  return vtksys::DynamicLoader::LibPrefix();
}

//------------------------------------------------------------------------------
const char* vtkDynamicLoader::LibExtension()
{
  return vtksys::DynamicLoader::LibExtension();
}

//------------------------------------------------------------------------------
const char* vtkDynamicLoader::LastError()
{
  return vtksys::DynamicLoader::LastError();
}
VTK_ABI_NAMESPACE_END
