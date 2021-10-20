/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenXRControlsHelper.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenXRControlsHelper.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkOpenXRControlsHelper);

//------------------------------------------------------------------------------
void vtkOpenXRControlsHelper::InitControlPosition()
{
  // Set this->ControlPositionLC to position tooltip on component.
  //     https://gitlab.kitware.com/vtk/vtk/-/issues/18332
  // See implementation in vtkOpenVRControlsHelper for example/starting point
}

//------------------------------------------------------------------------------
void vtkOpenXRControlsHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
