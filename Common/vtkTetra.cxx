/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTetra.cxx
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
#include "vtkTetra.h"
#include "vtkMath.h"
#include "vtkLine.h"
#include "vtkTriangle.h"
#include "vtkCellArray.h"
#include "vtkPointLocator.h"
#include "vtkUnstructuredGrid.h"
#include "vtkObjectFactory.h"

//--------------------------------------------------------------------------
vtkTetra* vtkTetra::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTetra");
  if(ret)
    {
    return (vtkTetra*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkTetra;
}

// Construct the tetra with four points.
vtkTetra::vtkTetra()
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
  this->Triangle = vtkTriangle::New();
}

vtkTetra::~vtkTetra()
{
  this->Triangle->Delete();
  this->Line->Delete();
}

vtkCell *vtkTetra::MakeObject()
{
  vtkCell *cell = vtkTetra::New();
  cell->DeepCopy(this);
  return cell;
}

int vtkTetra::EvaluatePosition(float x[3], float* closestPoint,
                              int& subId, float pcoords[3], 
                              float& minDist2, float *weights)
{
  float *pt1, *pt2, *pt3, *pt4;
  int i;
  float rhs[3], c1[3], c2[3], c3[3];
  float det, p4;

  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;

  pt1 = this->Points->GetPoint(1);
  pt2 = this->Points->GetPoint(2);
  pt3 = this->Points->GetPoint(3);
  pt4 = this->Points->GetPoint(0);

  for (i=0; i<3; i++)
    {  
    rhs[i] = x[i] - pt4[i];
    c1[i] = pt1[i] - pt4[i];
    c2[i] = pt2[i] - pt4[i];
    c3[i] = pt3[i] - pt4[i];
    }

  if ( (det = vtkMath::Determinant3x3(c1,c2,c3)) == 0.0 )
    {
    return -1;
    }

  pcoords[0] = vtkMath::Determinant3x3 (rhs,c2,c3) / det;
  pcoords[1] = vtkMath::Determinant3x3 (c1,rhs,c3) / det;
  pcoords[2] = vtkMath::Determinant3x3 (c1,c2,rhs) / det;
  p4 = 1.0 - pcoords[0] - pcoords[1] - pcoords[2];

  weights[0] = p4;
  weights[1] = pcoords[0];
  weights[2] = pcoords[1];
  weights[3] = pcoords[2];

  if ( pcoords[0] >= -0.001 && pcoords[0] <= 1.001 &&
  pcoords[1] >= -0.001 && pcoords[1] <= 1.001 &&
  pcoords[2] >= -0.001 && pcoords[2] <= 1.001 && p4 >= -0.001 && p4 <= 1.001 )
    {
    if (closestPoint)
      {
      closestPoint[0] = x[0]; 
      closestPoint[1] = x[1]; 
      closestPoint[2] = x[2];
      minDist2 = 0.0; //inside tetra
      }
    return 1; 
    }
  else
    { //could easily be sped up using parametric localization - next release
    float dist2, w[3], closest[3], pc[3];
    int sub;
    vtkTriangle *triangle;

    if (closestPoint)
      {
      for (minDist2=VTK_LARGE_FLOAT,i=0; i<4; i++)
	{
	triangle = (vtkTriangle *) this->GetFace (i);
	triangle->EvaluatePosition(x,closest,sub,pc,dist2,(float *)w);
	
	if ( dist2 < minDist2 )
	  {
	  closestPoint[0] = closest[0]; 
	  closestPoint[1] = closest[1]; 
	  closestPoint[2] = closest[2];
	  minDist2 = dist2;
	  }
	}
      }
    return 0;
    }
}

