/*=========================================================================

  Program:   Visualization Library
  Module:    Polygon.cc
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
#include "Polygon.hh"
#include "vlMath.hh"
#include "Line.hh"
#include "Plane.hh"

#define FAILURE 0
#define INTERSECTION 2
#define OUTSIDE 3
#define INSIDE 4
#define ON_LINE 6

void vlPolygon::ComputeNormal(vlPoints *p, int numPts, int *pts, float *n)
{
  int     i;
  float   *v1, *v2, *v3;
  float    length;
  float    ax, ay, az;
  float    bx, by, bz;
//
//  Because some polygon vertices are colinear, need to make sure
//  first non-zero normal is found.
//
  v1 = p->GetPoint(pts[0]);
  v2 = p->GetPoint(pts[1]);
  v3 = p->GetPoint(pts[2]);

  for (i=0; i<numPts; i++) 
    {
    ax = v2[0] - v1[0]; ay = v2[1] - v1[1]; az = v2[2] - v1[2];
    bx = v3[0] - v1[0]; by = v3[1] - v1[1]; bz = v3[2] - v1[2];
    n[0] = (ay * bz - az * by);
    n[1] = (az * bx - ax * bz);
    n[2] = (ax * by - ay * bx);

    length = sqrt (n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    if (length != 0.0) 
      {
      n[0] /= length;
      n[1] /= length;
      n[2] /= length;
      return;
      } 
    else 
      {
      v1 = v2;
      v2 = v3;
      v3 = p->GetPoint(pts[(i+3)%numPts]);
      }
    }
}

void vlPolygon::ComputeNormal(float *v1, float *v2, float *v3, float *n)
{
    float    length;
    float    ax, ay, az;
    float    bx, by, bz;

    // order is important!!! to maintain consistency with polygon vertex order 
    ax = v3[0] - v2[0]; ay = v3[1] - v2[1]; az = v3[2] - v2[2];
    bx = v1[0] - v2[0]; by = v1[1] - v2[1]; bz = v1[2] - v2[2];

    n[0] = (ay * bz - az * by);
    n[1] = (az * bx - ax * bz);
    n[2] = (ax * by - ay * bx);

    length = sqrt (n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    if (length != 0.0) {
        n[0] /= length;
        n[1] /= length;
        n[2] /= length;
    }
}

void vlPolygon::ComputeNormal(vlFloatPoints *p, float *n)
{
  int     i, numPts;
  float   *v1, *v2, *v3;
  float    length;
  float    ax, ay, az;
  float    bx, by, bz;
//
//  Because some polygon vertices are colinear, need to make sure
//  first non-zero normal is found.
//
  numPts = p->NumberOfPoints();
  v1 = p->GetPoint(0);
  v2 = p->GetPoint(1);
  v3 = p->GetPoint(2);

  for (i=0; i<numPts; i++) 
    {
    ax = v2[0] - v1[0]; ay = v2[1] - v1[1]; az = v2[2] - v1[2];
    bx = v3[0] - v1[0]; by = v3[1] - v1[1]; bz = v3[2] - v1[2];
    n[0] = (ay * bz - az * by);
    n[1] = (az * bx - ax * bz);
    n[2] = (ax * by - ay * bx);

    length = sqrt (n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    if (length != 0.0) 
      {
      n[0] /= length;
      n[1] /= length;
      n[2] /= length;
      return;
      } 
    else 
      {
      v1 = v2;
      v2 = v3;
      v3 = p->GetPoint((i+3)%numPts);
      }
    }
}

float vlPolygon::EvaluatePosition(float x[3], int& subId, float pcoords[3])
{
  int i;
  float p0[3], p10[3], l10, p20[3], l20, n[3];
  float xproj[3], ray[3];
  vlPlane plane;
  vlMath math;

  this->ParameterizePolygon(p0, p10, l10, p20, l20, n);
  plane.ProjectPoint(x,p0,n,xproj);

  for (i=0; i<3; i++) ray[i] = xproj[i] - p0[i];
  pcoords[0] = math.Dot(ray,p10) / l10;
  pcoords[1] = math.Dot(ray,p20) / l20;

  if ( pcoords[0] >= 0.0 && pcoords[0] <= 1.0 &&
  pcoords[1] >= 0.0 && pcoords[1] <= 1.0 &&
  this->PointInPolygon(this->GetBounds(),xproj,n) == INSIDE )
    {
    return math.Distance2BetweenPoints(x,xproj);      
    }
//
// If here, point is outside of polygon, so need to find distance to boundary
//
  vlLine line;
  float pc[3], dist2, minDist2;
  int ignoreId, numPts;
  vlFloatPoints pts(2);

  numPts = this->Points->NumberOfPoints();
  line.SetPoints(&pts);
  for (minDist2=LARGE_FLOAT,i=0; i<numPts - 1; i++)
    {
    pts.SetPoint(0,this->Points->GetPoint(i));
    pts.SetPoint(1,this->Points->GetPoint(i+1));
    dist2 = line.EvaluatePosition(x, ignoreId, pc);
    if ( dist2 < minDist2 )
      {
      minDist2 = dist2;
      }
    }

  return minDist2;
}

void vlPolygon::EvaluateLocation(int& subId, float pcoords[3], float x[3])
{
  int i;
  float p0[3], p10[3], l10, p20[3], l20, n[3];

  this->ParameterizePolygon(p0, p10, l10, p20, l20, n);
  for (i=0; i<3; i++)
    {
    x[i] = p0[i] + pcoords[0]*p10[i] + pcoords[1]*p20[i];
    }
}

//
//  Create a local s-t coordinate system for a polygon
//
int vlPolygon::ParameterizePolygon(float *p0, float *p10, float& l10, 
                                   float *p20,float &l20, float *n)
{
  int i, j;
  float s, t, p[3], p1[3], p2[3], sbounds[2], tbounds[2];
  int numPts;
  float *x1, *x2;
  vlMath math;
//
//  This is a two pass process: first create a p' coordinate system
//  that is then adjusted to insure that the polygon points are all in
//  the range 0<=s,t<=1.  The p' system is defined by the polygon normal, 
//  first vertex and the first edge.
//
  this->ComputeNormal (this->Points,n);
  x1 = this->Points->GetPoint(0);
  x2 = this->Points->GetPoint(1);
  for (i=0; i<3; i++) 
    {
    p0[i] = x1[i];
    p10[i] = x2[i] - x1[i];
    }
  math.Cross (n,p10,p20);
//
// Determine lengths of edges
//
  if ( (l10=math.Dot(p10,p10)) == 0.0 || (l20=math.Dot(p20,p20)) == 0.0 )
    return 0;
//
//  Now evalute all polygon points to determine min/max parametric
//  coordinate values.
//
  // first vertex has (s,t) = (0,0)
  sbounds[0] = 0.0; sbounds[1] = 0.0;
  tbounds[0] = 0.0; tbounds[1] = 0.0;

  for(i=1; i<numPts; i++) 
    {
    x1 = this->Points->GetPoint(i);
    for(j=0; j<3; j++) p[j] = x1[j] - p0[j];
    s = math.Dot(p,p10) / l10;
    t = math.Dot(p,p20) / l20;
    sbounds[0] = (s<sbounds[0]?s:sbounds[0]);
    sbounds[1] = (s>sbounds[1]?s:sbounds[1]);
    tbounds[0] = (t<tbounds[0]?t:tbounds[0]);
    tbounds[1] = (t>tbounds[1]?t:tbounds[1]);
    }
//
//  Re-evaluate coordinate system
//
  for (i=0; i<3; i++) 
    {
    p1[i] = p0[i] + sbounds[1]*p10[i] + tbounds[0]*p20[i];
    p2[i] = p0[i] + sbounds[0]*p10[i] + tbounds[1]*p20[i];
    p0[i] = p0[i] + sbounds[0]*p10[i] + tbounds[0]*p20[i];
    p10[i] = p1[i] - p0[i];
    p20[i] = p2[i] - p0[i];
    }
  l10 = math.Norm(p10);
  l20 = math.Norm(p20);

  return 1;
}

//
//  Function uses ray-casting to determine if point is inside polygon.
//  Works for arbitrary polygon shape (e.g., non-convex).
//
#define CERTAIN 1
#define UNCERTAIN 0
#define RAY_TOL 1.e-03 //Tolerance for ray firing
#define MAX_ITER 10    //Maximum iterations for ray-firing
#define VOTE_THRESHOLD 2

#define FALSE 0
#define TRUE 1

int vlPolygon::PointInPolygon (float bounds[6], float *x, float *n)
{
  float *x1, *x2, xray[3], u, v;
  float rayMag, mag, ray[3];
  int testResult, rayOK, status, numInts, i;
  int iterNumber;
  int maxComp, comps[2];
  float d2;
  int deltaVotes;
  vlMath math;
  vlLine line;
  int numPts=this->Points->NumberOfPoints();
//
//  Define a ray to fire.  The ray is a random ray normal to the
//  normal of the face.  The length of the ray is a function of the
//  size of the face bounding box.
//
  for (i=0; i<3; i++) ray[i] = ( bounds[2*i+1] - bounds[2*i] )*1.1;

  if ( (rayMag = math.Norm(ray)) == 0.0 )
    return OUTSIDE;
//
//  Get the maximum component of the normal.
//
  if ( fabs(n[0]) > fabs(n[1]) )
    {
    if ( fabs(n[0]) > fabs(n[2]) ) 
      {
      maxComp = 0;
      comps[0] = 1;
      comps[1] = 2;
      } 
    else 
      {
      maxComp = 2;
      comps[0] = 0;
      comps[1] = 1;
      }
    }
  else
    {
    if ( fabs(n[1]) > fabs(n[2]) ) 
      {
      maxComp = 1;
      comps[0] = 0;
      comps[1] = 2;
      } 
    else 
      {
      maxComp = 2;
      comps[0] = 0;
      comps[1] = 1;
      }
    }
//
//  Check that max component is non-zero
//
  if ( n[maxComp] == 0.0 )
    return FAILURE;
//
//  Enough information has been acquired to determine the random ray.
//  Random rays are generated until one is satisfactory (i.e.,
//  produces a ray of non-zero magnitude).  Also, since more than one
//  ray may need to be fired, the ray-firing occurs in a large loop.
//
//  The variable iterNumber counts the number of iterations and is
//  limited by the defined variable MAX_ITER.
//
//  The variable deltaVotes keeps track of the number of votes for
//  "in" versus "out" of the face.  When delta_vote > 0, more votes
//  have counted for "in" than "out".  When delta_vote < 0, more votes
//  have counted for "out" than "in".  When the delta_vote exceeds or
//  equals the defined variable VOTE_THRESHOLD, than the appropriate
//  "in" or "out" status is returned.
//
  for (deltaVotes = 0, iterNumber = 1;
  (iterNumber < MAX_ITER) && (fabs(deltaVotes) < VOTE_THRESHOLD);
  iterNumber++) 
    {
//
//  Generate ray
//
    for (rayOK = FALSE; rayOK == FALSE; ) 
      {
      ray[comps[0]] = math.Random(-rayMag, rayMag);
      ray[comps[1]] = math.Random(-rayMag, rayMag);
      ray[maxComp] = -(n[comps[0]]*ray[comps[0]] + 
                        n[comps[1]]*ray[comps[1]]) / n[maxComp];
      if ( (mag = math.Norm(ray)) > rayMag*TOL ) rayOK = TRUE;
      }
//
//  The ray must be appropriately sized.
//
      for (i=0; i<3; i++) xray[i] = x[i] + (rayMag/mag)*ray[i];
//
//  The ray may now be fired against all the edges
//
      for (numInts=0, testResult=CERTAIN, i=0; i<numPts; i++) 
        {
        x1 = this->Points->GetPoint(i);
        x2 = this->Points->GetPoint((i+1)%numPts);
//
//   Fire the ray and compute the number of intersections.  Be careful of 
//   degenerate cases (e.g., ray intersects at vertex).
//
        if (line.Intersection(x,xray,x1,x2,u,v) == INTERSECTION) 
          {
          if ( (RAY_TOL < v) && (v < 1.0-RAY_TOL) )
            numInts++;
          else
            testResult = UNCERTAIN;
          } 
        else if ( status == ON_LINE )
          {
          testResult = UNCERTAIN;
          }
        }
      if ( testResult == CERTAIN ) 
        {
        if ( (numInts % 2) == 0)
          --deltaVotes;
        else
          ++deltaVotes;
        }
    } /* try another ray */
//
//   If the number of intersections is odd, the point is in the polygon.
//
  if ( deltaVotes < 0 )
    return OUTSIDE;
  else
    return INSIDE;
}
