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

int vlBrick::EvaluatePosition(float x[3], int& subId, float pcoords[3], float& dist2)
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

  if ( pcoords[0] >= 0.0 && pcoords[1] <= 1.0 &&
  pcoords[1] >= 0.0 && pcoords[1] <= 1.0 &&
  pcoords[2] >= 0.0 && pcoords[2] <= 1.0 )
    {
    dist2 = 0.0; // inside brick
    return 1;
    }
  else
    {
    for (i=0; i<3; i++)
      {
      if (pcoords[i] < 0.0) pcoords[i] = 0.0;
      if (pcoords[i] > 1.0) pcoords[i] = 1.0;
      }
    this->EvaluateLocation(subId, pcoords, closestPoint);
    dist2 = math.Distance2BetweenPoints(closestPoint,x);
    return 0;
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

//
// Marching cubes case table
//
#include "MC_Cases.h"

void vlBrick::Contour(float value, vlFloatScalars *cellScalars, 
                      vlFloatPoints *points,
                      vlCellArray *verts, vlCellArray *lines, 
                      vlCellArray *polys, vlFloatScalars *scalars)
{
  static int CASE_MASK[8] = {1,2,4,8,16,32,64,128};
  TRIANGLE_CASES *triCase;
  EDGE_LIST  *edge;
  int i, j, index, *vert;
  static int vertMap[8] = { 0, 1, 3, 2, 4, 5, 7, 6 };
  static int edges[12][2] = { {0,1}, {1,3}, {3,2}, {2,0},
                              {4,5}, {5,7}, {7,6}, {6,4},
                              {0,4}, {1,5}, {2,6}, {3,7}};
  int pts[3];
  float t, *x1, *x2, x[3];

  // Build the case table
  for ( i=0, index = 0; i < 8; i++)
      if (cellScalars->GetScalar(vertMap[i]) >= value)
          index |= CASE_MASK[i];

  triCase = triCases + index;
  edge = triCase->edges;

  for ( ; edge[0] > -1; edge += 3 )
    {
    for (i=0; i<3; i++) // insert triangle
      {
      vert = edges[edge[i]];
      t = (value - cellScalars->GetScalar(vert[0])) /
          (cellScalars->GetScalar(vert[1]) - cellScalars->GetScalar(vert[0]));
      x1 = this->Points.GetPoint(vert[0]);
      x2 = this->Points.GetPoint(vert[1]);
      for (j=0; j<3; j++) x[j] = x1[j] + t * (x2[j] - x1[j]);
      pts[i] = points->InsertNextPoint(x);
      scalars->InsertNextScalar(value);
      }
    polys->InsertNextCell(3,pts);
    }
}

