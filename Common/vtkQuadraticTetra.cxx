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
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkQuadraticTetra, "1.3");
vtkStandardNewMacro(vtkQuadraticTetra);

// Construct the line with two points.
vtkQuadraticTetra::vtkQuadraticTetra()
{
  this->Edge = vtkQuadraticEdge::New();
  this->Face = vtkQuadraticTriangle::New();
  this->Region = vtkTetra::New();

  int i;
  this->Points->SetNumberOfPoints(10);
  this->PointIds->SetNumberOfIds(10);
  for (i = 0; i < 10; i++)
    {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    }
  for (i = 0; i < 10; i++)
    {
    this->PointIds->SetId(i,0);
    }
}

vtkQuadraticTetra::~vtkQuadraticTetra()
{
  this->Edge->Delete();
  this->Face->Delete();
  this->Region->Delete();
}


vtkCell *vtkQuadraticTetra::MakeObject()
{
  vtkQuadraticTetra *cell = vtkQuadraticTetra::New();
  cell->DeepCopy(this);
  return (vtkCell *)cell;
}

static int TetraEdge[6][3] = { {0,1,4}, {1,2,5}, {2,0,6}, 
                               {0,3,7}, {1,3,8}, {2,3,9} };

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

static int TetraFace[4][6] = { {0,1,3,4,8,7}, {1,2,3,5,9,8}, 
                               {2,0,3,6,7,9}, {0,2,1,6,5,4} };

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
  pcoords[0] = pcoords[1] = pcoords[2] = params[0] = params[1] = params[2]=0.5;

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
  return this->Region->CellBoundary(subId, pcoords, pts);
}

void vtkQuadraticTetra::Contour(float vtkNotUsed(value), 
                                vtkDataArray* vtkNotUsed(cellScalars), 
                                vtkPointLocator* vtkNotUsed(locator), 
                                vtkCellArray *vtkNotUsed(verts), 
                                vtkCellArray* vtkNotUsed(lines), 
                                vtkCellArray* vtkNotUsed(polys), 
                                vtkPointData* vtkNotUsed(inPd), 
                                vtkPointData* vtkNotUsed(outPd),
                                vtkCellData* vtkNotUsed(inCd), 
                                vtkIdType vtkNotUsed(cellId), 
                                vtkCellData* vtkNotUsed(outCd))
{
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
    for (int i=0; i<6; i++)
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

  ptIds->InsertId(0,this->PointIds->GetId(0));
  pts->InsertPoint(0,this->Points->GetPoint(0));

  ptIds->InsertId(1,this->PointIds->GetId(1));
  pts->InsertPoint(1,this->Points->GetPoint(1));

  return 1;
}

void vtkQuadraticTetra::Derivatives(int vtkNotUsed(subId), 
                                    float pcoords[3], float *values, 
                                    int dim, float *derivs)
{
  float *x0, *x1, *x2, deltaX[3], weights[3];
  int i, j;

  x0 = this->Points->GetPoint(0);
  x1 = this->Points->GetPoint(1);
  x2 = this->Points->GetPoint(2);

  this->InterpolationFunctions(pcoords,weights);
  this->InterpolationDerivs(pcoords,derivs);
  
  for (i=0; i<3; i++)
    {
    deltaX[i] = x1[i] - x0[i]              - x2[i];
    }
  for (i=0; i<dim; i++) 
    {
    for (j=0; j<3; j++)
      {
      if ( deltaX[j] != 0 )
        {
        derivs[3*i+j] = (values[2*i+1] - values[2*i]) / deltaX[j];
        }
      else
        {
        derivs[3*i+j] =0;
        }
      }
    }
}


// Clip this line using scalar value provided. Like contouring, except
// that it cuts the line to produce other lines.
void vtkQuadraticTetra::Clip(float vtkNotUsed(value), 
                            vtkDataArray* vtkNotUsed(cellScalars), 
                            vtkPointLocator* vtkNotUsed(locator),
                            vtkCellArray* vtkNotUsed(lines),
                            vtkPointData* vtkNotUsed(inPd), 
                            vtkPointData* vtkNotUsed(outPd),
                            vtkCellData* vtkNotUsed(inCd), 
                            vtkIdType vtkNotUsed(cellId), 
                            vtkCellData* vtkNotUsed(outCd),
                            int vtkNotUsed(insideOut))
{
}

int vtkQuadraticTetra::GetParametricCenter(float pcoords[3])
{
  pcoords[0] = pcoords[1] = pcoords[2] = 0.25;
  return 0;
}

// Compute interpolation functions. Node [2] is the mid-edge node.
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
  // r-derivatives
  derivs[0] = 0.0;
  derivs[1] = 0.0;
  derivs[2] = 0.0;
  derivs[3] = 0.0;
  derivs[4] = 0.0;
  derivs[5] = 0.0;
  derivs[6] = 0.0;
  derivs[7] = 0.0;
  derivs[8] = 0.0;
  derivs[9] = 0.0;

  // s-derivatives
  derivs[10] = 0.0;
  derivs[11] = 0.0;
  derivs[12] = 0.0;
  derivs[13] = 0.0;
  derivs[14] = 0.0;
  derivs[15] = 0.0;
  derivs[16] = 0.0;
  derivs[17] = 0.0;
  derivs[18] = 0.0;
  derivs[19] = 0.0;

  // t-derivatives
  derivs[20] = 0.0;
  derivs[21] = 0.0;
  derivs[22] = 0.0;
  derivs[23] = 0.0;
  derivs[24] = 0.0;
  derivs[25] = 0.0;
  derivs[26] = 0.0;
  derivs[27] = 0.0;
  derivs[28] = 0.0;
  derivs[29] = 0.0;
  
}