void vtkTetra::EvaluateLocation(int& vtkNotUsed(subId), float pcoords[3], 
                                float x[3], float *weights)
{
  float u4;
  float *pt1, *pt2, *pt3, *pt4;
  int i;

  pt1 = this->Points->GetPoint(1);
  pt2 = this->Points->GetPoint(2);
  pt3 = this->Points->GetPoint(3);
  pt4 = this->Points->GetPoint(0);

  u4 = 1.0 - pcoords[0] - pcoords[1] - pcoords[2];

  for (i=0; i<3; i++)
    {
    x[i] = pt1[i]*pcoords[0] + pt2[i]*pcoords[1] + pt3[i]*pcoords[2] +
           pt4[i]*u4;
    }

  weights[0] = u4;
  weights[1] = pcoords[0];
  weights[2] = pcoords[1];
  weights[3] = pcoords[2];
}

// Returns the set of points that are on the boundary of the tetrahedron that
// are closest parametrically to the point specified. This may include faces,
// edges, or vertices.
int vtkTetra::CellBoundary(int vtkNotUsed(subId), float pcoords[3], 
                           vtkIdList *pts)
{
  float minPCoord = 1.0 - pcoords[0] - pcoords[1] - pcoords[2];
  int i, idx=3;
  
  for ( i=0; i < 3; i++ )
    {
    if ( pcoords[i] < minPCoord )
      {
      minPCoord = pcoords[i];
      idx = i;
      }
    }
  
  pts->SetNumberOfIds(3);
  switch (idx) //find the face closest to the point
    {
    case 0:
      pts->SetId(0,this->PointIds->GetId(0));
      pts->SetId(1,this->PointIds->GetId(2));
      pts->SetId(2,this->PointIds->GetId(3));
      break;
      
    case 1:
      pts->SetId(0,this->PointIds->GetId(0));
      pts->SetId(1,this->PointIds->GetId(1));
      pts->SetId(2,this->PointIds->GetId(3));
      break;
      
    case 2:
      pts->SetId(0,this->PointIds->GetId(0));
      pts->SetId(1,this->PointIds->GetId(1));
      pts->SetId(2,this->PointIds->GetId(2));
      break;
      
    case 3:
      pts->SetId(0,this->PointIds->GetId(1));
      pts->SetId(1,this->PointIds->GetId(2));
      pts->SetId(2,this->PointIds->GetId(3));
      break;
    }

  if ( pcoords[0] < 0.0 || pcoords[1] < 0.0 || pcoords[2] < 0.0 ||
       pcoords[0] > 1.0 || pcoords[1] > 1.0 || pcoords[2] > 1.0 ||
       (1.0 - pcoords[0] - pcoords[1] - pcoords[2]) < 0.0 )
    {
    return 0;
    }
  else
    {
    return 1;
    }
}

// Marching tetrahedron
//
static int edges[6][2] = { {0,1}, {1,2}, {2,0}, 
                           {0,3}, {1,3}, {2,3} };
static int faces[4][3] = { {0,1,3}, {1,2,3}, {2,0,3}, {0,2,1} };

typedef int EDGE_LIST;
typedef struct {
       EDGE_LIST edges[7];
} TRIANGLE_CASES;

static TRIANGLE_CASES triCases[] = { 
  {{-1, -1, -1, -1, -1, -1, -1}},
  {{ 0, 3, 2, -1, -1, -1, -1}},
  {{ 0, 1, 4, -1, -1, -1, -1}},
  {{ 3, 2, 4, 4, 2, 1, -1}},
  {{ 1, 2, 5, -1, -1, -1, -1}},
  {{ 3, 5, 1, 3, 1, 0, -1}},
  {{ 0, 2, 5, 0, 5, 4, -1}},
  {{ 3, 5, 4, -1, -1, -1, -1}},
  {{ 3, 4, 5, -1, -1, -1, -1}},
  {{ 0, 4, 5, 0, 5, 2, -1}},
  {{ 0, 5, 3, 0, 1, 5, -1}},
  {{ 5, 2, 1, -1, -1, -1, -1}},
  {{ 3, 4, 1, 3, 1, 2, -1}},
  {{ 0, 4, 1, -1, -1, -1, -1}},
  {{ 0, 2, 3, -1, -1, -1, -1}},
  {{-1, -1, -1, -1, -1, -1, -1}}
};

