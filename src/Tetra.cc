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

int vlTetra::EvaluatePosition(float x[3], int& subId, float pcoords[3], float& dist2)
{
  float *pt1, *pt2, *pt3, *pt4;
  int i;
  float rhs[3], c1[3], c2[3], c3[3];
  float closestPoint[3], det;
  vlMath math;

  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;

  pt1 = this->Points.GetPoint(1);
  pt2 = this->Points.GetPoint(2);
  pt3 = this->Points.GetPoint(3);
  pt4 = this->Points.GetPoint(0);

  for (i=0; i<3; i++)
    {  
    rhs[i] = x[i] - pt4[i];
    c1[i] = pt1[i] - pt4[i];
    c2[i] = pt2[i] - pt4[i];
    c3[i] = pt3[i] - pt4[i];
    }

  if ( (det = math.Determinate3x3(c1,c2,c3)) == 0.0 )
    {
    dist2 = LARGE_FLOAT;
    return 0;
    }

  pcoords[0] = math.Determinate3x3 (rhs,c2,c3) / det;
  pcoords[1] = math.Determinate3x3 (c1,rhs,c3) / det;
  pcoords[2] = math.Determinate3x3 (c1,c2,rhs) / det;

  if ( pcoords[0] >= 0.0 && pcoords[1] <= 1.0 &&
  pcoords[1] >= 0.0 && pcoords[1] <= 1.0 &&
  pcoords[2] >= 0.0 && pcoords[2] <= 1.0 )
    {
    dist2 = 0.0;
    return 1; // inside tetra
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

void vlTetra::EvaluateLocation(int& subId, float pcoords[3], float x[3])
{
  float u4;
  float *pt1, *pt2, *pt3, *pt4;
  int i;

  pt1 = this->Points.GetPoint(1);
  pt2 = this->Points.GetPoint(2);
  pt3 = this->Points.GetPoint(3);
  pt4 = this->Points.GetPoint(0);

  u4 = 1.0 - pcoords[0] - pcoords[1] - pcoords[2];

  for (i=0; i<3; i++)
    {
    x[i] = pt1[i]*pcoords[0] + pt2[i]*pcoords[1] + pt3[i]*pcoords[2] +
           pt4[i]*u4;
    }
}

//
// Marching (convex) quadrilaterals
//
typedef int EDGE_LIST;
typedef struct {
       EDGE_LIST edges[7];
} TRIANGLE_CASES;

static TRIANGLE_CASES triCases[] = { 
  {-1, -1, -1, -1, -1, -1, -1},
  { 0, 3, 2, -1, -1, -1, -1},
  { 0, 1, 4, -1, -1, -1, -1},
  { 3, 2, 4, 4, 2, 1, -1},
  { 1, 2, 5, -1, -1, -1, -1},
  { 3, 5, 1, 3, 1, 0, -1},
  { 0, 2, 5, 0, 5, 4, -1},
  { 3, 5, 4, -1, -1, -1, -1},
  { 3, 4, 5, -1, -1, -1, -1},
  { 0, 4, 5, 0, 5, 2, -1},
  { 0, 5, 3, 0, 1, 5, -1},
  { 5, 2, 1, -1, -1, -1, -1},
  { 3, 4, 1, 3, 1, 2, -1},
  { 0, 4, 1, -1, -1, -1, -1},
  { 0, 2, 3, -1, -1, -1, -1},
  {-1, -1, -1, -1, -1, -1, -1}
};

void vlTetra::Contour(float value, vlFloatScalars *cellScalars, 
                      vlFloatPoints *points,
                      vlCellArray *verts, vlCellArray *lines, 
                      vlCellArray *polys, vlFloatScalars *scalars)
{
  static int CASE_MASK[4] = {1,2,4,8};
  TRIANGLE_CASES *triCase;
  EDGE_LIST  *edge;
  int i, j, index, *vert;
  static int edges[6][2] = { {0,1}, {1,2}, {2,0}, 
                             {0,3}, {1,3}, {2,3} };
  int pts[3];
  float t, *x1, *x2, x[3];

  // Build the case table
  for ( i=0, index = 0; i < 4; i++)
      if (cellScalars->GetScalar(i) >= value)
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

