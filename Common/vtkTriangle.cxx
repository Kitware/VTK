/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriangle.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkTriangle.h"
#include "vtkPolygon.h"
#include "vtkPlane.h"
#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkLine.h"
#include "vtkPointLocator.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkTriangle* vtkTriangle::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTriangle");
  if(ret)
    {
    return (vtkTriangle*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkTriangle;
}




// Construct the triangle with three points.
vtkTriangle::vtkTriangle()
{
  int i;
  
  this->Points->SetNumberOfPoints(3);
  this->PointIds->SetNumberOfIds(3);
  for (i = 0; i < 3; i++)
    {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    }
  for (i = 0; i < 3; i++)
    {
    this->PointIds->SetId(i,0);
    }
  this->Line = vtkLine::New();
}

vtkTriangle::~vtkTriangle()
{
  this->Line->Delete();
}


// Create a new cell and copy this triangle's information into the cell.
// Returns a poiner to the new cell created.
vtkCell *vtkTriangle::MakeObject()
{
  vtkCell *cell = vtkTriangle::New();
  cell->DeepCopy(this);
  return cell;
}

int vtkTriangle::EvaluatePosition(float x[3], float* closestPoint,
                                 int& subId, float pcoords[3], 
                                 float& dist2, float *weights)
{
  int i, j;
  float *pt1, *pt2, *pt3, n[3], fabsn;
  float rhs[2], c1[2], c2[2];
  float det;
  float maxComponent;
  int idx=0, indices[2];
  float dist2Point, dist2Line1, dist2Line2;
  float *closest, closestPoint1[3], closestPoint2[3], cp[3];

  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;

  //
  // Get normal for triangle, only the normal direction is needed, i.e. the
  // normal need not be normalized (unit length)
  //
  pt1 = this->Points->GetPoint(1);
  pt2 = this->Points->GetPoint(2);
  pt3 = this->Points->GetPoint(0);

  vtkTriangle::ComputeNormalDirection(pt1, pt2, pt3, n);

  //
  // Project point to plane
  //
  vtkPlane::GeneralizedProjectPoint(x,pt1,n,cp);
  
  
  //
  // Construct matrices.  Since we have over determined system, need to find
  // which 2 out of 3 equations to use to develop equations. (Any 2 should 
  // work since we've projected point to plane.)
  //
  for (maxComponent=0.0, i=0; i<3; i++)
    {
    // trying to avoid an expensive call to fabs()
    if (n[i] < 0)
      {
      fabsn = -n[i];
      }
    else
      {
      fabsn = n[i];
      }
    if (fabsn > maxComponent)
      {
      maxComponent = fabsn;
      idx = i;
      }
    }
  for (j=0, i=0; i<3; i++)  
    {
    if ( i != idx )
      {
      indices[j++] = i;
      }
    }
  
  for (i=0; i<2; i++)
    {  
    rhs[i] = cp[indices[i]] - pt3[indices[i]];
    c1[i] = pt1[indices[i]] - pt3[indices[i]];
    c2[i] = pt2[indices[i]] - pt3[indices[i]];
    }

  if ( (det = vtkMath::Determinant2x2(c1,c2)) == 0.0 )
    {
    return -1;
    }

  pcoords[0] = vtkMath::Determinant2x2(rhs,c2) / det;
  pcoords[1] = vtkMath::Determinant2x2(c1,rhs) / det;
  pcoords[2] = 1.0 - (pcoords[0] + pcoords[1]);
  //
  // Okay, now find closest point to element
  //
  weights[0] = pcoords[2];
  weights[1] = pcoords[0];
  weights[2] = pcoords[1];

  if ( pcoords[0] >= 0.0 && pcoords[0] <= 1.0 &&
       pcoords[1] >= 0.0 && pcoords[1] <= 1.0 &&
       pcoords[2] >= 0.0 && pcoords[2] <= 1.0 )
    {
    //projection distance
    if (closestPoint)
      {
      dist2 = vtkMath::Distance2BetweenPoints(cp,x); 
      closestPoint[0] = cp[0];
      closestPoint[1] = cp[1];
      closestPoint[2] = cp[2];
      }
    return 1;
    }
  else
    {
    float t;
    if (closestPoint)
      {
      if ( pcoords[0] < 0.0 && pcoords[1] < 0.0 )
	{
	dist2Point = vtkMath::Distance2BetweenPoints(x,pt3);
	dist2Line1 = vtkLine::DistanceToLine(x,pt1,pt3,t,closestPoint1);
	dist2Line2 = vtkLine::DistanceToLine(x,pt3,pt2,t,closestPoint2);
	if (dist2Point < dist2Line1)
	  {
	  dist2 = dist2Point;
	  closest = pt3;
	  }
	else
	  {
	  dist2 = dist2Line1;
	  closest = closestPoint1;
	  }
	if (dist2Line2 < dist2)
	  {
	  dist2 = dist2Line2;
	  closest = closestPoint2;
	  }
	for (i=0; i<3; i++)
	  {
	  closestPoint[i] = closest[i];
	  }
	}
      else if ( pcoords[1] < 0.0 && pcoords[2] < 0.0 )
	{
	dist2Point = vtkMath::Distance2BetweenPoints(x,pt1);
	dist2Line1 = vtkLine::DistanceToLine(x,pt1,pt3,t,closestPoint1);
	dist2Line2 = vtkLine::DistanceToLine(x,pt1,pt2,t,closestPoint2);
	if (dist2Point < dist2Line1)
	  {
	  dist2 = dist2Point;
	  closest = pt1;
	  }
	else
	  {
	  dist2 = dist2Line1;
	  closest = closestPoint1;
	  }
	if (dist2Line2 < dist2)
	  {
	  dist2 = dist2Line2;
	  closest = closestPoint2;
	  }
	for (i=0; i<3; i++)
	  {
	  closestPoint[i] = closest[i];
	  }
	}
      else if ( pcoords[0] < 0.0 && pcoords[2] < 0.0 )
	{
	dist2Point = vtkMath::Distance2BetweenPoints(x,pt2);
	dist2Line1 = vtkLine::DistanceToLine(x,pt2,pt3,t,closestPoint1);
	dist2Line2 = vtkLine::DistanceToLine(x,pt1,pt2,t,closestPoint2);
	if (dist2Point < dist2Line1)
	  {
	  dist2 = dist2Point;
	  closest = pt2;
	  }
	else
	  {
	  dist2 = dist2Line1;
	  closest = closestPoint1;
	  }
	if (dist2Line2 < dist2)
	  {
	  dist2 = dist2Line2;
	  closest = closestPoint2;
	  }
	for (i=0; i<3; i++)
	  {
	  closestPoint[i] = closest[i];
	  }
	}
      else if ( pcoords[0] < 0.0 )
	{
	dist2 = vtkLine::DistanceToLine(x,pt2,pt3,t,closestPoint);
	}
      else if ( pcoords[1] < 0.0 )
	{
	dist2 = vtkLine::DistanceToLine(x,pt1,pt3,t,closestPoint);
	}
      else if ( pcoords[2] < 0.0 )
	{
	dist2 = vtkLine::DistanceToLine(x,pt1,pt2,t,closestPoint);
	}
      }
    return 0;
    }
}

void vtkTriangle::EvaluateLocation(int& vtkNotUsed(subId), float pcoords[3],
				   float x[3], float *weights)
{
  float u3;
  float *pt0, *pt1, *pt2;
  int i;

  pt0 = this->Points->GetPoint(0);
  pt1 = this->Points->GetPoint(1);
  pt2 = this->Points->GetPoint(2);

  u3 = 1.0 - pcoords[0] - pcoords[1];

  for (i=0; i<3; i++)
    {
    x[i] = pt0[i]*u3 + pt1[i]*pcoords[0] + pt2[i]*pcoords[1];
    }

  weights[0] = u3;
  weights[1] = pcoords[0];
  weights[2] = pcoords[1];
}

int vtkTriangle::CellBoundary(int vtkNotUsed(subId), float pcoords[3],
			      vtkIdList *pts)
{
  float t1=pcoords[0]-pcoords[1];
  float t2=0.5*(1.0-pcoords[0])-pcoords[1];
  float t3=2.0*pcoords[0]+pcoords[1]-1.0;

  pts->SetNumberOfIds(2);

  // compare against three lines in parametric space that divide element
  // into three pieces
  if ( t1 >= 0.0 && t2 >= 0.0 )
    {
    pts->SetId(0,this->PointIds->GetId(0));
    pts->SetId(1,this->PointIds->GetId(1));
    }

  else if ( t2 < 0.0 && t3 >= 0.0 )
    {
    pts->SetId(0,this->PointIds->GetId(1));
    pts->SetId(1,this->PointIds->GetId(2));
    }

  else //( t1 < 0.0 && t3 < 0.0 )
    {
    pts->SetId(0,this->PointIds->GetId(2));
    pts->SetId(1,this->PointIds->GetId(0));
    }

  if ( pcoords[0] < 0.0 || pcoords[1] < 0.0 ||
       pcoords[0] > 1.0 || pcoords[1] > 1.0 ||
       (1.0 - pcoords[0] - pcoords[1]) < 0.0 )
    {
    return 0;
    }
  else
    {
    return 1;
    }

}

//
// Marching triangles
//
typedef int EDGE_LIST;
typedef struct {
       EDGE_LIST edges[3];
} LINE_CASES;

static LINE_CASES lineCases[] = { 
  {{-1, -1, -1}},
  {{0, 2, -1}},
  {{1, 0, -1}},
  {{1, 2, -1}},
  {{2, 1, -1}},
  {{0, 1, -1}},
  {{2, 0, -1}},
  {{-1, -1, -1}}
};

static int edges[3][2] = { {0,1}, {1,2}, {2,0} };

void vtkTriangle::Contour(float value, vtkDataArray *cellScalars, 
			  vtkPointLocator *locator,
			  vtkCellArray *vtkNotUsed(verts), 
			  vtkCellArray *lines, 
			  vtkCellArray *vtkNotUsed(polys), 
                          vtkPointData *inPd, vtkPointData *outPd,
                          vtkCellData *inCd, vtkIdType cellId,
                          vtkCellData *outCd)
{
  static int CASE_MASK[3] = {1,2,4};
  LINE_CASES *lineCase;
  EDGE_LIST  *edge;
  int i, j, index, *vert;
  vtkIdType pts[2];
  int e1, e2, newCellId;
  float t, x1[3], x2[3], x[3], deltaScalar;

  // Build the case table
  for ( i=0, index = 0; i < 3; i++)
    {
    if (cellScalars->GetComponent(i,0) >= value)
      {
      index |= CASE_MASK[i];
      }
    }

  lineCase = lineCases + index;
  edge = lineCase->edges;

  for ( ; edge[0] > -1; edge += 2 )
    {
    for (i=0; i<2; i++) // insert line
      {
      vert = edges[edge[i]];
      // calculate a preferred interpolation direction
      deltaScalar = (cellScalars->GetComponent(vert[1],0) 
		     - cellScalars->GetComponent(vert[0],0));
      if (deltaScalar > 0)
        {
        e1 = vert[0]; e2 = vert[1];
        }
      else
        {
	e1 = vert[1]; e2 = vert[0];
        deltaScalar = -deltaScalar;
        }
      
      // linear interpolation
      if (deltaScalar == 0.0)
	{
	t = 0.0;
	}
      else
	{
	t = (value - cellScalars->GetComponent(e1,0)) / deltaScalar;
	}

      this->Points->GetPoint(e1, x1);
      this->Points->GetPoint(e2, x2);

      for (j=0; j<3; j++)
	{
	x[j] = x1[j] + t * (x2[j] - x1[j]);
	}
      if ( locator->InsertUniquePoint(x, pts[i]) )
        {
        if ( outPd ) 
          {
          int p1 = this->PointIds->GetId(e1);
          int p2 = this->PointIds->GetId(e2);
          outPd->InterpolateEdge(inPd,pts[i],p1,p2,t);
          }
        }
      }
    // check for degenerate line
    if ( pts[0] != pts[1] )
      {
      newCellId = lines->InsertNextCell(2,pts);
      outCd->CopyData(inCd,cellId,newCellId);
      }
    }
}

// Get the edge specified by edgeId (range 0 to 2) and return that edge's coordinates.
vtkCell *vtkTriangle::GetEdge(int edgeId)
{
  int edgeIdPlus1 = edgeId + 1;

  if (edgeIdPlus1 > 2)
    {
    edgeIdPlus1 = 0;
    }

  // load point id's
  this->Line->PointIds->SetId(0,this->PointIds->GetId(edgeId));
  this->Line->PointIds->SetId(1,this->PointIds->GetId(edgeIdPlus1));

  // load coordinates
  this->Line->Points->SetPoint(0,this->Points->GetPoint(edgeId));
  this->Line->Points->SetPoint(1,this->Points->GetPoint(edgeIdPlus1));

  return this->Line;
}

// Plane intersection plus in/out test on triangle. The in/out test is 
// performed using tol as the tolerance.
int vtkTriangle::IntersectWithLine(float p1[3], float p2[3], float tol, 
                                  float& t, float x[3], float pcoords[3], 
                                  int& subId)
{
  float *pt1, *pt2, *pt3, n[3];
  float tol2 = tol*tol;
  float closestPoint[3];
  float dist2, weights[3];
  
  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;
  //
  // Get normal for triangle
  //
  pt1 = this->Points->GetPoint(1);
  pt2 = this->Points->GetPoint(2);
  pt3 = this->Points->GetPoint(0);

  vtkTriangle::ComputeNormal (pt1, pt2, pt3, n);
  //
  // Intersect plane of triangle with line
  //
  if ( ! vtkPlane::IntersectWithLine(p1,p2,n,pt1,t,x) )
    {
    return 0;
    }
  //
  // Evaluate position
  //
  if (this->EvaluatePosition(x, closestPoint, subId, pcoords, dist2, weights)
      >= 0)
    {
    if ( dist2 <= tol2 )
      {
      return 1;
      }
    }
  
  // so the easy test failed. The line is not intersecting the triangle.
  // Let's now do the 3d case check to see how close the line comes.
  // basically we just need to test against the three lines of the triangle
  this->Line->PointIds->InsertId(0,0);
  this->Line->PointIds->InsertId(1,1);

  if (pcoords[2] < 0.0)
    {
    this->Line->Points->InsertPoint(0,pt1);
    this->Line->Points->InsertPoint(1,pt2);
    if (this->Line->IntersectWithLine(p1,p2,tol,t,x,pcoords,subId)) 
      {
      return 1;
      }
    }

  if (pcoords[0] < 0.0)
    {
    this->Line->Points->InsertPoint(0,pt2);
    this->Line->Points->InsertPoint(1,pt3);
    if (this->Line->IntersectWithLine(p1,p2,tol,t,x,pcoords,subId)) 
      {
      return 1;
      }
    }

  if (pcoords[1] < 0.0)
    {
    this->Line->Points->InsertPoint(0,pt3);
    this->Line->Points->InsertPoint(1,pt1);
    if (this->Line->IntersectWithLine(p1,p2,tol,t,x,pcoords,subId)) 
      {
      return 1;
      }
    }
  
  
  return 0;
}

int vtkTriangle::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds, 
                             vtkPoints *pts)
{
  pts->Reset();
  ptIds->Reset();

  for ( int i=0; i < 3; i++ )
    {
    ptIds->InsertId(i,this->PointIds->GetId(i));
    pts->InsertPoint(i,this->Points->GetPoint(i));
    }

  return 1;
}