void vtkTetra::Contour(float value, vtkScalars *cellScalars, 
                       vtkPointLocator *locator,
                       vtkCellArray *vtkNotUsed(verts), 
                       vtkCellArray *vtkNotUsed(lines), 
                       vtkCellArray *polys,
                       vtkPointData *inPd, vtkPointData *outPd,
                       vtkCellData *inCd, int cellId, vtkCellData *outCd)
{
  static int CASE_MASK[4] = {1,2,4,8};
  TRIANGLE_CASES *triCase;
  EDGE_LIST  *edge;
  int i, j, index, *vert, newCellId;
  vtkIdType pts[3];
  float t, *x1, *x2, x[3];

  // Build the case table
  for ( i=0, index = 0; i < 4; i++)
    {
    if (cellScalars->GetScalar(i) >= value)
      {
      index |= CASE_MASK[i];
      }
    }

  triCase = triCases + index;
  edge = triCase->edges;

  for ( ; edge[0] > -1; edge += 3 )
    {
    for (i=0; i<3; i++) // insert triangle
      {
      vert = edges[edge[i]];
      t = (value - cellScalars->GetScalar(vert[0])) /
          (cellScalars->GetScalar(vert[1]) - cellScalars->GetScalar(vert[0]));
      x1 = this->Points->GetPoint(vert[0]);
      x2 = this->Points->GetPoint(vert[1]);
      for (j=0; j<3; j++)
	{
	x[j] = x1[j] + t * (x2[j] - x1[j]);
	}
      if ( locator->InsertUniquePoint(x, pts[i]) )
        {
        if ( outPd ) 
          {
          int p1 = this->PointIds->GetId(vert[0]);
          int p2 = this->PointIds->GetId(vert[1]);
          outPd->InterpolateEdge(inPd,pts[i],p1,p2,t);
          }
        }
      }
    // check for degenerate triangle
    if ( pts[0] != pts[1] &&
         pts[0] != pts[2] &&
         pts[1] != pts[2] )
      {
      newCellId = polys->InsertNextCell(3,pts);
      outCd->CopyData(inCd,cellId,newCellId);
      }
    }
}

int *vtkTetra::GetEdgeArray(int edgeId)
{
  return edges[edgeId];
}
vtkCell *vtkTetra::GetEdge(int edgeId)
{
  int *verts;

  verts = edges[edgeId];

  // load point id's
  this->Line->PointIds->SetId(0,this->PointIds->GetId(verts[0]));
  this->Line->PointIds->SetId(1,this->PointIds->GetId(verts[1]));

  // load coordinates
  this->Line->Points->SetPoint(0,this->Points->GetPoint(verts[0]));
  this->Line->Points->SetPoint(1,this->Points->GetPoint(verts[1]));

  return this->Line;
}

int *vtkTetra::GetFaceArray(int faceId)
{
  return faces[faceId];
}
vtkCell *vtkTetra::GetFace(int faceId)
{
  int *verts;

  verts = faces[faceId];

  // load point id's
  this->Triangle->PointIds->SetId(0,this->PointIds->GetId(verts[0]));
  this->Triangle->PointIds->SetId(1,this->PointIds->GetId(verts[1]));
  this->Triangle->PointIds->SetId(2,this->PointIds->GetId(verts[2]));

  // load coordinates
  this->Triangle->Points->SetPoint(0,this->Points->GetPoint(verts[0]));
  this->Triangle->Points->SetPoint(1,this->Points->GetPoint(verts[1]));
  this->Triangle->Points->SetPoint(2,this->Points->GetPoint(verts[2]));

  return this->Triangle;
}

