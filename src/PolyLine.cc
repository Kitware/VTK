/*=========================================================================

  Program:   Visualization Library
  Module:    PolyLine.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "PolyLine.hh"
#include "vlMath.hh"
#include "Line.hh"
#include "CellArr.hh"

//
// eliminate constructor / destructor calls
//
static vlLine line;
static vlMath math;


// Description:
// Deep copy of cell.
vlPolyLine::vlPolyLine(const vlPolyLine& pl)
{
  this->Points = pl.Points;
  this->PointIds = pl.PointIds;
}

int vlPolyLine::GenerateNormals(vlPoints *pts, vlCellArray *lines, vlFloatNormals *normals)
{
  int npts, *linePts;
  float s[3], q[3], norm[3], *n;
  float *p1, *p2;
  float s_norm, n_norm, q_norm;
  int i, j;
//
//  Loop over all lines
// 
  for (lines->InitTraversal(); lines->GetNextCell(npts,linePts); )
    {
//
//  Determine initial starting normal
// 
    if ( npts < 2 ) return 0;

    p1 = pts->GetPoint(linePts[0]);
    p2 = pts->GetPoint(linePts[1]);
    for (i=0; i<3; i++) s[i] = p2[i] - p1[i];

    if ( (s_norm = math.Norm(s)) == 0.0 ) return 0;
    for (i=0; i<3; i++) s[i] /= s_norm;

    for (i=0; i<3; i++) 
      {
      if ( s[i] != 0.0 ) 
        {
        norm[(i+2)%3] = 0.0;
        norm[(i+1)%3] = 1.0;
        norm[i] = -s[(i+1)%3]/s[i];
        break;
        }
      }
    n_norm = math.Norm(norm);
    for (i=0; i<3; i++) norm[i] /= n_norm;
    normals->InsertNormal(0,norm);
//
//  Generate normals for new point by projecting previous normal
// 
    for (j=1; j<npts; j++) 
      {
      p1 = pts->GetPoint(linePts[j-1]);
      p2 = pts->GetPoint(linePts[j]);
      for (i=0; i<3; i++) s[i] = p2[i] - p1[i];

      if ( (s_norm = math.Norm(s)) == 0.0 ) return 0;
      for (i=0; i<3; i++) s[i] /= s_norm;

      n = normals->GetNormal(j-1);

      math.Cross(s,n,q);

      if ( (q_norm = math.Norm(q)) == 0.0 ) return 0;
      for (i=0; i<3; i++) q[i] /= q_norm;

      math.Cross (q, s, norm);
      n_norm = math.Norm(norm);
      for (i=0; i<3; i++) norm[i] /= n_norm;
      normals->InsertNormal(j,norm);
      }
    }
  return 1;
}

int vlPolyLine::EvaluatePosition(float x[3], float closestPoint[3],
                                 int& subId, float pcoords[3], 
                                 float& minDist2, float weights[MAX_CELL_SIZE])
{
  float closest[3];
  float pc[3], dist2;
  int ignoreId, i, return_status, status;
  float lineWeights[2];

  pcoords[1] = pcoords[2] = 0.0;

  return_status = 0;
  weights[0] = 0.0;
  for (minDist2=LARGE_FLOAT,i=0; i<this->Points.GetNumberOfPoints()-1; i++)
    {
    line.Points.SetPoint(0,this->Points.GetPoint(i));
    line.Points.SetPoint(1,this->Points.GetPoint(i+1));
    status = line.EvaluatePosition(x,closest,ignoreId,pc,dist2,lineWeights);
    if ( dist2 < minDist2 )
      {
      return_status = status;
      closestPoint[0] = closest[0]; closestPoint[1] = closest[1]; closestPoint[2] = closest[2]; 
      subId = i;
      pcoords[0] = pc[0];
      minDist2 = dist2;
      weights[i] = lineWeights[0];
      weights[i+1] = lineWeights[1];
      }
    else
      {
      weights[i+1] = 0.0;
      }
    }

  return return_status;
}

void vlPolyLine::EvaluateLocation(int& subId, float pcoords[3], float x[3],
                                  float weights[MAX_CELL_SIZE])
{
  int i;
  float *a1 = this->Points.GetPoint(subId);
  float *a2 = this->Points.GetPoint(subId+1);

  for (i=0; i<3; i++) 
    {
    x[i] = a1[i] + pcoords[0]*(a2[i] - a1[i]);
    }
}

void vlPolyLine::Contour(float value, vlFloatScalars *cellScalars,
                         vlFloatPoints *points, vlCellArray *verts, 
                         vlCellArray *lines, vlCellArray *polys, 
                         vlFloatScalars *scalars)
{
  int i;
  vlFloatScalars lineScalars(2);

  for ( i=0; i<this->Points.GetNumberOfPoints()-1; i++)
    {
    line.Points.SetPoint(0,this->Points.GetPoint(i));
    line.Points.SetPoint(1,this->Points.GetPoint(i+1));

    lineScalars.SetScalar(0,cellScalars->GetScalar(i));
    lineScalars.SetScalar(1,cellScalars->GetScalar(i+1));

    line.Contour(value, &lineScalars, points, verts,
                 lines, polys, scalars);
    }

}

//
// Intersect with sub-lines
//
int vlPolyLine::IntersectWithLine(float p1[3], float p2[3],float tol,float& t,
                                  float x[3], float pcoords[3], int& subId)
{
  for (subId=0; subId<this->Points.GetNumberOfPoints()-1; subId++)
    {
    line.Points.SetPoint(0,this->Points.GetPoint(subId));
    line.Points.SetPoint(1,this->Points.GetPoint(subId+1));

    if ( line.IntersectWithLine(p1, p2, tol, t, x, pcoords, subId) )
      return 1;
    }

  return 0;
}
