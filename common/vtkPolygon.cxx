/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolygon.cc
  Language:  C++
  Date:      11/01/95
  Version:   1.31


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include <stdlib.h>
#include "vtkPolygon.h"
#include "vtkMath.h"
#include "vtkLine.h"
#include "vtkPlane.h"
#include "vtkDataSet.h"
#include "vtkTriangle.h"
#include "vtkCellArray.h"

static vtkPlane plane;

// Description:
// Deep copy of cell.
vtkPolygon::vtkPolygon(const vtkPolygon& p)
{
  this->Points = p.Points;
  this->PointIds = p.PointIds;
}

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

// Description:
// Compute the polygon normal from a points list, and a list of point ids
// that index into the points list. This version will handle non-convex
// polygons.
void vtkPolygon::ComputeNormal(vtkPoints *p, int numPts, int *pts, float *n)
{
  int i;
  float v0[3], v1[3], v2[3];
  float ax, ay, az, bx, by, bz;
// 
// Check for special triangle case. Saves extra work.
// 
  if ( numPts == 3 ) 
    {
    p->GetPoint(pts[0],v0);
    p->GetPoint(pts[1],v1);
    p->GetPoint(pts[2],v2);
    vtkTriangle::ComputeNormal(v0, v1, v2, n);
    return;
    }
//
//  Because polygon may be concave, need to accumulate cross products to 
//  determine true normal.
//
  p->GetPoint(pts[0],v1); //set things up for loop
  p->GetPoint(pts[1],v2);
  n[0] = n[1] = n[2] = 0.0;

  for (i=0; i < numPts; i++) 
    {
    v0[0] = v1[0]; v0[1] = v1[1]; v0[2] = v1[2];
    v1[0] = v2[0]; v1[1] = v2[1]; v1[2] = v2[2];
    p->GetPoint(pts[(i+2)%numPts],v2);

    // order is important!!! to maintain consistency with polygon vertex order 
    ax = v2[0] - v1[0]; ay = v2[1] - v1[1]; az = v2[2] - v1[2];
    bx = v0[0] - v1[0]; by = v0[1] - v1[1]; bz = v0[2] - v1[2];

    n[0] += (ay * bz - az * by);
    n[1] += (az * bx - ax * bz);
    n[2] += (ax * by - ay * bx);
    }

  vtkMath::Normalize(n);
}

// Description:
// Compute the polygon normal from a list of floating points. This version
// will handle non-convex polygons.
void vtkPolygon::ComputeNormal(vtkFloatPoints *p, float *n)
{
  int i, numPts;
  float *v0, *v1, *v2;
  float ax, ay, az, bx, by, bz;
//
// Polygon is assumed non-convex -> need to accumulate cross products to 
// find correct normal.
//
  numPts = p->GetNumberOfPoints();
  v1 = p->GetPoint(0); //set things up for loop
  v2 = p->GetPoint(1);
  n[0] = n[1] = n[2] = 0.0;

  for (i=0; i < numPts; i++) 
    {
    v0 = v1;
    v1 = v2;
    v2 = p->GetPoint((i+2)%numPts);

    // order is important!!! to maintain consistency with polygon vertex order 
    ax = v2[0] - v1[0]; ay = v2[1] - v1[1]; az = v2[2] - v1[2];
    bx = v0[0] - v1[0]; by = v0[1] - v1[1]; bz = v0[2] - v1[2];

    n[0] += (ay * bz - az * by);
    n[1] += (az * bx - ax * bz);
    n[2] += (ax * by - ay * bx);
    }//over all points

  vtkMath::Normalize(n);
}

// Description:
// Compute the polygon normal from an array of points. This version assumes that
// the polygon is convex, and looks for the first valid normal.
void vtkPolygon::ComputeNormal (int numPts, float *pts, float n[3])
{
  int i;
  float *v1, *v2, *v3;
  float length;
  float ax, ay, az;
  float bx, by, bz;
//
//  Because some polygon vertices are colinear, need to make sure
//  first non-zero normal is found.
//
  v1 = pts;
  v2 = pts + 3;
  v3 = pts + 6;

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
      v3 = pts + 9 + 3*i;
      }
    } //over all points
}


