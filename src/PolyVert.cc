/*=========================================================================

  Program:   Visualization Library
  Module:    PolyVert.cc
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
#include "PolyPts.hh"
#include "vlMath.hh"

float vlPolyPoints::EvaluatePosition(float x[3], int& subId, float pcoords[3])
{
  int numPts=this->Points.GetNumberOfPoints();
  float *X;
  float dist2, minDist2;
  int i;
  vlMath math;

  for (minDist2=LARGE_FLOAT, i=0; i<numPts; i++)
    {
    X = this->Points.GetPoint(i);
    dist2 = math.Distance2BetweenPoints(X,x);
    if (dist2 < minDist2)
      {
      minDist2 = dist2;
      subId = i;
      }
    }

  if (minDist2 == 0.0)
    {
    pcoords[0] = 0.0;
    }
  else
    {
    pcoords[0] = -10.0;
    }

  return minDist2;
}

void vlPolyPoints::EvaluateLocation(int& subId, float pcoords[3], float x[3])
{
  float *X = this->Points.GetPoint(subId);
  x[0] = X[0];
  x[1] = X[1];
  x[2] = X[2];
}

void vlPolyPoints::Contour(float value, vlFloatScalars *cellScalars, 
                           vlFloatPoints *points, vlCellArray *verts,
                           vlCellArray *lines, vlCellArray *polys, 
                           vlFloatScalars *scalars)
{
  int i, pts[1];

  for (i=0; i<this->Points.GetNumberOfPoints(); i++)
    {
    if ( value == cellScalars->GetScalar(i) )
      {
      scalars->InsertNextScalar(value);
      pts[0] = points->InsertNextPoint(this->Points.GetPoint(0));
      verts->InsertNextCell(1,pts);
      }
    }
}

