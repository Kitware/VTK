/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTetra.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
#include "vtkTetra.hh"
#include "vtkMath.hh"
#include "vtkLine.hh"
#include "vtkTriangle.hh"
#include "vtkCellArray.hh"
#include "vtkPointLocator.hh"


// Description:
// Deep copy of cell.
vtkTetra::vtkTetra(const vtkTetra& t)
{
  this->Points = t.Points;
  this->PointIds = t.PointIds;
}

int vtkTetra::EvaluatePosition(float x[3], float closestPoint[3],
                              int& subId, float pcoords[3], 
                              float& minDist2, float *weights)
{
  float *pt1, *pt2, *pt3, *pt4;
  int i;
  float rhs[3], c1[3], c2[3], c3[3];
  float det, p4;

  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;

  pt1 = this->Points.GetPoint(1);
  pt2 = this->Points.GetPoint(2);
  pt3 = this->Points.GetPoint(3);
  pt4 = this->Points.GetPoint(0);

  for (i=0; i<3; i++)
    {  
    rhs[i] = x[i] - pt4[i];
    c1[i] = pt1[i] - pt4[i];
    c2[i] = pt2[i] - pt4[i];
    c3[i] = pt3[i] - pt4[i];
    }

  if ( (det = vtkMath::Determinant3x3(c1,c2,c3)) == 0.0 ) return -1;

  pcoords[0] = vtkMath::Determinant3x3 (rhs,c2,c3) / det;
  pcoords[1] = vtkMath::Determinant3x3 (c1,rhs,c3) / det;
  pcoords[2] = vtkMath::Determinant3x3 (c1,c2,rhs) / det;
  p4 = 1.0 - pcoords[0] - pcoords[1] - pcoords[2];

  weights[0] = p4;
  weights[1] = pcoords[0];
  weights[2] = pcoords[1];
  weights[3] = pcoords[2];

  if ( pcoords[0] >= 0.0 && pcoords[1] <= 1.0 &&
  pcoords[1] >= 0.0 && pcoords[1] <= 1.0 &&
  pcoords[2] >= 0.0 && pcoords[2] <= 1.0 && p4 >= 0.0 && p4 <= 1.0 )
    {
    closestPoint[0] = x[0]; 
    closestPoint[1] = x[1]; 
    closestPoint[2] = x[2];
    minDist2 = 0.0; //inside tetra
    return 1; 
    }
  else
    { //could easily be sped up using parametric localization - next release
    float dist2, w[3], closest[3], pc[3];
    int sub;
    vtkTriangle *triangle;

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
    return 0;
    }
}

void vtkTetra::EvaluateLocation(int& vtkNotUsed(subId), float pcoords[3], 
				float x[3], float *weights)
{
  float u4;
  float *pt1, *pt2, *pt3, *pt4;
  int i;

  pt1 = this->Points.GetPoint(1);
  pt2 = this->Points.GetPoint(2);
  pt3 = this->Points.GetPoint(3);
  pt4 = this->Points.GetPoint(0);

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

int vtkTetra::CellBoundary(int vtkNotUsed(subId), float pcoords[3], 
			   vtkIdList& pts)
{
  float t1 = pcoords[0] - pcoords[1];
  float t2 = pcoords[1] - pcoords[2];
  float t3 = pcoords[0] - pcoords[2];
  float t4 = pcoords[0] + pcoords[1] + 2.0*pcoords[2] - 1.3333333;
  float t5 = pcoords[0] + 2.0*pcoords[1] + pcoords[2] - 1.3333333;
  float t6 = 2.0*pcoords[0] + pcoords[1] + pcoords[2] - 1.3333333;

  pts.Reset();

  // compare against three lines in parametric space that divide element
  // into three pieces
  if ( t3 >= 0.0 && t2 >= 0.0 && t4 < 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(0));
    pts.SetId(1,this->PointIds.GetId(2));
    pts.SetId(2,this->PointIds.GetId(1));
    }

  else if ( t1 >= 0.0 && t2 < 0.0 && t5 < 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(0));
    pts.SetId(1,this->PointIds.GetId(1));
    pts.SetId(2,this->PointIds.GetId(3));
    }

  else if ( t4 >= 0.0 && t5 >= 0.0 && t6 >= 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(1));
    pts.SetId(1,this->PointIds.GetId(2));
    pts.SetId(2,this->PointIds.GetId(3));
    }

  else //if ( t1 < 0.0 && t3 < 0.0 && t6 < 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(0));
    pts.SetId(1,this->PointIds.GetId(2));
    pts.SetId(2,this->PointIds.GetId(3));
    }

  if ( pcoords[0] < 0.0 || pcoords[1] < 0.0 || pcoords[2] < 0.0 ||
  pcoords[0] > 1.0 || pcoords[1] > 1.0 || pcoords[2] > 1.0 ||
  (1.0 - pcoords[0] - pcoords[1] - pcoords[2]) < 0.0 )
    return 0;
  else
    return 1;
}

