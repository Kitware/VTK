/*=========================================================================

  Program:   Visualization Library
  Module:    TriStrip.cc
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
#include "TriStrip.hh"
#include "Triangle.hh"

//
// Note: the ordering of the Points and PointIds is important.  See text.
//

float vlTriangleStrip::EvaluatePosition(float x[3], int& subId, float pcoords[3])
{
  vlTriangle tri;
  vlFloatPoints pts(3);
  float pc[3], dist2, minDist2;
  int ignoreId, i;

  pcoords[2] = 0.0;

  for (minDist2=LARGE_FLOAT,i=0; i<this->Points.NumberOfPoints()-2; i++)
    {
    tri.Points.SetPoint(0,this->Points.GetPoint(i));
    tri.Points.SetPoint(1,this->Points.GetPoint(i+1));
    tri.Points.SetPoint(2,this->Points.GetPoint(i+2));
    dist2 = tri.EvaluatePosition(x, ignoreId, pc);
    if ( dist2 < minDist2 )
      {
      subId = i;
      pcoords[0] = pc[0];
      pcoords[1] = pc[1];
      minDist2 = dist2;
      }
    }

  return minDist2;
}

void vlTriangleStrip::EvaluateLocation(int& subId, float pcoords[3], float x[3])
{
  int i;
  float *pt1 = this->Points.GetPoint(subId);
  float *pt2 = this->Points.GetPoint(subId+1);
  float *pt3 = this->Points.GetPoint(subId+2);
  float u3 = 1.0 - pcoords[0] - pcoords[1];

  for (i=0; i<3; i++)
    {
    x[i] = pt1[i]*pcoords[0] + pt2[i]*pcoords[1] + pt3[i]*u3;
    }
}

