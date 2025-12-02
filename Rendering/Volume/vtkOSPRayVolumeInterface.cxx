// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOSPRayVolumeInterface.h"

#include "vtkObjectFactory.h"

#include <iostream>

VTK_ABI_NAMESPACE_BEGIN
vtkObjectFactoryNewMacro(vtkOSPRayVolumeInterface);

//------------------------------------------------------------------------------
vtkOSPRayVolumeInterface::vtkOSPRayVolumeInterface() = default;

//------------------------------------------------------------------------------
vtkOSPRayVolumeInterface::~vtkOSPRayVolumeInterface() = default;

//------------------------------------------------------------------------------
void vtkOSPRayVolumeInterface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkOSPRayVolumeInterface::Render(vtkRenderer* vtkNotUsed(ren), vtkVolume* vtkNotUsed(vol))
{
  std::cerr << "Warning VTK is not linked to OSPRay so can not VolumeRender with it\n";
}
VTK_ABI_NAMESPACE_END
