/*=========================================================================

  Program:   Visualization Library
  Module:    Plane.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Plane.hh"
#include "vlMath.hh"

// Description
// Construct plane passing through origin and normal to z-axis.
vlPlane::vlPlane()
{
  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;

  this->Origin[0] = 0.0;
  this->Origin[1] = 0.0;
  this->Origin[2] = 0.0;
}

// Description
// Project a point x onto plane defined by origin and normal. The 
// projected point is returned in xproj. NOTE : normal assumed to
// have magnitude 1.
void vlPlane::ProjectPoint(float x[3], float origin[3], float normal[3], float xproj[3])
{
  int i;
  vlMath math;
  float t, xo[3];

  for (i=0; i<3; i++) xo[i] = x[i] - origin[i];
  t = math.Dot(normal,xo);
  for (i=0; i<3; i++) xproj[i] = x[i] - t * normal[i];
}

// Description
// Evaluate plane equation for point (x,y,z).
float vlPlane::Evaluate(float x, float y, float z)
{
  return ( this->Normal[0]*(x-this->Origin[0]) + 
           this->Normal[1]*(y-this->Origin[1]) + 
           this->Normal[2]*(z-this->Origin[2]) );
}

// Description
// Evaluate plane normal at point (x,y,z).
void vlPlane::EvaluateNormal(float x, float y, float z, float n[3])
{
  for (int i=0; i<3; i++) n[i] = this->Normal[i];
}

