/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCone.cxx
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
#include "vtkCone.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkCone, "1.24");
vtkStandardNewMacro(vtkCone);

// Construct cone with angle of 45 degrees.
vtkCone::vtkCone()
{
  this->Angle = 45.0;
}

// Evaluate cone equation.
float vtkCone::EvaluateFunction(float x[3])
{
  float tanTheta = (float) 
    tan((double)this->Angle*vtkMath::DegreesToRadians());
  return x[1]*x[1] + x[2]*x[2] - x[0]*x[0]*tanTheta*tanTheta;
}

// Evaluate cone normal.
void vtkCone::EvaluateGradient(float x[3], float g[3])
{
  float tanTheta = (float) 
    tan((double)this->Angle*vtkMath::DegreesToRadians());
  g[0] = -2.0*x[0]*tanTheta*tanTheta;
  g[1] = 2.0*x[1];
  g[2] = 2.0*x[2];
}

void vtkCone::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Angle: " << this->Angle << "\n";
}