int vtkPolygon::EvaluatePosition(float x[3], float closestPoint[3],
				 int& vtkNotUsed(subId), float pcoords[3], 
				 float& minDist2, float *weights)
{
  int i;
  float p0[3], p10[3], l10, p20[3], l20, n[3];
  float ray[3];

  this->ParameterizePolygon(p0, p10, l10, p20, l20, n);
  this->ComputeWeights(x,weights);
  vtkPlane::ProjectPoint(x,p0,n,closestPoint);

  for (i=0; i<3; i++) ray[i] = closestPoint[i] - p0[i];
  pcoords[0] = vtkMath::Dot(ray,p10) / (l10*l10);
  pcoords[1] = vtkMath::Dot(ray,p20) / (l20*l20);

  if ( pcoords[0] >= 0.0 && pcoords[0] <= 1.0 &&
  pcoords[1] >= 0.0 && pcoords[1] <= 1.0 &&
  (this->PointInPolygon(closestPoint, this->Points.GetNumberOfPoints(), 
  this->Points.GetPointer(0), this->GetBounds(),n) == INSIDE) )
    {
    minDist2 = vtkMath::Distance2BetweenPoints(x,closestPoint);
    return 1;
    }
//
// If here, point is outside of polygon, so need to find distance to boundary
//
  else
    {
    float t, dist2;
    int numPts;
    float closest[3];

    numPts = this->Points.GetNumberOfPoints();
    for (minDist2=VTK_LARGE_FLOAT,i=0; i<numPts; i++)
      {
      dist2 = vtkLine::DistanceToLine(x,this->Points.GetPoint(i),
                                  this->Points.GetPoint((i+1)%numPts),t,closest);
      if ( dist2 < minDist2 )
        {
        closestPoint[0] = closest[0]; 
        closestPoint[1] = closest[1]; 
        closestPoint[2] = closest[2];
        minDist2 = dist2;
        }
      }
    return 0;
    }
}

void vtkPolygon::EvaluateLocation(int& vtkNotUsed(subId), float pcoords[3], 
				  float x[3], float *weights)
{
  int i;
  float p0[3], p10[3], l10, p20[3], l20, n[3];

  this->ParameterizePolygon(p0, p10, l10, p20, l20, n);
  for (i=0; i<3; i++)
    {
    x[i] = p0[i] + pcoords[0]*p10[i] + pcoords[1]*p20[i];
    }

  this->ComputeWeights(x,weights);
}

// Description:
// Create a local s-t coordinate system for a polygon. The point p0 is
// the origin of the local system, p10 is s-axis vector, and p20 is the 
// t-axis vector. (These are expressed in the modelling coordinate system and
// are vectors of dimension [3].) The values l20 and l20 are the lengths of
// the vectors p10 and p20, and n is the polygon normal.
int vtkPolygon::ParameterizePolygon(float *p0, float *p10, float& l10, 
                                   float *p20,float &l20, float *n)
{
  int i, j;
  float s, t, p[3], p1[3], p2[3], sbounds[2], tbounds[2];
  int numPts=this->Points.GetNumberOfPoints();
  float *x1, *x2;
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
  vtkMath::Cross (n,p10,p20);
//
// Determine lengths of edges
//
  if ( (l10=vtkMath::Dot(p10,p10)) == 0.0 || (l20=vtkMath::Dot(p20,p20)) == 0.0 )
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
#ifdef BAD_WITH_NODEBUG
    s = vtkMath::Dot(p,p10) / l10;
    t = vtkMath::Dot(p,p20) / l20;
#endif
    s = (p[0]*p10[0] + p[1]*p10[1] + p[2]*p10[2]) / l10;
    t = (p[0]*p20[0] + p[1]*p20[1] + p[2]*p20[2]) / l20;
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
  l10 = vtkMath::Norm(p10);
  l20 = vtkMath::Norm(p20);

  return 1;
}

#define CERTAIN 1
#define UNCERTAIN 0
#define RAY_TOL 1.e-03 //Tolerance for ray firing
#define MAX_ITER 10    //Maximum iterations for ray-firing
#define VOTE_THRESHOLD 2