// 
// Intersect triangle faces against line.
//
int vtkTetra::IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                               float x[3], float pcoords[3], int& subId)
{
  int intersection=0;
  float *pt1, *pt2, *pt3;
  float tTemp;
  float pc[3], xTemp[3];
  int faceNum;

  t = VTK_LARGE_FLOAT;
  for (faceNum=0; faceNum<4; faceNum++)
    {
    pt1 = this->Points->GetPoint(faces[faceNum][0]);
    pt2 = this->Points->GetPoint(faces[faceNum][1]);
    pt3 = this->Points->GetPoint(faces[faceNum][2]);

    this->Triangle->Points->SetPoint(0,pt1);
    this->Triangle->Points->SetPoint(1,pt2);
    this->Triangle->Points->SetPoint(2,pt3);

    if ( this->Triangle->IntersectWithLine(p1, p2, tol, tTemp, 
					   xTemp, pc, subId) )
      {
      intersection = 1;
      if ( tTemp < t )
        {
        t = tTemp;
        x[0] = xTemp[0]; x[1] = xTemp[1]; x[2] = xTemp[2]; 
        switch (faceNum)
          {
          case 0:
            pcoords[0] = pc[0]; pcoords[1] = pc[1]; pcoords[2] = 0.0;
            break;

          case 1:
            pcoords[0] = 0.0; pcoords[1] = pc[1]; pcoords[2] = 0.0;
            break;

          case 2:
            pcoords[0] = pc[0]; pcoords[1] = 0.0; pcoords[2] = 0.0;
            break;

          case 3:
            pcoords[0] = pc[0]; pcoords[1] = pc[1]; pcoords[2] = pc[2];
            break;
          }
        }
      }
    }
  return intersection;
}

int vtkTetra::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds, vtkPoints *pts)
{
  ptIds->Reset();
  pts->Reset();
    
  for ( int i=0; i < 4; i++ )
    {
    ptIds->InsertId(i,this->PointIds->GetId(i));
    pts->InsertPoint(i,this->Points->GetPoint(i));
    }

  return 1;
}


void vtkTetra::Derivatives(int vtkNotUsed(subId), float vtkNotUsed(pcoords)[3],
                           float *values, int dim, float *derivs)
{
  double *jI[3], j0[3], j1[3], j2[3];
  float functionDerivs[12], sum[3], value;
  int i, j, k;

  // compute inverse Jacobian and interpolation function derivatives
  jI[0] = j0; jI[1] = j1; jI[2] = j2;
  this->JacobianInverse(jI, functionDerivs);

  // now compute derivates of values provided
  for (k=0; k < dim; k++) //loop over values per vertex
    {
    sum[0] = sum[1] = sum[2] = 0.0;
    for ( i=0; i < 4; i++) //loop over interp. function derivatives
      {
      value = values[dim*i + k];
      sum[0] += functionDerivs[i] * value;
      sum[1] += functionDerivs[4 + i] * value;
      sum[2] += functionDerivs[8 + i] * value;
      }

    for (j=0; j < 3; j++) //loop over derivative directions
      {
      derivs[3*k + j] = sum[0]*jI[j][0] + sum[1]*jI[j][1] + sum[2]*jI[j][2];
      }
    }
}

// Compute the center of the tetrahedron,
void vtkTetra::TetraCenter(float p1[3], float p2[3], float p3[3],
                           float p4[3], float center[3])
{
  center[0] = (p1[0]+p2[0]+p3[0]+p4[0]) / 4.0;
  center[1] = (p1[1]+p2[1]+p3[1]+p4[1]) / 4.0;
  center[2] = (p1[2]+p2[2]+p3[2]+p4[2]) / 4.0;
}

double vtkTetra::ComputeVolume(double  p1[3], double p2[3], double p3[3], 
                               double p4[3])
{
  return (vtkMath::Determinant3x3(p2[0]-p1[0], p3[0]-p1[0], p4[0]-p1[0],
                                  p2[1]-p1[1], p3[1]-p1[1], p4[1]-p1[1],
                                  p2[2]-p1[2], p3[2]-p1[2], p4[2]-p1[2])/6.0);
}
                               
