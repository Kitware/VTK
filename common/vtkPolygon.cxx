/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolygon.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <stdlib.h>
#include "vtkPolygon.h"
#include "vtkMath.h"
#include "vtkPlane.h"
#include "vtkDataSet.h"
#include "vtkCellArray.h"
#include "vtkPriorityQueue.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkPolygon* vtkPolygon::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPolygon");
  if(ret)
    {
    return (vtkPolygon*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPolygon;
}




// Instantiate polygon.
vtkPolygon::vtkPolygon()
{
  this->Tris = vtkIdList::New();
  this->Tris->Allocate(VTK_CELL_SIZE);
  this->Triangle = vtkTriangle::New();
  this->Quad = vtkQuad::New();
  this->TriScalars = vtkScalars::New();
  this->TriScalars->Allocate(3);
  this->Line = vtkLine::New();
}

vtkPolygon::~vtkPolygon()
{
  this->Tris->Delete();
  this->Triangle->Delete();
  this->Quad->Delete();
  this->TriScalars->Delete();
  this->Line->Delete();
}

vtkCell *vtkPolygon::MakeObject()
{
  vtkCell *cell = vtkPolygon::New();
  cell->DeepCopy(this);
  return cell;
}

#define VTK_POLYGON_FAILURE -1
#define VTK_POLYGON_OUTSIDE 0
#define VTK_POLYGON_INSIDE 1
#define VTK_POLYGON_INTERSECTION 2
#define VTK_POLYGON_ON_LINE 3

//
// In many of the functions that follow, the Points and PointIds members 
// of the Cell are assumed initialized.  This is usually done indirectly
// through the GetCell(id) method in the DataSet objects.
//

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

// Compute the polygon normal from a list of floating points. This version
// will handle non-convex polygons.
void vtkPolygon::ComputeNormal(vtkPoints *p, float *n)
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

  for (i=0; i<numPts-2; i++) 
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
      v3 += 3;
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

  for (i=0; i<3; i++)
    {
    ray[i] = closestPoint[i] - p0[i];
    }
  pcoords[0] = vtkMath::Dot(ray,p10) / (l10*l10);
  pcoords[1] = vtkMath::Dot(ray,p20) / (l20*l20);

  if ( pcoords[0] >= 0.0 && pcoords[0] <= 1.0 &&
       pcoords[1] >= 0.0 && pcoords[1] <= 1.0 &&
       (this->PointInPolygon(closestPoint, this->Points->GetNumberOfPoints(), 
			     ((vtkFloatArray *)this->Points->GetData())
			     ->GetPointer(0), this->GetBounds(),n)
	== VTK_POLYGON_INSIDE) )
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

    numPts = this->Points->GetNumberOfPoints();
    for (minDist2=VTK_LARGE_FLOAT,i=0; i<numPts; i++)
      {
      dist2 = vtkLine::DistanceToLine(x,this->Points->GetPoint(i),
                                  this->Points->GetPoint((i+1)%numPts),t,closest);
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
  int numPts=this->Points->GetNumberOfPoints();
  float *x1, *x2;
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
  vtkMath::Cross (n,p10,p20);
  //
  // Determine lengths of edges
  //
  if ( (l10=vtkMath::Dot(p10,p10)) == 0.0
       || (l20=vtkMath::Dot(p20,p20)) == 0.0 )
    {
    return 0;
    }
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
    for(j=0; j<3; j++)
      {
      p[j] = x1[j] - p0[j];
      }
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

#define VTK_POLYGON_CERTAIN 1
#define VTK_POLYGON_UNCERTAIN 0
#define VTK_POLYGON_RAY_TOL 1.e-03 //Tolerance for ray firing
#define VTK_POLYGON_MAX_ITER 10    //Maximum iterations for ray-firing
#define VTK_POLYGON_VOTE_THRESHOLD 2

#ifndef TRUE
#define FALSE 0
#define TRUE 1
#endif

// Determine whether point is inside polygon. Function uses ray-casting
// to determine if point is inside polygon. Works for arbitrary polygon shape
// (e.g., non-convex). Returns 0 if point is not in polygon; 1 if it is.
// Can also return -1 to indicate degenerate polygon. Note: a point in
// bounding box check is NOT performed prior to in/out check. You may want
// to do this to improve performance.
int vtkPolygon::PointInPolygon (float x[3], int numPts, float *pts, 
                                float bounds[6], float *n)
{
  float *x1, *x2, xray[3], u, v;
  float rayMag, mag=1, ray[3];
  int testResult, rayOK, status, numInts, i;
  int iterNumber;
  int maxComp, comps[2];
  int deltaVotes;

  // do a quick bounds check
  if ( x[0] < bounds[0] || x[0] > bounds[1] ||
       x[1] < bounds[2] || x[1] > bounds[3] ||
       x[2] < bounds[4] || x[2] > bounds[5])
    {
    return VTK_POLYGON_OUTSIDE;
    }
  
  //
  //  Define a ray to fire.  The ray is a random ray normal to the
  //  normal of the face.  The length of the ray is a function of the
  //  size of the face bounding box.
  //
  for (i=0; i<3; i++)
    {
    ray[i] = ( bounds[2*i+1] - bounds[2*i] )*1.1 +
      fabs((bounds[2*i+1] + bounds[2*i])/2.0 - x[i]);
    }

  if ( (rayMag = vtkMath::Norm(ray)) == 0.0 )
    {
    return VTK_POLYGON_OUTSIDE;
    }
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

  //  Check that max component is non-zero
  //
  if ( n[maxComp] == 0.0 )
    {
    return VTK_POLYGON_FAILURE;
    }
  //
  //  Enough information has been acquired to determine the random ray.
  //  Random rays are generated until one is satisfactory (i.e.,
  //  produces a ray of non-zero magnitude).  Also, since more than one
  //  ray may need to be fired, the ray-firing occurs in a large loop.
  //
  //  The variable iterNumber counts the number of iterations and is
  //  limited by the defined variable VTK_POLYGON_MAX_ITER.
  //
  //  The variable deltaVotes keeps track of the number of votes for
  //  "in" versus "out" of the face.  When delta_vote > 0, more votes
  //  have counted for "in" than "out".  When delta_vote < 0, more votes
  //  have counted for "out" than "in".  When the delta_vote exceeds or
  //  equals the defined variable VTK_POLYGON_VOTE_THRESHOLD, than the
  //  appropriate "in" or "out" status is returned.
  //
  for (deltaVotes = 0, iterNumber = 1;
       (iterNumber < VTK_POLYGON_MAX_ITER)
	 && (abs(deltaVotes) < VTK_POLYGON_VOTE_THRESHOLD);
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
      if ( (mag = vtkMath::Norm(ray)) > rayMag*VTK_TOL )
	{
	rayOK = TRUE;
	}
      }
    //
    //  The ray must be appropriately sized.
    //
    for (i=0; i<3; i++)
      {
      xray[i] = x[i] + (rayMag/mag)*ray[i];
      }
    //
    //  The ray may now be fired against all the edges
    //
    for (numInts=0, testResult=VTK_POLYGON_CERTAIN, i=0; i<numPts; i++) 
      {
      x1 = pts + 3*i;
      x2 = pts + 3*((i+1)%numPts);
      //
      //   Fire the ray and compute the number of intersections.  Be careful
      //   of degenerate cases (e.g., ray intersects at vertex).
      //
      if ((status=vtkLine::Intersection(x,xray,x1,x2,u,v)) == VTK_POLYGON_INTERSECTION) 
	{
	if ( (VTK_POLYGON_RAY_TOL < v) && (v < 1.0-VTK_POLYGON_RAY_TOL) )
	  {
	  numInts++;
	  }
	else
	  {
	  testResult = VTK_POLYGON_UNCERTAIN;
	  }
	} 
      else if ( status == VTK_POLYGON_ON_LINE )
	{
	testResult = VTK_POLYGON_UNCERTAIN;
	}
      }
    if ( testResult == VTK_POLYGON_CERTAIN ) 
      {
      if ( (numInts % 2) == 0)
	  {
          --deltaVotes;
	  }
      else
	{
	++deltaVotes;
	}
      }
    } //try another ray

  //   If the number of intersections is odd, the point is in the polygon.
  //
  if ( deltaVotes < 0 )
    {
    return VTK_POLYGON_OUTSIDE;
    }
  else
    {
    return VTK_POLYGON_INSIDE;
    }
}

#define VTK_POLYGON_TOLERANCE 1.0e-06

// Triangulate polygon. Tries to use the fast triangulation technique 
// first, and if that doesn't work, uses more complex routine that is
//  guaranteed to work.
int vtkPolygon::Triangulate(vtkIdList *outTris)
{
  int i, success;
  float *bounds, d;
  int numVerts=this->PointIds->GetNumberOfIds();
  int *verts = new int[numVerts];

  bounds = this->GetBounds();
  
  d = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
           (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
           (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));
  this->Tolerance = VTK_POLYGON_TOLERANCE * d;
  this->SuccessfulTriangulation = 1;
  this->ComputeNormal(this->Points, this->Normal);

  for (i=0; i<numVerts; i++)
    {
    verts[i] = i;
    }
  this->Tris->Reset();
  outTris->Reset();

  success = this->RecursiveTriangulate(numVerts, verts);
  delete [] verts;
  
  if ( success )
    {
    for (i=0; i<this->Tris->GetNumberOfIds(); i++)
      {
      outTris->InsertId(i,this->PointIds->GetId(this->Tris->GetId(i)));
      }
    return 1;
    }
  else //degenerate triangle encountered
    {
    vtkWarningMacro(<< "Degenerate polyogn encountered during triangulation");
    return 0;
    }
}

// A fast triangulation method. Uses recursive divide and 
// conquer based on plane splitting  to reduce loop into triangles.  
// The cell (e.g., triangle) is presumed properly initialized (i.e., 
// Points and PointIds).
int vtkPolygon::RecursiveTriangulate (int numVerts, int *verts)
{
  int i,j;
  int n1, n2;
  int fedges[2];

  if ( ! this->SuccessfulTriangulation )
    {
    return this->SuccessfulTriangulation;
    }

  switch (numVerts) 
    {
    case 0: case 1: case 2:
      //  In loops of less than 3 vertices no elements are created - shouldn't happen
      return this->SuccessfulTriangulation;

    case 3:
      //  A loop of three vertices makes one triangle!
      this->Tris->InsertNextId(verts[0]);
      this->Tris->InsertNextId(verts[1]);
      this->Tris->InsertNextId(verts[2]);
      return this->SuccessfulTriangulation;

    default:
      {
      //  Loops greater than three vertices must be subdivided.  This is
      //  done by finding the best splitting plane and creating two loops and 
      //  recursively triangulating.  To find the best splitting plane, try
      //  all possible combinations, keeping track of the one that gives the
      //  shortest distance between points.
      //
      vtkPriorityQueue *EdgeLengths;
      int *l1 = new int[numVerts], *l2 = new int[numVerts];
      int id;
      float dist2, *p1, *p2;

      // quick fix until constructors are changed
      EdgeLengths = vtkPriorityQueue::New();
      EdgeLengths->Allocate(VTK_CELL_SIZE);
      
      // find the minimum distance between points as candidates for the split line
      for (i=0; i<(numVerts-2); i++) 
        {
        for (j=i+2; j<numVerts; j++) 
          {
          if ( ((j+1) % numVerts) != i ) 
            {
            id = j*numVerts + i; //generated id
	    //we depend on using vtkPoints of type float
            p1 = this->Points->GetPoint(verts[i]); 
            p2 = this->Points->GetPoint(verts[j]);
            dist2 = vtkMath::Distance2BetweenPoints(p1,p2);
            EdgeLengths->Insert(dist2, id);
            }
          }
        }

      // now see whether we can split loop using priority-ordered 
      // split candidates
      while ( (id = EdgeLengths->Pop(dist2)) >= 0 )
        {
        fedges[0] = verts[id % numVerts];
        fedges[1] = verts[id / numVerts];

        if ( this->CanSplitLoop(fedges, numVerts, verts, n1, l1, n2, l2) )
          {
          this->RecursiveTriangulate (n1, l1);
          this->RecursiveTriangulate (n2, l2);

          delete [] l1;
          delete [] l2;
	  EdgeLengths->Delete();
          return this->SuccessfulTriangulation;
          }
        }
      
      this->SuccessfulTriangulation = 0;

      EdgeLengths->Delete();
      delete [] l1;
      delete [] l2;
      return this->SuccessfulTriangulation;
      }
    }
}

// Determine whether the loop can be split. Determines this by first checking
// to see whether points in each loop are on opposite sides of the split
// plane. If so, then the loop can be split; otherwise see whether one of the
// loops has all its points on one side of the split plane and the split line
// is inside the polygon.
int vtkPolygon::CanSplitLoop (int fedges[2], int numVerts, int *verts, 
                             int& n1, int *l1, int& n2, int *l2)
{
  int i, sign1, sign2, loop1Split=1, loop2Split=1;
  float *x, val, *sPt, *s2Pt, v21[3], sN[3];
  float n[3], den;

  //  Create two loops from the one using the splitting vertices provided.
  this->SplitLoop (fedges, numVerts, verts, n1, l1, n2, l2);

  // Create splitting plane.  Splitting plane is parallel to the loop
  // plane normal and contains the splitting vertices fedges[0] and fedges[1].
  sPt = this->Points->GetPoint(fedges[0]);
  s2Pt = this->Points->GetPoint(fedges[1]);
  for (i=0; i<3; i++)
    {
    v21[i] = s2Pt[i] - sPt[i];
    }

  vtkMath::Cross (v21,this->Normal,sN);
  if ( (den=vtkMath::Norm(sN)) != 0.0 )
    {
    for (i=0; i<3; i++)
      {
      sN[i] /= den;
      }
    }
  else
    {
    return 0;
    }

  // Evaluate the vertices in each loop to see which side of the split plane
  // they are on. We want to see whether this plane cleanly separates the loop
  // into two halves.
  for (sign1=0, i=0; i < n1; i++) //first loop
    {
    if ( !(l1[i] == fedges[0] || l1[i] == fedges[1]) ) 
      {
      x = this->Points->GetPoint(l1[i]);
      val = vtkPlane::Evaluate(sN,sPt,x);
      if ( !sign1 )
	{
        sign1 = (val > this->Tolerance ? 1 : -1);
	}
      else if ( sign1 != (val > 0 ? 1 : -1) )
        {
        loop1Split = 0;
        break;
        }
      }
    }

  for (sign2=0, i=0; i < n2; i++) //second loop
    {
    if ( !(l2[i] == fedges[0] || l2[i] == fedges[1]) ) 
      {
      x = this->Points->GetPoint(l2[i]);
      val = vtkPlane::Evaluate(sN,sPt,x);
      if ( !sign2 )
	{
        sign2 = (val > this->Tolerance ? 1 : -1);
	}
      else if ( sign2 != (val > 0 ? 1 : -1) )
        {
        loop2Split = 0;
        break;
        }
      }
    }

  // If both loops cleanly split the split line is okay; if only one loop
  // cleanly split check to see if it is valid. It is valid if the loop normal
  // points in the same direction as the polygon normal, and no edges of the
  // other loop intersect the split line.
  if ( loop1Split && loop2Split ) //both loops cleanly split
    {
    if ( sign1 != sign2 )
      {
      return 1; //on opposite sides of split plane
      }
    else
      {
      return 0; //on same side of plsit plane
      }
    }
  else if ( !loop1Split && !loop2Split ) //neither loop cleanly split - skip
    {
    return 0;
    }
  else //one loop cleanly split - need to do edge intersection/normal check
    {
    float u, v, *p1, *p2;
    int id1, id2, *loop, count, *otherLoop, otherCount;
    
    if ( loop1Split )
      {
      loop = l1; count = n1;
      otherLoop = l2; otherCount = n2;
      }
    else
      {
      loop = l2; count = n2;
      otherLoop = l1; otherCount = n1;
      }

    this->ComputeNormal(this->Points, count, loop, n);
    if ( vtkMath::Dot(n,this->Normal) < 0.0 )
      {
      return 0;
      }
    // Check line-line intersection
    for (i=0; i < otherCount; i++)
      {
      id1 = otherLoop[i];
      id2 = otherLoop[(i+1) % otherCount];
      if ( id1 != fedges[0] && id1 != fedges[1] &&
      id2 != fedges[0] && id2 != fedges[1] )
        {
        p1 = this->Points->GetPoint(id1);
        p2 = this->Points->GetPoint(id2);
        if ( vtkLine::Intersection(sPt,s2Pt,p1,p2,u,v) != 0 )
	  {
	  return 0;
	  }
        }
      }
    return 1;
    }
}

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

int vtkPolygon::CellBoundary(int vtkNotUsed(subId), float pcoords[3], 
                             vtkIdList *pts)
{
  int i, numPts=this->PointIds->GetNumberOfIds();
  float x[3], *weights, closest[3];
  int closestPoint=0, previousPoint, nextPoint;
  float largestWeight=0.0;
  float p0[3], p10[3], l10, p20[3], l20, n[3];

  pts->Reset();
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

  pts->InsertId(0,this->PointIds->GetId(closestPoint));

  previousPoint = closestPoint - 1;
  nextPoint = closestPoint + 1;
  if ( previousPoint < 0 )
    {
    previousPoint = numPts - 1;
    }
  if ( nextPoint >= numPts )
    {
    nextPoint = 0;
    }

  if ( weights[previousPoint] > weights[nextPoint] )
    {
    pts->InsertId(1,this->PointIds->GetId(previousPoint));
    }
  else
    {
    pts->InsertId(1,this->PointIds->GetId(nextPoint));
    }
  delete [] weights;

  // determine whether point is inside of polygon
  if ( pcoords[0] >= 0.0 && pcoords[0] <= 1.0 &&
       pcoords[1] >= 0.0 && pcoords[1] <= 1.0 &&
       (this->PointInPolygon(closest, this->Points->GetNumberOfPoints(), 
			     ((vtkFloatArray *)this->Points->GetData())
			     ->GetPointer(0), this->GetBounds(),n)
	== VTK_POLYGON_INSIDE) )
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

void vtkPolygon::Contour(float value, vtkScalars *cellScalars, 
                        vtkPointLocator *locator,
                        vtkCellArray *verts, vtkCellArray *lines, 
                        vtkCellArray *polys,
                        vtkPointData *inPd, vtkPointData *outPd,
                        vtkCellData *inCd, int cellId, vtkCellData *outCd)
{
  int i, success;
  int numVerts=this->Points->GetNumberOfPoints();
  float *bounds, d;
  int *polyVerts = new int[numVerts], p1, p2, p3;

  this->TriScalars->SetNumberOfScalars(3);

  bounds = this->GetBounds();
  
  d = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
           (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
           (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));
  this->Tolerance = VTK_POLYGON_TOLERANCE * d;
  this->SuccessfulTriangulation = 1;
  this->ComputeNormal(this->Points, this->Normal);

  for (i=0; i<numVerts; i++)
    {
    polyVerts[i] = i;
    }
  this->Tris->Reset();

  success = this->RecursiveTriangulate(numVerts, polyVerts);

  if ( !success ) // Just skip for now.
    {
    }
  else // Contour triangle
    {
    for (i=0; i<this->Tris->GetNumberOfIds(); i += 3)
      {
      p1 = this->Tris->GetId(i);
      p2 = this->Tris->GetId(i+1);
      p3 = this->Tris->GetId(i+2);

      this->Triangle->Points->SetPoint(0,this->Points->GetPoint(p1));
      this->Triangle->Points->SetPoint(1,this->Points->GetPoint(p2));
      this->Triangle->Points->SetPoint(2,this->Points->GetPoint(p3));

      if ( outPd )
        {
        this->Triangle->PointIds->SetId(0,this->PointIds->GetId(p1));
        this->Triangle->PointIds->SetId(1,this->PointIds->GetId(p2));
        this->Triangle->PointIds->SetId(2,this->PointIds->GetId(p3));
        }

      this->TriScalars->SetScalar(0,cellScalars->GetScalar(p1));
      this->TriScalars->SetScalar(1,cellScalars->GetScalar(p2));
      this->TriScalars->SetScalar(2,cellScalars->GetScalar(p3));

      this->Triangle->Contour(value, this->TriScalars, locator, verts,
                   lines, polys, inPd, outPd, inCd, cellId, outCd);
      }
    }
  delete [] polyVerts;
}

vtkCell *vtkPolygon::GetEdge(int edgeId)
{
  int numPts=this->Points->GetNumberOfPoints();

  // load point id's
  this->Line->PointIds->SetId(0,this->PointIds->GetId(edgeId));
  this->Line->PointIds->SetId(1,this->PointIds->GetId((edgeId+1) % numPts));

  // load coordinates
  this->Line->Points->SetPoint(0,this->Points->GetPoint(edgeId));
  this->Line->Points->SetPoint(1,this->Points->GetPoint((edgeId+1) % numPts));

  return this->Line;
}

//
// Compute interpolation weights using 1/r**2 normalized sum.
//
void vtkPolygon::ComputeWeights(float x[3], float *weights)
{
  int i;
  int numPts=this->Points->GetNumberOfPoints();
  float sum, *pt;

  for (sum=0.0, i=0; i<numPts; i++)
    {
    pt = this->Points->GetPoint(i);
    weights[i] = vtkMath::Distance2BetweenPoints(x,pt);
    if ( weights[i] == 0.0 ) //exact hit
      {
      for (int j=0; j<numPts; j++)
	{
	weights[j] = 0.0;
	}
      weights[i] = 1.0;
      return;
      }
    else
      {
      weights[i] = 1.0 / (weights[i]*weights[i]);
      sum += weights[i];
      }
    }

  for (i=0; i<numPts; i++)
    {
    weights[i] /= sum;
    }
}

//
// Intersect this plane with finite line defined by p1 & p2 with tolerance tol.
//
int vtkPolygon::IntersectWithLine(float p1[3], float p2[3], float tol,float& t,
                                 float x[3], float pcoords[3], int& subId)
{
  float *pt1, n[3];
  float tol2 = tol*tol;
  float closestPoint[3];
  float dist2;
  int npts = this->GetNumberOfPoints();
  float *weights;

  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;

  // Define a plane to intersect with
  //
  pt1 = this->Points->GetPoint(1);
  this->ComputeNormal (this->Points,n);
 
  // Intersect plane of triangle with line
  //
  if ( ! vtkPlane::IntersectWithLine(p1,p2,n,pt1,t,x) )
    {
    return 0;
    }

  // Evaluate position
  //
  weights = new float[npts];
  if ( this->EvaluatePosition(x, closestPoint, subId, pcoords, dist2, weights))
    {
    if ( dist2 <= tol2 ) 
      {
      delete [] weights;
      return 1;
      }
    }
  delete [] weights;
  return 0;

}

int vtkPolygon::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds, 
                            vtkPoints *pts)
{
  int i, success;
  float *bounds, d;
  int numVerts=this->PointIds->GetNumberOfIds();
  int *verts = new int[numVerts];

  pts->Reset();
  ptIds->Reset();

  bounds = this->GetBounds();
  d = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
           (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
           (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));
  this->Tolerance = VTK_POLYGON_TOLERANCE * d;
  this->SuccessfulTriangulation = 1;
  this->ComputeNormal(this->Points, this->Normal);

  for (i=0; i<numVerts; i++)
    {
    verts[i] = i;
    }
  this->Tris->Reset();

  success = this->RecursiveTriangulate(numVerts, verts);

  if ( !success ) // Use slower but always successful technique.
    {
    vtkErrorMacro(<<"Couldn't triangulate");
    }
  else // Copy the point id's into the supplied Id array
    {
    for (i=0; i<this->Tris->GetNumberOfIds(); i++)
      {
      ptIds->InsertId(i,this->PointIds->GetId(this->Tris->GetId(i)));
      pts->InsertPoint(i,this->Points->GetPoint(this->Tris->GetId(i)));
      }
    }

  delete [] verts;
  return success;
}

// Samples at three points to compute derivatives in local r-s coordinate 
// system and projects vectors into 3D model coordinate system.
// Note that the results are usually inaccurate because
// this method actually returns the derivative of the interpolation
// function  which  is obtained using 1/r**2 normalized sum.
#define VTK_SAMPLE_DISTANCE 0.01
void vtkPolygon::Derivatives(int vtkNotUsed(subId), float pcoords[3], 
                             float *values, int dim, float *derivs)
{
  int i, j, k, idx;

  if ( this->Points->GetNumberOfPoints() == 4 )
    {
    for(i=0; i<4; i++)
      {
      this->Quad->Points->SetPoint(i,this->Points->GetPoint(i));
      }
    this->Quad->Derivatives(0, pcoords, values, dim, derivs);
    return;
    }
  else if ( this->Points->GetNumberOfPoints() == 3 )
    {
    for(i=0; i<3; i++)
      {
      this->Triangle->Points->SetPoint(i,this->Points->GetPoint(i));
      }
    this->Triangle->Derivatives(0, pcoords, values, dim, derivs);
    return;
    }

  float p0[3], p10[3], l10, p20[3], l20, n[3];
  float x[3][3], l1, l2, v1[3], v2[3];
  int numVerts=this->PointIds->GetNumberOfIds();
  float *weights = new float[numVerts];
  float *sample = new float[dim*3];


  //setup parametric system and check for degeneracy
  if ( this->ParameterizePolygon(p0, p10, l10, p20, l20, n) == 0 )
    {
    for ( j=0; j < dim; j++ )
      {
      for ( i=0; i < 3; i++ )
	{
        derivs[j*dim + i] = 0.0;
	}
      }
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
    for ( j=0; j < dim; j++, idx++) //over number of derivates requested
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

void vtkPolygon::Clip(float value, vtkScalars *cellScalars, 
                      vtkPointLocator *locator, vtkCellArray *tris,
                      vtkPointData *inPD, vtkPointData *outPD,
                      vtkCellData *inCD, int cellId, vtkCellData *outCD,
                      int insideOut)
{
  int i, success;
  int numVerts=this->Points->GetNumberOfPoints();
  float *bounds, d;
  int *polyVerts = new int[numVerts], p1, p2, p3;

  this->TriScalars->SetNumberOfScalars(3);

  bounds = this->GetBounds();
  d = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
           (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
           (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));
  this->Tolerance = VTK_POLYGON_TOLERANCE * d;

  this->SuccessfulTriangulation = 1;
  this->ComputeNormal(this->Points, this->Normal);

  for (i=0; i<numVerts; i++)
    {
    polyVerts[i] = i;
    }
  this->Tris->Reset();

  success = this->RecursiveTriangulate(numVerts, polyVerts);

  if ( success ) // clip triangles
    {
    for (i=0; i<this->Tris->GetNumberOfIds(); i += 3)
      {
      p1 = this->Tris->GetId(i);
      p2 = this->Tris->GetId(i+1);
      p3 = this->Tris->GetId(i+2);

      this->Triangle->Points->SetPoint(0,this->Points->GetPoint(p1));
      this->Triangle->Points->SetPoint(1,this->Points->GetPoint(p2));
      this->Triangle->Points->SetPoint(2,this->Points->GetPoint(p3));

      this->Triangle->PointIds->SetId(0,this->PointIds->GetId(p1));
      this->Triangle->PointIds->SetId(1,this->PointIds->GetId(p2));
      this->Triangle->PointIds->SetId(2,this->PointIds->GetId(p3));

      this->TriScalars->SetScalar(0,cellScalars->GetScalar(p1));
      this->TriScalars->SetScalar(1,cellScalars->GetScalar(p2));
      this->TriScalars->SetScalar(2,cellScalars->GetScalar(p3));

      this->Triangle->Clip(value, this->TriScalars, locator, tris, 
                          inPD, outPD, inCD, cellId, outCD, insideOut);
      }
    }
  delete [] polyVerts;
}

// Method intersects two polygons. You must supply the number of points and
// point coordinates (npts, *pts) and the bounding box (bounds) of the two
// polygons. Also supply a tolerance squared for controlling
// error. The method returns 1 if there is an intersection, and 0 if
// not. A single point of intersection x[3] is also returned if there
// is an intersection.
int vtkPolygon::IntersectPolygonWithPolygon(int npts, float *pts,float bounds[6],
                                            int npts2, float *pts2, 
                                            float bounds2[6], float tol2,
                                            float x[3])
{
  float n[3], coords[3];
  int i, j;
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

    for (j=0; j<3; j++)
      {
      ray[j] = p2[j] - p1[j];
      }
    if ( ! vtkCell::HitBBox(bounds2, p1, ray, coords, t) )
      {
      continue;
      }

    if ( (vtkPlane::IntersectWithLine(p1,p2,n,pts2,t,x)) == 1 ) 
      {
      if ( (npts2==3
	    && vtkTriangle::PointInTriangle(x,pts2,pts2+3,pts2+6,tol2))
	   || (npts2>3
	       && vtkPolygon::PointInPolygon(x,npts2,pts2,bounds2,n)
	       ==VTK_POLYGON_INSIDE))
        {
        return 1;
        }
      } 
    else
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

    for (j=0; j<3; j++)
      {
      ray[j] = p2[j] - p1[j];
      }

    if ( ! vtkCell::HitBBox(bounds, p1, ray, coords, t) )
      {
      continue;
      }

    if ( (vtkPlane::IntersectWithLine(p1,p2,n,pts,t,x)) == 1 ) 
      {
      if ( (npts==3 && vtkTriangle::PointInTriangle(x,pts,pts+3,pts+6,tol2))
	   || (npts>3 && vtkPolygon::PointInPolygon(x,npts,pts,bounds,n)
	       ==VTK_POLYGON_INSIDE))
        {
        return 1;
        }
      } 
    else
      {
      return 0;
      }
    }

  return 0;
}

