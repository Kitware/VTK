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
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkQuadraticEdge, "1.2");
vtkStandardNewMacro(vtkQuadraticEdge);

// Construct the line with two points.
vtkQuadraticEdge::vtkQuadraticEdge()
{
  int i;
  
  this->Points->SetNumberOfPoints(3);
  this->PointIds->SetNumberOfIds(3);
  for (i = 0; i < 3; i++)
    {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    }
  for (i = 0; i < 3; i++)
    {
    this->PointIds->SetId(i,0);
    }
  
  this->Tesselation = NULL;
  this->InternalDataSet = NULL;
}

vtkQuadraticEdge::~vtkQuadraticEdge()
{
  if ( this->Tesselation )
    {
    this->Tesselation->Delete();
    this->InternalDataSet->Delete();
    }
}


vtkCell *vtkQuadraticEdge::MakeObject()
{
  vtkQuadraticEdge *cell = vtkQuadraticEdge::New();
  cell->DeepCopy(this);
  cell->SetError(this->GetError());
  return (vtkCell *)cell;
}

int vtkQuadraticEdge::EvaluatePosition(float x[3], float* closestPoint, 
                                       int& subId, float pcoords[3],
                                       float& dist2, float *weights)
{
  float *a1, *a2;

  subId = 0;
  pcoords[1] = pcoords[2] = 0.0;

  a1 = this->Points->GetPoint(0);
  a2 = this->Points->GetPoint(1);

  if (closestPoint)
    {
    // DistanceToLine sets pcoords[0] to a value t, 0 <= t <= 1
//    dist2 = this->DistanceToLine(x,a1,a2,pcoords[0],closestPoint);
    }

  // pcoords[0] == t, need weights to be 1-t and t
  weights[0] = 1.0 - pcoords[0];
  weights[1] = pcoords[0];

  if ( pcoords[0] < 0.0 || pcoords[0] > 1.0 )
    {
    return 0;
    }
  else
    {
    return 1;
    }
}

void vtkQuadraticEdge::EvaluateLocation(int& vtkNotUsed(subId), 
                                        float pcoords[3], 
                                        float x[3], float *weights)
{
  int i;
  float *a0 = this->Points->GetPoint(0);
  float *a1 = this->Points->GetPoint(1);
  float *a2 = this->Points->GetPoint(2); //midside node

  weights[0] = 2.0*(pcoords[0]-0.5)*(pcoords[0]-1.0);
  weights[1] = 2.0*pcoords[0]*(pcoords[0]-0.5);
  weights[2] = 4.0*pcoords[0]*(1.0 - pcoords[0]);
  
  for (i=0; i<3; i++) 
    {
    x[i] = a0[i]*weights[0] + a1[i]*weights[1] + a2[i]*weights[2];
    }
}

int vtkQuadraticEdge::CellBoundary(int vtkNotUsed(subId), float pcoords[3], 
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

void vtkQuadraticEdge::Contour(float value, vtkDataArray *cellScalars, 
                               vtkPointLocator *locator, vtkCellArray *verts, 
                               vtkCellArray *vtkNotUsed(lines), 
                               vtkCellArray *vtkNotUsed(polys), 
                               vtkPointData *inPd, vtkPointData *outPd,
                               vtkCellData *inCd, vtkIdType cellId, 
                               vtkCellData *outCd)
{
}

// Line-line intersection. Intersection has to occur within [0,1] parametric
// coordinates and with specified tolerance.
int vtkQuadraticEdge::IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                                        float x[3], float pcoords[3], int& subId)
{
  return 0;
}

int vtkQuadraticEdge::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds, vtkPoints *pts)
{
  pts->Reset();
  ptIds->Reset();

  ptIds->InsertId(0,this->PointIds->GetId(0));
  pts->InsertPoint(0,this->Points->GetPoint(0));

  ptIds->InsertId(1,this->PointIds->GetId(1));
  pts->InsertPoint(1,this->Points->GetPoint(1));

  return 1;
}

