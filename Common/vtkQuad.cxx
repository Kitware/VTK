/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuad.cxx
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
#include "vtkQuad.h"
#include "vtkTriangle.h"
#include "vtkPlane.h"
#include "vtkCellArray.h"
#include "vtkLine.h"
#include "vtkPointLocator.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------
vtkQuad* vtkQuad::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkQuad");
  if(ret)
    {
    return (vtkQuad*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkQuad;
}

// Construct the quad with four points.
vtkQuad::vtkQuad()
{
  int i;
  
  this->Points->SetNumberOfPoints(4);
  this->PointIds->SetNumberOfIds(4);
  for (i = 0; i < 4; i++)
    {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    }
  for (i = 0; i < 4; i++)
    {
    this->PointIds->SetId(i,0);
    }
  this->Line = vtkLine::New();
}

vtkQuad::~vtkQuad()
{
  this->Line->Delete();
}

vtkCell *vtkQuad::MakeObject()
{
  vtkCell *cell = vtkQuad::New();
  cell->DeepCopy(this);
  return cell;
}

static const int VTK_QUAD_MAX_ITERATION=20;
static const float VTK_QUAD_CONVERGED=1.e-05;

int vtkQuad::EvaluatePosition(float x[3], float* closestPoint,
                             int& subId, float pcoords[3], 
                             float& dist2, float *weights)
{
  int i, j;
  float *pt1, *pt2, *pt3, *pt, n[3];
  float det;
  float maxComponent;
  int idx=0, indices[2];
  int iteration, converged;
  float  params[2];
  float  fcol[2], rcol[2], scol[2], cp[3];
  float derivs[8];

  subId = 0;
  pcoords[0] = pcoords[1] = params[0] = params[1] = 0.5;
  //
  // Get normal for quadrilateral
  //
  pt1 = this->Points->GetPoint(0);
  pt2 = this->Points->GetPoint(1);
  pt3 = this->Points->GetPoint(2);

  vtkTriangle::ComputeNormal (pt1, pt2, pt3, n);
  //
  // Project point to plane
  //
  vtkPlane::ProjectPoint(x,pt1,n,cp);
  //
  // Construct matrices.  Since we have over determined system, need to find
  // which 2 out of 3 equations to use to develop equations. (Any 2 should 
  // work since we've projected point to plane.)
  //
  for (maxComponent=0.0, i=0; i<3; i++)
    {
    if (fabs(n[i]) > maxComponent)
      {
      maxComponent = fabs(n[i]);
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
  //
  // Use Newton's method to solve for parametric coordinates
  //  
  for (iteration=converged=0; !converged
         && (iteration < VTK_QUAD_MAX_ITERATION);
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
    for (i=0; i<2; i++) 
      {
      fcol[i] = rcol[i] = scol[i] = 0.0;
      }
    for (i=0; i<4; i++)
      {
      pt = this->Points->GetPoint(i);
      for (j=0; j<2; j++)
        {
        fcol[j] += pt[indices[j]] * weights[i];
        rcol[j] += pt[indices[j]] * derivs[i];
        scol[j] += pt[indices[j]] * derivs[i+4];
        }
      }

    for (j=0; j<2; j++)
      {
      fcol[j] -= cp[indices[j]];
      }
    //
    //  compute determinants and generate improvements
    //
    if ( (det=vtkMath::Determinant2x2(rcol,scol)) == 0.0 )
      {
      return -1;
      }

    pcoords[0] = params[0] - vtkMath::Determinant2x2 (fcol,scol) / det;
    pcoords[1] = params[1] - vtkMath::Determinant2x2 (rcol,fcol) / det;
    //
    //  check for convergence
    //
    if ( ((fabs(pcoords[0]-params[0])) < VTK_QUAD_CONVERGED) &&
         ((fabs(pcoords[1]-params[1])) < VTK_QUAD_CONVERGED) )
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
      }
    }
  //
  //  if not converged, set the parametric coordinates to arbitrary values
  //  outside of element
  //
  if ( !converged )
    {
    return -1;
    }

  this->InterpolationFunctions(pcoords, weights);

  if ( pcoords[0] >= -0.001 && pcoords[0] <= 1.001 &&
       pcoords[1] >= -0.001 && pcoords[1] <= 1.001 )
    {
    if (closestPoint)
      {
      dist2 = 
        vtkMath::Distance2BetweenPoints(cp,x); //projection distance
      closestPoint[0] = cp[0];
      closestPoint[1] = cp[1];
      closestPoint[2] = cp[2];
      }
    return 1;
    }
  else
    {
    float t;
    float *pt4;
    
    if (closestPoint)
      {
      pt4 = this->Points->GetPoint(3);
      
      if ( pcoords[0] < 0.0 && pcoords[1] < 0.0 )
        {
        dist2 = vtkMath::Distance2BetweenPoints(x,pt1);
        for (i=0; i<3; i++)
          {
        closestPoint[i] = pt1[i];
          }
        }
      else if ( pcoords[0] > 1.0 && pcoords[1] < 0.0 )
        {
        dist2 = vtkMath::Distance2BetweenPoints(x,pt2);
        for (i=0; i<3; i++)
          {
          closestPoint[i] = pt2[i];
          }
        }
      else if ( pcoords[0] > 1.0 && pcoords[1] > 1.0 )
        {
        dist2 = vtkMath::Distance2BetweenPoints(x,pt3);
        for (i=0; i<3; i++)
          {
          closestPoint[i] = pt3[i];
          }
        }
      else if ( pcoords[0] < 0.0 && pcoords[1] > 1.0 )
        {
        dist2 = vtkMath::Distance2BetweenPoints(x,pt4);
        for (i=0; i<3; i++)
          {
          closestPoint[i] = pt4[i];
          }
        }
      else if ( pcoords[0] < 0.0 )
        {
        dist2 = vtkLine::DistanceToLine(x,pt1,pt4,t,closestPoint);
        }
      else if ( pcoords[0] > 1.0 )
        {
        dist2 = vtkLine::DistanceToLine(x,pt2,pt3,t,closestPoint);
        }
      else if ( pcoords[1] < 0.0 )
        {
        dist2 = vtkLine::DistanceToLine(x,pt1,pt2,t,closestPoint);
        }
      else if ( pcoords[1] > 1.0 )
        {
        dist2 = vtkLine::DistanceToLine(x,pt3,pt4,t,closestPoint);
        }
      }
    return 0;
    }
}

void vtkQuad::EvaluateLocation(int& vtkNotUsed(subId), float pcoords[3], 
                               float x[3], float *weights)
{
  int i, j;
  float *pt;

  this->InterpolationFunctions(pcoords, weights);

  x[0] = x[1] = x[2] = 0.0;
  for (i=0; i<4; i++)
    {
    pt = this->Points->GetPoint(i);
    for (j=0; j<3; j++)
      {
      x[j] += pt[j] * weights[i];
      }
    }
}

//
// Compute iso-parametrix interpolation functions
//
void vtkQuad::InterpolationFunctions(float pcoords[3], float sf[4])
{
  double rm, sm;

  rm = 1. - pcoords[0];
  sm = 1. - pcoords[1];

  sf[0] = rm * sm;
  sf[1] = pcoords[0] * sm;
  sf[2] = pcoords[0] * pcoords[1];
  sf[3] = rm * pcoords[1];
}

void vtkQuad::InterpolationDerivs(float pcoords[3], float derivs[8])
{
  double rm, sm;

  rm = 1. - pcoords[0];
  sm = 1. - pcoords[1];

  derivs[0] = -sm;
  derivs[1] = sm;
  derivs[2] = pcoords[1];
  derivs[3] = -pcoords[1];
  derivs[4] = -rm;
  derivs[5] = -pcoords[0];
  derivs[6] = pcoords[0];
  derivs[7] = rm;
}

int vtkQuad::CellBoundary(int vtkNotUsed(subId), float pcoords[3], 
                          vtkIdList *pts)
{
  float t1=pcoords[0]-pcoords[1];
  float t2=1.0-pcoords[0]-pcoords[1];

  pts->SetNumberOfIds(2);

  // compare against two lines in parametric space that divide element
  // into four pieces.
  if ( t1 >= 0.0 && t2 >= 0.0 )
    {
    pts->SetId(0,this->PointIds->GetId(0));
    pts->SetId(1,this->PointIds->GetId(1));
    }

  else if ( t1 >= 0.0 && t2 < 0.0 )
    {
    pts->SetId(0,this->PointIds->GetId(1));
    pts->SetId(1,this->PointIds->GetId(2));
    }

  else if ( t1 < 0.0 && t2 < 0.0 )
    {
    pts->SetId(0,this->PointIds->GetId(2));
    pts->SetId(1,this->PointIds->GetId(3));
    }

  else //( t1 < 0.0 && t2 >= 0.0 )
    {
    pts->SetId(0,this->PointIds->GetId(3));
    pts->SetId(1,this->PointIds->GetId(0));
    }

  if ( pcoords[0] < 0.0 || pcoords[0] > 1.0 ||
       pcoords[1] < 0.0 || pcoords[1] > 1.0 )
    {
    return 0;
    }
  else
    {
    return 1;
    }
}

//
// Marching (convex) quadrilaterals
//
static int edges[4][2] = { {0,1}, {1,2}, {3,2}, {0,3} };

typedef int EDGE_LIST;
typedef struct {
       EDGE_LIST edges[5];
} LINE_CASES;

static LINE_CASES lineCases[] = { 
  {{-1, -1, -1, -1, -1}},
  {{0, 3, -1, -1, -1}},
  {{1, 0, -1, -1, -1}},
  {{1, 3, -1, -1, -1}},
  {{2, 1, -1, -1, -1}},
  {{0, 3, 2, 1, -1}},
  {{2, 0, -1, -1, -1}},
  {{2, 3, -1, -1, -1}},
  {{3, 2, -1, -1, -1}},
  {{0, 2, -1, -1, -1}},
  {{1, 0, 3, 2, -1}},
  {{1, 2, -1, -1, -1}},
  {{3, 1, -1, -1, -1}},
  {{0, 1, -1, -1, -1}},
  {{3, 0, -1, -1, -1}},
  {{-1, -1, -1, -1, -1}}
};

void vtkQuad::Contour(float value, vtkDataArray *cellScalars, 
                      vtkPointLocator *locator, 
                      vtkCellArray *vtkNotUsed(verts), 
                      vtkCellArray *lines, 
                      vtkCellArray *vtkNotUsed(polys), 
                      vtkPointData *inPd, vtkPointData *outPd,
                      vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd)
{
  static int CASE_MASK[4] = {1,2,4,8};
  LINE_CASES *lineCase;
  EDGE_LIST  *edge;
  int i, j, index, *vert;
  int newCellId;
  vtkIdType pts[2];
  int e1, e2;
  float t, x1[3], x2[3], x[3], deltaScalar;

  // Build the case table
  for ( i=0, index = 0; i < 4; i++)
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

vtkCell *vtkQuad::GetEdge(int edgeId)
{
  int edgeIdPlus1 = edgeId + 1;
  
  if (edgeIdPlus1 > 3)
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

// 
// Intersect plane; see whether point is in quadrilateral.
//
int vtkQuad::IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                              float x[3], float pcoords[3], int& subId)
{
  float *pt1, *pt2, *pt3, n[3];
  float tol2 = tol*tol;
  float closestPoint[3];
  float dist2, weights[4];

  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;
  //
  // Get normal for triangle
  //
  pt1 = this->Points->GetPoint(0);
  pt2 = this->Points->GetPoint(1);
  pt3 = this->Points->GetPoint(2);

  vtkTriangle::ComputeNormal (pt1, pt2, pt3, n);
  //
  // Intersect plane of triangle with line
  //
  if ( ! vtkPlane::IntersectWithLine(p1,p2,n,pt1,t,x) )
    {
    return 0;
    }
  //
  // See whether point is in triangle by evaluating its position.
  //
  if ( this->EvaluatePosition(x, closestPoint, subId,
                              pcoords, dist2, weights) == 1)
    {
    if ( dist2 <= tol2 )
      {
      return 1;
      }
    }

  return 0;
}

int vtkQuad::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds,
                         vtkPoints *pts)
{
  float d1, d2;

  pts->Reset();
  ptIds->Reset();

  // use minimum diagonal (Delaunay triangles) - assumed convex
  d1 = vtkMath::Distance2BetweenPoints(this->Points->GetPoint(0), 
                                   this->Points->GetPoint(2));
  d2 = vtkMath::Distance2BetweenPoints(this->Points->GetPoint(1), 
                                   this->Points->GetPoint(3));

  if ( d1 <= d2 )
    {
    ptIds->InsertId(0,this->PointIds->GetId(0));
    pts->InsertPoint(0,this->Points->GetPoint(0));
    ptIds->InsertId(1,this->PointIds->GetId(1));
    pts->InsertPoint(1,this->Points->GetPoint(1));
    ptIds->InsertId(2,this->PointIds->GetId(2));
    pts->InsertPoint(2,this->Points->GetPoint(2));

    ptIds->InsertId(3,this->PointIds->GetId(0));
    pts->InsertPoint(3,this->Points->GetPoint(0));
    ptIds->InsertId(4,this->PointIds->GetId(2));
    pts->InsertPoint(4,this->Points->GetPoint(2));
    ptIds->InsertId(5,this->PointIds->GetId(3));
    pts->InsertPoint(5,this->Points->GetPoint(3));
    }
  else
    {
    ptIds->InsertId(0,this->PointIds->GetId(0));
    pts->InsertPoint(0,this->Points->GetPoint(0));
    ptIds->InsertId(1,this->PointIds->GetId(1));
    pts->InsertPoint(1,this->Points->GetPoint(1));
    ptIds->InsertId(2,this->PointIds->GetId(3));
    pts->InsertPoint(2,this->Points->GetPoint(3));

    ptIds->InsertId(3,this->PointIds->GetId(1));
    pts->InsertPoint(3,this->Points->GetPoint(1));
    ptIds->InsertId(4,this->PointIds->GetId(2));
    pts->InsertPoint(4,this->Points->GetPoint(2));
    ptIds->InsertId(5,this->PointIds->GetId(3));
    pts->InsertPoint(5,this->Points->GetPoint(3));
    }

  return 1;
}

void vtkQuad::Derivatives(int vtkNotUsed(subId), float pcoords[3], 
                          float *values, int dim, float *derivs)
{
  float v0[2], v1[2], v2[2], v3[2], v10[3], v20[3], lenX;
  float *x0, *x1, *x2, *x3, n[3], vec20[3], vec30[3];
  double *J[2], J0[2], J1[2];
  double *JI[2], JI0[2], JI1[2];
  float funcDerivs[8], sum[2], dBydx, dBydy;
  int i, j;

  // Project points of quad into 2D system
  x0 = this->Points->GetPoint(0);
  x1 = this->Points->GetPoint(1);
  x2 = this->Points->GetPoint(2);
  x3 = this->Points->GetPoint(3);
  vtkTriangle::ComputeNormal (x0, x1, x2, n);

  for (i=0; i < 3; i++) 
    {
    v10[i] = x1[i] - x0[i];
    vec20[i] = x2[i] - x0[i];
    vec30[i] = x3[i] - x0[i];
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
  v2[0] = vtkMath::Dot(vec20,v10);
  v2[1] = vtkMath::Dot(vec20,v20);
  v3[0] = vtkMath::Dot(vec30,v10);
  v3[1] = vtkMath::Dot(vec30,v20);

  this->InterpolationDerivs(pcoords, funcDerivs);

  // Compute Jacobian and inverse Jacobian
  J[0] = J0; J[1] = J1;
  JI[0] = JI0; JI[1] = JI1;

  J[0][0] = v0[0]*funcDerivs[0] + v1[0]*funcDerivs[1] +
            v2[0]*funcDerivs[2] + v3[0]*funcDerivs[3];
  J[0][1] = v0[1]*funcDerivs[0] + v1[1]*funcDerivs[1] +
            v2[1]*funcDerivs[2] + v3[1]*funcDerivs[3];
  J[1][0] = v0[0]*funcDerivs[4] + v1[0]*funcDerivs[5] +
            v2[0]*funcDerivs[6] + v3[0]*funcDerivs[7];
  J[1][1] = v0[1]*funcDerivs[4] + v1[1]*funcDerivs[5] +
            v2[1]*funcDerivs[6] + v3[1]*funcDerivs[7];

  // Compute inverse Jacobian, return if Jacobian is singular
  if (!vtkMath::InvertMatrix(J,JI,2))
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

  // Loop over "dim" derivative values. For each set of values, 
  // compute derivatives
  // in local system and then transform into modelling system.
  // First compute derivatives in local x'-y' coordinate system
  for ( j=0; j < dim; j++ )
    {
    sum[0] = sum[1] = 0.0;
    for ( i=0; i < 4; i++) //loop over interp. function derivatives
      {
      sum[0] += funcDerivs[i] * values[dim*i + j]; 
      sum[1] += funcDerivs[4 + i] * values[dim*i + j];
      }
    dBydx = sum[0]*JI[0][0] + sum[1]*JI[0][1];
    dBydy = sum[0]*JI[1][0] + sum[1]*JI[1][1];

    // Transform into global system (dot product with global axes)
    derivs[3*j] = dBydx * v10[0] + dBydy * v20[0];
    derivs[3*j + 1] = dBydx * v10[1] + dBydy * v20[1];
    derivs[3*j + 2] = dBydx * v10[2] + dBydy * v20[2];
    }
}

// support quad clipping
typedef int QUAD_EDGE_LIST;
typedef struct {
       QUAD_EDGE_LIST edges[14];
} QUAD_CASES;
 
static QUAD_CASES quadCases[] = { 
{{  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 0
{{   3, 100,   0,   3,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 1
{{   3, 101,   1,   0,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 2
{{   4, 100, 101,   1,   3,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 3
{{   3, 102,   2,   1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 4
{{   3, 100,   0,   3,   3, 102,   2,   1,   4,   0,   1,   2,   3,  -1}}, // 5
{{   4, 101, 102,   2,   0,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 6
{{   3, 100, 101,   3,   3, 101,   2,   3,   3, 101, 102,   2,  -1,  -1}}, // 7
{{   3, 103,   3,   2,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 8
{{   4, 100,   0,   2, 103,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 9
{{   3, 101,   1,   0,   3, 103,   3,   2,   4,   0,   1,   2,   3,  -1}}, // 10
{{   3, 100, 101,   1,   3, 100,   1,   2,   3, 100,   2, 103,  -1,  -1}}, // 11
{{   4, 102, 103,   3,   1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 12
{{   3, 100,   0, 103,   3,   0,   1, 103,   3,   1, 102, 103,  -1,  -1}}, // 13
{{   3,   0, 101, 102,   3,   0, 102,   3,   3, 102, 103,   3,  -1,  -1}}, // 14
{{   4, 100, 101, 102, 103,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 15
};

static QUAD_CASES quadCasesComplement[] = { 
{{  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 0
{{   3, 100,   0,   3,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 1
{{   3, 101,   1,   0,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 2
{{   4, 100, 101,   1,   3,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 3
{{   3, 102,   2,   1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 4
{{   3, 100,   0,   3,   3, 102,   2,   1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 5
{{   4, 101, 102,   2,   0,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 6
{{   3, 100, 101,   3,   3, 101,   2,   3,   3, 101, 102,   2,  -1,  -1}}, // 7
{{   3, 103,   3,   2,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 8
{{   4, 100,   0,   2, 103,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 9
{{   3, 101,   1,   0,   3, 103,   3,   2,  -1,  -1,  -1,  -1,  -1,  -1}}, // 10
{{   3, 100, 101,   1,   3, 100,   1,   2,   3, 100,   2, 103,  -1,  -1}}, // 11
{{   4, 102, 103,   3,   1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 12
{{   3, 100,   0, 103,   3,   0,   1, 103,   3,   1, 102, 103,  -1,  -1}}, // 13
{{   3,   0, 101, 102,   3,   0, 102,   3,   3, 102, 103,   3,  -1,  -1}}, // 14
{{   4, 100, 101, 102, 103,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 15
};

// Clip this quad using scalar value provided. Like contouring, except
// that it cuts the quad to produce other quads and/or triangles.
void vtkQuad::Clip(float value, vtkDataArray *cellScalars, 
                   vtkPointLocator *locator, vtkCellArray *polys,
                   vtkPointData *inPd, vtkPointData *outPd,
                   vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
                   int insideOut)
{
  static int CASE_MASK[4] = {1,2,4,8};
  QUAD_CASES *quadCase;
  QUAD_EDGE_LIST  *edge;
  int i, j, index, *vert;
  int e1, e2;
  int newCellId;
  vtkIdType pts[4];
  int vertexId;
  float t, x1[3], x2[3], x[3], deltaScalar;
  float scalar0, scalar1, e1Scalar;

  // Build the index into the case table
  if ( insideOut )
    {    
    for ( i=0, index = 0; i < 4; i++)
      {
      if (cellScalars->GetComponent(i,0) <= value)
        {
        index |= CASE_MASK[i];
        }
      }
    // Select case based on the index and get the list of edges for this case
    quadCase = quadCases + index;
    }    
  else
    {
    for ( i=0, index = 0; i < 4; i++)
      {
      if (cellScalars->GetComponent(i,0) > value)
        {
        index |= CASE_MASK[i];
        }
      }
    // Select case based on the index and get the list of edges for this case
    quadCase = quadCasesComplement + index;
    }

  edge = quadCase->edges;

  // generate each quad
  for ( ; edge[0] > -1; edge += edge[0]+1 )
    {
    for (i=0; i < edge[0]; i++) // insert quad or triangle
      {
      // vertex exists, and need not be interpolated
      if (edge[i+1] >= 100)
        {
        vertexId = edge[i+1] - 100;
        this->Points->GetPoint(vertexId, x);
        if ( locator->InsertUniquePoint(x, pts[i]) )
          {
          outPd->CopyData(inPd,this->PointIds->GetId(vertexId),pts[i]);
          }
        }

      else //new vertex, interpolate
        {
        vert = edges[edge[i+1]];

        // calculate a preferred interpolation direction
        scalar0 = cellScalars->GetComponent(vert[0],0);
        scalar1 = cellScalars->GetComponent(vert[1],0);
        deltaScalar = scalar1 - scalar0;

        if (deltaScalar > 0)
          {
          e1 = vert[0]; e2 = vert[1];
          e1Scalar = scalar0;
          }
        else
          {
          e1 = vert[1]; e2 = vert[0];
          e1Scalar = scalar1;
          deltaScalar = -deltaScalar;
          }

        // linear interpolation
        if (deltaScalar == 0.0)
          {
          t = 0.0;
          }
        else
          {
          t = (value - e1Scalar) / deltaScalar;
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
    // check for degenerate output
    if ( edge[0] == 3 ) //i.e., a triangle
      {
      if (pts[0] == pts[1] || pts[0] == pts[2] || pts[1] == pts[2] )
        {
        continue;
        }
      }
    else // a quad
      {
      if ((pts[0] == pts[3] && pts[1] == pts[2]) || 
          (pts[0] == pts[1] && pts[3] == pts[2]) )
        {
        continue;
        }
      }

    newCellId = polys->InsertNextCell(edge[0],pts);
    outCd->CopyData(inCd,cellId,newCellId);
    }
}
