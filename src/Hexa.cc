/*=========================================================================

  Program:   Visualization Library
  Module:    Hexa.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Hexa.hh"
#include "vlMath.hh"
#include "Line.hh"
#include "Quad.hh"
#include "CellArr.hh"

// Description:
// Deep copy of cell.
vlHexahedron::vlHexahedron(const vlHexahedron& h)
{
  this->Points = h.Points;
  this->PointIds = h.PointIds;
}

//
//  Method to calculate parametric coordinates in an eight noded
//  linear hexahedron element from global coordinates.
//
#define MAX_ITERATION 10
#define CONVERGED 1.e-03

int vlHexahedron::EvaluatePosition(float x[3], float closestPoint[3],
                                   int& subId, float pcoords[3], 
                                   float& dist2, float weights[MAX_CELL_SIZE])
{
  int iteration, converged;
  float  params[3];
  float  fcol[3], rcol[3], scol[3], tcol[3];
  int i, j;
  float  d, *pt;
  float derivs[24];
  static vlMath math;
//
//  set initial position for Newton's method
//
  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = params[0] = params[1] = params[2] = 0.5;
//
//  enter iteration loop
///
  for (iteration=converged=0; !converged && (iteration < MAX_ITERATION);
  iteration++) 
    {
//
//  calculate element interpolation functions and derivatives
//
    this->InterpolationFunctions(pcoords, weights);
    this->InterpolationDerivs(pcoords, derivs);
//
//  calculate newton functions
//
    for (i=0; i<3; i++) 
      {
      fcol[i] = rcol[i] = scol[i] = tcol[i] = 0.0;
      }
    for (i=0; i<8; i++)
      {
      pt = this->Points.GetPoint(i);
      for (j=0; j<3; j++)
        {
        fcol[j] += pt[j] * weights[i];
        rcol[j] += pt[j] * derivs[i];
        scol[j] += pt[j] * derivs[i+8];
        tcol[j] += pt[j] * derivs[i+16];
        }
      }

    for (i=0; i<3; i++) fcol[i] -= x[i];
//
//  compute determinants and generate improvements
//
    if ( (d=math.Determinant3x3(rcol,scol,tcol)) == 0.0 )
      {
      dist2 = LARGE_FLOAT;
      return 0;
      }

    pcoords[0] = params[0] - math.Determinant3x3 (fcol,scol,tcol) / d;
    pcoords[1] = params[1] - math.Determinant3x3 (rcol,fcol,tcol) / d;
    pcoords[2] = params[2] - math.Determinant3x3 (rcol,scol,fcol) / d;
//
//  check for convergence
//
    if ( ((fabs(pcoords[0]-params[0])) < CONVERGED) &&
    ((fabs(pcoords[1]-params[1])) < CONVERGED) &&
    ((fabs(pcoords[2]-params[2])) < CONVERGED) )
      {
      converged = 1;
      }
//
//  if not converged, repeat
//
    else 
      {
      params[0] = pcoords[0];
      params[1] = pcoords[1];
      params[2] = pcoords[2];
      }
    }
//
//  if not converged, set the parametric coordinates to arbitrary values
//  outside of element
//
  if ( !converged )
    {
    pcoords[0] = pcoords[1] =  pcoords[2] = 10.0;
    dist2 = LARGE_FLOAT;
    return 0;
    }
  else
    {
    if ( pcoords[0] >= 0.0 && pcoords[0] <= 1.0 &&
    pcoords[1] >= 0.0 && pcoords[1] <= 1.0 &&
    pcoords[2] >= 0.0 && pcoords[2] <= 1.0 )
      {
      closestPoint[0] = x[0]; closestPoint[1] = x[1]; closestPoint[2] = x[2];
      dist2 = 0.0; // inside hexahedron
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
}
//
// Compute iso-parametrix interpolation functions
//
void vlHexahedron::InterpolationFunctions(float pcoords[3], float sf[8])
{
  double rm, sm, tm;

  rm = 1. - pcoords[0];
  sm = 1. - pcoords[1];
  tm = 1. - pcoords[2];

  sf[0] = rm*sm*tm;
  sf[1] = pcoords[0]*sm*tm;
  sf[2] = pcoords[0]*pcoords[1]*tm;
  sf[3] = rm*pcoords[1]*tm;
  sf[4] = rm*sm*pcoords[2];
  sf[5] = pcoords[0]*sm*pcoords[2];
  sf[6] = pcoords[0]*pcoords[1]*pcoords[2];
  sf[7] = rm*pcoords[1]*pcoords[2];
}

void vlHexahedron::InterpolationDerivs(float pcoords[3], float derivs[24])
{
  double rm, sm, tm;

  rm = 1. - pcoords[0];
  sm = 1. - pcoords[1];
  tm = 1. - pcoords[2];

  derivs[0] = -sm*tm;
  derivs[1] = sm*tm;
  derivs[2] = pcoords[1]*tm;
  derivs[3] = -pcoords[1]*tm;
  derivs[4] = -sm*pcoords[2];
  derivs[5] = sm*pcoords[2];
  derivs[6] = pcoords[1]*pcoords[2];
  derivs[7] = -pcoords[1]*pcoords[2];
  derivs[8] = -rm*tm;
  derivs[9] = -pcoords[0]*tm;
  derivs[10] = pcoords[0]*tm;
  derivs[11] = rm*tm;
  derivs[12] = -rm*pcoords[2];
  derivs[13] = -pcoords[0]*pcoords[2];
  derivs[14] = pcoords[0]*pcoords[2];
  derivs[15] = rm*pcoords[2];
  derivs[16] = -rm*sm;
  derivs[17] = -pcoords[0]*sm;
  derivs[18] = -pcoords[0]*pcoords[1];
  derivs[19] = -rm*pcoords[1];
  derivs[20] = rm*sm;
  derivs[21] = pcoords[0]*sm;
  derivs[22] = pcoords[0]*pcoords[1];
  derivs[23] = rm*pcoords[1];
}

void vlHexahedron::EvaluateLocation(int& subId, float pcoords[3], float x[3],
                                    float weights[MAX_CELL_SIZE])
{
  int i, j;
  float *pt, pc[3];

  this->InterpolationFunctions(pc, weights);

  x[0] = x[1] = x[2] = 0.0;
  for (i=0; i<8; i++)
    {
    pt = this->Points.GetPoint(i);
    for (j=0; j<3; j++)
      {
      x[j] += pt[j] * weights[i];
      }
    }
}

int vlHexahedron::CellBoundary(int subId, float pcoords[3], vlIdList& pts)
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
    pts.SetId(2,this->PointIds.GetId(2));
    pts.SetId(3,this->PointIds.GetId(3));
    }

  else if ( t1 >= 0.0 && t2 < 0.0 && t5 < 0.0 && t6 < 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(1));
    pts.SetId(1,this->PointIds.GetId(2));
    pts.SetId(2,this->PointIds.GetId(6));
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
    pts.SetId(2,this->PointIds.GetId(6));
    pts.SetId(3,this->PointIds.GetId(7));
    }

  else if ( t1 < 0.0 && t2 >= 0.0 && t5 >= 0.0 && t6 >= 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(0));
    pts.SetId(1,this->PointIds.GetId(4));
    pts.SetId(2,this->PointIds.GetId(7));
    pts.SetId(3,this->PointIds.GetId(3));
    }

  else // if ( t1 < 0.0 && t2 < 0.0 && t3 >= 0.0 && t6 < 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(2));
    pts.SetId(1,this->PointIds.GetId(3));
    pts.SetId(2,this->PointIds.GetId(7));
    pts.SetId(3,this->PointIds.GetId(6));
    }


  if ( pcoords[0] < 0.0 || pcoords[0] > 1.0 ||
  pcoords[1] < 0.0 || pcoords[1] > 1.0 || pcoords[2] < 0.0 || pcoords[2] > 1.0 )
    return 0;
  else
    return 1;
}

static int edges[12][2] = { {0,1}, {1,2}, {2,3}, {3,0},
                            {4,5}, {5,6}, {6,7}, {7,4},
                            {0,4}, {1,5}, {3,7}, {2,6}};
static int faces[6][4] = { {0,4,7,3}, {1,2,6,5},
                           {0,1,5,4}, {3,7,6,2},
                           {0,3,2,1}, {4,5,6,7} };
//
// Marching cubes case table
//
#include "MC_Cases.h"

void vlHexahedron::Contour(float value, vlFloatScalars *cellScalars, 
                      vlFloatPoints *points,
                      vlCellArray *verts, vlCellArray *lines, 
                      vlCellArray *polys, vlFloatScalars *scalars)
{
  static int CASE_MASK[8] = {1,2,4,8,16,32,64,128};
  TRIANGLE_CASES *triCase;
  EDGE_LIST  *edge;
  int i, j, index, *vert;
  int pts[3];
  float t, *x1, *x2, x[3];

  // Build the case table
  for ( i=0, index = 0; i < 8; i++)
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

vlCell *vlHexahedron::GetEdge(int edgeId)
{
  int *verts;
  static vlLine line;

  verts = edges[edgeId];

  // load point id's
  line.PointIds.SetId(0,this->PointIds.GetId(verts[0]));
  line.PointIds.SetId(1,this->PointIds.GetId(verts[1]));

  // load coordinates
  line.Points.SetPoint(0,this->Points.GetPoint(verts[0]));
  line.Points.SetPoint(1,this->Points.GetPoint(verts[1]));

  return &line;
}

vlCell *vlHexahedron::GetFace(int faceId)
{
  int *verts, i;
  static vlQuad theQuad; // using "quad" bothers IBM xlc compiler!

  verts = faces[faceId];

  for (i=0; i<4; i++)
    {
    theQuad.PointIds.SetId(i,this->PointIds.GetId(verts[i]));
    theQuad.Points.SetPoint(i,this->Points.GetPoint(verts[i]));
    }

  return &theQuad;
}
// 
// Intersect hexa faces against line. Each hexa face is a quadrilateral.
//
int vlHexahedron::IntersectWithLine(float p1[3], float p2[3], float tol,
                                    float &t, float x[3], float pcoords[3],
                                    int& subId)
{
  int intersection=0;
  float *pt1, *pt2, *pt3, *pt4;
  float tTemp;
  float pc[3], xTemp[3];
  int faceNum;
  static vlQuad theQuad; // using "quad" bothers IBM xlc compiler!

  t = LARGE_FLOAT;
  for (faceNum=0; faceNum<6; faceNum++)
    {
    pt1 = this->Points.GetPoint(faces[faceNum][0]);
    pt2 = this->Points.GetPoint(faces[faceNum][1]);
    pt3 = this->Points.GetPoint(faces[faceNum][2]);
    pt4 = this->Points.GetPoint(faces[faceNum][3]);

    theQuad.Points.SetPoint(0,pt1);
    theQuad.Points.SetPoint(1,pt2);
    theQuad.Points.SetPoint(2,pt3);
    theQuad.Points.SetPoint(3,pt4);

    if ( theQuad.IntersectWithLine(p1, p2, tol, tTemp, xTemp, pc, subId) )
      {
      intersection = 1;
      if ( tTemp < t )
        {
        t = tTemp;
        x[0] = xTemp[0]; x[1] = xTemp[1]; x[2] = xTemp[2]; 
        switch (faceNum)
          {
          case 0:
            pcoords[0] = 0.0; pcoords[0] = pc[0]; pcoords[1] = 0.0;
            break;

          case 1:
            pcoords[0] = 1.0; pcoords[0] = pc[0]; pcoords[1] = 0.0;
            break;

          case 2:
            pcoords[0] = pc[0]; pcoords[1] = 0.0; pcoords[2] = pc[1];
            break;

          case 3:
            pcoords[0] = pc[0]; pcoords[1] = 1.0; pcoords[2] = pc[1];
            break;

          case 4:
            pcoords[0] = pc[0]; pcoords[1] = pc[1]; pcoords[2] = 0.0;
            break;

          case 5:
            pcoords[0] = pc[0]; pcoords[1] = pc[1]; pcoords[2] = 1.0;
            break;
          }
        }
      }
    }
  return intersection;
}
