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
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkQuadraticHexahedron, "1.1");
vtkStandardNewMacro(vtkQuadraticHexahedron);

// Construct the line with two points.
vtkQuadraticHexahedron::vtkQuadraticHexahedron()
{
  int i;
  this->Points->SetNumberOfPoints(20);
  this->PointIds->SetNumberOfIds(20);
  for (i = 0; i < 20; i++)
    {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    }
  for (i = 0; i < 20; i++)
    {
    this->PointIds->SetId(i,0);
    }

  this->Edge = vtkQuadraticEdge::New();
  this->Face = vtkQuadraticQuad::New();
  this->Region = vtkHexahedron::New();
}

vtkQuadraticHexahedron::~vtkQuadraticHexahedron()
{
  this->Edge->Delete();
  this->Face->Delete();
  this->Region->Delete();
}


vtkCell *vtkQuadraticHexahedron::MakeObject()
{
  vtkQuadraticHexahedron *cell = vtkQuadraticHexahedron::New();
  cell->DeepCopy(this);
  return (vtkCell *)cell;
}

int vtkQuadraticHexahedron::EvaluatePosition(float* x, 
                                       float* closestPoint, 
                                       int& subId, float pcoords[3],
                                       float& vtkNotUsed(dist2), 
                                       float *weights)
{
  subId = 0;
  pcoords[1] = pcoords[2] = 0.0;

  return 0;
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

void vtkQuadraticHexahedron::EvaluateLocation(int& vtkNotUsed(subId), 
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

int vtkQuadraticHexahedron::CellBoundary(int vtkNotUsed(subId), 
                                         float pcoords[3], vtkIdList *pts)
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

void vtkQuadraticHexahedron::Contour(float vtkNotUsed(value), 
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

void vtkQuadraticHexahedron::Derivatives(int vtkNotUsed(subId), 
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

