/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraticEdge.cxx
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
#include "vtkQuadraticEdge.h"
#include "vtkPolyData.h"
#include "vtkPointLocator.h"
#include "vtkMath.h"
#include "vtkLine.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkQuadraticEdge, "1.15");
vtkStandardNewMacro(vtkQuadraticEdge);

// Construct the line with two points.
vtkQuadraticEdge::vtkQuadraticEdge()
{
  this->Line = vtkLine::New();

  int i;
  this->Points->SetNumberOfPoints(3);
  this->PointIds->SetNumberOfIds(3);
  for (i = 0; i < 3; i++)
    {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i,0);
    }
}

vtkQuadraticEdge::~vtkQuadraticEdge()
{
  this->Line->Delete();
}


vtkCell *vtkQuadraticEdge::MakeObject()
{
  vtkQuadraticEdge *cell = vtkQuadraticEdge::New();
  cell->DeepCopy(this);
  return (vtkCell *)cell;
}

int vtkQuadraticEdge::EvaluatePosition(float* x, float* closestPoint, 
                                       int& subId, float pcoords[3],
                                       float& minDist2, float *weights)
{
  float closest[3];
  float pc[3], dist2;
  int ignoreId, i, returnStatus, status;
  float lineWeights[2];

  pcoords[1] = pcoords[2] = 0.0;

  returnStatus = -1;
  weights[0] = 0.0;
  for (minDist2=VTK_LARGE_FLOAT,i=0; i < 2; i++)
    {
    if ( i == 0)
      {
      this->Line->Points->SetPoint(0,this->Points->GetPoint(0));
      this->Line->Points->SetPoint(1,this->Points->GetPoint(2));
      }
    else
      {
      this->Line->Points->SetPoint(0,this->Points->GetPoint(2));
      this->Line->Points->SetPoint(1,this->Points->GetPoint(1));
      }
    
    status = this->Line->EvaluatePosition(x,closest,ignoreId,pc,
                                          dist2,lineWeights);
    if ( status != -1 && dist2 < minDist2 )
      {
      returnStatus = status;
      minDist2 = dist2;
      subId = i;
      pcoords[0] = pc[0];
      }
    }

  // adjust parametric coordinate
  if ( returnStatus != -1 )
    {
    if ( subId == 0 ) //first part
      {
      pcoords[0] = pcoords[0]/2.0;
      }
    else
      {
      pcoords[0] = 0.5 + pcoords[0]/2.0;
      }
    this->EvaluateLocation(subId,pcoords,closestPoint,weights);
    }

  return returnStatus;
}

void vtkQuadraticEdge::EvaluateLocation(int& vtkNotUsed(subId), 
                                        float pcoords[3], 
                                        float x[3], float *weights)
{
  int i;
  float *a0 = this->Points->GetPoint(0);
  float *a1 = this->Points->GetPoint(1);
  float *a2 = this->Points->GetPoint(2); //midside node

  this->InterpolationFunctions(pcoords,weights);
  
  for (i=0; i<3; i++) 
    {
    x[i] = a0[i]*weights[0] + a1[i]*weights[1] + a2[i]*weights[2];
    }
}

int vtkQuadraticEdge::CellBoundary(int subId, float pcoords[3], 
                                   vtkIdList *pts)
{
  return this->Line->CellBoundary(subId, pcoords, pts);
}

static int linearLines[2][2] = { {0,2}, {2,1} };                             
    

void vtkQuadraticEdge::Contour(float value, vtkDataArray *cellScalars,
                               vtkPointLocator *locator, vtkCellArray *verts, 
                               vtkCellArray *lines, vtkCellArray *polys, 
                               vtkPointData *inPd, vtkPointData *outPd,
                               vtkCellData *inCd, vtkIdType cellId,
                               vtkCellData *outCd)
{
  int i;
  vtkDataArray *lineScalars=cellScalars->MakeObject();
  lineScalars->SetNumberOfTuples(2);

  for ( i=0; i < 2; i++)
    {
    this->Line->Points->SetPoint(0,this->Points->GetPoint(linearLines[i][0]));
    this->Line->Points->SetPoint(1,this->Points->GetPoint(linearLines[i][1]));

    if ( outPd )
      {
      this->Line->PointIds->SetId(0,this->PointIds->GetId(linearLines[i][0]));
      this->Line->PointIds->SetId(1,this->PointIds->GetId(linearLines[i][1]));
      }

    lineScalars->SetTuple(0,cellScalars->GetTuple(linearLines[i][0]));
    lineScalars->SetTuple(1,cellScalars->GetTuple(linearLines[i][1]));

    this->Line->Contour(value, lineScalars, locator, verts,
                       lines, polys, inPd, outPd, inCd, cellId, outCd);
    }
  lineScalars->Delete();
}

