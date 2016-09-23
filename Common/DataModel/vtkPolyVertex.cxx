/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyVertex.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyVertex.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkPoints.h"
#include "vtkVertex.h"

vtkStandardNewMacro(vtkPolyVertex);

//----------------------------------------------------------------------------
vtkPolyVertex::vtkPolyVertex()
{
  this->Vertex = vtkVertex::New();
}

//----------------------------------------------------------------------------
vtkPolyVertex::~vtkPolyVertex()
{
  this->Vertex->Delete();
}

//----------------------------------------------------------------------------
int vtkPolyVertex::EvaluatePosition(double x[3], double* closestPoint,
                                   int& subId, double pcoords[3],
                                   double& minDist2, double *weights)
{
  int numPts=this->Points->GetNumberOfPoints();
  double X[3];
  double dist2;
  int i;
  pcoords[1] = pcoords[2] = -1.0;

  for (minDist2=VTK_DOUBLE_MAX, i=0; i<numPts; i++)
  {
    this->Points->GetPoint(i, X);
    dist2 = vtkMath::Distance2BetweenPoints(X,x);
    if (dist2 < minDist2)
    {
      if (closestPoint)
      {
        closestPoint[0] = X[0]; closestPoint[1] = X[1]; closestPoint[2] = X[2];
      }
      minDist2 = dist2;
      subId = i;
    }
  }

  for (i=0; i<numPts; i++)
  {
    weights[i] = 0.0;
  }
  weights[subId] = 1.0;

  if (minDist2 == 0.0)
  {
    pcoords[0] = 0.0;
    return 1;
  }
  else
  {
    pcoords[0] = -1.0;
    return 0;
  }
}

//----------------------------------------------------------------------------
void vtkPolyVertex::EvaluateLocation(int& subId,
                                     double vtkNotUsed(pcoords)[3],
                                     double x[3], double *weights)
{
  int i;
  this->Points->GetPoint(subId, x);

  for (i=0; i<this->GetNumberOfPoints(); i++)
  {
    weights[i] = 0.0;
  }
  weights[subId] = 1.0;
}

//----------------------------------------------------------------------------
int vtkPolyVertex::CellBoundary(int subId, double pcoords[3], vtkIdList *pts)
{
  pts->SetNumberOfIds(1);
  pts->SetId(0,this->PointIds->GetId(subId));

  if ( pcoords[0] != 0.0 )
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

//----------------------------------------------------------------------------
void vtkPolyVertex::Contour(double value, vtkDataArray *cellScalars,
                            vtkIncrementalPointLocator *locator, vtkCellArray *verts,
                            vtkCellArray *vtkNotUsed(lines),
                            vtkCellArray *vtkNotUsed(polys),
                            vtkPointData *inPd, vtkPointData *outPd,
                            vtkCellData *inCd, vtkIdType cellId,
                            vtkCellData *outCd)
{
  int i, numPts=this->Points->GetNumberOfPoints(), newCellId;
  vtkIdType pts[1];

  for (i=0; i < numPts; i++)
  {
    if ( value == cellScalars->GetComponent(i,0) )
    {
      pts[0] = locator->InsertNextPoint(this->Points->GetPoint(i));
      if ( outPd )
      {
        outPd->CopyData(inPd,this->PointIds->GetId(i),pts[0]);
      }
      newCellId = verts->InsertNextCell(1,pts);
      outCd->CopyData(inCd,cellId,newCellId);
    }
  }
}

//----------------------------------------------------------------------------
//
// Intersect with sub-vertices
//
int vtkPolyVertex::IntersectWithLine(double p1[3], double p2[3],
                                    double tol, double& t, double x[3],
                                    double pcoords[3], int& subId)
{
  int subTest, numPts=this->Points->GetNumberOfPoints();

  for (subId=0; subId < numPts; subId++)
  {
    this->Vertex->Points->SetPoint(0,this->Points->GetPoint(subId));

    if ( this->Vertex->IntersectWithLine(p1, p2, tol, t, x, pcoords, subTest) )
    {
      return 1;
    }
  }

  return 0;
}

//----------------------------------------------------------------------------
int vtkPolyVertex::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds,
                               vtkPoints *pts)
{
  int subId;

  pts->Reset();
  ptIds->Reset();
  for (subId=0; subId < this->Points->GetNumberOfPoints(); subId++)
  {
    pts->InsertPoint(subId,this->Points->GetPoint(subId));
    ptIds->InsertId(subId,this->PointIds->GetId(subId));
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPolyVertex::Derivatives(int vtkNotUsed(subId),
                                double vtkNotUsed(pcoords)[3],
                                double *vtkNotUsed(values),
                                int dim, double *derivs)
{
  int i, idx;

  for (i=0; i<dim; i++)
  {
    idx = i*dim;
    derivs[idx] = 0.0;
    derivs[idx+1] = 0.0;
    derivs[idx+2] = 0.0;
  }
}

//----------------------------------------------------------------------------
void vtkPolyVertex::Clip(double value, vtkDataArray *cellScalars,
                         vtkIncrementalPointLocator *locator, vtkCellArray *verts,
                         vtkPointData *inPd, vtkPointData *outPd,
                         vtkCellData *inCd, vtkIdType cellId,
                         vtkCellData *outCd, int insideOut)
{
  double s, x[3];
  int i, newCellId, numPts=this->Points->GetNumberOfPoints();
  vtkIdType pts[1];

  for ( i=0; i < numPts; i++ )
  {
    s = cellScalars->GetComponent(i, 0);

    if ( (!insideOut && s > value) || (insideOut && s <= value) )
    {
      this->Points->GetPoint(i,x);
      if ( locator->InsertUniquePoint(x, pts[0]) )
      {
        outPd->CopyData(inPd,this->PointIds->GetId(i),pts[0]);
      }
      newCellId = verts->InsertNextCell(1,pts);
      outCd->CopyData(inCd,cellId,newCellId);
    }
  }
}

//----------------------------------------------------------------------------
// Return the center of the point cloud in parametric coordinates.
int vtkPolyVertex::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = pcoords[2] = 0.5;
  return (this->Points->GetNumberOfPoints() / 2);
}

//----------------------------------------------------------------------------
void vtkPolyVertex::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Vertex:\n";
  this->Vertex->PrintSelf(os,indent.GetNextIndent());
}
