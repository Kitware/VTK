/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriangle.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTriangle.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkPoints.h"
#include "vtkPolygon.h"
#include "vtkQuadric.h"

#include <limits>
#include <utility>

vtkStandardNewMacro(vtkTriangle);

//----------------------------------------------------------------------------
// Construct the triangle with three points.
vtkTriangle::vtkTriangle()
{
  this->Points->SetNumberOfPoints(3);
  this->PointIds->SetNumberOfIds(3);
  for (int i = 0; i < 3; i++)
  {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i,0);
  }
  this->Line = vtkLine::New();
}

//----------------------------------------------------------------------------
vtkTriangle::~vtkTriangle()
{
  this->Line->Delete();
}

//----------------------------------------------------------------------------
// This function simply calls the static function:
// vtkTriangle::TriangleArea(double p1[3], double p2[3], double p3[3])
// with the appropriate parameters from the instantiated vtkTriangle.
double vtkTriangle::ComputeArea()
{
  double p0[3];
  double p1[3];
  double p2[3];
  this->GetPoints()->GetPoint(0, p0);
  this->GetPoints()->GetPoint(1, p1);
  this->GetPoints()->GetPoint(2, p2);
  return vtkTriangle::TriangleArea(p0, p1, p2);
}


//----------------------------------------------------------------------------
// Create a new cell and copy this triangle's information into the cell.
// Returns a poiner to the new cell created.
int vtkTriangle::EvaluatePosition(double x[3], double* closestPoint,
                                 int& subId, double pcoords[3],
                                 double& dist2, double *weights)
{
  int i, j;
  double pt1[3], pt2[3], pt3[3], n[3], fabsn;
  double rhs[2], c1[2], c2[2];
  double det;
  double maxComponent;
  int idx=0, indices[2];
  double dist2Point, dist2Line1, dist2Line2;
  double *closest, closestPoint1[3], closestPoint2[3], cp[3];

  subId = 0;
  pcoords[2] = 0.0;

  // Get normal for triangle, only the normal direction is needed, i.e. the
  // normal need not be normalized (unit length)
  //
  this->Points->GetPoint(1, pt1);
  this->Points->GetPoint(2, pt2);
  this->Points->GetPoint(0, pt3);

  vtkTriangle::ComputeNormalDirection(pt1, pt2, pt3, n);

  // Project point to plane
  //
  vtkPlane::GeneralizedProjectPoint(x,pt1,n,cp);

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
    pcoords[0] = pcoords[1] = 0.0;
    return -1;
  }

  pcoords[0] = vtkMath::Determinant2x2(rhs,c2) / det;
  pcoords[1] = vtkMath::Determinant2x2(c1,rhs) / det;

  // Okay, now find closest point to element
  //
  weights[0] = 1 - (pcoords[0] + pcoords[1]);
  weights[1] = pcoords[0];
  weights[2] = pcoords[1];

  if ( weights[0] >= 0.0 && weights[0] <= 1.0 &&
       weights[1] >= 0.0 && weights[1] <= 1.0 &&
       weights[2] >= 0.0 && weights[2] <= 1.0 )
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
    double t;
    if (closestPoint)
    {
      if ( weights[1] < 0.0 && weights[2] < 0.0 )
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
      else if ( weights[2] < 0.0 && weights[0] < 0.0 )
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
      else if ( weights[1] < 0.0 && weights[0] < 0.0 )
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
      else if ( weights[0] < 0.0 )
      {
        dist2 = vtkLine::DistanceToLine(x,pt1,pt2,t,closestPoint);
      }
      else if ( weights[1] < 0.0 )
      {
        dist2 = vtkLine::DistanceToLine(x,pt2,pt3,t,closestPoint);
      }
      else if ( weights[2] < 0.0 )
      {
        dist2 = vtkLine::DistanceToLine(x,pt1,pt3,t,closestPoint);
      }
    }
    return 0;
  }
}