// Line-line intersection. Intersection has to occur within [0,1] parametric
// coordinates and with specified tolerance.

// The following arguments were modified to avoid warnings:
// float p1[3], float p2[3], float x[3], float pcoords[3], 

int vtkQuadraticEdge::IntersectWithLine(float p1[3], float p2[3], float tol,
                                        float& t, float x[3], float pcoords[3],
                                        int& subId)
{
  int subTest, numLines=2;

  for (subId=0; subId < numLines; subId++)
    {
    if ( subId == 0)
      {
      this->Line->Points->SetPoint(0,this->Points->GetPoint(0));
      this->Line->Points->SetPoint(1,this->Points->GetPoint(2));
      }
    else
      {
      this->Line->Points->SetPoint(0,this->Points->GetPoint(2));
      this->Line->Points->SetPoint(1,this->Points->GetPoint(1));
      }

    if ( this->Line->IntersectWithLine(p1, p2, tol, t, x, pcoords, subTest) )
      {
      return 1;
      }
    }

  return 0;
}

int vtkQuadraticEdge::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds, 
                                  vtkPoints *pts)
{
  pts->Reset();
  ptIds->Reset();

  // The first line
  ptIds->InsertId(0,this->PointIds->GetId(0));
  pts->InsertPoint(0,this->Points->GetPoint(0));

  ptIds->InsertId(1,this->PointIds->GetId(2));
  pts->InsertPoint(1,this->Points->GetPoint(2));

  // The second line
  ptIds->InsertId(2,this->PointIds->GetId(2));
  pts->InsertPoint(2,this->Points->GetPoint(2));

  ptIds->InsertId(3,this->PointIds->GetId(1));
  pts->InsertPoint(3,this->Points->GetPoint(1));

  return 1;
}

void vtkQuadraticEdge::Derivatives(int vtkNotUsed(subId), 
                                   float pcoords[3], float *values,
                                   int dim, float *derivs)
{
  float *x0 = this->Points->GetPoint(0);
  float *x1 = this->Points->GetPoint(1);
  float *x2 = this->Points->GetPoint(2); //midside node

  // set up Jacobian matrix and inverse matrix
  double *jTj[3], jTj0[3], jTj1[3], jTj2[3];
  jTj[0] = jTj0; jTj[1] = jTj1; jTj[2] = jTj2;
  double *jI[3], jI0[3], jI1[3], jI2[3];
  jI[0] = jI0; jI[1] = jI1; jI[2] = jI2;
  /*
  double *iI[3], iI0[3], iI1[3], iI2[3];
  iI[0] = iI0; iI[1] = iI1; iI[2] = iI2;
  */
  // Compute dx/dt, dy/dt, dz/dt
  this->InterpolationDerivs(pcoords,derivs);
  float dxdt = x0[0]*derivs[0] + x1[0]*derivs[1] + x2[0]*derivs[2];
  float dydt = x0[1]*derivs[0] + x1[1]*derivs[1] + x2[1]*derivs[2];
  float dzdt = x0[2]*derivs[0] + x1[2]*derivs[1] + x2[2]*derivs[2];
  
  // Compute the psuedo inverse (we are dealing with an overconstrained system,
  // i.e., a non-square Jacobian matrix). The pseudo inverse is ((jT*j)-1)*jT
  // with jT Jacobian transpose, -1 notation means inverse.
  // Compute jT * j
  jTj[0][0] = dxdt*dxdt;
  jTj[0][1] = dxdt*dydt;
  jTj[0][2] = dxdt*dzdt;
  jTj[1][0] = dydt*dxdt;
  jTj[1][1] = dydt*dydt;
  jTj[1][2] = dydt*dzdt;
  jTj[2][0] = dzdt*dxdt;
  jTj[2][1] = dzdt*dydt;
  jTj[2][2] = dzdt*dzdt;
  
  // Compute (jT * j) inverse
  // now find the inverse
  if ( vtkMath::InvertMatrix(jTj,jI,3) == 0 )
    {
    vtkErrorMacro(<<"Jacobian inverse not found");
    return;
    }
  
  // Multiply inverse by transpose (jT * j) * jT to yield pseudo inverse.
  // Here the pseudo inverse is a 3x1 matrix.
  float inv[3];
  inv[0] = jI[0][0]*dxdt + jI[0][1]*dydt + jI[0][2]*dzdt;
  inv[1] = jI[1][0]*dxdt + jI[1][1]*dydt + jI[1][2]*dzdt;
  inv[2] = jI[2][0]*dxdt + jI[2][1]*dydt + jI[2][2]*dzdt;

  //now compute the derivates of the data values
  float sum;
  int i, j, k;
  for (k=0; k<dim; k++)
    {
    sum = 0.0;
    for ( i=0; i < 3; i++) //loop over interp. function derivatives
      {
      sum += derivs[i] * values[dim*i + k]; 
      }
    for (j=0; j < 3; j++) //loop over derivative directions
      {
      derivs[3*k + j] = sum*inv[j];
      }
    }
}


