// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

VTK_ABI_NAMESPACE_END

extern "C"
{
  const char* VTK_ABI_NAMESPACE_MANGLE(GetVTKVersion)() { return vtkVersion::GetVTKVersion(); }
}