#ifndef TRUE
#define FALSE 0
#define TRUE 1
#endif

// Description:
// Determine whether point is inside polygon.
// Function uses ray-casting to determine if point is inside polygon.
// Works for arbitrary polygon shape (e.g., non-convex).
int vtkPolygon::PointInPolygon (float x[3], int numPts, float *pts, 
                                float bounds[6], float *n)
{
  float *x1, *x2, xray[3], u, v;
  float rayMag, mag=1, ray[3];
  int testResult, rayOK, status, numInts, i;
  int iterNumber;
  int maxComp, comps[2];
  int deltaVotes;
//
//  Define a ray to fire.  The ray is a random ray normal to the
//  normal of the face.  The length of the ray is a function of the
//  size of the face bounding box.
//
  for (i=0; i<3; i++) ray[i] = ( bounds[2*i+1] - bounds[2*i] )*1.1;

  if ( (rayMag = vtkMath::Norm(ray)) == 0.0 )
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
  (iterNumber < MAX_ITER) && (abs(deltaVotes) < VOTE_THRESHOLD);
  iterNumber++) 
    {
//
//  Generate ray
//
    for (rayOK = FALSE; rayOK == FALSE; ) 
      {
      ray[comps[0]] = vtkMath::Random(-rayMag, rayMag);
      ray[comps[1]] = vtkMath::Random(-rayMag, rayMag);
      ray[maxComp] = -(n[comps[0]]*ray[comps[0]] + 
                        n[comps[1]]*ray[comps[1]]) / n[maxComp];
      if ( (mag = vtkMath::Norm(ray)) > rayMag*VTK_TOL ) rayOK = TRUE;
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
        x1 = pts + 3*i;
        x2 = pts + 3*((i+1)%numPts);
//
//   Fire the ray and compute the number of intersections.  Be careful of 
//   degenerate cases (e.g., ray intersects at vertex).
//
        if ((status=vtkLine::Intersection(x,xray,x1,x2,u,v)) == INTERSECTION) 
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
    } //try another ray
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

#define TOLERANCE 1.0e-06

static  float   Tolerance; // Intersection tolerance
static  int     SuccessfulTriangulation; // Stops recursive tri. if necessary
static  float   Normal[3]; //polygon normal

// Description:
// Triangulate polygon. Tries to use the fast triangulation technique 
// first, and if that doesn't work, uses more complex routine that is
//  guaranteed to work.
int vtkPolygon::Triangulate(vtkIdList &outTris)
{
  int i, success;
  float *bounds, d;
  int numVerts=this->PointIds.GetNumberOfIds();
  int *verts = new int[numVerts];
  static vtkIdList Tris((VTK_CELL_SIZE-2)*3);

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
    float planeNormal[3];
    this->ComputeNormal (&this->Points,planeNormal);
    this->SlowTriangulate(numVerts, verts, planeNormal, Tris);
    }
  else // Copy the point id's into the supplied Id array
    {
    for (i=0; i<Tris.GetNumberOfIds(); i++)
      {
      outTris.InsertId(i,this->PointIds.GetId(Tris.GetId(i)));
      }
    }

  delete [] verts;
  
  return 1;
}

// Description: 
// A fast triangulation method. Uses recursive divide and 
// conquer based on plane splitting  to reduce loop into triangles.  
// The cell (e.g., triangle) is presumed properly initialized (i.e., 
// Points and PointIds).
int vtkPolygon::FastTriangulate (int numVerts, int *verts, vtkIdList& Tris)
{
  int i,j;
  int n1, n2;
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
      {
      int *l1 = new int[numVerts], *l2 = new int[numVerts];

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

        delete [] l1;
        delete [] l2;
        return 1;

        }
  
      SuccessfulTriangulation = 0;

      delete [] l1;
      delete [] l2;
      return 0;
      }
    }
}

