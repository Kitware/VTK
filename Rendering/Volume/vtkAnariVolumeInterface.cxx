// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAnariVolumeInterface.h"

#include "vtkLogger.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN

vtkObjectFactoryNewMacro(vtkAnariVolumeInterface);

// ----------------------------------------------------------------------------
void vtkAnariVolumeInterface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
void vtkAnariVolumeInterface::Render(vtkRenderer* vtkNotUsed(ren), vtkVolume* vtkNotUsed(vol))
{
  vtkLog(WARNING, "Warning VTK is not linked to ANARI so can not volume render with it");
}

VTK_ABI_NAMESPACE_END
