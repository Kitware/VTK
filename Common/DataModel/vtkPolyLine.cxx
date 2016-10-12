/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyLine.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyLine.h"

#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkObjectFactory.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkLine.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <algorithm>

vtkStandardNewMacro(vtkPolyLine);

//----------------------------------------------------------------------------
vtkPolyLine::vtkPolyLine()
{
  this->Line = vtkLine::New();
}

//----------------------------------------------------------------------------
vtkPolyLine::~vtkPolyLine()
{
  this->Line->Delete();
}

//----------------------------------------------------------------------------
int vtkPolyLine::GenerateSlidingNormals(vtkPoints *pts, vtkCellArray *lines,
                                        vtkDataArray *normals)
{
  return vtkPolyLine::GenerateSlidingNormals(pts, lines, normals, 0);
}


inline vtkIdType FindNextValidSegment(vtkPoints *points, vtkIdList *pointIds,
                                      vtkIdType start)
{
  vtkVector3d ps;
  points->GetPoint(pointIds->GetId(start), ps.GetData());

  vtkIdType end = start + 1;
  while (end < pointIds->GetNumberOfIds())
  {
    vtkVector3d pe;
    points->GetPoint(pointIds->GetId(end), pe.GetData());
    if (ps != pe)
    {
      return end - 1;
    }
    ++end;
  }

  return pointIds->GetNumberOfIds();
}

