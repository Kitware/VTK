/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointData.h"

#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPointData);
vtkStandardExtendedNewMacro(vtkPointData);

//------------------------------------------------------------------------------
vtkPointData::vtkPointData()
{
  this->GhostsToSkip = vtkDataSetAttributes::HIDDENPOINT;
}

//------------------------------------------------------------------------------
void vtkPointData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
