/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraticTetra.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkQuadraticTetra.h"
#include "vtkPolyData.h"
#include "vtkPointLocator.h"
#include "vtkMath.h"
#include "vtkQuadraticEdge.h"
#include "vtkQuadraticTriangle.h"
#include "vtkTetra.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkQuadraticTetra, "1.9");
vtkStandardNewMacro(vtkQuadraticTetra);

// Construct the line with two points.
vtkQuadraticTetra::vtkQuadraticTetra()
{
  this->Edge = vtkQuadraticEdge::New();
  this->Face = vtkQuadraticTriangle::New();
  this->Tetra = vtkTetra::New();
  this->Scalars = vtkFloatArray::New();
  this->Scalars->SetNumberOfTuples(4);

  int i;
  this->Points->SetNumberOfPoints(10);
  this->PointIds->SetNumberOfIds(10);
  for (i = 0; i < 10; i++)
    {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i,0);
    }
}

vtkQuadraticTetra::~vtkQuadraticTetra()
{
  this->Edge->Delete();
  this->Face->Delete();
  this->Tetra->Delete();
  this->Scalars->Delete();
}


vtkCell *vtkQuadraticTetra::MakeObject()
{
  vtkQuadraticTetra *cell = vtkQuadraticTetra::New();
  cell->DeepCopy(this);
  return (vtkCell *)cell;
}

static int TetraEdge[6][3] = { {0,1,4}, {1,2,5}, {2,0,6}, 
                               {0,3,7}, {1,3,8}, {2,3,9} };

static int TetraFace[4][6] = { {0,1,3,4,8,7}, {1,2,3,5,9,8}, 
                               {2,0,3,6,7,9}, {0,2,1,6,5,4} };

//clip each of the four vertices; the remaining octahedron is
//divided into two tetrahedron.
static int Tetras[6][4] = { {0,4,6,7}, {4,1,5,8}, {6,5,2,9}, 
                            {7,8,9,3}, {6,4,5,8}, {6,9,7,8} };

vtkCell *vtkQuadraticTetra::GetEdge(int edgeId)
{
  edgeId = (edgeId < 0 ? 0 : (edgeId > 5 ? 5 : edgeId ));

  // load point id's
  this->Edge->PointIds->SetId(0,this->PointIds->GetId(TetraEdge[edgeId][0]));
  this->Edge->PointIds->SetId(1,this->PointIds->GetId(TetraEdge[edgeId][1]));
  this->Edge->PointIds->SetId(2,this->PointIds->GetId(TetraEdge[edgeId][2]));

  // load coordinates
  this->Edge->Points->SetPoint(0,this->Points->GetPoint(TetraEdge[edgeId][0]));
  this->Edge->Points->SetPoint(1,this->Points->GetPoint(TetraEdge[edgeId][1]));
  this->Edge->Points->SetPoint(2,this->Points->GetPoint(TetraEdge[edgeId][2]));

  return this->Edge;
}

vtkCell *vtkQuadraticTetra::GetFace(int faceId)
{
  faceId = (faceId < 0 ? 0 : (faceId > 3 ? 3 : faceId ));

  // load point id's and coordinates
  for (int i=0; i< 6; i++)
    {
    this->Face->PointIds->SetId(
      i,this->PointIds->GetId(TetraFace[faceId][i]));
    this->Face->Points->SetPoint(
      i,this->Points->GetPoint(TetraFace[faceId][i]));
    }

  return this->Face;
}

static const float VTK_DIVERGED = 1.e6;
static const int VTK_TETRA_MAX_ITERATION=10;
static const float VTK_TETRA_CONVERGED=1.e-03;