// Compute the circumcenter (center[3]) and radius (method return value) of
// a tetrahedron defined by the four points x1, x2, x3, and x4.
double vtkTetra::Circumsphere(double  x1[3], double x2[3], double x3[3], 
                             double x4[3], double center[3])
{
  double n12[3], n13[3], n14[3], x12[3], x13[3], x14[3];
  double *A[3], rhs[3], sum, diff;
  int i;

  //  calculate normals and intersection points of bisecting planes.  
  //
  for (i=0; i<3; i++) 
    {
    n12[i] = x2[i] - x1[i];
    n13[i] = x3[i] - x1[i];
    n14[i] = x4[i] - x1[i];
    x12[i] = (x2[i] + x1[i]) / 2.0;
    x13[i] = (x3[i] + x1[i]) / 2.0;
    x14[i] = (x4[i] + x1[i]) / 2.0;
    }

  //  Compute solutions to the intersection of two bisecting lines
  //  (3-eqns. in 3-unknowns).
  //
  //  form system matrices
  //
  A[0] = n12;
  A[1] = n13;
  A[2] = n14;

  rhs[0] = vtkMath::Dot(n12,x12); 
  rhs[1] = vtkMath::Dot(n13,x13);
  rhs[2] = vtkMath::Dot(n14,x14);

  // Solve system of equations
  //
  if ( vtkMath::SolveLinearSystem(A,rhs,3) == 0 )
    {
    center[0] = center[1] = center[2] = 0.0;
    return VTK_LARGE_FLOAT;
    }
  else
    {
    for (i=0; i<3; i++)
      {
      center[i] = rhs[i];
      }
    }

  //determine average value of radius squared
  for (sum=0, i=0; i<3; i++) 
    {
    diff = x1[i] - rhs[i];
    sum += diff*diff;
    diff = x2[i] - rhs[i];
    sum += diff*diff;
    diff = x3[i] - rhs[i];
    sum += diff*diff;
    diff = x4[i] - rhs[i];
    sum += diff*diff;
    }

  if ( (sum /= 4.0) > VTK_LARGE_FLOAT )
    {
    return VTK_LARGE_FLOAT;
    }
  else
    {
    return sum;
    }
}

