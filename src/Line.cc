/*=========================================================================

  Program:   Visualization Library
  Module:    Line.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Line.hh"
#include "vlMath.hh"
#include "CellArr.hh"

static vlMath math;

// Description:
// Deep copy of cell.
vlLine::vlLine(const vlLine& l)
{
  this->Points = l.Points;
  this->PointIds = l.PointIds;
}

#define NO_INTERSECTION 1
#define INTERSECTION 2
#define ON_LINE 6

int vlLine::EvaluatePosition(float x[3], float closestPoint[3], 
                             int& subId, float pcoords[3],
                             float& dist2, float weights[MAX_CELL_SIZE])
{
  float *a1, *a2, a21[3], denom, num;
  int i, return_status;
  float *closest;

  subId = 0;
  pcoords[1] = pcoords[2] = 0.0;

  a1 = this->Points.GetPoint(0);
  a2 = this->Points.GetPoint(1);
//
//   Determine appropriate vectors
// 
  for (i=0; i<3; i++) a21[i] = a2[i] - a1[i];
//
//   Get parametric location
//
  num = a21[0]*(x[0]-a1[0]) + a21[1]*(x[1]-a1[1]) + a21[2]*(x[2]-a1[2]);
  denom = math.Dot(a21,a21);

  if ( (denom = math.Dot(a21,a21)) < fabs(TOL*num) )
    {
    dist2 = LARGE_FLOAT;
    }
  else 
    {
    pcoords[0] = num / denom;
    }
//
// If parametric coordinate is within 0<=p<=1, then the point is closest to
// the line.  Otherwise, it's closest to a point at the end of the line.
//
  if ( pcoords[0] < 0.0 )
    {
    closest = a1;
    return_status = 0;
    }
  else if ( pcoords[0] > 1.0 )
    {
    closest = a2;
    return_status = 0;
    }
  else
    {
    closest = a21;
    for (i=0; i<3; i++) a21[i] = a1[i] + pcoords[0]*a21[i];
    return_status = 1;
    }

  dist2 = math.Distance2BetweenPoints(closest,x);
  closestPoint[0] = closest[0]; closestPoint[1] = closest[1]; closestPoint[2] = closest[2]; 
  weights[0] = pcoords[0];
  weights[1] = 1.0 - pcoords[0];

  return return_status;
}

void vlLine::EvaluateLocation(int& subId, float pcoords[3], float x[3],
                              float weights[MAX_CELL_SIZE])
{
  int i;
  float *a1 = this->Points.GetPoint(0);
  float *a2 = this->Points.GetPoint(1);

  for (i=0; i<3; i++) 
    {
    x[i] = a1[i] + pcoords[0]*(a2[i] - a1[i]);
    }

  weights[0] = pcoords[0];
  weights[1] = 1.0 - pcoords[0];
}

//
//  Intersect two 3D lines
//
int vlLine::Intersection (float a1[3], float a2[3], float b1[3], float b2[3],
                          float& u, float& v)
{
  float a21[3], b21[3], b1a1[3];
  float sys[2][2], c[2], det;
  int i;
//
//  Initialize 
//
  u = v = 0.0;
//
//   Determine line vectors.
//
  for (i=0; i<3; i++) 
    {
    a21[i] = a2[i] - a1[i];
    b21[i] = b2[i] - b1[i];
    b1a1[i] = b1[i] - a1[i];
    }
//
//   Compute the system (least squares) matrix.
//
  sys[0][0] = math.Dot ( a21, a21 );
  sys[0][1] = -math.Dot ( a21, b21 );
  sys[1][0] = sys[0][1];
  sys[1][1] = math.Dot ( b21, b21 );
//
//   Compute the least squares system constant term.
//
  c[0] = math.Dot ( a21, b1a1 );
  c[1] = -math.Dot ( b21, b1a1 );
//
//  Solve the system of equations
//
  if ( (det=math.Determinant2x2(sys[0],sys[1])) <= TOL )
    {
    return ON_LINE;
    }
  else 
    {
    u = math.Determinant2x2(c,sys[1]) / det;
    v = math.Determinant2x2(sys[0],c) / det;
    }
//
//  Check parametric coordinates for intersection.
//
  if ( (0.0 <= u) && (u <= 1.0) && (0.0 <= v) && (v <= 1.0) )
    {
    return INTERSECTION;
    }
  else
    {
    return NO_INTERSECTION;
    }
}

int vlLine::CellBoundary(int subId, float pcoords[3], vlIdList& pts)
{
  pts.Reset();

  if ( pcoords[0] >= 0.5 )
    {
    pts.SetId(0,this->PointIds.GetId(1));
    if ( pcoords[0] > 1.0 ) return 0;
    else return 1;
    }
  else
    {
    pts.SetId(0,this->PointIds.GetId(0));
    if ( pcoords[0] < 0.0 ) return 0;
    else return 1;
    }
}

//
// marching lines case table
//
typedef int VERT_LIST;

typedef struct {
  VERT_LIST verts[2];
} LINE_CASES;

static LINE_CASES lineCases[]= {
  {-1,-1},
  {1,0},
  {0,1},
  {-1,-1}};

void vlLine::Contour(float value, vlFloatScalars *cellScalars, 
                     vlFloatPoints *points,
                     vlCellArray *verts, vlCellArray *lines, 
                     vlCellArray *polys, vlFloatScalars *scalars)
{
  static int CASE_MASK[2] = {1,2};
  int index, i;
  LINE_CASES *lineCase;
  VERT_LIST *vert;
  float t, x[3], *x1, *x2;
  int pts[1];
//
// Build the case table
//
  for ( i=0, index = 0; i < 2; i++)
    if (cellScalars->GetScalar(i) >= value) 
      index |= CASE_MASK[i];

  lineCase = lineCases + index;
  vert = lineCase->verts;

  while ( vert[0] > -1 )
    {
    t = (value - cellScalars->GetScalar(vert[0])) /
        (cellScalars->GetScalar(vert[1]) - cellScalars->GetScalar(vert[0]));
    x1 = this->Points.GetPoint(vert[0]);
    x2 = this->Points.GetPoint(vert[1]);
    for (i=0; i<3; i++) x[i] = x1[i] + t * (x2[i] - x1[i]);

    pts[0] = points->InsertNextPoint(x);
    verts->InsertNextCell(1,pts);
    scalars->InsertNextScalar(value);
    }
}

//
//  Determine the distance of the current vertex to the edge defined by
//  the vertices provided.  Returns distance squared.
//
float vlLine::DistanceToLine (float x[3], float p1[3], float p2[3])
{
  int i;
  float np1[3], p1p2[3], proj, den;

  for (i=0; i<3; i++) 
    {
    np1[i] = x[i] - p1[i];
    p1p2[i] = p1[i] - p2[i];
    }

  if ( (den=math.Norm(p1p2)) != 0.0 )
      for (i=0; i<3; i++)
          p1p2[i] /= den;
  else
      return math.Dot(np1,np1);

  proj = math.Dot(np1,p1p2);

  return (math.Dot(np1,np1) - proj*proj);
}

//
// Line-line intersection. Intersection has to occur within [0,1] parametric
// coordinates and with specified tolerance.
//
int vlLine::IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                              float x[3], float pcoords[3], int& subId)
{
  float *a1, *a2;
  float projXYZ[3];
  int i;

  subId = 0;
  pcoords[1] = pcoords[2] = 0.0;

  a1 = this->Points.GetPoint(0);
  a2 = this->Points.GetPoint(1);

  if ( this->Intersection(p1, p2, a1, a2, t, pcoords[0]) == NO_INTERSECTION )
    {
    return 0;
    }
  else //check to make sure lies within tolerance
    {
    for (i=0; i<3; i++)
      {
      x[i] = a1[i] + pcoords[i]*(a2[i]-a1[i]);
      projXYZ[i] = p1[i] + t*(p2[i]-p1[i]);
      }
    if ( math.Distance2BetweenPoints(x,projXYZ) <= tol*tol )
      return 1;
    else
      return 0;
    }
}
