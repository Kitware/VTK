/*=========================================================================

  Program:   Visualization Library
  Module:    Pixel.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Rect.hh"
#include "Quad.hh"
#include "Polygon.hh"
#include "Plane.hh"
#include "vlMath.hh"
#include "CellArr.hh"
#include "Line.hh"

// Description:
// Deep copy of cell.
vlRectangle::vlRectangle(const vlRectangle& r)
{
  this->Points = r.Points;
  this->PointIds = r.PointIds;
}

int vlRectangle::EvaluatePosition(float x[3], float closestPoint[3],
                                  int& subId, float pcoords[3], 
                                  float& dist2, float weights[MAX_CELL_SIZE])
{
  float *pt1, *pt2, *pt3;
  static vlPolygon poly;
  static vlPlane plane;
  int i;
  float p[3], p21[3], p31[3];
  float l21, l31, n[3];
  vlMath math;
  float tempPc[2];

  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;
//
// Get normal for rectangle
//
  pt1 = this->Points.GetPoint(0);
  pt2 = this->Points.GetPoint(1);
  pt3 = this->Points.GetPoint(2);

  poly.ComputeNormal (pt1, pt2, pt3, n);
//
// Project point to plane
//
  plane.ProjectPoint(x,pt1,n,closestPoint);

  for (i=0; i<3; i++)
    {
    p21[i] = pt2[i] - pt1[i];
    p31[i] = pt3[i] - pt1[i];
    p[i] = x[i] - pt1[i];
    }

  if ( (l21=math.Norm(p21)) == 0.0 ) l21 = 1.0;
  if ( (l31=math.Norm(p31)) == 0.0 ) l31 = 1.0;

  pcoords[0] = math.Dot(p21,p) / (l21*l21);
  pcoords[1] = math.Dot(p31,p) / (l31*l31);

  if ( pcoords[0] >= 0.0 && pcoords[1] <= 1.0 &&
  pcoords[1] >= 0.0 && pcoords[1] <= 1.0 )
    {
    dist2 = math.Distance2BetweenPoints(closestPoint,x); //projection distance
    this->ShapeFunctions(pcoords, weights);
    return 1;
    }
  else
    {
    for (i=0; i<2; i++)
      {
      if (pcoords[i] < 0.0) pcoords[i] = 0.0;
      if (pcoords[i] > 1.0) pcoords[i] = 1.0;
      }
    this->EvaluateLocation(subId, pcoords, closestPoint, weights);
    dist2 = math.Distance2BetweenPoints(closestPoint,x);
    return 0;
    }
}

void vlRectangle::EvaluateLocation(int& subId, float pcoords[3], float x[3],
                                   float weights[MAX_CELL_SIZE])
{
  float *pt1, *pt2, *pt3;
  int i;

  pt1 = this->Points.GetPoint(0);
  pt2 = this->Points.GetPoint(1);
  pt3 = this->Points.GetPoint(2);

  for (i=0; i<3; i++)
    {
    x[i] = pt1[i] + pcoords[0]*(pt2[i] - pt1[i]) +
                    pcoords[1]*(pt3[i] - pt1[i]);
    }

  this->ShapeFunctions(pcoords, weights);
}

void vlRectangle::Contour(float value, vlFloatScalars *cellScalars,
                     vlFloatPoints *points, vlCellArray *verts,
                     vlCellArray *lines, vlCellArray *polys, 
                     vlFloatScalars *scalars)
{
  int i;
  static vlQuad quad;

  quad.Points.SetPoint(0,this->Points.GetPoint(0));
  quad.Points.SetPoint(1,this->Points.GetPoint(1));
  quad.Points.SetPoint(2,this->Points.GetPoint(3));
  quad.Points.SetPoint(3,this->Points.GetPoint(2));

  quad.Contour(value, cellScalars, points, verts, lines,
                   polys, scalars);

}

static int edges[4][2] = { {0,1}, {1,3}, {3,2}, {2,0} };

vlCell *vlRectangle::GetEdge(int edgeId)
{
  static vlLine line;
  int *verts;

  verts = edges[edgeId];

  // load point id's
  line.PointIds.SetId(0,this->PointIds.GetId(verts[0]));
  line.PointIds.SetId(1,this->PointIds.GetId(verts[1]));

  // load coordinates
  line.Points.SetPoint(0,this->Points.GetPoint(verts[0]));
  line.Points.SetPoint(1,this->Points.GetPoint(verts[1]));

  return &line;
}
//
// Compute shape functions (similar but different than Quad shape functions)
//
void vlRectangle::ShapeFunctions(float pcoords[3], float sf[4])
{
  float rm, sm;

  rm = 1. - pcoords[0];
  sm = 1. - pcoords[1];

  sf[0] = rm * sm;
  sf[1] = pcoords[0] * sm;
  sf[2] = rm * pcoords[1];
  sf[3] = pcoords[0] * pcoords[1];
}
