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
// Construct cylinder with top at (0,0,1), bottom at (0,0,0) and radius=0.5.
vlCylinder::vlCylinder()
{
  this->Radius = 0.5;

  this->Top[0] = 0.0;
  this->Top[1] = 0.0;
  this->Top[2] = 1.0;

  this->Bottom[0] = 0.0;
  this->Bottom[1] = 0.0;
  this->Bottom[2] = 1.0;
}

// Description
// Evaluate cylinder equation R^2 - ((x-x0)^2 + (y-y0)^2 + (z-z0)^2) = 0.
float vlCylinder::Evaluate(float x, float y, float z)
{
  return 0;
}

// Description
// Evaluate cylinder function gradient.
void vlCylinder::EvaluateGradient(float x, float y, float z, float g[3])
{
}

void vlCylinder::PrintSelf(ostream& os, vlIndent indent)
{
  vlImplicitFunction::PrintSelf(os,indent);

  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Top: (" << this->Top[0] << ", " 
    << this->Top[1] << ", " << this->Top[2] << ")\n";
  os << indent << "Bottom: (" << this->Bottom[0] << ", " 
    << this->Bottom[1] << ", " << this->Bottom[2] << ")\n";
}
