// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkGPUInfo.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkGPUInfo);

//------------------------------------------------------------------------------
vtkGPUInfo::vtkGPUInfo()
{
  this->DedicatedVideoMemory = 0;
  this->DedicatedSystemMemory = 0;
  this->SharedSystemMemory = 0;
}

//------------------------------------------------------------------------------
vtkGPUInfo::~vtkGPUInfo() = default;

//------------------------------------------------------------------------------
void vtkGPUInfo::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Dedicated Video Memory in bytes: " << this->DedicatedVideoMemory << endl;
  os << indent << "Dedicated System Memory in bytes: " << this->DedicatedSystemMemory << endl;
  os << indent << "Shared System Memory in bytes: " << this->SharedSystemMemory << endl;
}
VTK_ABI_NAMESPACE_END
