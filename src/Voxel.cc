/*=========================================================================

  Program:   Visualization Library
  Module:    Voxel.cc
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
#include <math.h>
#include "Brick.hh"
#include "vlMath.hh"

//
// Note: the ordering of the Points and PointIds is important.  See text.
//

float vlBrick::EvaluatePosition(float x[3], int& subId, float pcoords[3])
{
  float *pt1, *pt2, *pt3, *pt4;
  int i;
  float closestPoint[3];
  float l21, l31, l41;
  vlMath math;  

  subId = 0;
//
// Get coordinate system
//
  pt1 = this->Points.GetPoint(0);
  pt2 = this->Points.GetPoint(1);
  pt3 = this->Points.GetPoint(2);
  pt4 = this->Points.GetPoint(4);
//
// Develop parametric coordinates
//
  pcoords[0] = (x[0] - pt1[0]) / (pt2[0] - pt1[0]);
  pcoords[1] = (x[1] - pt1[1]) / (pt3[1] - pt1[1]);
  pcoords[2] = (x[2] - pt1[2]) / (pt4[2] - pt1[2]);

  if ( pcoords[0] >= -1.0 && pcoords[1] <= 1.0 &&
  pcoords[1] >= -1.0 && pcoords[1] <= 1.0 &&
  pcoords[2] >= -1.0 && pcoords[2] <= 1.0 )
    {
    return 0.0; // inside brick
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

void vlBrick::EvaluateLocation(int& subId, float pcoords[3], float x[3])
{
  float *pt1, *pt2, *pt3, *pt4;
  int i;

  pt1 = this->Points.GetPoint(0);
  pt2 = this->Points.GetPoint(1);
  pt3 = this->Points.GetPoint(2);
  pt4 = this->Points.GetPoint(4);

  for (i=0; i<3; i++)
    {
    x[i] = pt1[i] + pcoords[0]*(pt2[i] - pt1[i]) +
                    pcoords[1]*(pt3[i] - pt1[i]) +
                    pcoords[2]*(pt4[i] - pt1[i]);
    }

}