//
// Marching (convex) tetrahedron
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

void vtkTetra::Contour(float value, vtkFloatScalars *cellScalars, 
		       vtkPointLocator *locator,
		       vtkCellArray *vtkNotUsed(verts), 
		       vtkCellArray *vtkNotUsed(lines), 
		       vtkCellArray *polys, vtkFloatScalars *scalars)
{
  static int CASE_MASK[4] = {1,2,4,8};
  TRIANGLE_CASES *triCase;
  EDGE_LIST  *edge;
  int i, j, index, *vert;
  int pts[3];
  float t, *x1, *x2, x[3];

  // Build the case table
  for ( i=0, index = 0; i < 4; i++)
      if (cellScalars->GetScalar(i) >= value)
          index |= CASE_MASK[i];

  triCase = triCases + index;
  edge = triCase->edges;

  for ( ; edge[0] > -1; edge += 3 )
    {
    for (i=0; i<3; i++) // insert triangle
      {
      vert = edges[edge[i]];
      t = (value - cellScalars->GetScalar(vert[0])) /
          (cellScalars->GetScalar(vert[1]) - cellScalars->GetScalar(vert[0]));
      x1 = this->Points.GetPoint(vert[0]);
      x2 = this->Points.GetPoint(vert[1]);
      for (j=0; j<3; j++) x[j] = x1[j] + t * (x2[j] - x1[j]);
      if ( (pts[i] = locator->IsInsertedPoint(x)) < 0 )
        {
        pts[i] = locator->InsertNextPoint(x);
        scalars->InsertScalar(pts[i],value);
        }
      }
    polys->InsertNextCell(3,pts);
    }
}

vtkCell *vtkTetra::GetEdge(int edgeId)
{
  static vtkLine line;
  int *verts;

  verts = edges[edgeId];

  // load point id's
  line.PointIds.SetId(0,this->PointIds.GetId(verts[0]));
  line.PointIds.SetId(1,this->PointIds.GetId(verts[1]));

  // load coordinates
  line.Points.SetPoint(0,this->Points.GetPoint(verts[0]));
  line.Points.SetPoint(1,this->Points.GetPoint(verts[1]));

  return &line;
}

