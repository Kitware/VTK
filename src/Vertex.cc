/*=========================================================================

  Program:   Visualization Library
  Module:    Vertex.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Vertex.hh"
#include "vlMath.hh"
#include "CellArr.hh"

static vlMath math;

// Description:
// Deep copy of cell.
vlVertex::vlVertex(const vlVertex& p)
{
  this->Points = p.Points;
  this->PointIds = p.PointIds;
}

int vlVertex::EvaluatePosition(float x[3], float closestPoint[3],
                              int& subId, float pcoords[3], 
                              float& dist2, float weights[MAX_CELL_SIZE])
{
  float *X;

  subId = 0;
  pcoords[1] = pcoords[2] = 0.0;

  X = this->Points.GetPoint(0);
  closestPoint[0] = X[0]; closestPoint[1] = X[1]; closestPoint[2] = X[2];

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

void vlVertex::EvaluateLocation(int& subId, float pcoords[3], float x[3],
                               float weights[MAX_CELL_SIZE])
{
  float *X = this->Points.GetPoint(0);
  x[0] = X[0];
  x[1] = X[1];
  x[2] = X[2];

  weights[0] = 1.0;
}

int vlVertex::CellBoundary(int subId, float pcoords[3], vlIdList& pts)
{

  pts.Reset();
  pts.SetId(0,this->PointIds.GetId(0));

  if ( pcoords[0] != 0.0 )  
    return 0;
  else
    return 1;

}

void vlVertex::Contour(float value, vlFloatScalars *cellScalars, 
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

// Project point on line. If it lies between 0<=t<=1 and distance off line
// is less than tolerance, intersection detected.
int vlVertex::IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                                float x[3], float pcoords[3], int& subId)
{
  int i;
  float *X, ray[3], rayFactor, projXYZ[3];

  subId = 0;
  pcoords[1] = pcoords[2] = 0.0;

  X = this->Points.GetPoint(0);

  for (i=0; i<3; i++) ray[i] = p2[i] - p1[i];
  if (( rayFactor = math.Dot(ray,ray)) == 0.0 ) return 0;
//
//  Project each point onto ray. Determine whether point is within tolerance.
//
  t = (ray[0]*(X[0]-p1[0]) + ray[1]*(X[1]-p1[1]) + ray[2]*(X[2]-p1[2]))
      / rayFactor;

  if ( t >= 0.0 && t <= 1.0 )
    {
    for (i=0; i<3; i++) 
      {
      projXYZ[i] = p1[i] + t*ray[i];
      if ( fabs(X[i]-projXYZ[i]) > tol ) break;
      }

    if ( i > 2 ) // within tolerance 
      {
      pcoords[0] = 0.0;
      x[0] = X[0]; x[1] = X[1]; x[2] = X[2]; 
      return 1;
      }
    }

  pcoords[0] = -10.0;
  return 0;
}
