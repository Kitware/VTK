/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHexahedron.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkHexahedron.hh"
#include "vtkMath.hh"
#include "vtkLine.hh"
#include "vtkQuad.hh"
#include "vtkCellArray.hh"

// Description:
// Deep copy of cell.
vtkHexahedron::vtkHexahedron(const vtkHexahedron& h)
{
  this->Points = h.Points;
  this->PointIds = h.PointIds;
}

//
//  Method to calculate parametric coordinates in an eight noded
//  linear hexahedron element from global coordinates.
//
#define MAX_ITERATION 10
#define CONVERGED 1.e-03

int vtkHexahedron::EvaluatePosition(float x[3], float closestPoint[3],
                                   int& subId, float pcoords[3], 
                                   float& dist2, float *weights)
{
  int iteration, converged;
  float  params[3];
  float  fcol[3], rcol[3], scol[3], tcol[3];
  int i, j;
  float  d, *pt;
  float derivs[24];
  static vtkMath math;
//
//  set initial position for Newton's method
//
  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = params[0] = params[1] = params[2] = 0.5;
//
//  enter iteration loop
///
  for (iteration=converged=0; !converged && (iteration < MAX_ITERATION);
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
    for (i=0; i<3; i++) 
      {
      fcol[i] = rcol[i] = scol[i] = tcol[i] = 0.0;
      }
    for (i=0; i<8; i++)
      {
      pt = this->Points.GetPoint(i);
      for (j=0; j<3; j++)
        {
        fcol[j] += pt[j] * weights[i];
        rcol[j] += pt[j] * derivs[i];
        scol[j] += pt[j] * derivs[i+8];
        tcol[j] += pt[j] * derivs[i+16];
        }
      }

    for (i=0; i<3; i++) fcol[i] -= x[i];
//
//  compute determinants and generate improvements
//
    if ( (d=math.Determinant3x3(rcol,scol,tcol)) == 0.0 ) return -1;

    pcoords[0] = params[0] - math.Determinant3x3 (fcol,scol,tcol) / d;
    pcoords[1] = params[1] - math.Determinant3x3 (rcol,fcol,tcol) / d;
    pcoords[2] = params[2] - math.Determinant3x3 (rcol,scol,fcol) / d;
//
//  check for convergence
//
    if ( ((fabs(pcoords[0]-params[0])) < CONVERGED) &&
    ((fabs(pcoords[1]-params[1])) < CONVERGED) &&
    ((fabs(pcoords[2]-params[2])) < CONVERGED) )
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
      params[2] = pcoords[2];
      }
    }
//
//  if not converged, set the parametric coordinates to arbitrary values
//  outside of element
//
  if ( !converged ) return -1;

  this->InterpolationFunctions(pcoords, weights);

  if ( pcoords[0] >= -0.001 && pcoords[0] <= 1.001 &&
  pcoords[1] >= -0.001 && pcoords[1] <= 1.001 &&
  pcoords[2] >= -0.001 && pcoords[2] <= 1.001 )
    {
    closestPoint[0] = x[0]; closestPoint[1] = x[1]; closestPoint[2] = x[2];
    dist2 = 0.0; //inside hexahedron
    return 1;
    }
  else
    {
    float pc[3], w[8];
    for (i=0; i<3; i++) //only approximate, not really true for warped hexa
      {
      if (pcoords[i] < 0.0) pc[i] = 0.0;
      else if (pcoords[i] > 1.0) pc[i] = 1.0;
      else pc[i] = pcoords[i];
      }
    this->EvaluateLocation(subId, pc, closestPoint, (float *)w);
    dist2 = math.Distance2BetweenPoints(closestPoint,x);
    return 0;
    }
}
//
// Compute iso-parametrix interpolation functions
//
void vtkHexahedron::InterpolationFunctions(float pcoords[3], float sf[8])
{
  double rm, sm, tm;

  rm = 1. - pcoords[0];
  sm = 1. - pcoords[1];
  tm = 1. - pcoords[2];

  sf[0] = rm*sm*tm;
  sf[1] = pcoords[0]*sm*tm;
  sf[2] = pcoords[0]*pcoords[1]*tm;
  sf[3] = rm*pcoords[1]*tm;
  sf[4] = rm*sm*pcoords[2];
  sf[5] = pcoords[0]*sm*pcoords[2];
  sf[6] = pcoords[0]*pcoords[1]*pcoords[2];
  sf[7] = rm*pcoords[1]*pcoords[2];
}