// Used a staged computation: first compute derivatives in local x'-y' coordinate 
// system; then convert into x-y-z modelling system.
void vtkTriangle::Derivatives(int vtkNotUsed(subId), float vtkNotUsed(pcoords)[3], 
                              float *values, int dim, float *derivs)
{
  float v0[2], v1[2], v2[2], v[3], v10[3], v20[3], lenX;
  float *x0, *x1, *x2, n[3];
  double *J[2], J0[2], J1[2];
  double *JI[2], JI0[2], JI1[2];
  float functionDerivs[6], sum[2], dBydx, dBydy;
  int i, j;

  // Project points of triangle into 2D system
  x0 = this->Points->GetPoint(0);
  x1 = this->Points->GetPoint(1);
  x2 = this->Points->GetPoint(2);
  vtkTriangle::ComputeNormal (x0, x1, x2, n);

  for (i=0; i < 3; i++) 
    {
    v10[i] = x1[i] - x0[i];
    v[i] = x2[i] - x0[i];
    }

  vtkMath::Cross(n,v10,v20); //creates local y' axis

  if ( (lenX=vtkMath::Normalize(v10)) <= 0.0
       || vtkMath::Normalize(v20) <= 0.0 ) //degenerate
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

  v0[0] = v0[1] = 0.0; //convert points to 2D (i.e., local system)
  v1[0] = lenX; v1[1] = 0.0;
  v2[0] = vtkMath::Dot(v,v10);
  v2[1] = vtkMath::Dot(v,v20);

  // Compute interpolation function derivatives
  functionDerivs[0] = -1; //r derivatives
  functionDerivs[1] = 1;
  functionDerivs[2] = 0;
  functionDerivs[3] = -1; //s derivatives
  functionDerivs[4] = 0;
  functionDerivs[5] = 1;

  // Compute Jacobian: Jacobian is constant for a triangle.
  J[0] = J0; J[1] = J1;
  JI[0] = JI0; JI[1] = JI1;

  J[0][0] = v1[0] - v0[0];
  J[1][0] = v2[0] - v0[0];
  J[0][1] = v1[1] - v0[1];
  J[1][1] = v2[1] - v0[1];

  // Compute inverse Jacobian
  vtkMath::InvertMatrix(J,JI,2);

  // Loop over "dim" derivative values. For each set of values, compute derivatives
  // in local system and then transform into modelling system.
  // First compute derivatives in local x'-y' coordinate system
  for ( j=0; j < dim; j++ )
    {
    sum[0] = sum[1] = 0.0;
    for ( i=0; i < 3; i++) //loop over interp. function derivatives
      {
      sum[0] += functionDerivs[i] * values[dim*i + j]; 
      sum[1] += functionDerivs[3 + i] * values[dim*i + j];
      }
    dBydx = sum[0]*JI[0][0] + sum[1]*JI[0][1];
    dBydy = sum[0]*JI[1][0] + sum[1]*JI[1][1];

    // Transform into global system (dot product with global axes)
    derivs[3*j] = dBydx * v10[0] + dBydy * v20[0];
    derivs[3*j + 1] = dBydx * v10[1] + dBydy * v20[1];
    derivs[3*j + 2] = dBydx * v10[2] + dBydy * v20[2];
    }
}

