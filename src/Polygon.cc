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
#include "DataSet.hh"
#include "Triangle.hh"

#define FAILURE 0
#define INTERSECTION 2
#define OUTSIDE 3
#define INSIDE 4
#define ON_LINE 6

//
// In many of the functions that follow, the Points and PointIds members 
// of the Cell are assumed initialized.  This is usually done indirectly
// through the GetCell(id) method in the DataSet objects.
//

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
  numPts = p->GetNumberOfPoints();
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
  pcoords[0] = math.Dot(ray,p10) / (l10*l10);
  pcoords[1] = math.Dot(ray,p20) / (l20*l20);

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

  numPts = this->Points.GetNumberOfPoints();
  for (minDist2=LARGE_FLOAT,i=0; i<numPts - 1; i++)
    {
    line.Points.SetPoint(0,this->Points.GetPoint(i));
    line.Points.SetPoint(1,this->Points.GetPoint(i+1));
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
  int numPts=this->Points.GetNumberOfPoints();
  float *x1, *x2;
  vlMath math;
//
//  This is a two pass process: first create a p' coordinate system
//  that is then adjusted to insure that the polygon points are all in
//  the range 0<=s,t<=1.  The p' system is defined by the polygon normal, 
//  first vertex and the first edge.
//
  this->ComputeNormal (&this->Points,n);
  x1 = this->Points.GetPoint(0);
  x2 = this->Points.GetPoint(1);
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
    x1 = this->Points.GetPoint(i);
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
  int numPts=this->Points.GetNumberOfPoints();
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
        x1 = this->Points.GetPoint(i);
        x2 = this->Points.GetPoint((i+1)%numPts);
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
//
// Following is used in a number of routines.  Made static to avoid 
// constructor / destructor calls.
//
static vlIdList Tris((MAX_CELL_SIZE-2)*3);

#define TOLERANCE 1.0e-06

static  float   Tolerance; // Intersection tolerance
static  int     SuccessfulTriangulation; // Stops recursive tri. if necessary
static  float   Normal[3]; //polygon normal

//
//  General triangulation entry point.  Tries to use the fast triangulation 
//  technique first, and if that doesn't work, something more complex but
//  guaranteed to work.
//
int vlPolygon::Triangulate(vlIdList &outTris)
{
  int i, success;
  float *bounds, d;
  int verts[MAX_CELL_SIZE];
  int numVerts=this->PointIds.GetNumberOfIds();

  bounds = this->GetBounds();
  
  d = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
           (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
           (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));
  Tolerance = TOLERANCE * d;
  SuccessfulTriangulation = 1;
  this->ComputeNormal(&this->Points, Normal);

  for (i=0; i<numVerts; i++) verts[i] = i;
  Tris.Reset();
  outTris.Reset();

  success = this->FastTriangulate(numVerts, verts, Tris);

  if ( !success ) // Use slower but always successful technique.
    {
    vlErrorMacro(<<"Couldn't triangulate");
    }
  else // Copy the point id's into the supplied Id array
    {
    for (i=0; i<Tris.GetNumberOfIds(); i++)
      {
      outTris.InsertId(i,this->PointIds.GetId(Tris.GetId(i)));
      }
    }
}

//
//  Triangulate loop.  Use recursive divide and conquer based on plane 
//  splitting  to reduce loop into triangles.  The cell is presumed 
//  properly initialized (i.e., Points and PointIds).
//
int vlPolygon::FastTriangulate (int numVerts, int *verts, vlIdList& Tris)
{
  int i,j;
  int n1, n2;
  int l1[MAX_CELL_SIZE], l2[MAX_CELL_SIZE];
  int fedges[2];
  float max, ar;
  int maxI, maxJ;

  if ( !SuccessfulTriangulation )
    return 0;

  switch (numVerts) 
    {
//
//  In loops of less than 3 vertices no elements are created
//
    case 0: case 1: case 2:
      return 1;
//
//  A loop of three vertices makes one triangle!  Replace an old
//  polygon with a newly created one.  
//
    case 3:
//
//  Create a triangle
//
      Tris.InsertNextId(verts[0]);
      Tris.InsertNextId(verts[1]);
      Tris.InsertNextId(verts[2]);

      return 1;
//
//  Loops greater than three vertices must be subdivided.  This is
//  done by finding the best splitting plane and creating two loop and 
//  recursively triangulating.  To find the best splitting plane, try
//  all possible combinations, keeping track of the one that gives the
//  largest dihedral angle combination.
//
    default:
      max = 0.0;
      maxI = maxJ = -1;
      for (i=0; i<(numVerts-2); i++) 
        {
        for (j=i+2; j<numVerts; j++) 
          {
          if ( ((j+1) % numVerts) != i ) 
            {
            fedges[0] = verts[i];
            fedges[1] = verts[j];

            if ( this->CanSplitLoop(fedges, numVerts, verts, 
            n1, l1, n2, l2, ar) && ar > max ) 
              {
              max = ar;
              maxI = i;
              maxJ = j;
              }
            }
          }
        }

      if ( maxI > -1 ) 
        {
        fedges[0] = verts[maxI];
        fedges[1] = verts[maxJ];

        this->SplitLoop (fedges, numVerts, verts, n1, l1, n2, l2);

        this->FastTriangulate (n1, l1, Tris);
        this->FastTriangulate (n2, l2, Tris);

        return 1;

        }
  
      SuccessfulTriangulation = 0;

      return 0;
    }
}

//
//  Determine whether the loop can be split / build loops
//
int vlPolygon::CanSplitLoop (int fedges[2], int numVerts, int *verts, 
                             int& n1, int *l1, int& n2, int *l2, float& ar)
{
  int i, sign;
  float *x, val, absVal, *sPt, *s2Pt, v21[3], sN[3];
  float den, dist=LARGE_FLOAT;
  void SplitLoop();
  vlMath math;
  vlPlane plane;
//
//  Create two loops from the one using the splitting vertices provided.
//
  SplitLoop (fedges, numVerts, verts, n1, l1, n2, l2);
//
//  Create splitting plane.  Splitting plane is parallel to the loop
//  plane normal and contains the splitting vertices fedges[0] and fedges[1].
//
  sPt = this->Points.GetPoint(fedges[0]);
  s2Pt = this->Points.GetPoint(fedges[1]);
  for (i=0; i<3; i++) v21[i] = s2Pt[i] - sPt[i];

  math.Cross (v21,Normal,sN);
  if ( (den=math.Norm(sN)) != 0.0 )
    for (i=0; i<3; i++)
      sN[i] /= den;
  else
    return 0;
//
//  This plane can only be split if all points of each loop lie on the
//  same side of the splitting plane.  Also keep track of the minimum 
//  distance to the plane.
//
  for (sign=0, i=0; i < n1; i++) // first loop
    {
    if ( !(l1[i] == fedges[0] || l1[i] == fedges[1]) ) 
      {
      x = this->Points.GetPoint(l1[i]);
      val = plane.Evaluate(sN,sPt,x);
      absVal = (float) fabs((double)val);
      dist = (absVal < dist ? absVal : dist);
      if ( !sign )
        sign = (val > Tolerance ? 1 : -1);
      else if ( sign != (val > 0 ? 1 : -1) )
        return 0;
      }
    }

  sign *= -1;
  for (i=0; i < n2; i++) // second loop
    {
    if ( !(l2[i] == fedges[0] || l2[i] == fedges[1]) ) 
      {
      x = this->Points.GetPoint(l2[i]);
      val = plane.Evaluate(sN,sPt,x);
      absVal = (float) fabs((double)val);
      dist = (absVal < dist ? absVal : dist);
      if ( !sign )
        sign = (val > Tolerance ? 1 : -1);
      else if ( sign != (val > 0 ? 1 : -1) )
        return 0;
      }
    }
//
//  Compute aspect ratio
//
  ar = (dist*dist)/(v21[0]*v21[0] + v21[1]*v21[1] + v21[2]*v21[2]);
  return 1;
}

//
//  Creates two loops from splitting plane provided
//
void vlPolygon::SplitLoop (int fedges[2], int numVerts, int *verts, 
                           int& n1, int *l1, int& n2, int* l2)
{
  int i;
  int *loop;
  int *count;

  n1 = n2 = 0;
  loop = l1;
  count = &n1;

  for (i=0; i < numVerts; i++) 
    {
    loop[(*count)++] = verts[i];
    if ( verts[i] == fedges[0] || verts[i] == fedges[1] ) 
      {
      loop = (loop == l1 ? l2 : l1);
      count = (count == &n1 ? &n2 : &n1);
      loop[(*count)++] = verts[i];
      }
    }

  return;
}

void vlPolygon::Contour(float value, vlFloatScalars *cellScalars, 
                        vlFloatPoints *points,
                        vlCellArray *verts, vlCellArray *lines, 
                        vlCellArray *polys, vlFloatScalars *scalars)
{
  int i, success;
  int numVerts=this->Points.GetNumberOfPoints();
  float *bounds, d;
  int polyVerts[MAX_CELL_SIZE];
  static vlTriangle tri;
  static vlFloatScalars triScalars(3);

  bounds = this->GetBounds();
  
  d = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
           (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
           (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));
  Tolerance = TOLERANCE * d;
  SuccessfulTriangulation = 1;
  this->ComputeNormal(&this->Points, Normal);

  for (i=0; i<numVerts; i++) polyVerts[i] = i;
  Tris.Reset();

  success = this->FastTriangulate(numVerts, polyVerts, Tris);

  if ( !success ) // Just skip for now.
    {
    ;
    }
  else // Contour triangle
    {
    for (i=0; i<Tris.GetNumberOfIds(); i += 3)
      {
      tri.Points.SetPoint(0,this->Points.GetPoint(i));
      tri.Points.SetPoint(1,this->Points.GetPoint(i+1));
      tri.Points.SetPoint(2,this->Points.GetPoint(i+2));

      triScalars.SetScalar(0,cellScalars->GetScalar(i));
      triScalars.SetScalar(1,cellScalars->GetScalar(i+1));
      triScalars.SetScalar(2,cellScalars->GetScalar(i+2));

      tri.Contour(value, &triScalars, points, verts,
                   lines, polys, scalars);
      }
    }
}

