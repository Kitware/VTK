/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TriStrip.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "TriStrip.hh"
#include "Triangle.hh"
#include "CellArr.hh"
#include "Line.hh"

//
// Static minimizes constructor / destructor calls
//
static vtkTriangle tri;

// Description:
// Deep copy of cell.
vtkTriangleStrip::vtkTriangleStrip(const vtkTriangleStrip& ts)
{
  this->Points = ts.Points;
  this->PointIds = ts.PointIds;
}

int vtkTriangleStrip::EvaluatePosition(float x[3], float closestPoint[3],
                                      int& subId, float pcoords[3], 
                                      float& minDist2, float weights[MAX_CELL_SIZE])
{
  float pc[3], dist2;
  int ignoreId, i, return_status, status;
  float tempWeights[3], activeWeights[3];
  float closest[3];

  pcoords[2] = 0.0;

  return_status = 0;
  for (minDist2=LARGE_FLOAT,i=0; i<this->Points.GetNumberOfPoints()-2; i++)
    {
    weights[i] = 0.0;
    tri.Points.SetPoint(0,this->Points.GetPoint(i));
    tri.Points.SetPoint(1,this->Points.GetPoint(i+1));
    tri.Points.SetPoint(2,this->Points.GetPoint(i+2));
    status = tri.EvaluatePosition(x,closest,ignoreId,pc,dist2,tempWeights);
    if ( dist2 < minDist2 )
      {
      return_status = status;
      closestPoint[0] = closest[0]; closestPoint[1] = closest[1]; closestPoint[2] = closest[2];
      subId = i;
      pcoords[0] = pc[0];
      pcoords[1] = pc[1];
      minDist2 = dist2;
      activeWeights[0] = tempWeights[0];
      activeWeights[1] = tempWeights[1];
      activeWeights[2] = tempWeights[2];
      }
    }

  weights[i] = 0.0;  
  weights[i+1] = 0.0;  

  weights[subId] = activeWeights[0];
  weights[subId+1] = activeWeights[1];
  weights[subId+2] = activeWeights[2];

  return return_status;
}

void vtkTriangleStrip::EvaluateLocation(int& subId, float pcoords[3], float x[3],
                                       float weights[MAX_CELL_SIZE])
{
  int i;
  float *pt1 = this->Points.GetPoint(subId);
  float *pt2 = this->Points.GetPoint(subId+1);
  float *pt3 = this->Points.GetPoint(subId+2);
  float u3 = 1.0 - pcoords[0] - pcoords[1];

  for (i=0; i<3; i++)
    {
    x[i] = pt1[i]*pcoords[0] + pt2[i]*pcoords[1] + pt3[i]*u3;
    }

  weights[0] = u3;
  weights[1] = pcoords[0];
  weights[2] = pcoords[1];
}

int vtkTriangleStrip::CellBoundary(int subId, float pcoords[3], vtkIdList& pts)
{
  return 0;
}

void vtkTriangleStrip::Contour(float value, vtkFloatScalars *cellScalars, 
                              vtkFloatPoints *points, vtkCellArray *verts, 
                              vtkCellArray *lines, vtkCellArray *polys, 
                              vtkFloatScalars *scalars)
{
  int i;
  vtkFloatScalars triScalars(3);

  for ( i=0; i<this->Points.GetNumberOfPoints()-2; i++)
    {
    tri.Points.SetPoint(0,this->Points.GetPoint(i));
    tri.Points.SetPoint(1,this->Points.GetPoint(i+1));
    tri.Points.SetPoint(2,this->Points.GetPoint(i+2));

    triScalars.SetScalar(0,cellScalars->GetScalar(i));
    triScalars.SetScalar(1,cellScalars->GetScalar(i+1));
    triScalars.SetScalar(2,cellScalars->GetScalar(i+2));

    tri.Contour(value, &triScalars, points, verts,
                 lines, polys, scalars);
    }
}


vtkCell *vtkTriangleStrip::GetEdge(int edgeId)
{
  static vtkLine line;
  int id1, id2;

  if ( edgeId == 0 )
    {
    id1 = 0;
    id2 = 1;
    }
  else if ( edgeId == (this->GetNumberOfPoints()-1) )
    {
    id1 = edgeId - 1;
    id2 = edgeId;
    }
  else
    {
    id1 = edgeId - 1;
    id2 = edgeId + 1;
    }

  line.PointIds.SetId(0,this->PointIds.GetId(id1));
  line.PointIds.SetId(1,this->PointIds.GetId(id2));
  line.Points.SetPoint(0,this->Points.GetPoint(id1));
  line.Points.SetPoint(1,this->Points.GetPoint(id2));

  return &line;
}

// 
// Intersect sub-triangles
//
int vtkTriangleStrip::IntersectWithLine(float p1[3], float p2[3], float tol,
                                       float& t, float x[3], float pcoords[3],
                                       int& subId)
{
  for (subId=0; subId<this->Points.GetNumberOfPoints()-2; subId++)
    {
    tri.Points.SetPoint(0,this->Points.GetPoint(subId));
    tri.Points.SetPoint(1,this->Points.GetPoint(subId+1));
    tri.Points.SetPoint(2,this->Points.GetPoint(subId+2));

    if ( tri.IntersectWithLine(p1, p2, tol, t, x, pcoords, subId) )
      return 1;
    }

  return 0;
}
