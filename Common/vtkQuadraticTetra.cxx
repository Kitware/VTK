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

vtkCxxRevisionMacro(vtkQuadraticTetra, "1.2");
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

static int TetraFace[4][6] = { {0,1,2,4,5,6}, {0,1,3,4,8,7}, 
                                {1,2,3,5,9,8}, {2,0,3,6,7,9} };

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

int vtkQuadraticTetra::EvaluatePosition(float* x, 
                                       float* closestPoint, 
                                       int& subId, float pcoords[3],
                                       float& vtkNotUsed(dist2), 
                                       float *weights)
{
  subId = 0;
  pcoords[1] = pcoords[2] = 0.0;

  return 0;
}

void vtkQuadraticTetra::EvaluateLocation(int& vtkNotUsed(subId), 
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

int vtkQuadraticTetra::CellBoundary(int vtkNotUsed(subId), float pcoords[3], 
                                   vtkIdList *pts)
{
  pts->SetNumberOfIds(1);

  if ( pcoords[0] >= 0.5 )
    {
    pts->SetId(0,this->PointIds->GetId(1));
    if ( pcoords[0] > 1.0 )
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
    pts->SetId(0,this->PointIds->GetId(0));
    if ( pcoords[0] < 0.0 )
      {
      return 0;
      }
    else
      {
      return 1;
      }
    }
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

// The following arguments were modified to avoid warnings:
// float p1[3], float p2[3], float x[3], float pcoords[3], 

int vtkQuadraticTetra::IntersectWithLine(float* vtkNotUsed(p1), 
                                        float* vtkNotUsed(p2), 
                                        float vtkNotUsed(tol), 
                                        float& vtkNotUsed(t),
                                        float* vtkNotUsed(x), 
                                        float* vtkNotUsed(pcoords), 
                                        int& vtkNotUsed(subId))
{
  return 0;
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
  derivs[0] = ;
  derivs[1] = ;
  derivs[2] = ;
  derivs[3] = ;
  derivs[4] = ;
  derivs[5] = ;
  derivs[6] = ;
  derivs[7] = ;
  derivs[8] = ;
  derivs[9] = ;

  // s-derivatives
  derivs[10] = ;
  derivs[11] = ;
  derivs[12] = ;
  derivs[13] = ;
  derivs[14] = ;
  derivs[15] = ;
  derivs[16] = ;
  derivs[17] = ;
  derivs[18] = ;
  derivs[19] = ;

  // t-derivatives
  derivs[20] = ;
  derivs[21] = ;
  derivs[22] = ;
  derivs[23] = ;
  derivs[24] = ;
  derivs[25] = ;
  derivs[26] = ;
  derivs[27] = ;
  derivs[28] = ;
  derivs[29] = ;
  
}