// Clip this quadratic edge using scalar value provided. Like contouring, 
// except that it cuts the edge to produce linear line segments.
void vtkQuadraticEdge::Clip(float value, vtkDataArray *cellScalars, 
                            vtkPointLocator *locator, vtkCellArray *lines,
                            vtkPointData *inPd, vtkPointData *outPd,
                            vtkCellData *inCd, vtkIdType cellId, 
                            vtkCellData *outCd, int insideOut)
{
  int i, numLines=2;
  vtkFloatArray *lineScalars=vtkFloatArray::New();
  lineScalars->SetNumberOfTuples(2);

  for ( i=0; i < numLines; i++)
    {
    if ( i == 0 )
      {
      this->Line->Points->SetPoint(0,this->Points->GetPoint(0));
      this->Line->Points->SetPoint(1,this->Points->GetPoint(2));

      this->Line->PointIds->SetId(0,this->PointIds->GetId(0));
      this->Line->PointIds->SetId(1,this->PointIds->GetId(2));

      lineScalars->SetComponent(0,0,cellScalars->GetComponent(0,0));
      lineScalars->SetComponent(1,0,cellScalars->GetComponent(2,0));
      }
    else
      {
      this->Line->Points->SetPoint(0,this->Points->GetPoint(2));
      this->Line->Points->SetPoint(1,this->Points->GetPoint(1));

      this->Line->PointIds->SetId(0,this->PointIds->GetId(2));
      this->Line->PointIds->SetId(1,this->PointIds->GetId(1));

      lineScalars->SetComponent(0,0,cellScalars->GetComponent(2,0));
      lineScalars->SetComponent(1,0,cellScalars->GetComponent(1,0));
      }

    this->Line->Clip(value, lineScalars, locator, lines, inPd, outPd, 
                     inCd, cellId, outCd, insideOut);
    }
  
  lineScalars->Delete();
}

// Compute interpolation functions. Node [2] is the mid-edge node.
void vtkQuadraticEdge::InterpolationFunctions(float pcoords[3], float weights[3])
{
  float r = pcoords[0];

  weights[0] = 2.0 * (r - 0.5) * (r - 1.0);
  weights[1] = 2.0 * r * (r - 0.5);
  weights[2] = 4.0 * r * (1.0 - r);
}

// Derivatives in parametric space.
void vtkQuadraticEdge::InterpolationDerivs(float pcoords[3], float derivs[3])
{
  float r = pcoords[0];

  derivs[0] = 4.0 * r - 3.0;
  derivs[1] = 4.0 * r - 1.0;
  derivs[2] = 4.0 - r * 8.0;
}

