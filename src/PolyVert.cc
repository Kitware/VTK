/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PolyVert.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "PolyVert.hh"
#include "vtkMath.hh"
#include "CellArr.hh"
#include "Vertex.hh"

static vtkMath math;

// Description:
// Deep copy of cell.
vtkPolyVertex::vtkPolyVertex(const vtkPolyVertex& pp)
{
  this->Points = pp.Points;
  this->PointIds = pp.PointIds;
}

int vtkPolyVertex::EvaluatePosition(float x[3], float closestPoint[3],
                                   int& subId, float pcoords[3], 
                                   float& minDist2, float weights[MAX_CELL_SIZE])
{
  int numPts=this->Points.GetNumberOfPoints();
  float *X;
  float dist2;
  int i;

  for (minDist2=LARGE_FLOAT, i=0; i<numPts; i++)
    {
    X = this->Points.GetPoint(i);
    dist2 = math.Distance2BetweenPoints(X,x);
    if (dist2 < minDist2)
      {
      closestPoint[0] = X[0]; closestPoint[1] = X[1]; closestPoint[2] = X[2];
      minDist2 = dist2;
      subId = i;
      }
    }

  for (i=0; i<numPts; i++) weights[i] = 0.0;
  weights[subId] = 1.0;

  if (minDist2 == 0.0)
    {
    return 1;
    pcoords[0] = 0.0;
    }
  else
    {
    return 0;
    pcoords[0] = -10.0;
    }

}

void vtkPolyVertex::EvaluateLocation(int& subId, float pcoords[3], 
                                    float x[3], float weights[MAX_CELL_SIZE])
{
  int i;
  float *X = this->Points.GetPoint(subId);
  x[0] = X[0];
  x[1] = X[1];
  x[2] = X[2];

  for (i=0; i<this->GetNumberOfPoints(); i++) weights[i] = 0.0;
  weights[subId] = 1.0;
}

int vtkPolyVertex::CellBoundary(int subId, float pcoords[3], vtkIdList& pts)
{
  pts.Reset();
  pts.SetId(subId,this->PointIds.GetId(subId));

  if ( pcoords[0] != 0.0 )  
    return 0;
  else
    return 1;

}

void vtkPolyVertex::Contour(float value, vtkFloatScalars *cellScalars, 
                           vtkFloatPoints *points, vtkCellArray *verts,
                           vtkCellArray *lines, vtkCellArray *polys, 
                           vtkFloatScalars *scalars)
{
  int i, pts[1];

  for (i=0; i<this->Points.GetNumberOfPoints(); i++)
    {
    if ( value == cellScalars->GetScalar(i) )
      {
      scalars->InsertNextScalar(value);
      pts[0] = points->InsertNextPoint(this->Points.GetPoint(0));
      verts->InsertNextCell(1,pts);
      }
    }
}

//
// Intersect with sub-vertices
//
int vtkPolyVertex::IntersectWithLine(float p1[3], float p2[3], 
                                    float tol, float& t, float x[3], 
                                    float pcoords[3], int& subId)
{
  static vtkVertex vertex;

  for (subId=0; subId<this->Points.GetNumberOfPoints(); subId++)
    {
    vertex.Points.SetPoint(0,this->Points.GetPoint(subId));

    if ( vertex.IntersectWithLine(p1, p2, tol, t, x, pcoords, subId) )
      return 1;
    }

  return 0;
}

int vtkPolyVertex::Triangulate(int index, vtkFloatPoints &pts)
{
  int subId;

  pts.Reset();
  for (subId=0; subId<this->Points.GetNumberOfPoints(); subId++)
    {
    pts.InsertPoint(subId,this->Points.GetPoint(subId));
    }
  return 1;
}

void vtkPolyVertex::Derivatives(int subId, float pcoords[3], float *values, 
                            int dim, float *derivs)
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

