/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphere.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSphere.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkSphere, "1.24");
vtkStandardNewMacro(vtkSphere);

// Construct sphere with center at (0,0,0) and radius=0.5.
vtkSphere::vtkSphere()
{
  this->Radius = 0.5;

  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;
}

// Evaluate sphere equation ((x-x0)^2 + (y-y0)^2 + (z-z0)^2) - R^2.
float vtkSphere::EvaluateFunction(float x[3])
{
  return ( ((x[0] - this->Center[0]) * (x[0] - this->Center[0]) + 
           (x[1] - this->Center[1]) * (x[1] - this->Center[1]) + 
           (x[2] - this->Center[2]) * (x[2] - this->Center[2])) - 
           this->Radius*this->Radius );
}

// Evaluate sphere gradient.
void vtkSphere::EvaluateGradient(float x[3], float n[3])
{
  n[0] = 2.0 * (x[0] - this->Center[0]);
  n[1] = 2.0 * (x[1] - this->Center[1]);
  n[2] = 2.0 * (x[2] - this->Center[2]);
}

void vtkSphere::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Center: (" << this->Center[0] << ", " 
     << this->Center[1] << ", " << this->Center[2] << ")\n";
}
