/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractElectronicData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAbstractElectronicData.h"

//----------------------------------------------------------------------------
vtkAbstractElectronicData::vtkAbstractElectronicData()
  : Padding(0.0)
{
}

//----------------------------------------------------------------------------
vtkAbstractElectronicData::~vtkAbstractElectronicData()
{
}

//----------------------------------------------------------------------------
void vtkAbstractElectronicData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Padding: " << this->Padding << "\n";
}

//----------------------------------------------------------------------------
void vtkAbstractElectronicData::DeepCopy(vtkDataObject *obj)
{
  vtkAbstractElectronicData *aed =
      vtkAbstractElectronicData::SafeDownCast(obj);
  if (!aed)
  {
    vtkErrorMacro("Can only deep copy from vtkAbstractElectronicData "
                  "or subclass.");
    return;
  }

  // Call superclass
  this->Superclass::DeepCopy(aed);

  // Copy ivars
  this->Padding = aed->Padding;
}
