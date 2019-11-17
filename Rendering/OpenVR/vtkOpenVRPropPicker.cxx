/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRPropPicker.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenVRPropPicker.h"

#include "vtkAssemblyNode.h"
#include "vtkAssemblyPath.h"
#include "vtkBox.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor3D.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkOpenVRPropPicker);

vtkOpenVRPropPicker::vtkOpenVRPropPicker() {}

vtkOpenVRPropPicker::~vtkOpenVRPropPicker() {}

// set up for a pick
void vtkOpenVRPropPicker::Initialize()
{
#ifndef VTK_LEGACY_SILENT
  vtkErrorMacro(
    "This class is deprecated: Please use vtkPropPicker directly instead of this class");
#endif
  this->vtkAbstractPropPicker::Initialize();
}

void vtkOpenVRPropPicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