int vtkQuadraticTetra::EvaluatePosition(float* x, 
                                        float* closestPoint, 
                                        int& subId, float pcoords[3],
                                        float& dist2, float *weights)
{
  int iteration, converged;
  float  params[3];
  float  fcol[3], rcol[3], scol[3], tcol[3];
  int i, j;
  float  d, *pt;
  float derivs[30];

  //  set initial position for Newton's method
  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = params[0] = params[1] = params[2]=0.333;

  //  enter iteration loop
  for (iteration=converged=0;
       !converged && (iteration < VTK_TETRA_MAX_ITERATION);  iteration++) 
    {
    //  calculate element interpolation functions and derivatives
    this->InterpolationFunctions(pcoords, weights);
    this->InterpolationDerivs(pcoords, derivs);

    //  calculate newton functions
    for (i=0; i<3; i++) 
      {
      fcol[i] = rcol[i] = scol[i] = tcol[i] = 0.0;
      }
    for (i=0; i<10; i++)
      {
      pt = this->Points->GetPoint(i);
      for (j=0; j<3; j++)
        {
        fcol[j] += pt[j] * weights[i];
        rcol[j] += pt[j] * derivs[i];
        scol[j] += pt[j] * derivs[i+10];
        tcol[j] += pt[j] * derivs[i+20];
        }
      }

    for (i=0; i<3; i++)
      {
      fcol[i] -= x[i];
      }

    //  compute determinants and generate improvements
    d=vtkMath::Determinant3x3(rcol,scol,tcol);
    if ( fabs(d) < 1.e-20) 
      {
      return -1;
      }

    pcoords[0] = params[0] - vtkMath::Determinant3x3 (fcol,scol,tcol) / d;
    pcoords[1] = params[1] - vtkMath::Determinant3x3 (rcol,fcol,tcol) / d;
    pcoords[2] = params[2] - vtkMath::Determinant3x3 (rcol,scol,fcol) / d;

    //  check for convergence
    if ( ((fabs(pcoords[0]-params[0])) < VTK_TETRA_CONVERGED) &&
         ((fabs(pcoords[1]-params[1])) < VTK_TETRA_CONVERGED) &&
         ((fabs(pcoords[2]-params[2])) < VTK_TETRA_CONVERGED) )
      {
      converged = 1;
      }

    // Test for bad divergence (S.Hirschberg 11.12.2001)
    else if ((fabs(pcoords[0]) > VTK_DIVERGED) || 
             (fabs(pcoords[1]) > VTK_DIVERGED) || 
             (fabs(pcoords[2]) > VTK_DIVERGED))
      {
      return -1;
      }

    //  if not converged, repeat
    else 
      {
      params[0] = pcoords[0];
      params[1] = pcoords[1];
      params[2] = pcoords[2];
      }
    }

  //  if not converged, set the parametric coordinates to arbitrary values
  //  outside of element
  if ( !converged )
    {
    return -1;
    }

  this->InterpolationFunctions(pcoords, weights);

  if ( pcoords[0] >= -0.001 && pcoords[0] <= 1.001 &&
  pcoords[1] >= -0.001 && pcoords[1] <= 1.001 &&
  pcoords[2] >= -0.001 && pcoords[2] <= 1.001 )
    {
    if (closestPoint)
      {
      closestPoint[0] = x[0]; closestPoint[1] = x[1]; closestPoint[2] = x[2];
      dist2 = 0.0; //inside hexahedron
      }
    return 1;
    }
  else
    {
    float pc[3], w[10];
    if (closestPoint)
      {
      for (i=0; i<3; i++) //only approximate, not really true for warped hexa
        {
        if (pcoords[i] < 0.0)
          {
          pc[i] = 0.0;
          }
        else if (pcoords[i] > 1.0)
          {
          pc[i] = 1.0;
          }
        else
          {
          pc[i] = pcoords[i];
          }
        }
      this->EvaluateLocation(subId, pc, closestPoint, (float *)w);
      dist2 = vtkMath::Distance2BetweenPoints(closestPoint,x);
      }
    return 0;
    }
}

void vtkQuadraticTetra::EvaluateLocation(int& vtkNotUsed(subId), 
                                        float pcoords[3], 
                                        float x[3], float *weights)
{
  int i, j;
  float *pt;

  this->InterpolationFunctions(pcoords, weights);

  x[0] = x[1] = x[2] = 0.0;
  for (i=0; i<10; i++)
    {
    pt = this->Points->GetPoint(i);
    for (j=0; j<3; j++)
      {
      x[j] += pt[j] * weights[i];
      }
    }
}

