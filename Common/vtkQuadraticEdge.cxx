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
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkQuadraticEdge, "1.1");
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
  
  this->Tessellation = NULL;
}

vtkQuadraticEdge::~vtkQuadraticEdge()
{
  if ( this->Tessellation )
    {
    this->Tessellation->Delete();
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

}

void vtkQuadraticEdge::InternalTessellate()
{
  vtkPoints *pts;
  vtkCellArray *lines;
  
  if ( this->Tessellation == NULL )
    {
    this->Tessellation = vtkPolyData::New();
    vtkPoints *pts = vtkPoints::New();
    this->Tessellation->SetPoints(pts);
    pts->Delete();
    vtkCellArray *lines = vtkCellArray::New();
    this->Tessellation->SetLines(lines);
    lines->Delete();
    }
  else
    {
    pts = this->Tessellation->GetPoints();
    pts->Reset();
    lines = this->Tessellation->GetLines();
    lines->Reset();
    }
  
#if 0
  int i, subId;
  float pcoords[3], x[3], weights[3], incr;
  
  // Determine the number of points to generate
  // Use a course approximation based on circle approximation.
  int numPts = 2;
  for (i=0; i<this->Resolution; i++)
    {
    numPts *= 2;
    }
  
  // Create the points
  incr = 1.0 / (float)numPts;
  this->LinearPoints->SetNumberOfPoints(numPts+1);
  this->LinearPoints->SetPoint(0,this->Points->GetPoint(0));
  this->LinearCells->Reset();
  this->LinearCells->InsertNextCell(numPts+1);
  this->LinearCells->InsertCellPoint(0);

  for (i=1; i<numPts; i++)
    {
    pcoords[0] = i*incr;
    this->EvaluateLocation(subId,pcoords,x,weights);
    this->LinearPoints->SetPoint(i,x);
    this->LinearCells->InsertCellPoint(i);
    }

  this->LinearPoints->SetPoint(0,this->Points->GetPoint(1));
  this->LinearCells->InsertCellPoint(numPts);
#endif
}
