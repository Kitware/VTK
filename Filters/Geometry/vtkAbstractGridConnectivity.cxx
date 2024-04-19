// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAbstractGridConnectivity.h"

VTK_ABI_NAMESPACE_BEGIN
vtkAbstractGridConnectivity::vtkAbstractGridConnectivity()
{
  this->NumberOfGrids = 0;
  this->NumberOfGhostLayers = 0;
  this->AllocatedGhostDataStructures = false;
}

//------------------------------------------------------------------------------
vtkAbstractGridConnectivity::~vtkAbstractGridConnectivity()
{
  this->DeAllocateUserRegisterDataStructures();
  this->DeAllocateInternalDataStructures();
}

//------------------------------------------------------------------------------
void vtkAbstractGridConnectivity::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "NumberOfGrids: " << this->NumberOfGrids << std::endl;
  os << "NumberOfGhostLayers: " << this->NumberOfGhostLayers << std::endl;
}
VTK_ABI_NAMESPACE_END
