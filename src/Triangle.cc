/*=========================================================================

  Program:   Visualization Library
  Module:    Triangle.cc
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
#include "Triangle.hh"
#include "Polygon.hh"
#include "Plane.hh"
#include "vlMath.hh"

float vlTriangle::EvaluatePosition(float x[3], int& subId, float pcoords[3])
{
  int i, j;
  vlPolygon poly;
  float *pt1, *pt2, *pt3, n[3], xProj[3];
  float rhs[2], c1[2], c2[2];
  float closestPoint[3], det;
  vlPlane plane;
  vlMath math;
  float maxComponent;
  int idx, indices[2];

  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;
//
// Get normal for triangle
//
  pt1 = this->Points.GetPoint(0);
  pt2 = this->Points.GetPoint(1);
  pt3 = this->Points.GetPoint(2);

  poly.ComputeNormal (pt1, pt2, pt3, n);
//
// Project point to plane
//
  plane.ProjectPoint(x,pt1,n,xProj);
//
// Construct matrices.  Since we have over determined system, need to find
// which 2 out of 3 equations to use to develop equations. (Any 2 should 
// work since we've projected point to plane.)
//
  for (maxComponent=0.0, i=0; i<3; i++)
    {
    if (n[i] > maxComponent)
      {
      maxComponent = n[i];
      idx = i;
      }
    }
  for (j=0, i=0; i<3; i++)  
    {
    if ( i != idx ) indices[j++] = i;
    }
  
  for (i=0; i<2; i++)
    {  
    rhs[i] = x[indices[i]] - pt3[indices[i]];
    c1[i] = pt1[indices[i]] - pt3[indices[i]];
    c2[i] = pt2[indices[i]] - pt3[indices[i]];
    }

  if ( (det = math.Determinate2x2(c1,c2)) == 0.0 )
    return LARGE_FLOAT;

  pcoords[0] = math.Determinate2x2 (rhs,c2) / det;
  pcoords[1] = math.Determinate2x2 (c1,rhs) / det;
//
// Okay, now find closest point to element
//
  if ( pcoords[0] >= 0.0 && pcoords[1] <= 1.0 &&
  pcoords[1] >= 0.0 && pcoords[1] <= 1.0 )
    {
    return math.Distance2BetweenPoints(xProj,x); //projection distance
    }
  else
    {
    for (i=0; i<2; i++)
      {
      if (pcoords[i] < 0.0) pcoords[i] = 0.0;
      if (pcoords[i] > 1.0) pcoords[i] = 1.0;
      }
    this->EvaluateLocation(subId, pcoords, closestPoint);
    return math.Distance2BetweenPoints(closestPoint,x);
    }
}

void vlTriangle::EvaluateLocation(int& subId, float pcoords[3], float x[3])
{
  float u3;
  float *pt1, *pt2, *pt3;
  int i;

  pt1 = this->Points.GetPoint(0);
  pt2 = this->Points.GetPoint(1);
  pt3 = this->Points.GetPoint(2);

  u3 = 1.0 - pcoords[0] - pcoords[1];

  for (i=0; i<3; i++)
    {
    x[i] = pt1[i]*pcoords[0] + pt2[i]*pcoords[1] + pt3[i]*u3;
    }
}

