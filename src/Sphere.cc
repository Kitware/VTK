/*=========================================================================

  Program:   Visualization Library
  Module:    Sphere.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Sphere.hh"

// Description
// Construct sphere with center at (0,0,0) and radius=0.5.
vlSphere::vlSphere()
{
  this->Radius = 0.5;

  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;
}

// Description
// Evaluate sphere equation ((x-x0)^2 + (y-y0)^2 + (z-z0)^2) - R^2.
float vlSphere::Evaluate(float x, float y, float z)
{
  return ( ((x - this->Center[0]) * (x - this->Center[0]) + 
           (y - this->Center[1]) * (y - this->Center[1]) + 
           (z - this->Center[2]) * (z - this->Center[2])) - 
           this->Radius*this->Radius );
}

// Description
// Evaluate sphere normal.
void vlSphere::EvaluateNormal(float x, float y, float z, float n[3])
{
  n[0] = 2.0 * (x - this->Center[0]);
  n[1] = 2.0 * (y - this->Center[1]);
  n[2] = 2.0 * (z - this->Center[2]);
}

void vlSphere::PrintSelf(ostream& os, vlIndent indent)
{
  vlImplicitFunction::PrintSelf(os,indent);

  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Center: (" << this->Center[0] << ", " 
    << this->Center[1] << ", " << this->Center[2] << ")\n";
}
