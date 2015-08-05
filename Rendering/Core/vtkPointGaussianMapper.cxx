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
#include "vtkPiecewiseFunction.h"

//-----------------------------------------------------------------------------
vtkAbstractObjectFactoryNewMacro(vtkPointGaussianMapper)

vtkCxxSetObjectMacro(vtkPointGaussianMapper, ScalarOpacityFunction, vtkPiecewiseFunction);

//-----------------------------------------------------------------------------
vtkPointGaussianMapper::vtkPointGaussianMapper()
{
  this->ScaleArray = 0;
  this->OpacityArray = 0;
  this->SplatShaderCode = 0;
  this->ScalarOpacityFunction = 0;
  this->DefaultRadius = 1.0;
  this->Emissive = 1;
}

//-----------------------------------------------------------------------------
vtkPointGaussianMapper::~vtkPointGaussianMapper()
{
  this->SetScaleArray(0);
  this->SetOpacityArray(0);
  this->SetSplatShaderCode(0);
  this->SetScalarOpacityFunction(0);
}

//-----------------------------------------------------------------------------
void vtkPointGaussianMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Scale Array: " << (this->ScaleArray ? this->ScaleArray : "(none)") << "\n";
  os << indent << "Opacity Array: " << (this->OpacityArray ? this->OpacityArray : "(none)") << "\n";
  os << indent << "SplatShaderCode: " << (this->SplatShaderCode ? this->SplatShaderCode : "(none)") << "\n";
  os << indent << "Default Radius: " << this->DefaultRadius << "\n";
  os << indent << "Emissive: " << this->Emissive << "\n";
}
