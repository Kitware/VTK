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
// Construct cylinder with height of 1.0 and radius of 0.5.
vlCylinder::vlCylinder()
{
  this->Height = 1.0;
  this->Radius = 0.5;
}

// Description
// Evaluate cylinder equation R^2 - ((x-x0)^2 + (y-y0)^2 + (z-z0)^2) = 0.
float vlCylinder::EvaluateFunction(float x[3])
{
  return 0;
}

// Description
// Evaluate cylinder function gradient.
void vlCylinder::EvaluateGradient(float x[3], float g[3])
{
}

void vlCylinder::PrintSelf(ostream& os, vlIndent indent)
{
  vlImplicitFunction::PrintSelf(os,indent);

  os << indent << "Height: " << this->Height << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
}
