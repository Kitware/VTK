/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCylinder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCylinder.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkCylinder);

// Construct cylinder radius of 0.5.
vtkCylinder::vtkCylinder()
{
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
  this->Radius = 0.5;
}

// Evaluate cylinder equation F(x,y,z) = (x-x0)^2 + (z-z0)^2 - R^2.
double vtkCylinder::EvaluateFunction(double xyz[3])
{
  double x = xyz[0] - this->Center[0];
  double z = xyz[2] - this->Center[2];

  return ( x * x + z * z - this->Radius*this->Radius );
}

// Evaluate cylinder function gradient.
void vtkCylinder::EvaluateGradient(double xyz[3], double g[3])
{
  double x = xyz[0] - this->Center[0];
  double z = xyz[2] - this->Center[2];

  g[0] = 2.0 * (x - this->Center[0]);
  g[1] = 0.0;
  g[2] = 2.0 * (z - this->Center[2]);
}

void vtkCylinder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Center: " << "( " << this->Center[0] << ", " <<
     this->Center[1] << ", " << this->Center[2] << " )";

  os << indent << "Radius: " << this->Radius << "\n";
}
