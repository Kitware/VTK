/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Voxel.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Voxel.hh"
#include "vtkMath.hh"
#include "Line.hh"
#include "Pixel.hh"
#include "CellArr.hh"

static vtkMath math;  
static vtkLine line;

// Description:
// Deep copy of cell.
vtkVoxel::vtkVoxel(const vtkVoxel& b)
{
  this->Points = b.Points;
  this->PointIds = b.PointIds;
}

int vtkVoxel::EvaluatePosition(float x[3], float closestPoint[3],
                              int& subId, float pcoords[3], 
                              float& dist2, float weights[MAX_CELL_SIZE])
{
  float *pt1, *pt2, *pt3, *pt4;
  int i;

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
    closestPoint[0] = x[0]; closestPoint[1] = x[1]; closestPoint[2] = x[2];
    dist2 = 0.0; // inside voxel
    this->InterpolationFunctions(pcoords,weights);
    return 1;
    }
  else
    {
    for (i=0; i<3; i++)
      {
      if (pcoords[i] < 0.0) pcoords[i] = 0.0;
      if (pcoords[i] > 1.0) pcoords[i] = 1.0;
      }
    this->EvaluateLocation(subId, pcoords, closestPoint, weights);
    dist2 = math.Distance2BetweenPoints(closestPoint,x);
    return 0;
    }
}

void vtkVoxel::EvaluateLocation(int& subId, float pcoords[3], float x[3],
                               float weights[MAX_CELL_SIZE])
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
  
  this->InterpolationFunctions(pcoords,weights);
}

//
// Compute Interpolation functions
//
void vtkVoxel::InterpolationFunctions(float pcoords[3], float sf[8])
{
  float rm, sm, tm;

  rm = 1. - pcoords[0];
  sm = 1. - pcoords[1];
  tm = 1. - pcoords[2];

  sf[0] = rm * sm * tm;
  sf[1] = pcoords[0] * sm * tm;
  sf[2] = rm * pcoords[1] * tm;
  sf[3] = pcoords[0] * pcoords[1] * tm;
  sf[4] = rm * sm * pcoords[2];
  sf[5] = pcoords[0] * sm * pcoords[2];
  sf[6] = rm * pcoords[1] * pcoords[2];
  sf[7] = pcoords[0] * pcoords[1] * pcoords[2];
}

int vtkVoxel::CellBoundary(int subId, float pcoords[3], vtkIdList& pts)
{
  float t1=pcoords[0]-pcoords[1];
  float t2=1.0-pcoords[0]-pcoords[1];
  float t3=pcoords[1]-pcoords[2];
  float t4=1.0-pcoords[1]-pcoords[2];
  float t5=pcoords[2]-pcoords[0];
  float t6=1.0-pcoords[2]-pcoords[0];

  pts.Reset();

  // compare against six planes in parametric space that divide element
  // into six pieces.
  if ( t3 >= 0.0 && t4 >= 0.0 && t5 < 0.0 && t6 >= 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(0));
    pts.SetId(1,this->PointIds.GetId(1));
    pts.SetId(2,this->PointIds.GetId(3));
    pts.SetId(3,this->PointIds.GetId(2));
    }

  else if ( t1 >= 0.0 && t2 < 0.0 && t5 < 0.0 && t6 < 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(1));
    pts.SetId(1,this->PointIds.GetId(3));
    pts.SetId(2,this->PointIds.GetId(7));
    pts.SetId(3,this->PointIds.GetId(5));
    }

  else if ( t1 >= 0.0 && t2 >= 0.0 && t3 < 0.0 && t4 >= 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(0));
    pts.SetId(1,this->PointIds.GetId(1));
    pts.SetId(2,this->PointIds.GetId(5));
    pts.SetId(3,this->PointIds.GetId(4));
    }

  else if ( t3 < 0.0 && t4 < 0.0 && t5 >= 0.0 && t6 < 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(4));
    pts.SetId(1,this->PointIds.GetId(5));
    pts.SetId(2,this->PointIds.GetId(7));
    pts.SetId(3,this->PointIds.GetId(6));
    }

  else if ( t1 < 0.0 && t2 >= 0.0 && t5 >= 0.0 && t6 >= 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(0));
    pts.SetId(1,this->PointIds.GetId(4));
    pts.SetId(2,this->PointIds.GetId(6));
    pts.SetId(3,this->PointIds.GetId(2));
    }

  else // if ( t1 < 0.0 && t2 < 0.0 && t3 >= 0.0 && t6 < 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(3));
    pts.SetId(1,this->PointIds.GetId(2));
    pts.SetId(2,this->PointIds.GetId(6));
    pts.SetId(3,this->PointIds.GetId(7));
    }

  if ( pcoords[0] < 0.0 || pcoords[0] > 1.0 ||
  pcoords[1] < 0.0 || pcoords[1] > 1.0 || pcoords[2] < 0.0 || pcoords[2] > 1.0 )
    return 0;
  else
    return 1;
}

static int edges[12][2] = { {0,1}, {1,3}, {3,2}, {2,0},
                            {4,5}, {5,7}, {7,6}, {6,4},
                            {0,4}, {1,5}, {2,6}, {3,7}};
// define in terms vtkPixel understands
static int faces[6][4] = { {0,2,4,6}, {1,3,5,7},
                           {0,1,4,5}, {2,3,6,7},
                           {0,1,2,3}, {4,5,6,7} };

//
// Marching cubes case table
//
#include "MC_Cases.h"

void vtkVoxel::Contour(float value, vtkFloatScalars *cellScalars, 
                      vtkFloatPoints *points,
                      vtkCellArray *verts, vtkCellArray *lines, 
                      vtkCellArray *polys, vtkFloatScalars *scalars)
{
  static int CASE_MASK[8] = {1,2,4,8,16,32,64,128};
  TRIANGLE_CASES *triCase;
  EDGE_LIST  *edge;
  int i, j, index, *vert;
  static int vertMap[8] = { 0, 1, 3, 2, 4, 5, 7, 6 };
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


vtkCell *vtkVoxel::GetEdge(int edgeId)
{
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

vtkCell *vtkVoxel::GetFace(int faceId)
{
  static vtkPixel pixel;
  int *verts, i;

  verts = faces[faceId];

  for (i=0; i<4; i++)
    {
    pixel.PointIds.SetId(i,this->PointIds.GetId(verts[i]));
    pixel.Points.SetPoint(i,this->Points.GetPoint(verts[i]));
    }

  return &pixel;
}

// 
// Intersect voxel with line using "bounding box" intersection.
//
int vtkVoxel::IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                               float x[3], float pcoords[3], int& subId)
{
  float *minPt, *maxPt;
  float bounds[6], p21[3];
  int i;

  subId = 0;

  minPt = this->Points.GetPoint(0);
  maxPt = this->Points.GetPoint(7);

  for (i=0; i<3; i++)
    {
    p21[i] = p2[i] - p1[i];
    bounds[2*i] = minPt[i];
    bounds[2*i+1] = maxPt[i];
    }

  if ( ! this->HitBBox(bounds, p1, p21, x, t) )
    return 0;
//
// Evaluate intersection
//
  for (i=0; i<3; i++)
    pcoords[i] = (x[i] - minPt[i]) / (maxPt[i] - minPt[i]);

  return 1;
}
