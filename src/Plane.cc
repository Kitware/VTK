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

static vlMath math;

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
  float t, xo[3];

  for (i=0; i<3; i++) xo[i] = x[i] - origin[i];
  t = math.Dot(normal,xo);
  for (i=0; i<3; i++) xproj[i] = x[i] - t * normal[i];
}

// Description
// Evaluate plane equation for point x[3].
float vlPlane::EvaluateFunction(float x[3])
{
  return ( this->Normal[0]*(x[0]-this->Origin[0]) + 
           this->Normal[1]*(x[1]-this->Origin[1]) + 
           this->Normal[2]*(x[2]-this->Origin[2]) );
}

// Description
// Evaluate function gradient at point x[3].
void vlPlane::EvaluateGradient(float x[3], float n[3])
{
  for (int i=0; i<3; i++) n[i] = this->Normal[i];
}

// Description:
// Given a line defined by the two points p1,p2; and a plane defined by the
// normal n and point p0, compute an intersection. The parametric
// coordinate along the line is returned in t, and the coordinates of 
// intersection are returned in x. A 0 is returned is the plane and line
// are parallel.
//
#define TOL 1.0e-06

int vlPlane::IntersectWithLine(float p1[3], float p2[3], float n[3], 
                               float p0[3], float& t, float x[3])
{
  float num, den, p21[3];
  int i;
//
// Compute line vector
// 
  for (i=0; i<3; i++) p21[i] = p2[i] - p1[i];
//
// Compute denominator.  If ~0, line and plane are parallel.
// 
  num = math.Dot(n,p0) - ( n[0]*p1[0] + n[1]*p1[1] + n[2]*p1[2] ) ;
  den = n[0]*p21[0] + n[1]*p21[1] + n[2]*p21[2];
//
// If denominator with respect to numerator is "zero", then the line and
// plane are considered parallel. 
//
  if ( fabs(den) <= fabs(TOL*num)) return 0;

  t = num / den;
  for (i=0; i<3; i++) x[i] = p1[i] + t*p21[i];

  if ( t >= 0.0 && t <= 1.0 ) return 1;
  else return 0;
}

void vlPlane::PrintSelf(ostream& os, vlIndent indent)
{
  vlImplicitFunction::PrintSelf(os,indent);

  os << indent << "Normal: (" << this->Normal[0] << ", " 
    << this->Normal[1] << ", " << this->Normal[2] << ")\n";

  os << indent << "Origin: (" << this->Origin[0] << ", " 
    << this->Origin[1] << ", " << this->Origin[2] << ")\n";
}