void vtkQuadraticEdge::Derivatives(int vtkNotUsed(subId), 
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
    deltaX[i] = x1[i] - x0[i];
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
void vtkQuadraticEdge::Clip(float value, vtkDataArray *cellScalars, 
                            vtkPointLocator *locator, vtkCellArray *lines,
                            vtkPointData *inPd, vtkPointData *outPd,
                            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
                            int insideOut)
{
}

// Compute interpolation functions. Node [2] is the mid-edge node.
void vtkQuadraticEdge::InterpolationFunctions(float pcoords[3], float weights[3])
{
  weights[0] = 2.0 * (pcoords[0] - 0.5) * (pcoords[0] - 1.0);
  weights[1] = 2.0 * pcoords[0] * (pcoords[0] - 0.5);
  weights[2] = 4.0 * pcoords[0] * (1.0 - pcoords[0]);
}

// Derivatives in parametric space.
void vtkQuadraticEdge::InterpolationDerivs(float pcoords[3], float derivs[3])
{
  derivs[0] = 4.0 * pcoords[0] - 3.0;
  derivs[1] = 4.0 * pcoords[0] - 1.0;
  derivs[2] = 4.0 - pcoords[0] * 8.0;
}

// Create linear primitives from this quadratic cell.
void vtkQuadraticEdge::Tesselate(vtkIdType cellId, 
                                 vtkDataSet *input, vtkPolyData *output, 
                                 vtkPointLocator *locator)
{
  vtkPointData *inPD = input->GetPointData();
  vtkCellData *inCD = input->GetCellData();
  vtkPointData *outPD = output->GetPointData();
  vtkCellData *outCD = output->GetCellData();

  vtkCellArray *outputLines = output->GetLines();
  vtkPoints *pts = output->GetPoints();

  float *x0 = this->Points->GetPoint(0);
  float *x1 = this->Points->GetPoint(1);
  float *x2 = this->Points->GetPoint(2);
  
  float l2 = vtkMath::Distance2BetweenPoints(x0,x1);
  float d2 = vtkLine::DistanceToLine(x2, x0,x1); //midnode to endpoints
  float e2 = this->Error*this->Error;
  
  //the error divided by the maximum permissable error is an approximation to
  //the number of subdivisions.
  int numDivs = ceil( d2/(l2*e2) );
  int numPts = numDivs + 1;
  
  //add new points to the output
  vtkIdType newCellId;
  vtkIdType p0 = this->InsertPoint(locator,pts,x0);
  vtkIdType p1 = this->InsertPoint(locator,pts,x1);
  vtkIdType p2 = this->InsertPoint(locator,pts,x2);
  outPD->CopyData(inPD,this->PointIds->GetId(0),p0);
  outPD->CopyData(inPD,this->PointIds->GetId(1),p1);
  outPD->CopyData(inPD,this->PointIds->GetId(2),p2);
  
  //at a minimum the edge is subdivided into two polylines
  if ( numPts <= 3 )
    {//end points plus mid-node
    newCellId = outputLines->InsertNextCell(3);
    outputLines->InsertCellPoint(p0);
    outputLines->InsertCellPoint(p1);
    outputLines->InsertCellPoint(p2);
    outCD->CopyData(inCD,cellId,newCellId);
    }
  else
    {//have to interpolate points
    float pcoords[3], weights[3], x[3];
    vtkIdType p;

    newCellId = outputLines->InsertNextCell(numDivs);
    outputLines->InsertCellPoint(p0);
    float delta = 1.0/numDivs;
    for (int i=1; i<(numPts-1); i++)
      {
      pcoords[0] = i*delta;
      this->InterpolationFunctions(pcoords,weights);
      x[0] = x0[0]*weights[0] + x1[0]*weights[1] + x2[0]*weights[2];
      x[1] = x0[1]*weights[0] + x1[1]*weights[1] + x2[1]*weights[2];
      x[2] = x0[2]*weights[0] + x1[2]*weights[1] + x2[2]*weights[2];
      p = this->InsertPoint(locator,pts,x);
      outPD->InterpolatePoint(inPD,p,this->PointIds,weights);
      outputLines->InsertCellPoint(p);
      }
    outputLines->InsertCellPoint(p1);
    outCD->CopyData(inCD,cellId,newCellId);
    }
}

void vtkQuadraticEdge::InternalTesselate()
{
  vtkPoints *pts;
  vtkCellArray *lines;
  
  if ( this->Tesselation == NULL )
    {
    this->Tesselation = vtkPolyData::New();
    vtkPoints *pts = vtkPoints::New();
    this->Tesselation->SetPoints(pts);
    pts->Delete();
    vtkCellArray *lines = vtkCellArray::New();
    this->Tesselation->SetLines(lines);
    lines->Delete();

    this->InternalDataSet = vtkPolyData::New();
    }
  else
    {
    pts = this->Tesselation->GetPoints();
    pts->Reset();
    lines = this->Tesselation->GetLines();
    lines->Reset();
    }
}