//----------------------------------------------------------------------------
void vtkTriangle::EvaluateLocation(int& vtkNotUsed(subId), double pcoords[3],
                                   double x[3], double *weights)
{
  double u3;
  double pt0[3], pt1[3], pt2[3];
  int i;

  this->Points->GetPoint(0, pt0);
  this->Points->GetPoint(1, pt1);
  this->Points->GetPoint(2, pt2);

  u3 = 1.0 - pcoords[0] - pcoords[1];

  for (i=0; i<3; i++)
  {
    x[i] = pt0[i]*u3 + pt1[i]*pcoords[0] + pt2[i]*pcoords[1];
  }

  weights[0] = u3;
  weights[1] = pcoords[0];
  weights[2] = pcoords[1];
}

//----------------------------------------------------------------------------
// Compute iso-parametric interpolation functions
//
void vtkTriangle::InterpolationFunctions(double pcoords[3], double sf[3])
{
  sf[0] = 1. - pcoords[0] - pcoords[1];
  sf[1] = pcoords[0];
  sf[2] = pcoords[1];
}

//----------------------------------------------------------------------------
void vtkTriangle::InterpolationDerivs(double *, double derivs[6])
{
  //r-derivatives
  derivs[0] = -1;
  derivs[1] = 1;
  derivs[2] = 0;

  //s-derivatives
  derivs[3] = -1;
  derivs[4] = 0;
  derivs[5] = 1;
}

//----------------------------------------------------------------------------
int vtkTriangle::CellBoundary(int vtkNotUsed(subId), double pcoords[3],
                              vtkIdList *pts)
{
  double t1=pcoords[0]-pcoords[1];
  double t2=0.5*(1.0-pcoords[0])-pcoords[1];
  double t3=2.0*pcoords[0]+pcoords[1]-1.0;

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

//----------------------------------------------------------------------------
//
// Marching triangles
//
namespace { //required so we don't violate ODR
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
}

static int edges[3][2] = { {0,1}, {1,2}, {2,0} };

//----------------------------------------------------------------------------
int *vtkTriangle::GetEdgeArray(int edgeId)
{
  return edges[edgeId];
}

//----------------------------------------------------------------------------
void vtkTriangle::Contour(double value, vtkDataArray *cellScalars,
                          vtkIncrementalPointLocator *locator,
                          vtkCellArray *verts,
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
  double t, x1[3], x2[3], x[3], deltaScalar;
  vtkIdType offset = verts->GetNumberOfCells();

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
          vtkIdType p1 = this->PointIds->GetId(e1);
          vtkIdType p2 = this->PointIds->GetId(e2);
          outPd->InterpolateEdge(inPd,pts[i],p1,p2,t);
        }
      }
    }
    // check for degenerate line
    if ( pts[0] != pts[1] )
    {
      newCellId = offset + lines->InsertNextCell(2,pts);
      outCd->CopyData(inCd,cellId,newCellId);
    }
  }
}

//----------------------------------------------------------------------------
// Get the edge specified by edgeId (range 0 to 2) and return that edge's
// coordinates.
vtkCell *vtkTriangle::GetEdge(int edgeId)
{
  int edgeIdPlus1 = (edgeId > 1 ? 0 : (edgeId+1) );

  // load point id's
  this->Line->PointIds->SetId(0,this->PointIds->GetId(edgeId));
  this->Line->PointIds->SetId(1,this->PointIds->GetId(edgeIdPlus1));

  // load coordinates
  this->Line->Points->SetPoint(0,this->Points->GetPoint(edgeId));
  this->Line->Points->SetPoint(1,this->Points->GetPoint(edgeIdPlus1));

  return this->Line;
}

