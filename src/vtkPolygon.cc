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
#include "vtkPolygon.hh"
#include "vtkMath.hh"
#include "vtkLine.hh"
#include "vtkPlane.hh"
#include "vtkDataSet.hh"
#include "vtkTriangle.hh"
#include "vtkCellArray.hh"

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
// that index into the points list.
void vtkPolygon::ComputeNormal(vtkPoints *p, int numPts, int *pts, float *n)
{
  int i;
  float v1[3], v2[3], v3[3];
  float length;
  float ax, ay, az;
  float bx, by, bz;
//
//  Because some polygon vertices are colinear, need to make sure
//  first non-zero normal is found.
//
  p->GetPoint(pts[0],v1);
  p->GetPoint(pts[1],v2);
  p->GetPoint(pts[2],v3);

  for (i=0; i<numPts; i++) 
    {
    // order is important!!! to maintain consistency with polygon vertex order 
    ax = v3[0] - v2[0]; ay = v3[1] - v2[1]; az = v3[2] - v2[2];
    bx = v1[0] - v2[0]; by = v1[1] - v2[1]; bz = v1[2] - v2[2];

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
      v1[0] = v2[0]; v1[1] = v2[1]; v1[2] = v2[2];
      v2[0] = v3[0]; v2[1] = v3[1]; v2[2] = v3[2];
      p->GetPoint(pts[(i+3)%numPts],v3);
      }
    }
}

// Description:
// Compute the polygon normal from three points.
void vtkPolygon::ComputeNormal(float *v1, float *v2, float *v3, float *n)
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

    length = sqrt((double) (n[0]*n[0] + n[1]*n[1] + n[2]*n[2]));

    if (length != 0.0) 
      {
      n[0] /= length;
      n[1] /= length;
      n[2] /= length;
      }
}

// Description:
// Compute the polygon normal from a list of floating points.
void vtkPolygon::ComputeNormal(vtkFloatPoints *p, float *n)
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
    ax = v3[0] - v2[0]; ay = v3[1] - v2[1]; az = v3[2] - v2[2];
    bx = v1[0] - v2[0]; by = v1[1] - v2[1]; bz = v1[2] - v2[2];

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

