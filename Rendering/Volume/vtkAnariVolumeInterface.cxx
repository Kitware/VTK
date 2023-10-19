/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAnariVolumeInterface.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