// Compute the triangle normal from a points list, and a list of point ids
// that index into the points list.
void vtkTriangle::ComputeNormal(vtkPoints *p, int vtkNotUsed(numPts),
                                vtkIdType *pts, float n[3])
{
  float v1[3], v2[3], v3[3];

  p->GetPoint(pts[0],v1);
  p->GetPoint(pts[1],v2);
  p->GetPoint(pts[2],v3);

  vtkTriangle::ComputeNormal(v1,v2,v3,n);
}

// Compute the circumcenter (center[3]) and radius (method return value) of
// a triangle defined by the three points x1, x2, and x3. (Note that the
// coordinates are 2D. 3D points can be used but the z-component will be
// ignored.)
double vtkTriangle::Circumcircle(double  x1[2], double x2[2], double x3[2], 
                                 double center[2])
{
  double n12[2], n13[2], x12[2], x13[2];
  double *A[2], rhs[2], sum, diff;
  int i;

  //  calculate normals and intersection points of bisecting planes.  
  //
  for (i=0; i<2; i++) 
    {
    n12[i] = x2[i] - x1[i];
    n13[i] = x3[i] - x1[i];
    x12[i] = (x2[i] + x1[i])/2.0;
    x13[i] = (x3[i] + x1[i])/2.0;
    }

  //  Compute solutions to the intersection of two bisecting lines
  //  (2-eqns. in 2-unknowns).
  //
  //  form system matrices
  //
  A[0] = n12;
  A[1] = n13;

  rhs[0] = vtkMath::Dot2D(n12,x12);
  rhs[1] = vtkMath::Dot2D(n13,x13);

  // Solve system of equations
  //
  if ( vtkMath::SolveLinearSystem(A,rhs,2) == 0 )
    {
    center[0] = center[1] = 0.0;
    return VTK_LARGE_FLOAT;
    }
  else
    {
    center[0] = rhs[0]; center[1] = rhs[1];
    }

  //determine average value of radius squared
  for (sum=0, i=0; i<2; i++) 
    {
    diff = x1[i] - center[i];
    sum += diff*diff;
    diff = x2[i] - center[i];
    sum += diff*diff;
    diff = x3[i] - center[i];
    sum += diff*diff;
    }

  if ( (sum /= 3.0) > VTK_LARGE_FLOAT )
    {
    return VTK_LARGE_FLOAT;
    }
  else
    {
    return sum;
    }
}