// Description:
// Determine whether the loop can be split / build loops
int vtkPolygon::CanSplitLoop (int fedges[2], int numVerts, int *verts, 
                             int& n1, int *l1, int& n2, int *l2, float& ar)
{
  int i, sign;
  float *x, val, absVal, *sPt, *s2Pt, v21[3], sN[3];
  float den, dist=VTK_LARGE_FLOAT;
  void SplitLoop();
//
//  Create two loops from the one using the splitting vertices provided.
//
  this->SplitLoop (fedges, numVerts, verts, n1, l1, n2, l2);
//
//  Create splitting plane.  Splitting plane is parallel to the loop
//  plane normal and contains the splitting vertices fedges[0] and fedges[1].
//
  sPt = this->Points.GetPoint(fedges[0]);
  s2Pt = this->Points.GetPoint(fedges[1]);
  for (i=0; i<3; i++) v21[i] = s2Pt[i] - sPt[i];

  vtkMath::Cross (v21,Normal,sN);
  if ( (den=vtkMath::Norm(sN)) != 0.0 )
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
      val = vtkPlane::Evaluate(sN,sPt,x);
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
      val = vtkPlane::Evaluate(sN,sPt,x);
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

// Description:
// Creates two loops from splitting plane provided
void vtkPolygon::SplitLoop (int fedges[2], int numVerts, int *verts, 
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

// Description: 
// Polygon triangulation that will always work - although may be slow.
// Uses explicit analysis technique to split triangles into loops, which are
// recursively split until only triangles remain. Very similar to the 
// FastTriangulate method, except that extra edge intersection and 
// containment tests are used to insure a valid splitting edge.
int vtkPolygon::SlowTriangulate (int numVerts, int *verts, 
                                 float planeNormal[3], vtkIdList& Tris)
{
  int i,j;
  int n1, n2;
  int fedges[2];
  float max, ar;
  int maxI, maxJ;

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
      //  done by finding the best splitting edge and creating two loop and 
      //  recursively triangulating.  To find the best splitting edge, try
      //  all possible combinations, keeping track of the one that gives the
      //  largest dihedral angle combination.
      //
    default:
      {
      int *l1 = new int[numVerts], *l2 = new int[numVerts];

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

      delete [] l1;
      delete [] l2;
      return 0;
      }
    }
}

int vtkPolygon::CellBoundary(int vtkNotUsed(subId), float pcoords[3], 
                             vtkIdList& pts)
{
  int i, numPts=this->PointIds.GetNumberOfIds();
  float x[3], *weights, closest[3];
  int closestPoint=0, previousPoint, nextPoint;
  float largestWeight=0.0;
  float p0[3], p10[3], l10, p20[3], l20, n[3];

  pts.Reset();
  weights = new float[numPts];

  // determine global coordinates given parametric coordinates
  this->ParameterizePolygon(p0, p10, l10, p20, l20, n);
  for (i=0; i<3; i++)
    {
    x[i] = p0[i] + pcoords[0]*p10[i] + pcoords[1]*p20[i];
    }

  //find edge with largest and next largest weight values. This will be
  //the closest edge.
  this->ComputeWeights(x,weights);
  for ( i=0; i < numPts; i++ )
    {
    if ( weights[i] > largestWeight )
      {
      closestPoint = i;
      largestWeight = weights[i];
      }
    }

  pts.InsertId(0,this->PointIds.GetId(closestPoint));

  previousPoint = closestPoint - 1;
  nextPoint = closestPoint + 1;
  if ( previousPoint < 0 ) previousPoint = numPts - 1;
  if ( nextPoint >= numPts ) nextPoint = 0;

  if ( weights[previousPoint] > weights[nextPoint] )
    {
    pts.InsertId(1,this->PointIds.GetId(previousPoint));
    }
  else
    {
    pts.InsertId(1,this->PointIds.GetId(nextPoint));
    }
  delete [] weights;

  // determine whether point is inside of polygon
  if ( pcoords[0] >= 0.0 && pcoords[0] <= 1.0 &&
  pcoords[1] >= 0.0 && pcoords[1] <= 1.0 &&
  (this->PointInPolygon(closest, this->Points.GetNumberOfPoints(), 
  this->Points.GetPointer(0), this->GetBounds(),n) == INSIDE) )
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

void vtkPolygon::Contour(float value, vtkFloatScalars *cellScalars, 
                        vtkPointLocator *locator,
                        vtkCellArray *verts, vtkCellArray *lines, 
                        vtkCellArray *polys,
                        vtkPointData *inPd, vtkPointData *outPd)
{
  int i, success;
  int numVerts=this->Points.GetNumberOfPoints();
  float *bounds, d;
  static vtkTriangle tri;
  static vtkIdList Tris((VTK_CELL_SIZE-2)*3);
  static vtkFloatScalars triScalars(3);
  int *polyVerts = new int[numVerts], p1, p2, p3;

  triScalars.SetNumberOfScalars(3);

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
    }
  else // Contour triangle
    {
    for (i=0; i<Tris.GetNumberOfIds(); i += 3)
      {
      p1 = Tris.GetId(i);
      p2 = Tris.GetId(i+1);
      p3 = Tris.GetId(i+2);

      tri.Points.SetPoint(0,this->Points.GetPoint(p1));
      tri.Points.SetPoint(1,this->Points.GetPoint(p2));
      tri.Points.SetPoint(2,this->Points.GetPoint(p3));

      if ( outPd )
        {
        tri.PointIds.SetId(0,this->PointIds.GetId(p1));
        tri.PointIds.SetId(1,this->PointIds.GetId(p2));
        tri.PointIds.SetId(2,this->PointIds.GetId(p3));
        }

      triScalars.SetScalar(0,cellScalars->GetScalar(p1));
      triScalars.SetScalar(1,cellScalars->GetScalar(p2));
      triScalars.SetScalar(2,cellScalars->GetScalar(p3));

      tri.Contour(value, &triScalars, locator, verts,
                   lines, polys, inPd, outPd);
      }
    }
  delete [] polyVerts;
}

vtkCell *vtkPolygon::GetEdge(int edgeId)
{
  int numPts=this->Points.GetNumberOfPoints();
  static vtkLine line;

  // load point id's
  line.PointIds.SetId(0,this->PointIds.GetId(edgeId));
  line.PointIds.SetId(1,this->PointIds.GetId((edgeId+1) % numPts));

  // load coordinates
  line.Points.SetPoint(0,this->Points.GetPoint(edgeId));
  line.Points.SetPoint(1,this->Points.GetPoint((edgeId+1) % numPts));

  return &line;
}

//
// Description:
// Compute interpolation weights using 1/r**2 normalized sum.
//
void vtkPolygon::ComputeWeights(float x[3], float *weights)
{
  int i;
  int numPts=this->Points.GetNumberOfPoints();
  float maxDist2, sum, *pt;

  for (sum=0.0, maxDist2=0.0, i=0; i<numPts; i++)
    {
    pt = this->Points.GetPoint(i);
    weights[i] = vtkMath::Distance2BetweenPoints(x,pt);
    if ( weights[i] == 0.0 ) //exact hit
      {
      for (int j=0; j<numPts; j++) weights[j] = 0.0;
      weights[i] = 1.0;
      return;
      }
    else
      {
      weights[i] = 1.0 / (weights[i]*weights[i]);
      sum += weights[i];
      }
    }

  for (i=0; i<numPts; i++) weights[i] /= sum;
}

//
// Intersect this plane with finite line defined by p1 & p2 with tolerance tol.
//
int vtkPolygon::IntersectWithLine(float p1[3], float p2[3], float tol,float& t,
                                 float x[3], float pcoords[3], int& subId)
{
  float *pt1, *pt2, *pt3, n[3];
  float tol2 = tol*tol;
  float closestPoint[3];
  float dist2;
  int npts = this->GetNumberOfPoints();
  float *weights = new float[npts];

  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;
//
// Get normal for triangle
//
  pt1 = this->Points.GetPoint(1);
  pt2 = this->Points.GetPoint(2);
  pt3 = this->Points.GetPoint(0);

  this->ComputeNormal (&this->Points,n);
//
// Intersect plane of triangle with line
//
  if ( ! vtkPlane::IntersectWithLine(p1,p2,n,pt1,t,x) ) return 0;
//
// Evaluate position
//
  if ( this->EvaluatePosition(x, closestPoint, subId, pcoords, dist2, weights) )
    if ( dist2 <= tol2 ) return 1;

  return 0;

}

int vtkPolygon::Triangulate(int vtkNotUsed(index), vtkIdList &ptIds, 
                            vtkFloatPoints &pts)
{
  int i, success;
  float *bounds, d;
  int numVerts=this->PointIds.GetNumberOfIds();
  int *verts = new int[numVerts];
  static vtkIdList Tris((VTK_CELL_SIZE-2)*3);

  pts.Reset();
  ptIds.Reset();

  bounds = this->GetBounds();
  d = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
           (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
           (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));
  Tolerance = TOLERANCE * d;
  SuccessfulTriangulation = 1;
  this->ComputeNormal(&this->Points, Normal);

  for (i=0; i<numVerts; i++) verts[i] = i;
  Tris.Reset();

  success = this->FastTriangulate(numVerts, verts, Tris);

  if ( !success ) // Use slower but always successful technique.
    {
    vtkErrorMacro(<<"Couldn't triangulate");
    }
  else // Copy the point id's into the supplied Id array
    {
    for (i=0; i<Tris.GetNumberOfIds(); i++)
      {
      ptIds.InsertId(i,this->PointIds.GetId(Tris.GetId(i)));
      pts.InsertPoint(i,this->Points.GetPoint(Tris.GetId(i)));
      }
    }

  delete [] verts;
  return success;
}

// Sample at three points to compute derivatives in local r-s coordinate 
// system. The project vectors into 3D model coordinate system.
#define VTK_SAMPLE_DISTANCE 0.01
void vtkPolygon::Derivatives(int vtkNotUsed(subId), float pcoords[3], 
                             float *values, int dim, float *derivs)
{
  int i, j, k, idx;
  float p0[3], p10[3], l10, p20[3], l20, n[3];
  float x[3][3], l1, l2, v1[3], v2[3];
  int numVerts=this->PointIds.GetNumberOfIds();
  float *weights = new float[numVerts];
  float *sample = new float[dim*3];

  //setup parametric system and check for degeneracy
  if ( this->ParameterizePolygon(p0, p10, l10, p20, l20, n) == 0 )
    {
    for ( j=0; j < dim; j++ )
      for ( i=0; i < 3; i++ )
        derivs[j*dim + i] = 0.0;
    return;
    }

  //compute positions of three sample points
  for (i=0; i<3; i++)
    {
    x[0][i] = p0[i] + pcoords[0]*p10[i] + pcoords[1]*p20[i];
    x[1][i] = p0[i] + (pcoords[0]+VTK_SAMPLE_DISTANCE)*p10[i] + 
              pcoords[1]*p20[i];
    x[2][i] = p0[i] + pcoords[0]*p10[i] + 
              (pcoords[1]+VTK_SAMPLE_DISTANCE)*p20[i];
    }

  //for each sample point, sample data values
  for ( idx=0, k=0; k < 3; k++ ) //loop over three sample points
    {
    this->ComputeWeights(x[k],weights);
    for ( j=0; j < dim; i++, idx++) //over number of derivates requested
      {
      sample[idx] = 0.0;
      for ( i=0; i < numVerts; i++ )
        {
        sample[idx] += weights[i] * values[j + i*dim];
        }
      }
    }

  //compute differences along the two axes
  for ( i=0; i < 3; i++ )
    {
    v1[i] = x[1][i] - x[0][i];
    v2[i] = x[2][i] - x[0][i];
    }
  l1 = vtkMath::Normalize(v1);
  l2 = vtkMath::Normalize(v2);

  //compute derivatives along x-y-z axes
  for ( j=0; j < dim; j++ )
    {
    l1 = (sample[dim+j] - sample[j]) / l1;
    l2 = (sample[2*dim+j] - sample[j]) / l2;
    derivs[3*j]     = l1 * v1[0] + l2 * v2[0];
    derivs[3*j + 1] = l1 * v1[1] + l2 * v2[1];
    derivs[3*j + 2] = l1 * v1[2] + l2 * v2[2];
    }

  delete [] weights;
  delete [] sample;
}

void vtkPolygon::Clip(float value, vtkFloatScalars *cellScalars, 
                      vtkPointLocator *locator, vtkCellArray *tris,
                      vtkPointData *inPD, vtkPointData *outPD,
                      int insideOut)
{
  int i, success;
  int numVerts=this->Points.GetNumberOfPoints();
  float *bounds, d;
  static vtkTriangle tri;
  static vtkIdList Tris(VTK_CELL_SIZE);
  static vtkFloatScalars triScalars(3);
  int *polyVerts = new int[numVerts], p1, p2, p3;

  triScalars.SetNumberOfScalars(3);

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
    }
  else // clip triangles
    {
    for (i=0; i<Tris.GetNumberOfIds(); i += 3)
      {
      p1 = Tris.GetId(i);
      p2 = Tris.GetId(i+1);
      p3 = Tris.GetId(i+2);

      tri.Points.SetPoint(0,this->Points.GetPoint(p1));
      tri.Points.SetPoint(1,this->Points.GetPoint(p2));
      tri.Points.SetPoint(2,this->Points.GetPoint(p3));

      tri.PointIds.SetId(0,this->PointIds.GetId(p1));
      tri.PointIds.SetId(1,this->PointIds.GetId(p2));
      tri.PointIds.SetId(2,this->PointIds.GetId(p3));

      triScalars.SetScalar(0,cellScalars->GetScalar(p1));
      triScalars.SetScalar(1,cellScalars->GetScalar(p2));
      triScalars.SetScalar(2,cellScalars->GetScalar(p3));

      tri.Clip(value, &triScalars, locator, tris, inPD, outPD, insideOut);
      }
    }
  delete [] polyVerts;
}

