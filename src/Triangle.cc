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

int vlTriangle::EvaluatePosition(float x[3], int& subId, float pcoords[3], float& dist2)
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
  pt1 = this->Points.GetPoint(1);
  pt2 = this->Points.GetPoint(2);
  pt3 = this->Points.GetPoint(0);

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
  pcoords[2] = 1.0 - pcoords[0] - pcoords[1];
//
// Okay, now find closest point to element
//
  if ( pcoords[0] >= 0.0 && pcoords[1] <= 1.0 &&
  pcoords[1] >= 0.0 && pcoords[1] <= 1.0 &&
  pcoords[2] >= 0.0 && pcoords[2] <= 1.0 )
    {
    dist2 = math.Distance2BetweenPoints(xProj,x); //projection distance
    return 1;
    }
  else
    {
    for (i=0; i<2; i++)
      {
      if (pcoords[i] < 0.0) pcoords[i] = 0.0;
      if (pcoords[i] > 1.0) pcoords[i] = 1.0;
      }
    this->EvaluateLocation(subId, pcoords, closestPoint);
    dist2 = math.Distance2BetweenPoints(closestPoint,x);
    return 0;
    }
}

void vlTriangle::EvaluateLocation(int& subId, float pcoords[3], float x[3])
{
  float u3;
  float *pt1, *pt2, *pt3;
  int i;

  pt1 = this->Points.GetPoint(1);
  pt2 = this->Points.GetPoint(2);
  pt3 = this->Points.GetPoint(0);

  u3 = 1.0 - pcoords[0] - pcoords[1];

  for (i=0; i<3; i++)
    {
    x[i] = pt1[i]*pcoords[0] + pt2[i]*pcoords[1] + pt3[i]*u3;
    }
}

//
// Marching triangles
//
typedef int EDGE_LIST;
typedef struct {
       EDGE_LIST edges[3];
} LINE_CASES;

static LINE_CASES lineCases[] = { 
  {-1, -1, -1},
  {0, 2, -1},
  {1, 0, -1},
  {1, 2, -1},
  {2, 1, -1},
  {0, 1, -1},
  {2, 0, -1},
  {-1, -1, -1}
};

void vlTriangle::Contour(float value, vlFloatScalars *cellScalars, 
                         vlFloatPoints *points,
                         vlCellArray *verts, vlCellArray *lines, 
                         vlCellArray *polys, vlFloatScalars *scalars)
{
  static int CASE_MASK[3] = {1,2,4};
  LINE_CASES *lineCase;
  EDGE_LIST  *edge;
  int i, j, index, *vert;
  static int edges[3][2] = { {0,1}, {1,2}, {2,0} };
  int pts[2];
  float t, *x1, *x2, x[3];

  // Build the case table
  for ( i=0, index = 0; i < 3; i++)
      if (cellScalars->GetScalar(i) >= value)
          index |= CASE_MASK[i];

  lineCase = lineCases + index;
  edge = lineCase->edges;

  for ( ; edge[0] > -1; edge += 2 )
    {
    for (i=0; i<2; i++) // insert line
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
    lines->InsertNextCell(2,pts);
    }
}

