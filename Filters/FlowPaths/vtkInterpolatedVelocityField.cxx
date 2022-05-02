/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInterpolatedVelocityField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// VTK_DEPRECATED_IN_9_2_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkInterpolatedVelocityField.h"

#include "vtkClosestPointStrategy.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkInterpolatedVelocityField);

//------------------------------------------------------------------------------
vtkInterpolatedVelocityField::vtkInterpolatedVelocityField()
{
  // Create the default FindCellStrategy. Note that it is deleted by the
  // superclass.
  this->FindCellStrategy = vtkClosestPointStrategy::New();
}

//------------------------------------------------------------------------------
void vtkInterpolatedVelocityField::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