// Description:
// Method intersects two polygons. You must supply the number of points and
// point coordinates (npts, *pts) and the bounding box (bounds) of the two
// polygons. Also supply a tolerance squared for controlling error.
int vtkPolygon::IntersectPolygonWithPolygon(int npts, float *pts,float bounds[6],
                                            int npts2, float *pts2, 
                                            float bounds2[6], float tol2)
{
  float n[3], x[3], coords[3];
  int i, j, retStat;
  float *p1, *p2, ray[3];
  float t;
//
//  Intersect each edge of first polygon against second
//
  vtkPolygon::ComputeNormal(npts2, pts2, n);

  for (i=0; i<npts; i++) 
    {
    p1 = pts + 3*i;
    p2 = pts + 3*((i+1)%npts);

    for (j=0; j<3; j++) ray[j] = p2[j] - p1[j];
    if ( ! vtkCell::HitBBox(bounds2, p1, ray, coords, t) )
      continue;

    if ( (retStat=vtkPlane::IntersectWithLine(p1,p2,n,pts2,t,x)) == 1 ) 
      {
      if ( (npts2==3 && vtkTriangle::PointInTriangle(x,pts2,pts2+3,pts2+6,tol2))
      || (npts2>3 && vtkPolygon::PointInPolygon(x,npts2,pts2,bounds2,n)==INSIDE))
        {
        return 1;
        }
      } 
    else //if ( retStat == ON_PLANE ) 
      {
      return 0;
      }
    }
//
//  Intersect each edge of second polygon against first
//
  vtkPolygon::ComputeNormal(npts, pts, n);

  for (i=0; i<npts2; i++) 
    {
    p1 = pts2 + 3*i;
    p2 = pts2 + 3*((i+1)%npts2);

    for (j=0; j<3; j++) ray[j] = p2[j] - p1[j];

    if ( ! vtkCell::HitBBox(bounds, p1, ray, coords, t) )
      continue;

    if ( (retStat=vtkPlane::IntersectWithLine(p1,p2,n,pts,t,x)) == 1 ) 
      {
      if ( (npts==3 && vtkTriangle::PointInTriangle(x,pts,pts+3,pts+6,tol2))
      || (npts>3 && vtkPolygon::PointInPolygon(x,npts,pts,bounds,n)==INSIDE))
        {
        return 1;
        }
      } 
    else //if ( retStat == ON_PLANE ) 
      {
      return 0;
      }
    }

  return 0;
}