// Given a 2D point x[2], determine the barycentric coordinates of the point.
// Barycentric coordinates are a natural coordinate system for simplices that
// express a position as a linear combination of the vertices. For a 
// triangle, there are three barycentric coordinates (because there are
// fourthree vertices), and the sum of the coordinates must equal 1. If a 
// point x is inside a simplex, then all three coordinates will be strictly 
// positive.  If two coordinates are zero (so the third =1), then the 
// point x is on a vertex. If one coordinates are zero, the point x is on an 
// edge. In this method, you must specify the vertex coordinates x1->x3. 
// Returns 0 if triangle is degenerate.
int vtkTriangle::BarycentricCoords(double x[2], double  x1[2], double x2[2], 
                                   double x3[2], double bcoords[3])
{
  double *A[3], p[3], a1[3], a2[3], a3[3];
  int i;

  // Homogenize the variables; load into arrays.
  //
  a1[0] = x1[0]; a1[1] = x2[0]; a1[2] = x3[0]; 
  a2[0] = x1[1]; a2[1] = x2[1]; a2[2] = x3[1]; 
  a3[0] = 1.0;   a3[1] = 1.0;   a3[2] = 1.0;   
  p[0] = x[0]; p[1] = x[1]; p[2] = 1.0;

  //   Now solve system of equations for barycentric coordinates
  //
  A[0] = a1;
  A[1] = a2;
  A[2] = a3;

  if ( vtkMath::SolveLinearSystem(A,p,3) )
    {
    for (i=0; i<3; i++)
      {
      bcoords[i] = p[i];
      }
    return 1;
    }
  else
    {
    return 0;
    }
}