void vtkHexahedron::InterpolationDerivs(float pcoords[3], float derivs[24])
{
  double rm, sm, tm;

  rm = 1. - pcoords[0];
  sm = 1. - pcoords[1];
  tm = 1. - pcoords[2];

  // r-derivatives
  derivs[0] = -sm*tm;
  derivs[1] = sm*tm;
  derivs[2] = pcoords[1]*tm;
  derivs[3] = -pcoords[1]*tm;
  derivs[4] = -sm*pcoords[2];
  derivs[5] = sm*pcoords[2];
  derivs[6] = pcoords[1]*pcoords[2];
  derivs[7] = -pcoords[1]*pcoords[2];

  // s-derivatives
  derivs[8] = -rm*tm;
  derivs[9] = -pcoords[0]*tm;
  derivs[10] = pcoords[0]*tm;
  derivs[11] = rm*tm;
  derivs[12] = -rm*pcoords[2];
  derivs[13] = -pcoords[0]*pcoords[2];
  derivs[14] = pcoords[0]*pcoords[2];
  derivs[15] = rm*pcoords[2];

  // t-derivatives
  derivs[16] = -rm*sm;
  derivs[17] = -pcoords[0]*sm;
  derivs[18] = -pcoords[0]*pcoords[1];
  derivs[19] = -rm*pcoords[1];
  derivs[20] = rm*sm;
  derivs[21] = pcoords[0]*sm;
  derivs[22] = pcoords[0]*pcoords[1];
  derivs[23] = rm*pcoords[1];
}

void vtkHexahedron::EvaluateLocation(int& vtkNotUsed(subId), float pcoords[3],
				     float x[3], float *weights)
{
  int i, j;
  float *pt;

  this->InterpolationFunctions(pcoords, weights);

  x[0] = x[1] = x[2] = 0.0;
  for (i=0; i<8; i++)
    {
    pt = this->Points.GetPoint(i);
    for (j=0; j<3; j++)
      {
      x[j] += pt[j] * weights[i];
      }
    }
}

int vtkHexahedron::CellBoundary(int vtkNotUsed(subId), float pcoords[3], 
				vtkIdList& pts)
{
  float t1=pcoords[0]-pcoords[1];
  float t2=1.0-pcoords[0]-pcoords[1];
  float t3=pcoords[1]-pcoords[2];
  float t4=1.0-pcoords[1]-pcoords[2];
  float t5=pcoords[2]-pcoords[0];
  float t6=1.0-pcoords[2]-pcoords[0];

  pts.Reset();

  // compare against six planes in parametric space that divide element
  // into six pieces.
  if ( t3 >= 0.0 && t4 >= 0.0 && t5 < 0.0 && t6 >= 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(0));
    pts.SetId(1,this->PointIds.GetId(1));
    pts.SetId(2,this->PointIds.GetId(2));
    pts.SetId(3,this->PointIds.GetId(3));
    }

  else if ( t1 >= 0.0 && t2 < 0.0 && t5 < 0.0 && t6 < 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(1));
    pts.SetId(1,this->PointIds.GetId(2));
    pts.SetId(2,this->PointIds.GetId(6));
    pts.SetId(3,this->PointIds.GetId(5));
    }

  else if ( t1 >= 0.0 && t2 >= 0.0 && t3 < 0.0 && t4 >= 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(0));
    pts.SetId(1,this->PointIds.GetId(1));
    pts.SetId(2,this->PointIds.GetId(5));
    pts.SetId(3,this->PointIds.GetId(4));
    }

  else if ( t3 < 0.0 && t4 < 0.0 && t5 >= 0.0 && t6 < 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(4));
    pts.SetId(1,this->PointIds.GetId(5));
    pts.SetId(2,this->PointIds.GetId(6));
    pts.SetId(3,this->PointIds.GetId(7));
    }

  else if ( t1 < 0.0 && t2 >= 0.0 && t5 >= 0.0 && t6 >= 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(0));
    pts.SetId(1,this->PointIds.GetId(4));
    pts.SetId(2,this->PointIds.GetId(7));
    pts.SetId(3,this->PointIds.GetId(3));
    }

  else // if ( t1 < 0.0 && t2 < 0.0 && t3 >= 0.0 && t6 < 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(2));
    pts.SetId(1,this->PointIds.GetId(3));
    pts.SetId(2,this->PointIds.GetId(7));
    pts.SetId(3,this->PointIds.GetId(6));
    }


  if ( pcoords[0] < 0.0 || pcoords[0] > 1.0 ||
       pcoords[1] < 0.0 || pcoords[1] > 1.0 || 
       pcoords[2] < 0.0 || pcoords[2] > 1.0 )
    return 0;
  else
    return 1;
}

static int edges[12][2] = { {0,1}, {1,2}, {3,2}, {0,3},
                            {4,5}, {5,6}, {7,6}, {4,7},
                            {0,4}, {1,5}, {3,7}, {2,6}};
static int faces[6][4] = { {0,4,7,3}, {1,2,6,5},
                           {0,1,5,4}, {3,7,6,2},
                           {0,3,2,1}, {4,5,6,7} };
//
// Marching cubes case table
//
#include "vtkMarchingCubesCases.hh"