//----------------------------------------------------------------------------
// Plane intersection plus in/out test on triangle. The in/out test is
// performed using tol as the tolerance.
int vtkTriangle::IntersectWithLine(double p1[3], double p2[3], double tol,
                                  double& t, double x[3], double pcoords[3],
                                  int& subId)
{
  double pt1[3], pt2[3], pt3[3], n[3];
  double tol2 = tol*tol;
  double closestPoint[3];
  double dist2, weights[3];

  subId = 0;
  pcoords[2] = 0.0;

  // Get normal for triangle
  //
  this->Points->GetPoint(1, pt1);
  this->Points->GetPoint(2, pt2);
  this->Points->GetPoint(0, pt3);

  vtkTriangle::ComputeNormal (pt1, pt2, pt3, n);
  if (n[0] != 0 || n[1] != 0 || n[2] != 0)
  {
    // Intersect plane of triangle with line
    //
    if ( ! vtkPlane::IntersectWithLine(p1,p2,n,pt1,t,x) )
    {
      pcoords[0] = pcoords[1] = 0.0;
      return 0;
    }

    // Evaluate position
    //
    int inside;
    if ( (inside = this->EvaluatePosition(x, closestPoint, subId, pcoords,
          dist2, weights)) >= 0)
    {
      if ( dist2 <= tol2 )
      {
        return 1;
      }
      return inside;
    }
  }

  // Normals are null, so the triangle is degenerated and
  // we still need to check intersection between line and
  // the longest edge.
  double dist2Pt1Pt2 = vtkMath::Distance2BetweenPoints(pt1, pt2);
  double dist2Pt2Pt3 = vtkMath::Distance2BetweenPoints(pt2, pt3);
  double dist2Pt3Pt1 = vtkMath::Distance2BetweenPoints(pt3, pt1);
  if (dist2Pt1Pt2 > dist2Pt2Pt3 && dist2Pt1Pt2 > dist2Pt3Pt1)
  {
    this->Line->Points->InsertPoint(0,pt1);
    this->Line->Points->InsertPoint(1,pt2);
  }
  else if (dist2Pt2Pt3 > dist2Pt3Pt1 && dist2Pt2Pt3 > dist2Pt1Pt2)
  {
    this->Line->Points->InsertPoint(0,pt2);
    this->Line->Points->InsertPoint(1,pt3);
  }
  else
  {
    this->Line->Points->InsertPoint(0,pt3);
    this->Line->Points->InsertPoint(1,pt1);
  }

  if (this->Line->IntersectWithLine(p1,p2,tol,t,x,pcoords,subId))
  {
    // Compute r and s manually, using dot and norm.
    double pt3Pt1[3];
    double pt3Pt2[3];
    double pt3X[3];
    for (int i = 0; i < 3; i++)
    {
      pt3Pt1[i] = pt1[i] - pt3[i];
      pt3Pt2[i] = pt2[i] - pt3[i];
      pt3X[i] = x[i] - pt3[i];
    }
    pcoords[0] = vtkMath::Dot(pt3X, pt3Pt1) / dist2Pt3Pt1;
    pcoords[1] = vtkMath::Dot(pt3X, pt3Pt2) / dist2Pt2Pt3;
    return 1;
  }

  pcoords[0] = pcoords[1] = 0.0;
  return 0;
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
// Used a staged computation: first compute derivatives in local x'-y'
// coordinate system; then convert into x-y-z modelling system.
void vtkTriangle::Derivatives(int vtkNotUsed(subId), double vtkNotUsed(pcoords)[3],
                              double *values, int dim, double *derivs)
{
  double v0[2], v1[2], v2[2], v[3], v10[3], v20[3], lenX;
  double x0[3], x1[3], x2[3], n[3];
  double *J[2], J0[2], J1[2];
  double *JI[2], JI0[2], JI1[2];
  double functionDerivs[6], sum[2], dBydx, dBydy;
  int i, j;

  // Project points of triangle into 2D system
  this->Points->GetPoint(0, x0);
  this->Points->GetPoint(1, x1);
  this->Points->GetPoint(2, x2);
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
  vtkTriangle::InterpolationDerivs(NULL,functionDerivs);

  // Compute Jacobian: Jacobian is constant for a triangle.
  J[0] = J0; J[1] = J1;
  JI[0] = JI0; JI[1] = JI1;

  J[0][0] = v1[0] - v0[0];
  J[1][0] = v2[0] - v0[0];
  J[0][1] = v1[1] - v0[1];
  J[1][1] = v2[1] - v0[1];

  // Compute inverse Jacobian
  vtkMath::InvertMatrix(J,JI,2);

  // Loop over "dim" derivative values. For each set of values, compute
  // derivatives in local system and then transform into modelling system.
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

//----------------------------------------------------------------------------
// Compute the triangle normal from a points list, and a list of point ids
// that index into the points list.
void vtkTriangle::ComputeNormal(vtkPoints *p, int vtkNotUsed(numPts),
                                vtkIdType *pts, double n[3])
{
  double v1[3], v2[3], v3[3];

  p->GetPoint(pts[0],v1);
  p->GetPoint(pts[1],v2);
  p->GetPoint(pts[2],v3);

  vtkTriangle::ComputeNormal(v1,v2,v3,n);
}

//----------------------------------------------------------------------------
// Compute the circumcenter (center[3]) and radius squared (method
// return value) of a triangle defined by the three points x1, x2, and
// x3. (Note that the coordinates are 2D. 3D points can be used but
// the z-component will be ignored.)
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
    return VTK_DOUBLE_MAX;
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

  if ( (sum /= 3.0) > VTK_DOUBLE_MAX )
  {
    return VTK_DOUBLE_MAX;
  }
  else
  {
    return sum;
  }
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
// Project triangle defined in 3D to 2D coordinates. Returns 0 if degenerate
// triangle; non-zero value otherwise. Input points are x1->x3; output 2D
// points are v1->v3.
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

  // The first point is at (0,0); the next at (xLen,0); compute the other
  // point relative to the first two.
  v1[0] = v1[1] = 0.0;
  v2[0] = xLen; v2[1] = 0.0;

  vtkMath::Cross(n,v21,v);

  v3[0] = vtkMath::Dot(v31,v21);
  v3[1] = vtkMath::Dot(v31,v);

  return 1;
}

//----------------------------------------------------------------------------
// Support triangle clipping. Note that the table defines triangles (three ids
// at a time define a triangle, -1 ends the list). Numbers in the list >= 100
// correspond to already existing vertices; otherwise the numbers refer to edge
// ids.
namespace { //required so we don't violate ODR
typedef int TRIANGLE_EDGE_LIST;
typedef struct {
       TRIANGLE_EDGE_LIST edges[7];
} TRIANGLE_CASES;

static TRIANGLE_CASES triangleCases[] = {
{{-1, -1, -1, -1, -1, -1, -1}}, // 0
{{0, 2, 100, -1, -1, -1, -1}},  // 1
{{1, 0, 101, -1, -1, -1, -1}},  // 2
{{1, 2, 100, 1, 100, 101, -1}}, // 3
{{2, 1, 102, -1, -1, -1, -1}},  // 4
{{0, 1, 102, 102, 100, 0, -1}}, // 5
{{0, 101, 2, 2, 101, 102, -1}}, // 6
{{100, 101, 102, -1, -1, -1, -1}}       // 7
};
}

//----------------------------------------------------------------------------
// Clip this triangle using scalar value provided. Like contouring, except
// that it cuts the triangle to produce other triangles.
void vtkTriangle::Clip(double value, vtkDataArray *cellScalars,
                       vtkIncrementalPointLocator *locator, vtkCellArray *tris,
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
  double t, x1[3], x2[3], x[3], deltaScalar;

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
        deltaScalar = (cellScalars->GetComponent(vert[1],0) -
                       cellScalars->GetComponent(vert[0],0));
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
          vtkIdType p1 = this->PointIds->GetId(e1);
          vtkIdType p2 = this->PointIds->GetId(e2);
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

//----------------------------------------------------------------------------
namespace
{
double Determinant(double a[3], double b[3], double c[3], double d[3])
{
  // If > 0, d lies above the plane defined by (a,b,c)
  // if < 0, d lies below the plane defined by (a,b,c)
  // if = 0, d lies in the plane defined by (a,b,c)
  return vtkMath::Determinant3x3( a[0]-d[0], a[1]-d[1], a[2]-d[2],
                                  b[0]-d[0], b[1]-d[1], b[2]-d[2],
                                  c[0]-d[0], c[1]-d[1], c[2]-d[2] );
}

static const double eps = 256*std::numeric_limits<double>::epsilon();

// The orientation values are chosen so that any combination of 3 will produce
// a unique value.
enum OrientationState
{
  Colinear = 1,         // binary 1
  Clockwise = 2,        // binary 10
  Counterclockwise = 4, // binary 100
};

int Orientation(const double p1[2], const double p2[2], const double p3[2])
{
  // Return 4 if the path connecting p1,p2,p3 is counterclockwise.
  // Return 2 if the path connecting p1,p2,p3 is clockwise.
  // Return 1 if the points are colinear.
  double v1[2] = { p2[0]-p1[0], p2[1]-p1[1] };
  double v2[2] = { p3[0]-p1[0], p3[1]-p1[1] };
  double signedArea = v1[0]*v2[1] - v1[1]*v2[0];
  if ( std::abs( signedArea ) < eps )
  {
    return Colinear;
  }
  return ( signedArea > 0. ? Counterclockwise : Clockwise );
}

int CoplanarTrianglesIntersect(double p1[2], double q1[2], double r1[2],
                               double p2[2], double q2[2], double r2[2])
{
  // Determine whether or not triangle T1 = (p1,q1,r1) intersects triangle
  // T2 = (p2,q2,r2), assumming that they are coplanar. This method is adapted
  // from Olivier Devillers, Philippe Guigue. Faster Triangle-Triangle
  // Intersection Tests. RR-4488, IN-RIA. 2002. <inria-00072100>

  // First, we swap vertices if necessary so that T1 and T2 are oriented
  // counterclockwise.
  if ( Orientation( p1, q1, r1 ) == Clockwise )
  {
    std::swap(q1,r1);
  }
  if ( Orientation( p2, q2, r2 ) == Clockwise )
  {
    std::swap(q2,r2);
  }

  // Next, we compute the orientation of p1 w.r.t. the edges that comprise T2
  int p1Orientation[3] = { Orientation( p2, q2, p1 ),
                           Orientation( q2, r2, p1 ),
                           Orientation( r2, p2, p1 ) };

  // Three conditions for positive intersection:
  // 1. If all three orientations are counterclockwise, then p1 lies within T2.
  // 2. If two orientations are colinear, then p1 lies on a vertex of T2.
  // 3. If one orientation is colinear and the other two are counterclockwise,
  //    then p1 lies on an edge of T2.
  int sumOfSigns = p1Orientation[0] + p1Orientation[1] + p1Orientation[2];

 static const int Three_CounterClockwise = 3*Counterclockwise;
 static const int Two_Colinear_One_Clockwise = 2*Colinear + Clockwise;
 static const int Two_Colinear_One_Counterclockwise = (2*Colinear +
                                                       Counterclockwise);
 static const int One_Colinear_Two_Counterclockwise = (Colinear +
                                                       2*Counterclockwise);

  if (sumOfSigns == Three_CounterClockwise            || // condition 1
      sumOfSigns == Two_Colinear_One_Clockwise        || // condition 2
      sumOfSigns == Two_Colinear_One_Counterclockwise || // condition 2
      sumOfSigns == One_Colinear_Two_Counterclockwise )  // condition 3
  {
    return 1;
  }

  // If we have reached this point, then either
  // 1. Two orientations are counterclockwise and one is clockwise, or
  // 2. Two orientations are clockwise and one is counterclockwise.
  // Equivalently, from "Faster Triangle-Triangle Intersection Tests":
  // 1. p1 belongs to region R1
  // 2. p1 belongs to region R2
  // We permute T2 so that we have the following orienatation pattern:
  // (counterclockwise, either orientation, clockwise).
  // This orientation corresponds to p1 lying in either region R1 or R2
  int index;
  for (index = 0; index < 3; index++)
  {
    if (p1Orientation[index] == Counterclockwise &&
        p1Orientation[(index+2)%3] == Clockwise)
    {
      break;
    }
  }

  if (index == 3)
  {
    return 0;
  }

  double* T2[3] = {p2,q2,r2};
  p2 = T2[index];
  q2 = T2[(index+1)%3];
  r2 = T2[(index+2)%3];

  // First decision tree (p1 belongs to region R1)
  if (p1Orientation[(index+1)%3] == Counterclockwise)
  {
    if (Orientation( r2, p2, q1 ) != Clockwise) // Test I
    {
      if (Orientation( r2, p1, q1 ) != Clockwise) // Test II.a
      {
        if (Orientation( p1, p2, q1 ) != Clockwise) // Test III.a
        {
          return 1;
        }
        else
        {
          if (Orientation( p1, p2, r1 ) != Clockwise) // Test IV.a
          {
            if (Orientation( q1, r1, p2 ) != Clockwise) // Test V
            {
              return 1;
            }
            else
            {
              return 0;
            }
          }
          else
          {
            return 0;
          }
        }
      }
      else
      {
        return 0;
      }
    }
    else
    {
      if (Orientation( r2, p2, r1 ) != Clockwise) // Test II.b
      {
        if (Orientation( q1, r1, r2 ) == Clockwise) // Test III.b
        {
          return 0;
        }
        else
        {
          // The diagram in the paper has an error. Check the text for the
          // correct test.
          if (Orientation( p1, p2, r1 ) == Clockwise) // Test IV.b
          {
            return 0;
          }
          else
          {
            return 1;
          }
        }
      }
      else
      {
        return 0;
      }
    }
  }
  // Second decision tree (p1 belongs to region R2)
  else
  {
    if (Orientation( r2, p2, q1 ) != Clockwise) // Test I
    {
      if (Orientation( q2, r2, q1 ) != Clockwise) // Test II.a
      {
        if (Orientation( p1, p2, q1 ) != Clockwise) // Test III.a
        {
          if (Orientation( p1, q2, q1 ) == Counterclockwise) // Test IV.a
          {
            return 0;
          }
          else
          {
            return 1;
          }
        }
        else
        {
          if (Orientation( p1, p2, r1 ) == Clockwise) // Test IV.b
          {
            return 0;
          }
          else
          {
            // The paper has an error here.
            if (Orientation( p1, p2, r1 ) == Clockwise) // Test V.a
            {
              return 0;
            }
            else
            {
              return 1;
            }
          }
        }
      }
      else
      {
        // the paper has an error here: q1 is in Region R25 when (p1,q2,q1) is
        // clockwise.
        if (Orientation( p1, q2, q1 ) != Counterclockwise) // Test III.b
        {
          if (Orientation( q2, r2, r1 ) == Clockwise) // Test IV.c
          {
            return 0;
          }
          else
          {
            if (Orientation( q1, r1, q2 ) == Clockwise) // Test V.b
            {
              return 0;
            }
            else
            {
              return 1;
            }
          }
        }
        else
        {
          return 0;
        }
      }
    }
    else
    {
      if (Orientation( r2, p2, r1 ) == Clockwise) // Test II.b
      {
        return 0;
      }
      else
      {
        if (Orientation( q1, r1, r2 ) != Clockwise) // Test III.c
        {
          if (Orientation( r1, p1, p2 ) == Clockwise) // Test IV.d
          {
            return 0;
          }
          else
          {
            return 1;
          }
        }
        else
        {
          if (Orientation( q1, r1, q2 ) == Clockwise) // Test IV.e
          {
            return 0;
          }
          else
          {
            if (Orientation( q2, r2, r1 ) == Clockwise) // Test V.c
            {
              return 0;
            }
            else
            {
              return 1;
            }
          }
        }
      }
    }
  }
}

}

// Determine whether or not triangle (p1,q1,r1) intersects triangle (p2,q2,r2).
// This method is adapted from Olivier Devillers, Philippe Guigue. Faster
// Triangle-Triangle Intersection Tests. RR-4488, IN-RIA. 2002. <inria-00072100>
int vtkTriangle::TrianglesIntersect(double p1[3], double q1[3], double r1[3],
                                    double p2[3], double q2[3], double r2[3])
{
  // Triangle T1 = (p1,q1,r1) and lies in plane Pi1
  // Triangle T2 = (p2,q2,r2) and lies in plane Pi2

  // First, we determine whether T1 intersects Pi2
  double det1[3] = { Determinant( p2, q2, r2, p1 ),
                     Determinant( p2, q2, r2, q1 ),
                     Determinant( p2, q2, r2, r1 ) };

  if ( std::abs( det1[0] ) < eps && std::abs( det1[1] ) < eps &&
       std::abs( det1[2] ) < eps)
  {
    // The triangles are coplanar. We pick the Cartesian principal plane that
    // maximizes their projected area and perform the query in 2-D.
    double v1[3], v2[3];
    for (int i=0;i<3;i++)
    {
      v1[i] = q1[i] - p1[i];
      v2[i] = r1[i] - p1[i];
    }
    double normal[3];
    vtkMath::Cross( v1, v2, normal );

    int index = 0;
    for (int i=1;i<3;i++)
    {
      if (std::abs( normal[index] ) > std::abs( normal[i] ))
      {
        index = i;
      }
    }

    if (index == 0)
    {
      return CoplanarTrianglesIntersect(&p1[1], &q1[1], &r1[1],
                                        &p2[1], &q2[1], &r2[1]);
    }
    if (index == 1)
    {
      double p1_[2] = { p1[0], p1[2] };
      double q1_[2] = { q1[0], q1[2] };
      double r1_[2] = { r1[0], r1[2] };
      double p2_[2] = { p2[0], p2[2] };
      double q2_[2] = { q2[0], q2[2] };
      double r2_[2] = { r2[0], r2[2] };

      return CoplanarTrianglesIntersect(p1_, q1_, r1_, p2_, q2_, r2_);
    }
    else
    {
      return CoplanarTrianglesIntersect(p1, q1, r1, p2, q2, r2);
    }
  }

  bool degenerate = false;
  double* points[3] = {p1,q1,r1};
  for (int i=0;i<3;i++)
  {
    if (std::abs( det1[i] ) < eps)
    {
      degenerate = true;
      if (vtkTriangle::PointInTriangle(points[i], p2,q2,r2, eps))
      {
        return 1;
      }
    }
  }

  if (degenerate)
  {
    return 0;
  }

  // Do the three vertices of T1 lie in the same half-space defined by Pi2?
  {
    int sumOfSigns = ( det1[0] > 0. ) + ( det1[1] > 0. ) + ( det1[2] > 0. );
    if (sumOfSigns == 0 || sumOfSigns == 3)
    {
      // Yes.
      return 0;
    }
  }

  // Next, we determine whether T2 intersects Pi1
  double det2[3] = { Determinant( p1, q1, r1, p2 ),
                     Determinant( p1, q1, r1, q2 ),
                     Determinant( p1, q1, r1, r2 ) };

  // Do the three vertices of T2 lie in the same half-space defined by Pi1?
  {
    int sumOfSigns = ( det2[0] > 0. ) + ( det2[1] > 0. ) + ( det2[2] > 0. );
    if (sumOfSigns == 0 || sumOfSigns == 3)
    {
      // Yes.
      return 0;
    }
  }

  // We know that one point in T1 lies on one side of Pi2 and the other two
  // points lie on the other side (sim. for T2 and Pi1). We permute our vertices
  // so p1 is alone in its half-space, and q1, r1 are in the other halfspace
  // (sim. for p2, q2, r2). Additionally, we swap q2 and r2 (sim. for q1 and r1)
  // if necessary so that p1 lies in the positive half-space of Pi2 (sim for p2
  // and Pi1).
  int index1;
  for (index1 = 0; index1 < 3; index1++)
  {
    int sumOfSigns = ( det1[(index1+1)%3] > 0. ) + ( det1[(index1+2)%3] > 0. );
    if (sumOfSigns != 1)
    {
      break;
    }
  }
  assert(index1 >= 0 && index1 < 3);

  double* T1[3] = {p1,q1,r1};
  p1 = T1[index1];
  q1 = T1[(index1+1)%3];
  r1 = T1[(index1+2)%3];
  bool swap1 = (det1[index1] < -eps);

  int index2;
  for (index2 = 0; index2 < 3; index2++)
  {
    int sumOfSigns = ( det2[(index2+1)%3] > 0. ) + ( det2[(index2+2)%3] > 0. );
    if (sumOfSigns != 1)
    {
      break;
    }
  }
  assert(index2 >= 0 && index2 < 3);

  double* T2[3] = {p2,q2,r2};
  p2 = T2[index2];
  q2 = T2[(index2+1)%3];
  r2 = T2[(index2+2)%3];
  bool swap2 = (det2[index2] < -eps);

  if (swap1)
  {
    std::swap(q2,r2);
  }

  if (swap2)
  {
    std::swap(q1,r1);
  }

  // The final step is to determine whether or not the line segments formed by
  // the intersection of T1 and Pi2 and the intersection of T2 and Pi1 overlap.
  // This is done by checking the following predicate:
  // Determinant(p1,q1,p2,q2) <= 0. ^ Determinant(p1,r1,r2,p2) <= 0.
  if ((Determinant( p1, q1, p2, q2 ) <= 0.) *
      (Determinant( p1, r1, r2, p2 ) <= 0.))
  {
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
// Given a point x, determine whether it is inside (within the
// tolerance squared, tol2) the triangle defined by the three
// coordinate values p1, p2, p3. Method is via comparing dot products.
// (Note: in current implementation the tolerance only works in the
// neighborhood of the three vertices of the triangle.
int vtkTriangle::PointInTriangle(double x[3], double p1[3], double p2[3],
                                 double p3[3], double tol2)
{
  double       x1[3], x2[3], x3[3], v13[3], v21[3], v32[3];
  double       n1[3], n2[3], n3[3];
  int           i;

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

  //  Check whether ALL the three normals go in same direction
  //
  if ( (vtkMath::Dot(n1,n2) >= 0.0) &&
       (vtkMath::Dot(n2,n3) >= 0.0) &&
       (vtkMath::Dot(n1,n3) >= 0.0) )
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------------
double vtkTriangle::GetParametricDistance(double pcoords[3])
{
  int i;
  double pDist, pDistMax=0.0;
  double pc[3];

  pc[0] = pcoords[0];
  pc[1] = pcoords[1];
  pc[2] = 1.0 - pcoords[0] - pcoords[1];

  for (i=0; i<3; i++)
  {
    if ( pc[i] < 0.0 )
    {
      pDist = -pc[i];
    }
    else if ( pc[i] > 1.0 )
    {
      pDist = pc[i] - 1.0;
    }
    else //inside the cell in the parametric direction
    {
      pDist = 0.0;
    }
    if ( pDist > pDistMax )
    {
      pDistMax = pDist;
    }
  }

  return pDistMax;
}


//----------------------------------------------------------------------------
void vtkTriangle::ComputeQuadric(double x1[3], double x2[3], double x3[3],
                                 double quadric[4][4])
{
  double crossX1X2[3], crossX2X3[3], crossX3X1[3];
  double determinantABC;
  double ABCx[3][3];
  double n[4];
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

//----------------------------------------------------------------------------
void vtkTriangle::ComputeQuadric(double x1[3], double x2[3], double x3[3],
                                 vtkQuadric *quadric)
{
  double quadricMatrix[4][4];

  ComputeQuadric(x1, x2, x3, quadricMatrix);
  quadric->SetCoefficients(quadricMatrix[0][0], quadricMatrix[1][1],
                           quadricMatrix[2][2], 2*quadricMatrix[0][1],
                           2*quadricMatrix[1][2], 2*quadricMatrix[0][2],
                           2*quadricMatrix[0][3], 2*quadricMatrix[1][3],
                           2*quadricMatrix[2][3], quadricMatrix[3][3]);
}

//----------------------------------------------------------------------------
static double vtkTriangleCellPCoords[9] =
{0.0,0.0,0.0, 1.0,0.0,0.0, 0.0,1.0,0.0};
double *vtkTriangle::GetParametricCoords()
{
  return vtkTriangleCellPCoords;
}

//----------------------------------------------------------------------------
void vtkTriangle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Line:\n";
  this->Line->PrintSelf(os,indent.GetNextIndent());
}