// Project triangle defined in 3D to 2D coordinates. Returns 0 if degenerate triangle;
// non-zero value otherwise. Input points are x1->x3; output 2D points are v1->v3.
int vtkTriangle::ProjectTo2D(double x1[3], double x2[3], double x3[3],
                             double v1[2], double v2[2], double v3[2])
{
  double n[3], v21[3], v31[3], v[3], xLen;

  // Get normal for triangle
  vtkTriangle::ComputeNormal (x1, x2, x3, n);

  for (int i=0; i < 3; i++) 
    {
    v21[i] = x2[i] - x1[i];
    v31[i] = x3[i] - x1[i];
    }

  if ( (xLen=vtkMath::Normalize(v21)) <= 0.0 )
    {
    return 0;
    }

  // The first point is at (0,0); the next at (xLen,0); compute the other point relative 
  // to the first two.
  v1[0] = v1[1] = 0.0;
  v2[0] = xLen; v2[1] = 0.0;

  vtkMath::Cross(n,v21,v);

  v3[0] = vtkMath::Dot(v31,v21);
  v3[1] = vtkMath::Dot(v31,v);

  return 1;
}

// support triangle clipping
typedef int TRIANGLE_EDGE_LIST;
typedef struct {
       TRIANGLE_EDGE_LIST edges[7];
} TRIANGLE_CASES;
 