int vtkQuadraticTetra::CellBoundary(int subId, float pcoords[3], 
                                    vtkIdList *pts)
{
  return this->Tetra->CellBoundary(subId, pcoords, pts);
}

void vtkQuadraticTetra::Contour(float value, vtkDataArray* cellScalars, 
                                vtkPointLocator* locator, 
                                vtkCellArray *verts, vtkCellArray* lines, 
                                vtkCellArray* polys, 
                                vtkPointData* inPd, vtkPointData* outPd,
                                vtkCellData* inCd, vtkIdType cellId, 
                                vtkCellData* outCd)
{
  for ( int i=0; i < 6; i++) //for each subdivided tetra
    {
    for ( int j=0; j<4; j++) //for each of the four vertices of the tetra
      {
      this->Tetra->Points->SetPoint(j,this->Points->GetPoint(Tetras[i][j]));
      this->Tetra->PointIds->SetId(j,this->PointIds->GetId(Tetras[i][j]));
      this->Scalars->SetTuple(j,cellScalars->GetTuple(Tetras[i][j]));
      }

    this->Tetra->Contour(value, this->Scalars, locator, verts,
                         lines, polys, inPd, outPd, inCd, cellId, outCd);
    }
}

// Line-line intersection. Intersection has to occur within [0,1] parametric
// coordinates and with specified tolerance.
int vtkQuadraticTetra::IntersectWithLine(float* p1, float* p2, 
                                         float tol, float& t,
                                         float* x, float* pcoords, int& subId)
{
  int intersection=0;
  float tTemp;
  float pc[3], xTemp[3];
  int faceNum;

  t = VTK_LARGE_FLOAT;
  for (faceNum=0; faceNum<4; faceNum++)
    {
    for (int i=0; i<4; i++)
      {
      this->Face->Points->SetPoint(i,this->Points->GetPoint(TetraFace[faceNum][i]));
      }

    if ( this->Face->IntersectWithLine(p1, p2, tol, tTemp, 
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

int vtkQuadraticTetra::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds, 
                                  vtkPoints *pts)
{
  pts->Reset();
  ptIds->Reset();

  for ( int i=0; i < 6; i++)
    {
    for ( int j=0; j < 4; j++)
      {
      ptIds->InsertId(4*i+j,this->PointIds->GetId(Tetras[i][j]));
      pts->InsertPoint(4*i+j,this->Points->GetPoint(Tetras[i][j]));
      }
    }

  return 1;
}

// Given parametric coordinates compute inverse Jacobian transformation
// matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
// function derivatives.
void vtkQuadraticTetra::JacobianInverse(float pcoords[3], double **inverse, 
                                        float derivs[60])
{
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

  for ( j=0; j < 10; j++ )
    {
    x = this->Points->GetPoint(j);
    for ( i=0; i < 3; i++ )
      {
      m0[i] += x[i] * derivs[j];
      m1[i] += x[i] * derivs[10 + j];
      m2[i] += x[i] * derivs[20 + j];
      }
    }

  // now find the inverse
  if ( vtkMath::InvertMatrix(m,inverse,3) == 0 )
    {
    vtkErrorMacro(<<"Jacobian inverse not found");
    return;
    }
}

void vtkQuadraticTetra::Derivatives(int vtkNotUsed(subId), 
                                    float pcoords[3], float *values, 
                                    int dim, float *derivs)
{
  double *jI[3], j0[3], j1[3], j2[3];
  float functionDerivs[30], sum[3];
  int i, j, k;

  // compute inverse Jacobian and interpolation function derivatives
  jI[0] = j0; jI[1] = j1; jI[2] = j2;
  this->JacobianInverse(pcoords, jI, functionDerivs);

  // now compute derivates of values provided
  for (k=0; k < dim; k++) //loop over values per vertex
    {
    sum[0] = sum[1] = sum[2] = 0.0;
    for ( i=0; i < 10; i++) //loop over interp. function derivatives
      {
      sum[0] += functionDerivs[i] * values[dim*i + k]; 
      sum[1] += functionDerivs[10 + i] * values[dim*i + k];
      sum[2] += functionDerivs[20 + i] * values[dim*i + k];
      }
    for (j=0; j < 3; j++) //loop over derivative directions
      {
      derivs[3*k + j] = sum[0]*jI[j][0] + sum[1]*jI[j][1] + sum[2]*jI[j][2];
      }
    }
}

// Clip this quadratic tetra using the scalar value provided. Like contouring,
// except that it cuts the tetra to produce other tetra.
void vtkQuadraticTetra::Clip(float value, vtkDataArray* cellScalars, 
                            vtkPointLocator* locator, vtkCellArray* tetras,
                            vtkPointData* inPd, vtkPointData* outPd,
                            vtkCellData* inCd, vtkIdType cellId, 
                            vtkCellData* outCd, int insideOut)
{
  for ( int i=0; i < 6; i++) //for each subdivided tetra
    {
    for ( int j=0; j<4; j++) //for each of the four vertices of the tetra
      {
      this->Tetra->Points->SetPoint(j,this->Points->GetPoint(Tetras[i][j]));
      this->Tetra->PointIds->SetId(j,this->PointIds->GetId(Tetras[i][j]));
      this->Scalars->SetTuple(j,cellScalars->GetTuple(Tetras[i][j]));
      }

    this->Tetra->Clip(value, this->Scalars, locator, tetras, inPd, outPd, 
                      inCd, cellId, outCd, insideOut);
    }
}

int vtkQuadraticTetra::GetParametricCenter(float pcoords[3])
{
  pcoords[0] = pcoords[1] = pcoords[2] = 0.25;
  return 0;
}

// Compute interpolation functions. First four nodes are the 
// tetrahedron corner vertices; the others are mid-edge nodes.
void vtkQuadraticTetra::InterpolationFunctions(float pcoords[3], 
                                               float weights[3])
{
  float r = pcoords[0];
  float s = pcoords[1];
  float t = pcoords[2];
  float u = 1.0 - r - s - t;

  // corners
  weights[0] = u*(2.0*u - 1.0);
  weights[1] = r*(2.0*r - 1.0);
  weights[2] = s*(2.0*s - 1.0);
  weights[3] = t*(2.0*t - 1.0);

  // midedge
  weights[4] = 4.0 * u * r;
  weights[5] = 4.0 * r * s;
  weights[6] = 4.0 * s * u;
  weights[7] = 4.0 * u * t;
  weights[8] = 4.0 * s * t;
  weights[9] = 4.0 * r * t;
}

// Derivatives in parametric space.
void vtkQuadraticTetra::InterpolationDerivs(float pcoords[3], float derivs[30])
{
  float r = pcoords[0];
  float s = pcoords[1];
  float t = pcoords[2];

  // r-derivatives
  derivs[0] = 4.0*r + 4.0*s + 4.0*t - 3.0;
  derivs[1] = 4.0*r - 1.0;
  derivs[2] = 0.0;
  derivs[3] = 0.0;
  derivs[4] = 4.0 - 8.0*r - 4.0*s - 4.0*t;
  derivs[5] = 4.0*s;
  derivs[6] = -4.0*s;
  derivs[7] = -4.0*t;
  derivs[8] = 0.0;
  derivs[9] = 4.0*t;

  // s-derivatives
  derivs[10] = 4.0*r + 4.0*s + 4.0*t - 3.0;
  derivs[11] = 0.0;
  derivs[12] = 4.0*s - 1.0;
  derivs[13] = 0.0;
  derivs[14] = -4.0*r;
  derivs[15] =  4.0*r;
  derivs[16] = 4.0 - 4.0*r - 8.0*s - 4.0*t;
  derivs[17] = -4.0*t;
  derivs[18] =  4.0*t;
  derivs[19] = 0.0;

  // t-derivatives
  derivs[20] = 4.0*r + 4.0*s + 4.0*t - 3.0;
  derivs[21] = 0.0;
  derivs[22] = 0.0;
  derivs[23] = 4.0*t - 1.0;
  derivs[24] = -4.0*r;
  derivs[25] = 0.0;
  derivs[26] = -4.0*t;
  derivs[27] = 4.0 - 4.0*r - 8.0*s - 4.0*t;
  derivs[28] = 4.0*s;
  derivs[29] = 4.0*t;
  
}