void vtkHexahedron::Contour(float value, vtkFloatScalars *cellScalars, 
			    vtkFloatPoints *points,
			    vtkCellArray *vtkNotUsed(verts), 
			    vtkCellArray *vtkNotUsed(lines), 
			    vtkCellArray *polys, vtkFloatScalars *scalars)
{
  static int CASE_MASK[8] = {1,2,4,8,16,32,64,128};
  TRIANGLE_CASES *triCase;
  EDGE_LIST  *edge;
  int i, j, index, *vert;
  int pts[3];
  float t, *x1, *x2, x[3];

  // Build the case table
  for ( i=0, index = 0; i < 8; i++)
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
      pts[i] = points->InsertNextPoint(x);
      scalars->InsertNextScalar(value);
      }
    polys->InsertNextCell(3,pts);
    }
}

vtkCell *vtkHexahedron::GetEdge(int edgeId)
{
  int *verts;
  static vtkLine line;

  verts = edges[edgeId];

  // load point id's
  line.PointIds.SetId(0,this->PointIds.GetId(verts[0]));
  line.PointIds.SetId(1,this->PointIds.GetId(verts[1]));

  // load coordinates
  line.Points.SetPoint(0,this->Points.GetPoint(verts[0]));
  line.Points.SetPoint(1,this->Points.GetPoint(verts[1]));

  return &line;
}

vtkCell *vtkHexahedron::GetFace(int faceId)
{
  int *verts, i;
  static vtkQuad theQuad; // using "quad" bothers IBM xlc compiler!

  verts = faces[faceId];

  for (i=0; i<4; i++)
    {
    theQuad.PointIds.SetId(i,this->PointIds.GetId(verts[i]));
    theQuad.Points.SetPoint(i,this->Points.GetPoint(verts[i]));
    }

  return &theQuad;
}
// 
// Intersect hexa faces against line. Each hexa face is a quadrilateral.
//
int vtkHexahedron::IntersectWithLine(float p1[3], float p2[3], float tol,
                                    float &t, float x[3], float pcoords[3],
                                    int& subId)
{
  int intersection=0;
  float *pt1, *pt2, *pt3, *pt4;
  float tTemp;
  float pc[3], xTemp[3];
  int faceNum;
  static vtkQuad theQuad; // using "quad" bothers IBM xlc compiler!

  t = VTK_LARGE_FLOAT;
  for (faceNum=0; faceNum<6; faceNum++)
    {
    pt1 = this->Points.GetPoint(faces[faceNum][0]);
    pt2 = this->Points.GetPoint(faces[faceNum][1]);
    pt3 = this->Points.GetPoint(faces[faceNum][2]);
    pt4 = this->Points.GetPoint(faces[faceNum][3]);

    theQuad.Points.SetPoint(0,pt1);
    theQuad.Points.SetPoint(1,pt2);
    theQuad.Points.SetPoint(2,pt3);
    theQuad.Points.SetPoint(3,pt4);

    if ( theQuad.IntersectWithLine(p1, p2, tol, tTemp, xTemp, pc, subId) )
      {
      intersection = 1;
      if ( tTemp < t )
        {
        t = tTemp;
        x[0] = xTemp[0]; x[1] = xTemp[1]; x[2] = xTemp[2]; 
        switch (faceNum)
          {
          case 0:
            pcoords[0] = 0.0; pcoords[0] = pc[0]; pcoords[1] = 0.0;
            break;

          case 1:
            pcoords[0] = 1.0; pcoords[0] = pc[0]; pcoords[1] = 0.0;
            break;

          case 2:
            pcoords[0] = pc[0]; pcoords[1] = 0.0; pcoords[2] = pc[1];
            break;

          case 3:
            pcoords[0] = pc[0]; pcoords[1] = 1.0; pcoords[2] = pc[1];
            break;

          case 4:
            pcoords[0] = pc[0]; pcoords[1] = pc[1]; pcoords[2] = 0.0;
            break;

          case 5:
            pcoords[0] = pc[0]; pcoords[1] = pc[1]; pcoords[2] = 1.0;
            break;
          }
        }
      }
    }
  return intersection;
}