static TRIANGLE_CASES triangleCases[] = { 
{{-1, -1, -1, -1, -1, -1, -1}},	// 0
{{0, 2, 100, -1, -1, -1, -1}},	// 1
{{1, 0, 101, -1, -1, -1, -1}},	// 2
{{1, 2, 100, 1, 100, 101, -1}},	// 3
{{2, 1, 102, -1, -1, -1, -1}},	// 4
{{0, 1, 102, 102, 100, 0, -1}},	// 5
{{0, 101, 2, 2, 101, 102, -1}},	// 6
{{100, 101, 102, -1, -1, -1, -1}}	// 7
};

// Clip this triangle using scalar value provided. Like contouring, except
// that it cuts the triangle to produce other triangles.
void vtkTriangle::Clip(float value, vtkDataArray *cellScalars, 
                       vtkPointLocator *locator, vtkCellArray *tris,
                       vtkPointData *inPd, vtkPointData *outPd,
                       vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
                       int insideOut)
{
  static int CASE_MASK[3] = {1,2,4};
  TRIANGLE_CASES *triangleCase;
  TRIANGLE_EDGE_LIST  *edge;
  int i, j, index, *vert;
  int e1, e2, newCellId;
  vtkIdType pts[3];
  int vertexId;
  float t, x1[3], x2[3], x[3], deltaScalar;

  // Build the case table
  if ( insideOut )
    {    
    for ( i=0, index = 0; i < 3; i++)
      {
      if (cellScalars->GetComponent(i,0) <= value)
	{
	index |= CASE_MASK[i];
	}
      }
    }    
  else
    {
    for ( i=0, index = 0; i < 3; i++)
      {
      if (cellScalars->GetComponent(i,0) > value)
	{
        index |= CASE_MASK[i];
	}
      }
    }

  // Select the case based on the index and get the list of edges for this case
  triangleCase = triangleCases + index;
  edge = triangleCase->edges;

  // generate each triangle
  for ( ; edge[0] > -1; edge += 3 )
    {
    for (i=0; i<3; i++) // insert triangle
      {
      // vertex exists, and need not be interpolated
      if (edge[i] >= 100)
        {
	vertexId = edge[i] - 100;
        this->Points->GetPoint(vertexId, x);
        if ( locator->InsertUniquePoint(x, pts[i]) )
          {
          outPd->CopyData(inPd,this->PointIds->GetId(vertexId),pts[i]);
          }
	}

      else //new vertex, interpolate
        {
        vert = edges[edge[i]];

        // calculate a preferred interpolation direction
        deltaScalar = (cellScalars->GetComponent(vert[1],0) - cellScalars->GetComponent(vert[0],0));
        if (deltaScalar > 0)
          {
	  e1 = vert[0]; e2 = vert[1];
          }
        else
          {
	   e1 = vert[1]; e2 = vert[0];
           deltaScalar = -deltaScalar;
          }

	// linear interpolation
        if (deltaScalar == 0.0)
	  {
	  t = 0.0;
	  }
        else
	  {
	  t = (value - cellScalars->GetComponent(e1,0)) / deltaScalar;
	  }

        this->Points->GetPoint(e1, x1);
        this->Points->GetPoint(e2, x2);

        for (j=0; j<3; j++)
	  {
	  x[j] = x1[j] + t * (x2[j] - x1[j]);
	  }
        if ( locator->InsertUniquePoint(x, pts[i]) )
          {
          int p1 = this->PointIds->GetId(e1);
          int p2 = this->PointIds->GetId(e2);
          outPd->InterpolateEdge(inPd,pts[i],p1,p2,t);
          }
        }
      }
    // check for degenerate tri's
    if (pts[0] == pts[1] || pts[0] == pts[2] || pts[1] == pts[2])
      {
      continue;
      }

    newCellId = tris->InsertNextCell(3,pts);
    outCd->CopyData(inCd,cellId,newCellId);
    }
}

