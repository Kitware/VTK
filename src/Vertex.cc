/*=========================================================================

  Program:   Visualization Library
  Module:    Vertex.cc
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
#include "Point.hh"
#include "vlMath.hh"
#include "CellArr.hh"

int vlPoint::EvaluatePosition(float x[3], int& subId, float pcoords[3], 
                              float& dist2, float weights[MAX_CELL_SIZE])
{
  int numPts;
  float *X;
  vlMath math;

  subId = 0;
  pcoords[1] = pcoords[2] = 0.0;

  X = this->Points.GetPoint(0);

  dist2 = math.Distance2BetweenPoints(X,x);
  weights[0] = 1.0;

  if (dist2 == 0.0)
    {
    pcoords[0] = 0.0;
    return 1;
    }
  else
    {
    pcoords[0] = -10.0;
    return 0;
    }
}

void vlPoint::EvaluateLocation(int& subId, float pcoords[3], float x[3],
                               float weights[MAX_CELL_SIZE])
{
  float *X = this->Points.GetPoint(0);
  x[0] = X[0];
  x[1] = X[1];
  x[2] = X[2];

  weights[0] = 1.0;
}

void vlPoint::Contour(float value, vlFloatScalars *cellScalars, 
                      vlFloatPoints *points,                      
                      vlCellArray *verts, vlCellArray *lines, 
                      vlCellArray *polys, vlFloatScalars *scalars)
{
  if ( value == cellScalars->GetScalar(0) )
    {
    int pts[1];
    scalars->InsertNextScalar(value);
    pts[0] = points->InsertNextPoint(this->Points.GetPoint(0));
    verts->InsertNextCell(1,pts);
    }
}
