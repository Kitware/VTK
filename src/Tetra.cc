/*=========================================================================

  Program:   Visualization Library
  Module:    Tetra.cc
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
#include "Tetra.hh"
#include "vlMath.hh"

float vlTetra::EvaluatePosition(float x[3], int& subId, float pcoords[3])
{
  float *pt1, *pt2, *pt3, *pt4;
  int i;
  float rhs[3], c1[3], c2[3], c3[3];
  float closestPoint[3], det;
  vlMath math;

  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;

  pt1 = this->Points->GetPoint(0);
  pt2 = this->Points->GetPoint(1);
  pt3 = this->Points->GetPoint(2);
  pt4 = this->Points->GetPoint(3);

  for (i=0; i<3; i++)
    {  
    rhs[i] = x[i] - pt4[i];
    c1[i] = pt1[i] - pt4[i];
    c2[i] = pt2[i] - pt4[i];
    c3[i] = pt3[i] - pt4[i];
    }

  if ( (det = math.Determinate3x3(c1,c2,c3)) == 0.0 )
    return LARGE_FLOAT;

  pcoords[0] = math.Determinate3x3 (rhs,c2,c3) / det;
  pcoords[1] = math.Determinate3x3 (c1,rhs,c3) / det;
  pcoords[2] = math.Determinate3x3 (c1,c2,rhs) / det;

  if ( pcoords[0] >= 0.0 && pcoords[1] <= 1.0 &&
  pcoords[1] >= 0.0 && pcoords[1] <= 1.0 &&
  pcoords[2] >= 0.0 && pcoords[2] <= 1.0 )
    {
    return 0.0; // inside tetra
    }
  else
    {
    for (i=0; i<3; i++)
      {
      if (pcoords[i] < -1.0) pcoords[i] = -1.0;
      if (pcoords[i] > 1.0) pcoords[i] = 1.0;
      }
    this->EvaluateLocation(subId, pcoords, closestPoint);
    return math.Distance2BetweenPoints(closestPoint,x);
    }
}

void vlTetra::EvaluateLocation(int& subId, float pcoords[3], float x[3])
{
  float u4;
  float *pt1, *pt2, *pt3, *pt4;
  int i;

  pt1 = this->Points->GetPoint(0);
  pt2 = this->Points->GetPoint(1);
  pt3 = this->Points->GetPoint(2);
  pt4 = this->Points->GetPoint(3);

  u4 = 1.0 - pcoords[0] - pcoords[1] - pcoords[2];

  for (i=0; i<3; i++)
    {
    x[i] = pt1[i]*pcoords[0] + pt2[i]*pcoords[1] + pt3[i]*pcoords[2] +
           pt4[i]*u4;
    }
}