int vtkPolygon::EvaluatePosition(float x[3], float closestPoint[3],
				 int& vtkNotUsed(subId), float pcoords[3], 
				 float& minDist2, float *weights)
{
  int i;
  float p0[3], p10[3], l10, p20[3], l20, n[3];
  float ray[3];
  static vtkLine line;

  this->ParameterizePolygon(p0, p10, l10, p20, l20, n);
  this->ComputeWeights(x,weights);
  plane.ProjectPoint(x,p0,n,closestPoint);

  for (i=0; i<3; i++) ray[i] = closestPoint[i] - p0[i];
  pcoords[0] = vtkMath::Dot(ray,p10) / (l10*l10);
  pcoords[1] = vtkMath::Dot(ray,p20) / (l20*l20);

  if ( pcoords[0] >= 0.0 && pcoords[0] <= 1.0 &&
  pcoords[1] >= 0.0 && pcoords[1] <= 1.0 &&
  this->PointInPolygon(this->GetBounds(),closestPoint,n) == INSIDE )
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
    for (minDist2=VTK_LARGE_FLOAT,i=0; i<numPts - 1; i++)
      {
      dist2 = line.DistanceToLine(x,this->Points.GetPoint(i),
                                  this->Points.GetPoint(i+1),t,closest);
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
//  Create a local s-t coordinate system for a polygon
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
    s = vtkMath::Dot(p,p10) / l10;
    t = vtkMath::Dot(p,p20) / l20;
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
int vtkPolygon::PointInPolygon (float bounds[6], float *x, float *n)
{
  float *x1, *x2, xray[3], u, v;
  float rayMag, mag, ray[3];
  int testResult, rayOK, status, numInts, i;
  int iterNumber;
  int maxComp, comps[2];
  int deltaVotes;
  int numPts=this->Points.GetNumberOfPoints();
  static vtkLine line;
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
  (iterNumber < MAX_ITER) && (fabs(deltaVotes) < VOTE_THRESHOLD);
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
        x1 = this->Points.GetPoint(i);
        x2 = this->Points.GetPoint((i+1)%numPts);
//
//   Fire the ray and compute the number of intersections.  Be careful of 
//   degenerate cases (e.g., ray intersects at vertex).
//
        if ((status=line.Intersection(x,xray,x1,x2,u,v)) == INTERSECTION) 
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
    vtkErrorMacro(<<"Couldn't triangulate");
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

// Description: A fast triangulation method. Uses recursive divide and 
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
//  Determine whether the loop can be split / build loops
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

int vtkPolygon::CellBoundary(int vtkNotUsed(subId), float pcoords[3], vtkIdList& pts)
{
  return 0;
}

void vtkPolygon::Contour(float value, vtkFloatScalars *cellScalars, 
                        vtkPointLocator *locator,
                        vtkCellArray *verts, vtkCellArray *lines, 
                        vtkCellArray *polys, vtkFloatScalars *scalars)
{
  int i, success;
  int numVerts=this->Points.GetNumberOfPoints();
  float *bounds, d;
  static vtkTriangle tri;
  static vtkIdList Tris((VTK_CELL_SIZE-2)*3);
  static vtkFloatScalars triScalars(3);
  int *polyVerts = new int[numVerts];

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
      tri.Points.SetPoint(0,this->Points.GetPoint(i));
      tri.Points.SetPoint(1,this->Points.GetPoint(i+1));
      tri.Points.SetPoint(2,this->Points.GetPoint(i+2));

      triScalars.SetScalar(0,cellScalars->GetScalar(i));
      triScalars.SetScalar(1,cellScalars->GetScalar(i+1));
      triScalars.SetScalar(2,cellScalars->GetScalar(i+2));

      tri.Contour(value, &triScalars, locator, verts,
                   lines, polys, scalars);
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
  if ( ! plane.IntersectWithLine(p1,p2,n,pt1,t,x) ) return 0;
//
// Evaluate position
//
  if ( this->EvaluatePosition(x, closestPoint, subId, pcoords, dist2, weights) )
    if ( dist2 <= tol2 ) return 1;

  return 0;

}

int vtkPolygon::Triangulate(int vtkNotUsed(index), vtkFloatPoints &pts)
{
  int i, success;
  float *bounds, d;
  int numVerts=this->PointIds.GetNumberOfIds();
  int *verts = new int[numVerts];
  static vtkIdList Tris((VTK_CELL_SIZE-2)*3);

  pts.Reset();

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
      pts.InsertPoint(i,this->Points.GetPoint(Tris.GetId(i)));
      }
    }

  delete [] verts;
  return success;
}

// Sample at three points to compute derivatives in local r-s coordinate system.
// Project vectors into 3D model coordinate system
#define VTK_SAMPLE_DISTANCE 0.01
void vtkPolygon::Derivatives(int vtkNotUsed(subId), float pcoords[3], float *values, 
                             int dim, float *derivs)
{
  int i, j;
  float p0[3], p10[3], l10, p20[3], l20, n[3];
  float x[3][3], value;
  int numVerts=this->PointIds.GetNumberOfIds();
  float *weights = new float[numVerts];

  //setup parametric system
  this->ParameterizePolygon(p0, p10, l10, p20, l20, n);

  //compute positions of three sample points
  for (i=0; i<3; i++)
    {
    x[0][i] = p0[i] + pcoords[0]*p10[i] + pcoords[1]*p20[i];
    x[1][i] = p0[i] + (pcoords[0]+VTK_SAMPLE_DISTANCE)*p10[i] + pcoords[1]*p20[i];
    x[2][i] = p0[i] + pcoords[0]*p10[i] + (pcoords[1]+VTK_SAMPLE_DISTANCE)*p20[i];
    }

  //for each sample point, compute values and data values.

  this->ComputeWeights(x[0],weights);
  for ( j=0; j < dim; i++)
    {
    value = 0.0;
    for ( i=0; i < numVerts; i++ )
      {
      value += weights[i] * values[numVerts*j + i];
      }
    }
}

