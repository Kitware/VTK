/*=========================================================================

  Program:   Visualization Library
  Module:    Plane.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Plane.hh"
#include "vlMath.hh"

vlPlane::vlPlane()
{
  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;

  this->Origin[0] = 0.0;
  this->Origin[1] = 0.0;
  this->Origin[2] = 0.0;
}

int vlPlane::ProjectPoint(float x[3], float origin[3], float normal[3], float xproj[3])
{
  int i;
  vlMath math;
  float t, xo[3];

  for (i=0; i<3; i++) xo[i] = x[i] - origin[i];
  t = math.Dot(normal,xo);
  for (i=0; i<3; i++) xproj[i] = x[i] - t * normal[i];
}

float vlPlane::Evaluate(float x, float y, float z)
{
  return ( this->Normal[0]*(x-this->Origin[0]) + 
           this->Normal[1]*(y-this->Origin[1]) + 
           this->Normal[2]*(z-this->Origin[2]) );
}