// Given a 3D point x[3], determine the barycentric coordinates of the point.
// Barycentric coordinates are a natural coordinate system for simplices that
// express a position as a linear combination of the vertices. For a 
// tetrahedron, there are four barycentric coordinates (because there are
// four vertices), and the sum of the coordinates must equal 1. If a 
// point x is inside a simplex, then all four coordinates will be strictly 
// positive.  If three coordinates are zero (so the fourth =1), then the 
// point x is on a vertex. If two coordinates are zero, the point x is on an 
// edge (and so on). In this method, you must specify the vertex coordinates
// x1->x4. Returns 0 if tetrahedron is degenerate.
int vtkTetra::BarycentricCoords(double x[3], double  x1[3], double x2[3], 
                                double x3[3], double x4[3], double bcoords[4])
{
  double *A[4], p[4], a1[4], a2[4], a3[4], a4[4];
  int i;

  // Homogenize the variables; load into arrays.
  //
  a1[0] = x1[0]; a1[1] = x2[0]; a1[2] = x3[0]; a1[3] = x4[0];
  a2[0] = x1[1]; a2[1] = x2[1]; a2[2] = x3[1]; a2[3] = x4[1];
  a3[0] = x1[2]; a3[1] = x2[2]; a3[2] = x3[2]; a3[3] = x4[2];
  a4[0] = 1.0;   a4[1] = 1.0;   a4[2] = 1.0;   a4[3] = 1.0;
  p[0] = x[0]; p[1] = x[1]; p[2] = x[2]; p[3] = 1.0;

  //   Now solve system of equations for barycentric coordinates
  //
  A[0] = a1;
  A[1] = a2;
  A[2] = a3;
  A[3] = a4;

  if ( vtkMath::SolveLinearSystem(A,p,4) )
    {
    for (i=0; i<4; i++)
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

//
// Compute iso-parametrix interpolation functions
//
void vtkTetra::InterpolationFunctions(float pcoords[3], float sf[4])
{
  sf[0] = 1.0 - pcoords[0] - pcoords[1] - pcoords[2];
  sf[1] = pcoords[0];
  sf[2] = pcoords[1];
  sf[3] = pcoords[2];
}

void vtkTetra::InterpolationDerivs(float derivs[12])
{
  // r-derivatives
  derivs[0] = -1.0;
  derivs[1] = 1.0;
  derivs[2] = 0.0;
  derivs[3] = 0.0;

  // s-derivatives
  derivs[4] = -1.0;
  derivs[5] = 0.0;
  derivs[6] = 1.0;
  derivs[7] = 0.0;

  // t-derivatives
  derivs[8] = -1.0;
  derivs[9] = 0.0;
  derivs[10] = 0.0;
  derivs[11] = 1.0;
}

// Given parametric coordinates compute inverse Jacobian transformation
// matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
// function derivatives. Returns 0 if no inverse exists.
int vtkTetra::JacobianInverse(double **inverse, float derivs[12])
{
  int i, j;
  double *m[3], m0[3], m1[3], m2[3];
  float *x;

  // compute interpolation function derivatives
  this->InterpolationDerivs(derivs);

  // create Jacobian matrix
  m[0] = m0; m[1] = m1; m[2] = m2;
  for (i=0; i < 3; i++) //initialize matrix
    {
    m0[i] = m1[i] = m2[i] = 0.0;
    }

  for ( j=0; j < 4; j++ )
    {
    x = this->Points->GetPoint(j);
    for ( i=0; i < 3; i++ )
      {
      m0[i] += x[i] * derivs[j];
      m1[i] += x[i] * derivs[4 + j];
      m2[i] += x[i] * derivs[8 + j];
      }
    }

  // now find the inverse
  if ( vtkMath::InvertMatrix(m,inverse,3) == 0 )
    {
#define VTK_MAX_WARNS 3    
    static int numWarns=0;
    if ( numWarns++ < VTK_MAX_WARNS )
      {
      vtkErrorMacro(<<"Jacobian inverse not found");
      vtkErrorMacro(<<"Matrix:" << m[0][0] << " " << m[0][1] << " " << m[0][2]
      << m[1][0] << " " << m[1][1] << " " << m[1][2] 
      << m[2][0] << " " << m[2][1] << " " << m[2][2] );
      return 0;
      }
    }

  return 1;
}

void vtkTetra::GetEdgePoints(int edgeId, int* &pts)
{
  pts = this->GetEdgeArray(edgeId);
}

void vtkTetra::GetFacePoints(int faceId, int* &pts)
{
  pts = this->GetFaceArray(faceId);
}

// Overload vtkCell3D::Clip() because there are cases when we want to
// insert just ourselves into the output (i.e., a case that we want to
// template).
void vtkTetra::Clip(float value, vtkScalars *cellScalars, 
                    vtkPointLocator *locator, vtkCellArray *tets,
                    vtkPointData *inPD, vtkPointData *outPD,
                    vtkCellData *inCD, int cellId, vtkCellData *outCD,
                    int insideOut)
{
  int inject;

  if ( insideOut )
    {    
    for (inject=0; inject<4; inject++)
      {
      if ( cellScalars->GetScalar(inject) > value )
        {
        break;
        }
      }
    }
  else
    {
    for (inject=0; inject<4; inject++)
      {
      if ( cellScalars->GetScalar(inject) <= value )
        {
        break;
        }
      }
    }
  
  if ( inject >= 4 ) //all points are above/below
    {
    vtkIdType pts[4];
    float x[3];
    for (inject=0; inject < 4; inject++)
      {
      this->Points->GetPoint(inject, x);
      if ( locator->InsertUniquePoint(x, pts[inject]) )
        {
        outPD->CopyData(inPD,this->PointIds->GetId(inject),pts[inject]);
        }
      }
    int newCellId = tets->InsertNextCell(4,pts);
    outCD->CopyData(inCD,cellId,newCellId);
    }
  else //defer to superclass
    {
    vtkCell3D::Clip(value, cellScalars, locator, tets, inPD, outPD,
                    inCD, cellId, outCD, insideOut);
    }
}