//----------------------------------------------------------------------------
// Given points and lines, compute normals to lines. These are not true
// normals, they are "orientation" normals used by classes like vtkTubeFilter
// that control the rotation around the line. The normals try to stay pointing
// in the same direction as much as possible (i.e., minimal rotation) w.r.t the
// firstNormal (computed if NULL). Allways returns 1 (success).
int vtkPolyLine::GenerateSlidingNormals(vtkPoints *pts, vtkCellArray *lines,
                                        vtkDataArray *normals,
                                        double* firstNormal)
{
  vtkVector3d normal(0.0, 0.0, 1.0); // arbitrary default value

  vtkIdType lid = 0;
  vtkNew<vtkIdList> linePts;
  for (lines->InitTraversal(); lines->GetNextCell(linePts.GetPointer()); ++lid)
  {
    vtkIdType npts = linePts->GetNumberOfIds();
    if (npts <= 0)
    {
      continue;
    }
    if ( npts == 1 ) //return arbitrary
    {
      normals->InsertTuple(linePts->GetId(0), normal.GetData());
      continue;
    }


    vtkIdType sNextId = 0;
    vtkVector3d sPrev, sNext;

    sNextId = FindNextValidSegment(pts, linePts.GetPointer(), 0);
    if (sNextId != npts) // atleast one valid segment
    {
      vtkVector3d pt1, pt2;
      pts->GetPoint(linePts->GetId(sNextId), pt1.GetData());
      pts->GetPoint(linePts->GetId(sNextId + 1), pt2.GetData());
      sPrev = (pt2 - pt1).Normalized();
    }
    else // no valid segments
    {
      for (vtkIdType i = 0; i < npts; ++i)
      {
        normals->InsertTuple(linePts->GetId(i), normal.GetData());
      }
      continue;
    }

    // compute first normal
    if (firstNormal)
    {
      normal = vtkVector3d(firstNormal);
    }
    else
    {
      // find the next valid, non-parallel segment
      while (++sNextId < npts)
      {
        sNextId = FindNextValidSegment(pts, linePts.GetPointer(), sNextId);
        if (sNextId != npts)
        {
          vtkVector3d pt1, pt2;
          pts->GetPoint(linePts->GetId(sNextId), pt1.GetData());
          pts->GetPoint(linePts->GetId(sNextId + 1), pt2.GetData());
          sNext = (pt2 - pt1).Normalized();

          // now the starting normal should simply be the cross product
          // in the following if statement we check for the case where
          // the two segments are parallel, in which case, continue searching
          // for the next valid segment
          vtkVector3d n;
          n = sPrev.Cross(sNext);
          if (n.Norm() > 1.0E-3)
          {
            normal = n;
            sPrev = sNext;
            break;
          }
        }
      }

      if (sNextId >= npts) // only one valid segment
      {
        // a little trick to find othogonal normal
        for (int i = 0; i < 3; ++i)
        {
          if (sPrev[i] != 0.0)
          {
            normal[(i+2)%3] = 0.0;
            normal[(i+1)%3] = 1.0;
            normal[i] = -sPrev[(i+1)%3]/sPrev[i];
            break;
          }
        }
      }
    }
    normal.Normalize();

    // compute remaining normals
    vtkIdType lastNormalId = 0;
    while (++sNextId < npts)
    {
      sNextId = FindNextValidSegment(pts, linePts.GetPointer(), sNextId);
      if (sNextId == npts)
      {
        break;
      }

      vtkVector3d pt1, pt2;
      pts->GetPoint(linePts->GetId(sNextId), pt1.GetData());
      pts->GetPoint(linePts->GetId(sNextId + 1), pt2.GetData());
      sNext = (pt2- pt1).Normalized();

      //compute rotation vector
      vtkVector3d w = sPrev.Cross(normal);
      if (w.Normalize() == 0.0) // cant use this segment
      {
        continue;
      }

      //compute rotation of line segment
      vtkVector3d q = sNext.Cross(sPrev);
      if (q.Normalize() == 0.0) // cant use this segment
      {
        continue;
      }

      double f1 = q.Dot(normal);
      double f2 = 1.0 - (f1 * f1);
      if (f2 > 0.0)
      {
        f2 = sqrt(1.0 - (f1 * f1));
      }
      else
      {
        f2 = 0.0;
      }

      vtkVector3d c = (sNext + sPrev).Normalized();
      w = c.Cross(q);
      c = sPrev.Cross(q);
      if ((normal.Dot(c) * w.Dot(c)) < 0)
      {
        f2 = -1.0 * f2;
      }

      // insert current normal before updating
      for (vtkIdType i = lastNormalId; i < sNextId; ++i)
      {
        normals->InsertTuple(linePts->GetId(i), normal.GetData());
      }
      lastNormalId = sNextId;
      sPrev = sNext;

      // compute next normal
      normal = (f1 * q) + (f2 * w);
    }

    // insert last normal for the remaining points
    for (vtkIdType i = lastNormalId; i < npts; ++i)
    {
      normals->InsertTuple(linePts->GetId(i), normal.GetData());
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPolyLine::EvaluatePosition(double x[3], double* closestPoint,
                                 int& subId, double pcoords[3],
                                 double& minDist2, double *weights)
{
  double closest[3];
  double pc[3], dist2;
  int ignoreId, i, return_status, status;
  double lineWeights[2], closestWeights[2];

  pcoords[1] = pcoords[2] = 0.0;

  return_status = 0;
  subId = -1;
  closestWeights[0] = closestWeights[1] = 0.0;  // Shut up, compiler
  for (minDist2=VTK_DOUBLE_MAX,i=0; i<this->Points->GetNumberOfPoints()-1; i++)
  {
    this->Line->Points->SetPoint(0,this->Points->GetPoint(i));
    this->Line->Points->SetPoint(1,this->Points->GetPoint(i+1));
    status = this->Line->EvaluatePosition(x,closest,ignoreId,pc,
                                          dist2,lineWeights);
    if ( status != -1 && dist2 < minDist2 )
    {
      return_status = status;
      if (closestPoint)
      {
        closestPoint[0] = closest[0];
        closestPoint[1] = closest[1];
        closestPoint[2] = closest[2];
      }
      minDist2 = dist2;
      subId = i;
      pcoords[0] = pc[0];
      closestWeights[0] = lineWeights[0];
      closestWeights[1] = lineWeights[1];
    }
  }

  std::fill_n(weights, this->Points->GetNumberOfPoints(), 0.0);
  if (subId >= 0)
  {
    weights[subId] = closestWeights[0];
    weights[subId+1] = closestWeights[1];
  }

  return return_status;
}

//----------------------------------------------------------------------------
void vtkPolyLine::EvaluateLocation(int& subId, double pcoords[3], double x[3],
                                   double *weights)
{
  int i;
  double a1[3];
  double a2[3];
  this->Points->GetPoint(subId, a1);
  this->Points->GetPoint(subId+1, a2);

  for (i=0; i<3; i++)
  {
    x[i] = a1[i] + pcoords[0]*(a2[i] - a1[i]);
  }

  weights[0] = 1.0 - pcoords[0];
  weights[1] = pcoords[0];
}

//----------------------------------------------------------------------------
int vtkPolyLine::CellBoundary(int subId, double pcoords[3], vtkIdList *pts)
{
  pts->SetNumberOfIds(1);

  if ( pcoords[0] >= 0.5 )
  {
    pts->SetId(0,this->PointIds->GetId(subId+1));
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
    pts->SetId(0,this->PointIds->GetId(subId));
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

//----------------------------------------------------------------------------
void vtkPolyLine::Contour(double value, vtkDataArray *cellScalars,
                          vtkIncrementalPointLocator *locator, vtkCellArray *verts,
                          vtkCellArray *lines, vtkCellArray *polys,
                          vtkPointData *inPd, vtkPointData *outPd,
                          vtkCellData *inCd, vtkIdType cellId,
                          vtkCellData *outCd)
{
  int i, numLines=this->Points->GetNumberOfPoints() - 1;
  vtkDataArray *lineScalars=cellScalars->NewInstance();
  lineScalars->SetNumberOfComponents(cellScalars->GetNumberOfComponents());
  lineScalars->SetNumberOfTuples(2);

  for ( i=0; i < numLines; i++)
  {
    this->Line->Points->SetPoint(0,this->Points->GetPoint(i));
    this->Line->Points->SetPoint(1,this->Points->GetPoint(i+1));

    if ( outPd )
    {
      this->Line->PointIds->SetId(0,this->PointIds->GetId(i));
      this->Line->PointIds->SetId(1,this->PointIds->GetId(i+1));
    }

    lineScalars->SetTuple(0,cellScalars->GetTuple(i));
    lineScalars->SetTuple(1,cellScalars->GetTuple(i+1));

    this->Line->Contour(value, lineScalars, locator, verts,
                       lines, polys, inPd, outPd, inCd, cellId, outCd);
  }
  lineScalars->Delete();
}

//----------------------------------------------------------------------------
// Intersect with sub-lines
//
int vtkPolyLine::IntersectWithLine(double p1[3], double p2[3],double tol,double& t,
                                  double x[3], double pcoords[3], int& subId)
{
  int subTest, numLines=this->Points->GetNumberOfPoints() - 1;

  for (subId=0; subId < numLines; subId++)
  {
    this->Line->Points->SetPoint(0,this->Points->GetPoint(subId));
    this->Line->Points->SetPoint(1,this->Points->GetPoint(subId+1));

    if ( this->Line->IntersectWithLine(p1, p2, tol, t, x, pcoords, subTest) )
    {
      return 1;
    }
  }

  return 0;
}

//----------------------------------------------------------------------------
int vtkPolyLine::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds,
                             vtkPoints *pts)
{
  int numLines=this->Points->GetNumberOfPoints() - 1;
  pts->Reset();
  ptIds->Reset();

  for (int subId=0; subId < numLines; subId++)
  {
    pts->InsertNextPoint(this->Points->GetPoint(subId));
    ptIds->InsertNextId(this->PointIds->GetId(subId));

    pts->InsertNextPoint(this->Points->GetPoint(subId+1));
    ptIds->InsertNextId(this->PointIds->GetId(subId+1));
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPolyLine::Derivatives(int subId, double pcoords[3], double *values,
                              int dim, double *derivs)
{
  this->Line->PointIds->SetNumberOfIds(2);

  this->Line->Points->SetPoint(0,this->Points->GetPoint(subId));
  this->Line->Points->SetPoint(1,this->Points->GetPoint(subId+1));

  this->Line->Derivatives(0, pcoords, values+dim*subId, dim, derivs);
}

//----------------------------------------------------------------------------
void vtkPolyLine::Clip(double value, vtkDataArray *cellScalars,
                       vtkIncrementalPointLocator *locator, vtkCellArray *lines,
                       vtkPointData *inPd, vtkPointData *outPd,
                       vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
                       int insideOut)
{
  int i, numLines=this->Points->GetNumberOfPoints() - 1;
  vtkDoubleArray *lineScalars=vtkDoubleArray::New();
  lineScalars->SetNumberOfTuples(2);

  for ( i=0; i < numLines; i++)
  {
    this->Line->Points->SetPoint(0,this->Points->GetPoint(i));
    this->Line->Points->SetPoint(1,this->Points->GetPoint(i+1));

    this->Line->PointIds->SetId(0,this->PointIds->GetId(i));
    this->Line->PointIds->SetId(1,this->PointIds->GetId(i+1));

    lineScalars->SetComponent(0,0,cellScalars->GetComponent(i,0));
    lineScalars->SetComponent(1,0,cellScalars->GetComponent(i+1,0));

    this->Line->Clip(value, lineScalars, locator, lines, inPd, outPd,
                    inCd, cellId, outCd, insideOut);
  }

  lineScalars->Delete();
}

//----------------------------------------------------------------------------
// Return the center of the point cloud in parametric coordinates.
int vtkPolyLine::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = 0.5; pcoords[1] = pcoords[2] = 0.0;
  return ((this->Points->GetNumberOfPoints() - 1) / 2);
}

//----------------------------------------------------------------------------
void vtkPolyLine::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Line:\n";
  this->Line->PrintSelf(os,indent.GetNextIndent());
}