// Given a point x, determine whether it is inside (within the
// tolerance squared, tol2) the triangle defined by the three 
// coordinate values p1, p2, p3. Method is via comparing dot products.
// (Note: in current implementation the tolerance only works in the
// neighborhood of the three vertices of the triangle.
int vtkTriangle::PointInTriangle(float x[3], float p1[3], float p2[3], 
                                 float p3[3], float tol2)
{
  float       x1[3], x2[3], x3[3], v13[3], v21[3], v32[3];
  float       n1[3], n2[3], n3[3];
  int		i;
//
//  Compute appropriate vectors
//
  for (i=0; i<3; i++) 
    {
    x1[i] = x[i] - p1[i];
    x2[i] = x[i] - p2[i];
    x3[i] = x[i] - p3[i];
    v13[i] = p1[i] - p3[i];
    v21[i] = p2[i] - p1[i];
    v32[i] = p3[i] - p2[i];
    }
//
//  See whether intersection point is within tolerance of a vertex.
//
  if ( (x1[0]*x1[0] + x1[1]*x1[1] + x1[2]*x1[2]) <= tol2 ||
  (x2[0]*x2[0] + x2[1]*x2[1] + x2[2]*x2[2]) <= tol2 ||
  (x3[0]*x3[0] + x3[1]*x3[1] + x3[2]*x3[2]) <= tol2 )
    {
    return 1;
    }

//  If not near a vertex, check whether point is inside of triangular face.
//
//  Obtain normal off of triangular face
//
  vtkMath::Cross (x1, v13, n1);
  vtkMath::Cross (x2, v21, n2);
  vtkMath::Cross (x3, v32, n3);
//
//  Check whether normals go in same direction
//
  if ( (vtkMath::Dot(n1,n2) > 0.0) && (vtkMath::Dot(n2,n3) > 0.0) )
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
void vtkTriangle::ComputeQuadric(float x1[3], float x2[3], float x3[3],
				 float quadric[4][4])
{
  float crossX1X2[3], crossX2X3[3], crossX3X1[3];
  float determinantABC;
  float ABCx[3][3];
  float n[4];
  int i, j;
  
  for (i = 0; i < 3; i++)
    {
    ABCx[0][i] = x1[i];
    ABCx[1][i] = x2[i];
    ABCx[2][i] = x3[i];
    }
  
  vtkMath::Cross(x1, x2, crossX1X2);
  vtkMath::Cross(x2, x3, crossX2X3);
  vtkMath::Cross(x3, x1, crossX3X1);
  determinantABC = vtkMath::Determinant3x3(ABCx);
  
  n[0] = crossX1X2[0] + crossX2X3[0] + crossX3X1[0];
  n[1] = crossX1X2[1] + crossX2X3[1] + crossX3X1[1];
  n[2] = crossX1X2[2] + crossX2X3[2] + crossX3X1[2];
  n[3] = -determinantABC;
  
  for (i = 0; i < 4; i++)
    {
    for (j = 0; j < 4; j++)
      {
      quadric[i][j] = n[i] * n[j];
      }
    }
}

void vtkTriangle::ComputeQuadric(float x1[3], float x2[3], float x3[3],
				 vtkQuadric *quadric)
{
  float quadricMatrix[4][4];
  
  ComputeQuadric(x1, x2, x3, quadricMatrix);
  quadric->SetCoefficients(quadricMatrix[0][0], quadricMatrix[1][1],
			   quadricMatrix[2][2], 2*quadricMatrix[0][1],
			   2*quadricMatrix[1][2], 2*quadricMatrix[0][2],
			   2*quadricMatrix[0][3], 2*quadricMatrix[1][3],
			   2*quadricMatrix[2][3], quadricMatrix[3][3]);
}