int vtkHexahedron::Triangulate(int index, vtkFloatPoints &pts)
{
  pts.Reset();
//
// Create five tetrahedron. Triangulation varies depending upon index. This
// is necessary to insure compatible voxel triangulations.
//
  if ( index % 2 )
    {
    pts.InsertNextPoint(this->Points.GetPoint(0));
    pts.InsertNextPoint(this->Points.GetPoint(1));
    pts.InsertNextPoint(this->Points.GetPoint(4));
    pts.InsertNextPoint(this->Points.GetPoint(3));

    pts.InsertNextPoint(this->Points.GetPoint(1));
    pts.InsertNextPoint(this->Points.GetPoint(4));
    pts.InsertNextPoint(this->Points.GetPoint(7));
    pts.InsertNextPoint(this->Points.GetPoint(5));

    pts.InsertNextPoint(this->Points.GetPoint(1));
    pts.InsertNextPoint(this->Points.GetPoint(4));
    pts.InsertNextPoint(this->Points.GetPoint(3));
    pts.InsertNextPoint(this->Points.GetPoint(6));

    pts.InsertNextPoint(this->Points.GetPoint(1));
    pts.InsertNextPoint(this->Points.GetPoint(3));
    pts.InsertNextPoint(this->Points.GetPoint(2));
    pts.InsertNextPoint(this->Points.GetPoint(6));

    pts.InsertNextPoint(this->Points.GetPoint(3));
    pts.InsertNextPoint(this->Points.GetPoint(6));
    pts.InsertNextPoint(this->Points.GetPoint(4));
    pts.InsertNextPoint(this->Points.GetPoint(7));
    }
  else
    {
    pts.InsertNextPoint(this->Points.GetPoint(2));
    pts.InsertNextPoint(this->Points.GetPoint(1));
    pts.InsertNextPoint(this->Points.GetPoint(0));
    pts.InsertNextPoint(this->Points.GetPoint(5));

    pts.InsertNextPoint(this->Points.GetPoint(0));
    pts.InsertNextPoint(this->Points.GetPoint(2));
    pts.InsertNextPoint(this->Points.GetPoint(7));
    pts.InsertNextPoint(this->Points.GetPoint(3));

    pts.InsertNextPoint(this->Points.GetPoint(2));
    pts.InsertNextPoint(this->Points.GetPoint(5));
    pts.InsertNextPoint(this->Points.GetPoint(7));
    pts.InsertNextPoint(this->Points.GetPoint(6));

    pts.InsertNextPoint(this->Points.GetPoint(0));
    pts.InsertNextPoint(this->Points.GetPoint(7));
    pts.InsertNextPoint(this->Points.GetPoint(5));
    pts.InsertNextPoint(this->Points.GetPoint(4));

    pts.InsertNextPoint(this->Points.GetPoint(1));
    pts.InsertNextPoint(this->Points.GetPoint(2));
    pts.InsertNextPoint(this->Points.GetPoint(5));
    pts.InsertNextPoint(this->Points.GetPoint(7));
    }

  return 1;
}

//
// Compute derivatives in x-y-z directions. Use chain rule in combination
// with interpolation function derivatives.
//
void vtkHexahedron::Derivatives(int vtkNotUsed(subId), float pcoords[3], 
                                float *values, int dim, float *derivs)
{
  double *jI[3], j0[3], j1[3], j2[3];
  float functionDerivs[24], sum[3];
  int i, j, k;

  // compute inverse Jacobian and interpolation function derivatives
  jI[0] = j0; jI[1] = j1; jI[2] = j2;
  this->JacobianInverse(pcoords, jI, functionDerivs);

  // now compute derivates of values provided
  for (k=0; k < dim; k++) //loop over values per vertex
    {
    for (j=0; j < 3; j++) //loop over derivative directions
      {
      sum[0] = sum[1] = sum[2] = 0.0;
      for ( i=0; i < 8; i++) //loop over interp. function derivatives
        {
        sum[0] += functionDerivs[i] * values[dim*i + k]; 
        sum[1] += functionDerivs[8 + i] * values[dim*i + k];
        sum[2] += functionDerivs[16 + i] * values[dim*i + k];
        }
      derivs[3*k + j] = sum[0]*jI[j][0] + sum[1]*jI[j][1] + sum[2]*jI[j][2];
      }
    }
}

// Description:
// Given parametric coordinates compute inverse Jacobian transformation
// matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
// function derivatives.
void vtkHexahedron::JacobianInverse(float pcoords[3], double **inverse,
                                    float derivs[24])
{
  static vtkMath math;
  int i, j;
  double *m[3], m0[3], m1[3], m2[3];
  float *x;

  // compute interpolation function derivatives
  this->InterpolationDerivs(pcoords, derivs);

  // create Jacobian matrix
  m[0] = m0; m[1] = m1; m[2] = m2;
  for (i=0; i < 3; i++) //initialize matrix
    {
    m0[i] = m1[i] = m2[i] = 0.0;
    }

  for ( j=0; j < 8; j++ )
    {
    x = this->Points.GetPoint(j);
    for ( i=0; i < 3; i++ )
      {
      m0[i] += x[i] * derivs[j];
      m1[i] += x[i] * derivs[8 + j];
      m2[i] += x[i] * derivs[16 + j];
      }
    }

  // now find the inverse
  if ( math.InvertMatrix(m,inverse,3) == 0 )
    {
    vtkErrorMacro(<<"Jacobian inverse not found");
    return;
    }
}
