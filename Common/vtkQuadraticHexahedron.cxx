/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraticHexahedron.cxx
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
#include "vtkQuadraticHexahedron.h"
#include "vtkPolyData.h"
#include "vtkPointLocator.h"
#include "vtkMath.h"
#include "vtkQuadraticEdge.h"
#include "vtkQuadraticQuad.h"
#include "vtkHexahedron.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkQuadraticHexahedron, "1.3");
vtkStandardNewMacro(vtkQuadraticHexahedron);

// Construct the hex with 20 points + 7 extra points for internal
// computation.
vtkQuadraticHexahedron::vtkQuadraticHexahedron()
{
  int i;
  this->Points->SetNumberOfPoints(27);
  this->PointIds->SetNumberOfIds(27);
  for (i = 0; i < 27; i++)
    {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    }
  for (i = 0; i < 27; i++)
    {
    this->PointIds->SetId(i,0);
    }

  this->Edge = vtkQuadraticEdge::New();
  this->Face = vtkQuadraticQuad::New();
  this->Region = vtkHexahedron::New();

  this->PointData = vtkPointData::New();
  this->CellData = vtkCellData::New();
  this->Scalars = vtkFloatArray::New();
  this->Scalars->SetNumberOfTuples(8);
}

vtkQuadraticHexahedron::~vtkQuadraticHexahedron()
{
  this->Edge->Delete();
  this->Face->Delete();
  this->Region->Delete();

  this->PointData->Delete();
  this->CellData->Delete();
  this->Scalars->Delete();
}


vtkCell *vtkQuadraticHexahedron::MakeObject()
{
  vtkQuadraticHexahedron *cell = vtkQuadraticHexahedron::New();
  cell->DeepCopy(this);
  return (vtkCell *)cell;
}

vtkCell *vtkQuadraticHexahedron::GetEdge(int edgeId)
{
  edgeId = (edgeId < 0 ? 0 : (edgeId > 11 ? 11 : edgeId ));

  // load point id's
  this->Edge->PointIds->SetId(0,this->PointIds->GetId(edgeId));
  this->Edge->PointIds->SetId(1,this->PointIds->GetId(edgeId+1));
  this->Edge->PointIds->SetId(2,this->PointIds->GetId(edgeId+3));

  // load coordinates
  this->Edge->Points->SetPoint(0,this->Points->GetPoint(edgeId));
  this->Edge->Points->SetPoint(1,this->Points->GetPoint(edgeId+1));
  this->Edge->Points->SetPoint(2,this->Points->GetPoint(edgeId+3));

  return this->Edge;
}

vtkCell *vtkQuadraticHexahedron::GetFace(int faceId)
{
  faceId = (faceId < 0 ? 0 : (faceId > 5 ? 5 : faceId ));

  // load point id's
  this->Face->PointIds->SetId(0,this->PointIds->GetId(faceId));
  this->Face->PointIds->SetId(1,this->PointIds->GetId(faceId+1));
  this->Face->PointIds->SetId(2,this->PointIds->GetId(faceId+3));

  // load coordinates
  this->Face->Points->SetPoint(0,this->Points->GetPoint(faceId));
  this->Face->Points->SetPoint(1,this->Points->GetPoint(faceId+1));
  this->Face->Points->SetPoint(2,this->Points->GetPoint(faceId+3));

  return this->Face;
}

static float MidPoints[7][3] = { {-0.5,0.0,0.0}, {0.5,0.0,0.0}, 
                                 {0.0,-0.5,0.0}, {0.0,0.5,0.0},
                                 {0.0,0.0,-0.5}, {0.0,0.0,0.5},
                                 {0.5,0.5,0.5} };

void vtkQuadraticHexahedron::Subdivide(float *weights)
{
  int numMidPts, i, j;
  float x[3];

  float *p = ((vtkFloatArray *)this->Points->GetData())->GetPointer(0);
  for ( numMidPts=0; numMidPts < 7; numMidPts++ )
    {
    this->InterpolationFunctions(MidPoints[numMidPts], weights);

    for (j=0; j<3; j++) 
      {
      x[j] = 0.0;
      for (i=0; i<20; i++) 
        {
        x[j] += p[3*i+j] * weights[i];
        }
      }
    this->Points->SetPoint(20+numMidPts,x);
    }
}

