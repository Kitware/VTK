/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVersion.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVersion.h"
#include "vtkObjectFactory.h"
#include "vtkVersionFull.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkVersion);

void vtkVersion::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

const char* vtkVersion::GetVTKVersionFull()
{
  // Since VTK_VERSION_FULL changes with every commit, it is kept out of the
  // header file to avoid excessive rebuilds.
  return VTK_VERSION_FULL;
}

const char* GetVTKVersion()
{
  return vtkVersion::GetVTKVersion();
}
VTK_ABI_NAMESPACE_END