vtkCell *vtkTetra::GetFace(int faceId)
{
  int *verts;
  static vtkTriangle tri;

  verts = faces[faceId];

  // load point id's
  tri.PointIds.SetId(0,this->PointIds.GetId(verts[0]));
  tri.PointIds.SetId(1,this->PointIds.GetId(verts[1]));
  tri.PointIds.SetId(2,this->PointIds.GetId(verts[2]));

  // load coordinates
  tri.Points.SetPoint(0,this->Points.GetPoint(verts[0]));
  tri.Points.SetPoint(1,this->Points.GetPoint(verts[1]));
  tri.Points.SetPoint(2,this->Points.GetPoint(verts[2]));

  return &tri;
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
  static vtkTriangle tri;

  t = VTK_LARGE_FLOAT;
  for (faceNum=0; faceNum<4; faceNum++)
    {
    pt1 = this->Points.GetPoint(faces[faceNum][0]);
    pt2 = this->Points.GetPoint(faces[faceNum][1]);
    pt3 = this->Points.GetPoint(faces[faceNum][2]);

    tri.Points.SetPoint(0,pt1);
    tri.Points.SetPoint(1,pt2);
    tri.Points.SetPoint(2,pt3);

    if ( tri.IntersectWithLine(p1, p2, tol, tTemp, xTemp, pc, subId) )
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

int vtkTetra::Triangulate(int vtkNotUsed(index), vtkFloatPoints &pts)
{
  pts.Reset();
  pts.InsertPoint(0,this->Points.GetPoint(0));
  pts.InsertPoint(1,this->Points.GetPoint(1));
  pts.InsertPoint(2,this->Points.GetPoint(2));
  pts.InsertPoint(3,this->Points.GetPoint(3));

  return 1;
}

void vtkTetra::Derivatives(int vtkNotUsed(subId), float vtkNotUsed(pcoords)[3],
                           float *values, int dim, float *derivs)
{
  double *jI[3], j0[3], j1[3], j2[3];
  float functionDerivs[12], sum[3];
  int i, j, k;

  // compute inverse Jacobian and interpolation function derivatives
  jI[0] = j0; jI[1] = j1; jI[2] = j2;
  this->JacobianInverse(jI, functionDerivs);

  // now compute derivates of values provided
  for (k=0; k < dim; k++) //loop over values per vertex
    {
    for (j=0; j < 3; j++) //loop over derivative directions
      {
      sum[0] = sum[1] = sum[2] = 0.0;
      for ( i=0; i < 4; i++) //loop over interp. function derivatives
        {
        sum[0] += functionDerivs[i] * values[dim*i + k]; 
        sum[1] += functionDerivs[4 + i] * values[dim*i + k];
        sum[2] += functionDerivs[8 + i] * values[dim*i + k];
        }
      derivs[3*k + j] = sum[0]*jI[j][0] + sum[1]*jI[j][1] + sum[2]*jI[j][2];
      }
    }
}

// Description:
// Compute the center of the tetrahedron,
void vtkTetra::TetraCenter(float p1[3], float p2[3], float p3[3],
                           float p4[3], float center[3])
{
  center[0] = (p1[0]+p2[0]+p3[0]+p4[0]) / 4.0;
  center[1] = (p1[1]+p2[1]+p3[1]+p4[1]) / 4.0;
  center[2] = (p1[2]+p2[2]+p3[2]+p4[2]) / 4.0;
}

#define VTK_DOT(x,y) x[0]*y[0] + x[1]*y[1] + x[2]*y[2]

// Description:
// Compute the circumcenter (center[3]) and radius (method return value) of
// a tetrahedron defined by the four points x1, x2, x3, and x4.
float vtkTetra::Circumsphere(float  x1[3], float x2[3], float x3[3], 
                             float x4[3], float center[3])
{
  double n12[3], n13[3], n14[3], x12[3], x13[3], x14[3];
  double *A[3], rhs[3], sum, diff;
  int i;
//
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
//
//  Compute solutions to the intersection of two bisecting lines
//  (3-eqns. in 3-unknowns).
//
//  form system matrices
//
  A[0] = n12;
  A[1] = n13;
  A[2] = n14;

  rhs[0] = VTK_DOT(n12,x12); 
  rhs[1] = VTK_DOT(n13,x13);
  rhs[2] = VTK_DOT(n14,x14);
//
// Solve system of equations
//
  if ( vtkMath::SolveLinearSystem(A,rhs,3) == 0 )
    {
    center[0] = center[1] = center[2] = 0.0;
    return VTK_LARGE_FLOAT;
    }
  else
    {
    for (i=0; i<3; i++) center[i] = rhs[i];
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

  if ( (sum /= 4.0) > VTK_LARGE_FLOAT ) return VTK_LARGE_FLOAT;
  else return sum;
}
#undef VTK_DOT

// Description:
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
int vtkTetra::BarycentricCoords(float x[3], float  x1[3], float x2[3], 
                                float x3[3], float x4[3], float bcoords[4])
{
  double *A[4], p[4], a1[4], a2[4], a3[4], a4[4];
  int i;

  //
  // Homogenize the variables; load into arrays.
  //
  a1[0] = x1[0]; a1[1] = x2[0]; a1[2] = x3[0]; a1[3] = x4[0];
  a2[0] = x1[1]; a2[1] = x2[1]; a2[2] = x3[1]; a2[3] = x4[1];
  a3[0] = x1[2]; a3[1] = x2[2]; a3[2] = x3[2]; a3[3] = x4[2];
  a4[0] = 1.0;   a4[1] = 1.0;   a4[2] = 1.0;   a4[3] = 1.0;
  p[0] = x[0]; p[1] = x[1]; p[2] = x[2]; p[3] = 1.0;

  //
  //   Now solve system of equations for barycentric coordinates
  //
  A[0] = a1;
  A[1] = a2;
  A[2] = a3;
  A[3] = a4;

  if ( vtkMath::SolveLinearSystem(A,p,4) )
    {
    for (i=0; i<4; i++) bcoords[i] = (float) p[i];
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

// Description:
// Given parametric coordinates compute inverse Jacobian transformation
// matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
// function derivatives.
void vtkTetra::JacobianInverse(double **inverse, float derivs[12])
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
    x = this->Points.GetPoint(j);
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
    vtkErrorMacro(<<"Jacobian inverse not found");
    return;
    }
}
