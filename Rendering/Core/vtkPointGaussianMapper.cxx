/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointGaussianMapper.h"

#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkAbstractObjectFactoryNewMacro(vtkPointGaussianMapper)

//-----------------------------------------------------------------------------
vtkPointGaussianMapper::vtkPointGaussianMapper()
{
  this->ScaleArray = 0;
}

//-----------------------------------------------------------------------------
vtkPointGaussianMapper::~vtkPointGaussianMapper()
{
  this->SetScaleArray(0);
}

//-----------------------------------------------------------------------------
void vtkPointGaussianMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Scale Array: " << (this->ScaleArray ? this->ScaleArray : "(none)") << "\n";
  os << indent << "Default Radius: " << this->DefaultRadius << "\n";
}
