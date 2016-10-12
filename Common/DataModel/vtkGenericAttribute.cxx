/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericAttribute.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericAttribute - Objects that manage some attribute data.
// .SECTION Description

#include "vtkGenericAttribute.h"
#include <cassert>


//---------------------------------------------------------------------------
vtkGenericAttribute::vtkGenericAttribute()
{
}

//---------------------------------------------------------------------------
vtkGenericAttribute::~vtkGenericAttribute()
{
}

//---------------------------------------------------------------------------
void vtkGenericAttribute::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Name: " << this->GetName() << endl;
  os << indent << "Number of components: " << this->GetNumberOfComponents() << endl;
  os << indent << "Centering: ";

  switch(this->GetCentering())
  {
    case vtkPointCentered:
      os << "on points";
      break;
    case vtkCellCentered:
      os << "on cells";
      break;
    case vtkBoundaryCentered:
      os << "on boundaries";
      break;
    default:
      assert("check: Impossible case" && 0);
      break;
  }
  os << endl;
}