void vtkQuadraticHexahedron::InterpolateAttributes(vtkPointData *inPd, 
                                                   vtkCellData *inCd, 
                                                   vtkIdType cellId,
                                                   float *weights)
{
  this->PointData->CopyAllocate(inPd,27);
  this->CellData->CopyAllocate(inCd,8);
  
  // copy the point data over into point ids 0->7
  for (int i=0; i<20; i++)
    {
    this->PointData->CopyData(inPd,this->PointIds->GetId(i),i);
    }

  // now interpolate the center point
  this->PointData->InterpolatePoint(inPd, 8, this->PointIds, weights);
  
  // copy the cell data over to the linear cell
  this->CellData->CopyData(inCd,cellId,0);
}

static const float VTK_DIVERGED = 1.e6;
static const int VTK_HEX_MAX_ITERATION=10;
static const float VTK_HEX_CONVERGED=1.e-03;

int vtkQuadraticHexahedron::EvaluatePosition(float* x, 
                                             float* closestPoint, 
                                             int& subId, float pcoords[3],
                                             float& dist2, float *weights)
{
  int iteration, converged;
  float  params[3];
  float  fcol[3], rcol[3], scol[3], tcol[3];
  int i, j;
  float  d, *pt;
  float derivs[60];

  //  set initial position for Newton's method
  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = params[0] = params[1] = params[2]=0.5;

  //  enter iteration loop
  for (iteration=converged=0;
       !converged && (iteration < VTK_HEX_MAX_ITERATION);  iteration++) 
    {
    //  calculate element interpolation functions and derivatives
    this->InterpolationFunctions(pcoords, weights);
    this->InterpolationDerivs(pcoords, derivs);

    //  calculate newton functions
    for (i=0; i<3; i++) 
      {
      fcol[i] = rcol[i] = scol[i] = tcol[i] = 0.0;
      }
    for (i=0; i<20; i++)
      {
      pt = this->Points->GetPoint(i);
      for (j=0; j<3; j++)
        {
        fcol[j] += pt[j] * weights[i];
        rcol[j] += pt[j] * derivs[i];
        scol[j] += pt[j] * derivs[i+20];
        tcol[j] += pt[j] * derivs[i+40];
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
    if ( ((fabs(pcoords[0]-params[0])) < VTK_HEX_CONVERGED) &&
         ((fabs(pcoords[1]-params[1])) < VTK_HEX_CONVERGED) &&
         ((fabs(pcoords[2]-params[2])) < VTK_HEX_CONVERGED) )
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
    float pc[3], w[20];
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

void vtkQuadraticHexahedron::EvaluateLocation(int& vtkNotUsed(subId), 
                                              float pcoords[3], 
                                              float x[3], float *weights)
{
  int i, j;
  float *pt;

  this->InterpolationFunctions(pcoords, weights);

  x[0] = x[1] = x[2] = 0.0;
  for (i=0; i<20; i++)
    {
    pt = this->Points->GetPoint(i);
    for (j=0; j<3; j++)
      {
      x[j] += pt[j] * weights[i];
      }
    }
}

int vtkQuadraticHexahedron::CellBoundary(int subId, float pcoords[3], 
                                         vtkIdList *pts)
{
  return this->Region->CellBoundary(subId, pcoords, pts);
}

static int LinearHexs[8][8] = { {0,8,24,11,16,17,26,20},
                                {8,1,9,24,22,17,21,26},
                                {11,24,10,3,20,26,23,19},
                                {24,9,2,10,26,21,18,23},
                                {16,22,26,20,4,12,25,15},
                                {22,17,21,26,12,5,13,25},
                                {20,26,23,19,15,25,14,7},
                                {26,21,18,23,25,13,6,14} };


void vtkQuadraticHexahedron::Contour(float value, 
                                     vtkDataArray* cellScalars, 
                                     vtkPointLocator* locator, 
                                     vtkCellArray *verts, 
                                     vtkCellArray* lines, 
                                     vtkCellArray* polys, 
                                     vtkPointData* inPd, 
                                     vtkPointData* outPd,
                                     vtkCellData* inCd, 
                                     vtkIdType cellId, 
                                     vtkCellData* outCd)
{
  float weights[20];

  //first define the midquad point
  this->Subdivide(weights);
  
  //interpolate point and cell data
  this->InterpolateAttributes(inPd,inCd,cellId,weights);
  
  //contour each linear quad separately
  vtkDataArray *localScalars = this->PointData->GetScalars();
  for (int i=0; i<8; i++)
    {
    for (int j=0; j<8; j++)
      {
      this->Scalars->SetValue(j,localScalars->GetTuple1(LinearHexs[i][j]));
      }
    this->Region->Contour(value,this->Scalars,locator,verts,lines,polys,
                          this->PointData,outPd,this->CellData,0,outCd);
    }
}

// Line-line intersection. Intersection has to occur within [0,1] parametric
// coordinates and with specified tolerance.

// The following arguments were modified to avoid warnings:
// float p1[3], float p2[3], float x[3], float pcoords[3], 

int vtkQuadraticHexahedron::IntersectWithLine(float* vtkNotUsed(p1), 
                                              float* vtkNotUsed(p2), 
                                              float vtkNotUsed(tol), 
                                              float& vtkNotUsed(t),
                                              float* vtkNotUsed(x), 
                                              float* vtkNotUsed(pcoords), 
                                              int& vtkNotUsed(subId))
{
  return 0;
}

int vtkQuadraticHexahedron::Triangulate(int vtkNotUsed(index), 
                                        vtkIdList *ptIds, vtkPoints *pts)
{
  pts->Reset();
  ptIds->Reset();

  ptIds->InsertId(0,this->PointIds->GetId(0));
  pts->InsertPoint(0,this->Points->GetPoint(0));

  ptIds->InsertId(1,this->PointIds->GetId(1));
  pts->InsertPoint(1,this->Points->GetPoint(1));

  return 1;
}

// Given parametric coordinates compute inverse Jacobian transformation
// matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
// function derivatives.
void vtkQuadraticHexahedron::JacobianInverse(float pcoords[3], 
                                             double **inverse, 
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

  for ( j=0; j < 20; j++ )
    {
    x = this->Points->GetPoint(j);
    for ( i=0; i < 3; i++ )
      {
      m0[i] += x[i] * derivs[j];
      m1[i] += x[i] * derivs[20 + j];
      m2[i] += x[i] * derivs[40 + j];
      }
    }

  // now find the inverse
  if ( vtkMath::InvertMatrix(m,inverse,3) == 0 )
    {
    vtkErrorMacro(<<"Jacobian inverse not found");
    return;
    }
}

void vtkQuadraticHexahedron::Derivatives(int vtkNotUsed(subId), 
                                         float pcoords[3], float *values, 
                                         int dim, float *derivs)
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
    sum[0] = sum[1] = sum[2] = 0.0;
    for ( i=0; i < 20; i++) //loop over interp. function derivatives
      {
      sum[0] += functionDerivs[i] * values[dim*i + k]; 
      sum[1] += functionDerivs[20 + i] * values[dim*i + k];
      sum[2] += functionDerivs[40 + i] * values[dim*i + k];
      }
    for (j=0; j < 3; j++) //loop over derivative directions
      {
      derivs[3*k + j] = sum[0]*jI[j][0] + sum[1]*jI[j][1] + sum[2]*jI[j][2];
      }
    }
}


// Clip this line using scalar value provided. Like contouring, except
// that it cuts the line to produce other lines.
void vtkQuadraticHexahedron::Clip(float vtkNotUsed(value), 
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

// Compute interpolation functions. Node [2] is the mid-edge node.
void vtkQuadraticHexahedron::InterpolationFunctions(float pcoords[3], 
                                                    float weights[3])
{
  weights[0] = 2.0 * (pcoords[0] - 0.5) * (pcoords[0] - 1.0);
  weights[1] = 2.0 * pcoords[0] * (pcoords[0] - 0.5);
  weights[2] = 4.0 * pcoords[0] * (1.0 - pcoords[0]);
}

// Derivatives in parametric space.
void vtkQuadraticHexahedron::InterpolationDerivs(float pcoords[3], 
                                                 float derivs[3])
{
  derivs[0] = 4.0 * pcoords[0] - 3.0;
  derivs[1] = 4.0 * pcoords[0] - 1.0;
  derivs[2] = 4.0 - pcoords[0] * 8.0;
}

