/*=========================================================================

  Program:   Visualization Library
  Module:    Hexa.cc
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
#include "Hexa.hh"
#include "vlMath.hh"
#include "Brick.hh"

//
// Note: the ordering of the Points and PointIds is important.  See text.
//

//
//  Method to calculate parametric coordinates in an eight noded
//  linear hexahedron element from global coordinates.  Note: the natural 
//  formulation calls for r,s,t parametric coordinates to range range 
//  from -1<=r,s,t<=1. We need to shift to 0<=r,s,t<=1.
//  Uses Newton's method to solve the non-linear equations.
//
#define MAX_ITERATION 10
#define CONVERGED 1.e-03

float vlHexahedron::EvaluatePosition(float x[3], int& subId, float pcoords[3])
{
  int iteration, converged;
  float  params[3];
  float  fcol[3], rcol[3], scol[3], tcol[3];
  int i,j,numPts,idx;
  float  d, *pt;
  float closestPoint[3];
  vlMath math;
  float sf[8], derivs[24];
//
//  set initial position for Newton's method
//
  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = params[0] = params[1] = params[2] = 0.0;
//
//  enter iteration loop
///
  for (iteration=converged=0; !converged && (iteration < MAX_ITERATION);
  iteration++) 
    {
//
//  calculate element shape functions and derivatives
//
    this->ShapeFunctions(pcoords, sf);
    this->ShapeDerivs(pcoords, derivs);
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
        fcol[j] += pt[j] * sf[i];
        rcol[j] += pt[j] * derivs[i];
        scol[j] += pt[j] * derivs[i+8];
        tcol[j] += pt[j] * derivs[i+16];
        }
      }

    for (i=0; i<3; i++) fcol[i] -= x[i];
//
//  compute determinates and generate improvements
//
    if ( (d=math.Determinate3x3(rcol,scol,tcol)) == 0.0 )
      {
      return LARGE_FLOAT;
      }

    pcoords[0] = params[0] - math.Determinate3x3 (fcol,scol,tcol) / d;
    pcoords[1] = params[1] - math.Determinate3x3 (rcol,fcol,tcol) / d;
    pcoords[2] = params[2] - math.Determinate3x3 (rcol,scol,fcol) / d;
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
    return LARGE_FLOAT;
    }
  else
    {
    if ( pcoords[0] >= -1.0 && pcoords[1] <= 1.0 &&
    pcoords[1] >= -1.0 && pcoords[1] <= 1.0 &&
    pcoords[2] >= -1.0 && pcoords[2] <= 1.0 )
      {
      for(i=0; i<3; i++) pcoords[i] = 0.5*(pcoords[i]+1.0); // shift to (0,1)
      return 0.0; // inside hexahedron
      }
    else
      {
      for (i=0; i<3; i++)
        {
        if (pcoords[i] < -1.0) pcoords[i] = -1.0;
        if (pcoords[i] > 1.0) pcoords[i] = 1.0;
        }
      this->EvaluateLocation(subId, pcoords, closestPoint);
      for(i=0; i<3; i++) pcoords[i] = 0.5*(pcoords[i]+1.0); // shift to (0,1)
      return math.Distance2BetweenPoints(closestPoint,x);
      }
    }
}
//
// Compute iso-parametrix shape functions
//
void vlHexahedron::ShapeFunctions(float pcoords[3], float sf[8])
{
  double rm, rp, sm, sp, tm, tp;

  rm = 1. - pcoords[0];
  rp = 1. + pcoords[0];
  sm = 1. - pcoords[1];
  sp = 1. + pcoords[1];
  tm = 1. - pcoords[2];
  tp = 1. + pcoords[2];

  sf[0] = 0.125*rm*sm*tm;
  sf[1] = 0.125*rp*sm*tm;
  sf[2] = 0.125*rp*sp*tm;
  sf[3] = 0.125*rm*sp*tm;
  sf[4] = 0.125*rm*sm*tp;
  sf[5] = 0.125*rp*sm*tp;
  sf[6] = 0.125*rp*sp*tp;
  sf[7] = 0.125*rm*sp*tp;
}

void vlHexahedron::ShapeDerivs(float pcoords[3], float derivs[24])
{
  double rm, rp, sm, sp, tm, tp;

  rm = 1. - pcoords[0];
  rp = 1. + pcoords[0];
  sm = 1. - pcoords[1];
  sp = 1. + pcoords[1];
  tm = 1. - pcoords[2];
  tp = 1. + pcoords [2];

  derivs[0] = -0.125*sm*tm;
  derivs[1] = 0.125*sm*tm;
  derivs[2] = 0.125*sp*tm;
  derivs[3] = -0.125*sp*tm;
  derivs[4] = -0.125*sm*tp;
  derivs[5] = 0.125*sm*tp;
  derivs[6] = 0.125*sp*tp;
  derivs[7] = -0.125*sp*tp;
  derivs[8] = -0.125*rm*tm;
  derivs[9] = -0.125*rp*tm;
  derivs[10] = 0.125*rp*tm;
  derivs[11] = 0.125*rm*tm;
  derivs[12] = -0.125*rm*tp;
  derivs[13] = -0.125*rp*tp;
  derivs[14] = 0.125*rp*tp;
  derivs[15] = 0.125*rm*tp;
  derivs[16] = -0.125*rm*sm;
  derivs[17] = -0.125*rp*sm;
  derivs[18] = -0.125*rp*sp;
  derivs[19] = -0.125*rm*sp;
  derivs[20] = 0.125*rm*sm;
  derivs[21] = 0.125*rp*sm;
  derivs[22] = 0.125*rp*sp;
  derivs[23] = 0.125*rm*sp;
}

void vlHexahedron::EvaluateLocation(int& subId, float pcoords[3], float x[3])
{
  int i, j;
  float sf[8], *pt, pc[3];

  for (i=0;i<3;i++) pc[i] = 2.0*pcoords[i] - 1.0; //shift to -1<=r,s,t<=1
  this->ShapeFunctions(pc, sf);

  x[0] = x[1] = x[2] = 0.0;
  for (i=0; i<8; i++)
    {
    pt = this->Points.GetPoint(i);
    for (j=0; j<3; j++)
      {
      x[j] += pt[j] * sf[i];
      }
    }
}

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
  static int edges[12][2] = { {0,1}, {1,2}, {2,3}, {3,0},
                              {4,5}, {5,6}, {6,7}, {7,4},
                              {0,4}, {1,5}, {3,7}, {2,6}};
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
