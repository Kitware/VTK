/*=========================================================================

  Program:   Visualization Library
  Module:    Cone.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Cone.hh"
#include "vlMath.hh"

// Description
// Construct cone with angle of 45 degrees.
vlCone::vlCone()
{
  this->Angle = 45.0;
}

// Description
// Evaluate cone equation.
float vlCone::EvaluateFunction(float x[3])
{
  static vlMath math;
  float tanTheta = (float) tan((double)this->Angle*math.DegreesToRadians());
  return x[0]*x[0] + x[1]*x[1] - x[2]*tanTheta;
}

// Description
// Evaluate cone normal.
void vlCone::EvaluateGradient(float x[3], float g[3])
{
  static vlMath math;
  float tanTheta = (float) tan((double)this->Angle*math.DegreesToRadians());
  g[0] = 2.0*x[0];
  g[1] = 2.0*x[1];
  g[2] = -tanTheta;
}

void vlCone::PrintSelf(ostream& os, vlIndent indent)
{
  vlImplicitFunction::PrintSelf(os,indent);

  os << indent << "Angle: " << this->Angle << "\n";
}
