/*=========================================================================

  Program:   Visualization Library
  Module:    Cylinder.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Cylinder.hh"

// Description
// Construct cylinder radius of 0.5.
vlCylinder::vlCylinder()
{
  this->Radius = 0.5;
}

// Description
// Evaluate cylinder equation F(x,y,z) = (x-x0)^2 + (y-y0)^2 - R^2.
float vlCylinder::EvaluateFunction(float x[3])
{
  return x[0]*x[0] + x[1]*x[1] - this->Radius*this->Radius;
}

// Description
// Evaluate cylinder function gradient.
void vlCylinder::EvaluateGradient(float x[3], float g[3])
{
  g[0] = 2.0 * x[0];
  g[1] = 2.0 * x[1];
  g[2] = 0.0;
}

void vlCylinder::PrintSelf(ostream& os, vlIndent indent)
{
  vlImplicitFunction::PrintSelf(os,indent);

  os << indent << "Radius: " << this->Radius << "\n";
}
