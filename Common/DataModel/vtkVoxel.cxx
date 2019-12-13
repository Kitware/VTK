/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoxel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVoxel.h"

#include "vtkBox.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkLine.h"
#include "vtkMarchingCubesTriangleCases.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPixel.h"
#include "vtkPointData.h"
#include "vtkPoints.h"

#include <cassert>
#ifndef VTK_LEGACY_REMOVE // needed temporarily in deprecated methods
#include <vector>
#endif

vtkStandardNewMacro(vtkVoxel);

//----------------------------------------------------------------------------
// Construct the voxel with eight points.
vtkVoxel::vtkVoxel()
{
  int i;

  this->Points->SetNumberOfPoints(8);
  this->PointIds->SetNumberOfIds(8);
  for (i = 0; i < 8; i++)
  {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
  }
  for (i = 0; i < 8; i++)
  {
    this->PointIds->SetId(i, 0);
  }
  this->Line = nullptr;
  this->Pixel = nullptr;
}

//----------------------------------------------------------------------------
vtkVoxel::~vtkVoxel()
{
  if (this->Line)
  {
    this->Line->Delete();
  }
  if (this->Pixel)
  {
    this->Pixel->Delete();
  }
}

//----------------------------------------------------------------------------
bool vtkVoxel::GetCentroid(double centroid[3]) const
{
  return vtkVoxel::ComputeCentroid(this->Points, nullptr, centroid);
}

//----------------------------------------------------------------------------
bool vtkVoxel::ComputeCentroid(vtkPoints* points, const vtkIdType* pointIds, double centroid[3])
{
  double p[3];
  if (pointIds)
  {
    points->GetPoint(pointIds[0], centroid);
    points->GetPoint(pointIds[7], p);
  }
  else
  {
    points->GetPoint(0, centroid);
    points->GetPoint(7, p);
  }
  centroid[0] += p[0];
  centroid[1] += p[1];
  centroid[2] += p[2];
  centroid[0] *= 0.5;
  centroid[1] *= 0.5;
  centroid[2] *= 0.5;
  return true;
}

//----------------------------------------------------------------------------
bool vtkVoxel::IsInsideOut()
{
  double pt1[3], pt2[3];
  this->Points->GetPoint(0, pt1);
  this->Points->GetPoint(7, pt2);
  return (pt2[0] - pt1[0]) * (pt2[1] - pt1[1]) * (pt2[2] - pt1[2]) < 0.0;
}

//----------------------------------------------------------------------------
int vtkVoxel::EvaluatePosition(const double x[3], double closestPoint[3], int& subId,
  double pcoords[3], double& dist2, double weights[])
{
  double pt1[3], pt2[3], pt3[3], pt4[3];
  int i;

  subId = 0;
  //
  // Get coordinate system
  //
  this->Points->GetPoint(0, pt1);
  this->Points->GetPoint(1, pt2);
  this->Points->GetPoint(2, pt3);
  this->Points->GetPoint(4, pt4);
  //
  // Develop parametric coordinates
  //
  pcoords[0] = (x[0] - pt1[0]) / (pt2[0] - pt1[0]);
  pcoords[1] = (x[1] - pt1[1]) / (pt3[1] - pt1[1]);
  pcoords[2] = (x[2] - pt1[2]) / (pt4[2] - pt1[2]);

  if (pcoords[0] >= 0.0 && pcoords[0] <= 1.0 && pcoords[1] >= 0.0 && pcoords[1] <= 1.0 &&
    pcoords[2] >= 0.0 && pcoords[2] <= 1.0)
  {
    if (closestPoint)
    {
      closestPoint[0] = x[0];
      closestPoint[1] = x[1];
      closestPoint[2] = x[2];
    }
    dist2 = 0.0; // inside voxel
    this->InterpolationFunctions(pcoords, weights);
    return 1;
  }
  else
  {
    double pc[3], w[8];
    if (closestPoint)
    {
      for (i = 0; i < 3; i++)
      {
        if (pcoords[i] < 0.0)
        {
          pc[i] = 0.0;
        }
        else if (pcoords[i] > 1.0)
        {
          pc[i] = 1.0;
        }
        else
        {
          pc[i] = pcoords[i];
        }
      }
      this->EvaluateLocation(subId, pc, closestPoint, static_cast<double*>(w));
      dist2 = vtkMath::Distance2BetweenPoints(closestPoint, x);
    }
    return 0;
  }
}

//----------------------------------------------------------------------------
void vtkVoxel::EvaluateLocation(
  int& vtkNotUsed(subId), const double pcoords[3], double x[3], double* weights)
{
  double pt1[3], pt2[3], pt3[3], pt4[3];
  int i;

  this->Points->GetPoint(0, pt1);
  this->Points->GetPoint(1, pt2);
  this->Points->GetPoint(2, pt3);
  this->Points->GetPoint(4, pt4);

  for (i = 0; i < 3; i++)
  {
    x[i] = pt1[i] + pcoords[0] * (pt2[i] - pt1[i]) + pcoords[1] * (pt3[i] - pt1[i]) +
      pcoords[2] * (pt4[i] - pt1[i]);
  }

  this->InterpolationFunctions(pcoords, weights);
}

//----------------------------------------------------------------------------
//
// Compute Interpolation functions
//
void vtkVoxel::InterpolationFunctions(const double pcoords[3], double sf[8])
{
  double rm, sm, tm;

  double r = pcoords[0], s = pcoords[1], t = pcoords[2];

  rm = 1. - r;
  sm = 1. - s;
  tm = 1. - t;

  sf[0] = rm * sm * tm;
  sf[1] = r * sm * tm;
  sf[2] = rm * s * tm;
  sf[3] = r * s * tm;
  sf[4] = rm * sm * t;
  sf[5] = r * sm * t;
  sf[6] = rm * s * t;
  sf[7] = r * s * t;
}

//----------------------------------------------------------------------------
void vtkVoxel::InterpolationDerivs(const double pcoords[3], double derivs[24])
{
  double rm, sm, tm;

  rm = 1. - pcoords[0];
  sm = 1. - pcoords[1];
  tm = 1. - pcoords[2];

  // r derivatives
  derivs[0] = -sm * tm;
  derivs[1] = sm * tm;
  derivs[2] = -pcoords[1] * tm;
  derivs[3] = pcoords[1] * tm;
  derivs[4] = -sm * pcoords[2];
  derivs[5] = sm * pcoords[2];
  derivs[6] = -pcoords[1] * pcoords[2];
  derivs[7] = pcoords[1] * pcoords[2];

  // s derivatives
  derivs[8] = -rm * tm;
  derivs[9] = -pcoords[0] * tm;
  derivs[10] = rm * tm;
  derivs[11] = pcoords[0] * tm;
  derivs[12] = -rm * pcoords[2];
  derivs[13] = -pcoords[0] * pcoords[2];
  derivs[14] = rm * pcoords[2];
  derivs[15] = pcoords[0] * pcoords[2];

  // t derivatives
  derivs[16] = -rm * sm;
  derivs[17] = -pcoords[0] * sm;
  derivs[18] = -rm * pcoords[1];
  derivs[19] = -pcoords[0] * pcoords[1];
  derivs[20] = rm * sm;
  derivs[21] = pcoords[0] * sm;
  derivs[22] = rm * pcoords[1];
  derivs[23] = pcoords[0] * pcoords[1];
}

//----------------------------------------------------------------------------
int vtkVoxel::CellBoundary(int vtkNotUsed(subId), const double pcoords[3], vtkIdList* pts)
{
  double t1 = pcoords[0] - pcoords[1];
  double t2 = 1.0 - pcoords[0] - pcoords[1];
  double t3 = pcoords[1] - pcoords[2];
  double t4 = 1.0 - pcoords[1] - pcoords[2];
  double t5 = pcoords[2] - pcoords[0];
  double t6 = 1.0 - pcoords[2] - pcoords[0];

  pts->SetNumberOfIds(4);

  // compare against six planes in parametric space that divide element
  // into six pieces.
  if (t3 >= 0.0 && t4 >= 0.0 && t5 < 0.0 && t6 >= 0.0)
  {
    pts->SetId(0, this->PointIds->GetId(0));
    pts->SetId(1, this->PointIds->GetId(1));
    pts->SetId(2, this->PointIds->GetId(3));
    pts->SetId(3, this->PointIds->GetId(2));
  }

  else if (t1 >= 0.0 && t2 < 0.0 && t5 < 0.0 && t6 < 0.0)
  {
    pts->SetId(0, this->PointIds->GetId(1));
    pts->SetId(1, this->PointIds->GetId(3));
    pts->SetId(2, this->PointIds->GetId(7));
    pts->SetId(3, this->PointIds->GetId(5));
  }

  else if (t1 >= 0.0 && t2 >= 0.0 && t3 < 0.0 && t4 >= 0.0)
  {
    pts->SetId(0, this->PointIds->GetId(0));
    pts->SetId(1, this->PointIds->GetId(1));
    pts->SetId(2, this->PointIds->GetId(5));
    pts->SetId(3, this->PointIds->GetId(4));
  }

  else if (t3 < 0.0 && t4 < 0.0 && t5 >= 0.0 && t6 < 0.0)
  {
    pts->SetId(0, this->PointIds->GetId(4));
    pts->SetId(1, this->PointIds->GetId(5));
    pts->SetId(2, this->PointIds->GetId(7));
    pts->SetId(3, this->PointIds->GetId(6));
  }

  else if (t1 < 0.0 && t2 >= 0.0 && t5 >= 0.0 && t6 >= 0.0)
  {
    pts->SetId(0, this->PointIds->GetId(0));
    pts->SetId(1, this->PointIds->GetId(4));
    pts->SetId(2, this->PointIds->GetId(6));
    pts->SetId(3, this->PointIds->GetId(2));
  }

  else // if ( t1 < 0.0 && t2 < 0.0 && t3 >= 0.0 && t6 < 0.0 )
  {
    pts->SetId(0, this->PointIds->GetId(3));
    pts->SetId(1, this->PointIds->GetId(2));
    pts->SetId(2, this->PointIds->GetId(6));
    pts->SetId(3, this->PointIds->GetId(7));
  }

  if (pcoords[0] < 0.0 || pcoords[0] > 1.0 || pcoords[1] < 0.0 || pcoords[1] > 1.0 ||
    pcoords[2] < 0.0 || pcoords[2] > 1.0)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

namespace
{
//----------------------------------------------------------------------------
// Voxel topology
//
//  2_______3
//  |\     /|
//  |6\___/7|
//  | |   | |
//  | |___| |
//  |4/   \5|
//  |/_____\|
//  0       1
//

static constexpr vtkIdType edges[vtkVoxel::NumberOfEdges][2] = {
  { 0, 1 }, // 0
  { 1, 3 }, // 1
  { 2, 3 }, // 2
  { 0, 2 }, // 3
  { 4, 5 }, // 4
  { 5, 7 }, // 5
  { 6, 7 }, // 6
  { 4, 6 }, // 7
  { 0, 4 }, // 8
  { 1, 5 }, // 9
  { 2, 6 }, // 10
  { 3, 7 }, // 11
};
// define in terms vtkPixel understands
static constexpr vtkIdType faces[vtkVoxel::NumberOfFaces][vtkVoxel::MaximumFaceSize + 1] = {
  { 2, 0, 6, 4, -1 }, // 0
  { 1, 3, 5, 7, -1 }, // 1
  { 0, 1, 4, 5, -1 }, // 2
  { 3, 2, 7, 6, -1 }, // 3
  { 1, 0, 3, 2, -1 }, // 4
  { 4, 5, 6, 7, -1 }, // 5
};
static constexpr vtkIdType edgeToAdjacentFaces[vtkVoxel::NumberOfEdges][2] = {
  { 2, 4 }, // 0
  { 1, 4 }, // 1
  { 3, 4 }, // 2
  { 0, 4 }, // 3
  { 2, 5 }, // 4
  { 1, 5 }, // 5
  { 3, 5 }, // 6
  { 0, 5 }, // 7
  { 0, 2 }, // 8
  { 1, 2 }, // 9
  { 0, 3 }, // 10
  { 1, 3 }, // 11
};
static constexpr vtkIdType
  faceToAdjacentFaces[vtkVoxel::NumberOfFaces][vtkVoxel::MaximumFaceSize] = {
    { 5, 3, 4, 2 }, // 0
    { 4, 3, 5, 2 }, // 1
    { 4, 1, 5, 0 }, // 2
    { 4, 0, 5, 1 }, // 3
    { 2, 0, 3, 1 }, // 4
    { 2, 1, 3, 0 }, // 5
  };
static constexpr vtkIdType
  pointToIncidentEdges[vtkVoxel::NumberOfPoints][vtkVoxel::MaximumValence] = {
    { 0, 8, 3 },  // 0
    { 0, 1, 9 },  // 1
    { 2, 3, 10 }, // 2
    { 1, 2, 11 }, // 3
    { 4, 7, 8 },  // 4
    { 4, 9, 5 },  // 5
    { 6, 10, 7 }, // 6
    { 5, 11, 6 }, // 7
  };
static constexpr vtkIdType
  pointToIncidentFaces[vtkVoxel::NumberOfPoints][vtkVoxel::MaximumValence] = {
    { 2, 0, 4 }, // 0
    { 4, 1, 2 }, // 1
    { 4, 0, 3 }, // 2
    { 4, 3, 1 }, // 3
    { 5, 0, 2 }, // 4
    { 2, 1, 5 }, // 5
    { 3, 0, 5 }, // 6
    { 1, 3, 5 }, // 7
  };
static constexpr vtkIdType
  pointToOneRingPoints[vtkVoxel::NumberOfPoints][vtkVoxel::MaximumValence] = {
    { 1, 4, 2 }, // 0
    { 0, 3, 5 }, // 1
    { 3, 0, 6 }, // 2
    { 1, 2, 7 }, // 3
    { 5, 6, 0 }, // 4
    { 4, 1, 7 }, // 5
    { 7, 2, 4 }, // 6
    { 5, 3, 6 }, // 7
  };
}

//----------------------------------------------------------------------------
//
// Marching cubes case table
//
#include "vtkMarchingCubesTriangleCases.h"

void vtkVoxel::Contour(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
  vtkCellArray* verts, vtkCellArray* lines, vtkCellArray* polys, vtkPointData* inPd,
  vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd)
{
  static const int CASE_MASK[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };
  vtkMarchingCubesTriangleCases* triCase;
  EDGE_LIST* edge;
  int i, j, index;
  const vtkIdType* vert;
  static const int vertMap[8] = { 0, 1, 3, 2, 4, 5, 7, 6 };
  int newCellId;
  vtkIdType pts[3];
  double t, x1[3], x2[3], x[3];
  vtkIdType offset = verts->GetNumberOfCells() + lines->GetNumberOfCells();

  // Build the case table
  for (i = 0, index = 0; i < 8; i++)
  {
    if (cellScalars->GetComponent(vertMap[i], 0) >= value)
    {
      index |= CASE_MASK[i];
    }
  }

  triCase = vtkMarchingCubesTriangleCases::GetCases() + index;
  edge = triCase->edges;

  for (; edge[0] > -1; edge += 3)
  {
    for (i = 0; i < 3; i++) // insert triangle
    {
      vert = edges[edge[i]];
      t = (value - cellScalars->GetComponent(vert[0], 0)) /
        (cellScalars->GetComponent(vert[1], 0) - cellScalars->GetComponent(vert[0], 0));
      this->Points->GetPoint(vert[0], x1);
      this->Points->GetPoint(vert[1], x2);
      for (j = 0; j < 3; j++)
      {
        x[j] = x1[j] + t * (x2[j] - x1[j]);
      }
      if (locator->InsertUniquePoint(x, pts[i]))
      {
        if (outPd)
        {
          int p1 = this->PointIds->GetId(vert[0]);
          int p2 = this->PointIds->GetId(vert[1]);
          outPd->InterpolateEdge(inPd, pts[i], p1, p2, t);
        }
      }
    }
    // check for degenerate triangle
    if (pts[0] != pts[1] && pts[0] != pts[2] && pts[1] != pts[2])
    {
      newCellId = offset + polys->InsertNextCell(3, pts);
      if (outCd)
      {
        outCd->CopyData(inCd, cellId, newCellId);
      }
    }
  }
}

//----------------------------------------------------------------------------
const vtkIdType* vtkVoxel::GetEdgeArray(vtkIdType edgeId)
{
  return edges[edgeId];
}

//----------------------------------------------------------------------------
// Return the case table for table-based isocontouring (aka marching cubes
// style implementations). A linear 3D cell with N vertices will have 2**N
// cases. The cases list three edges in order to produce one output triangle.
int* vtkVoxel::GetTriangleCases(int caseId)
{
  return &(*(vtkMarchingCubesTriangleCases::GetCases() + caseId)->edges);
}

//----------------------------------------------------------------------------
vtkCell* vtkVoxel::GetEdge(int edgeId)
{
  if (!this->Line)
  {
    this->Line = vtkLine::New();
  }

  const vtkIdType* verts;

  verts = edges[edgeId];

  // load point id's
  this->Line->PointIds->SetId(0, this->PointIds->GetId(verts[0]));
  this->Line->PointIds->SetId(1, this->PointIds->GetId(verts[1]));

  // load coordinates
  this->Line->Points->SetPoint(0, this->Points->GetPoint(verts[0]));
  this->Line->Points->SetPoint(1, this->Points->GetPoint(verts[1]));

  return this->Line;
}

//----------------------------------------------------------------------------
const vtkIdType* vtkVoxel::GetFaceArray(vtkIdType faceId)
{
  return faces[faceId];
}

//----------------------------------------------------------------------------
vtkCell* vtkVoxel::GetFace(int faceId)
{
  if (!this->Pixel)
  {
    this->Pixel = vtkPixel::New();
  }

  const vtkIdType* verts;
  int i;

  verts = faces[faceId];

  for (i = 0; i < 4; i++)
  {
    this->Pixel->PointIds->SetId(i, this->PointIds->GetId(verts[i]));
    this->Pixel->Points->SetPoint(i, this->Points->GetPoint(verts[i]));
  }

  return this->Pixel;
}

//----------------------------------------------------------------------------
//
// Intersect voxel with line using "bounding box" intersection.
//
int vtkVoxel::IntersectWithLine(const double p1[3], const double p2[3], double vtkNotUsed(tol),
  double& t, double x[3], double pcoords[3], int& subId)
{
  double minPt[3], maxPt[3];
  double bounds[6];
  double p21[3];
  int i;

  subId = 0;

  this->Points->GetPoint(0, minPt);
  this->Points->GetPoint(7, maxPt);

  for (i = 0; i < 3; i++)
  {
    p21[i] = p2[i] - p1[i];
    bounds[2 * i] = minPt[i];
    bounds[2 * i + 1] = maxPt[i];
  }

  if (!vtkBox::IntersectBox(bounds, p1, p21, x, t))
  {
    return 0;
  }

  //
  // Evaluate intersection
  //
  for (i = 0; i < 3; i++)
  {
    pcoords[i] = (x[i] - minPt[i]) / (maxPt[i] - minPt[i]);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkVoxel::Triangulate(int index, vtkIdList* ptIds, vtkPoints* pts)
{
  int p[4], i;

  ptIds->Reset();
  pts->Reset();
  //
  // Create five tetrahedron. Triangulation varies depending upon index. This
  // is necessary to insure compatible voxel triangulations.
  //
  if ((index % 2))
  {
    p[0] = 0;
    p[1] = 1;
    p[2] = 2;
    p[3] = 4;
    for (i = 0; i < 4; i++)
    {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }

    p[0] = 1;
    p[1] = 4;
    p[2] = 5;
    p[3] = 7;
    for (i = 0; i < 4; i++)
    {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }

    p[0] = 1;
    p[1] = 4;
    p[2] = 7;
    p[3] = 2;
    for (i = 0; i < 4; i++)
    {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }

    p[0] = 1;
    p[1] = 2;
    p[2] = 7;
    p[3] = 3;
    for (i = 0; i < 4; i++)
    {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }

    p[0] = 2;
    p[1] = 7;
    p[2] = 6;
    p[3] = 4;
    for (i = 0; i < 4; i++)
    {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }
  }
  else
  {
    p[0] = 3;
    p[1] = 1;
    p[2] = 5;
    p[3] = 0;
    for (i = 0; i < 4; i++)
    {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }

    p[0] = 0;
    p[1] = 3;
    p[2] = 2;
    p[3] = 6;
    for (i = 0; i < 4; i++)
    {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }

    p[0] = 3;
    p[1] = 5;
    p[2] = 7;
    p[3] = 6;
    for (i = 0; i < 4; i++)
    {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }

    p[0] = 0;
    p[1] = 6;
    p[2] = 4;
    p[3] = 5;
    for (i = 0; i < 4; i++)
    {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }

    p[0] = 0;
    p[1] = 3;
    p[2] = 6;
    p[3] = 5;
    for (i = 0; i < 4; i++)
    {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkVoxel::Derivatives(
  int vtkNotUsed(subId), const double pcoords[3], const double* values, int dim, double* derivs)
{
  double functionDerivs[24], sum;
  int i, j, k;
  double x0[3], x1[3], x2[3], x4[3], spacing[3];

  this->Points->GetPoint(0, x0);
  this->Points->GetPoint(1, x1);
  spacing[0] = x1[0] - x0[0];

  this->Points->GetPoint(2, x2);
  spacing[1] = x2[1] - x0[1];

  this->Points->GetPoint(4, x4);
  spacing[2] = x4[2] - x0[2];

  // get derivatives in r-s-t directions
  this->InterpolationDerivs(pcoords, functionDerivs);

  // since the x-y-z axes are aligned with r-s-t axes, only need to scale
  // the derivative values by the data spacing.
  for (k = 0; k < dim; k++) // loop over values per point
  {
    for (j = 0; j < 3; j++) // loop over derivative directions
    {
      for (sum = 0.0, i = 0; i < 8; i++) // loop over interp. function derivatives
      {
        sum += functionDerivs[8 * j + i] * values[dim * i + k];
      }
      derivs[3 * k + j] = sum / spacing[j];
    }
  }
}

//----------------------------------------------------------------------------
vtkIdType vtkVoxel::GetPointToOneRingPoints(vtkIdType pointId, const vtkIdType*& pts)
{
  assert(pointId < vtkVoxel::NumberOfPoints && "pointId too large");
  pts = pointToOneRingPoints[pointId];
  return vtkVoxel::MaximumValence;
}

//----------------------------------------------------------------------------
vtkIdType vtkVoxel::GetPointToIncidentFaces(vtkIdType pointId, const vtkIdType*& faceIds)
{
  assert(pointId < vtkVoxel::NumberOfPoints && "pointId too large");
  faceIds = pointToIncidentFaces[pointId];
  return vtkVoxel::MaximumValence;
}

//----------------------------------------------------------------------------
vtkIdType vtkVoxel::GetPointToIncidentEdges(vtkIdType pointId, const vtkIdType*& edgeIds)
{
  assert(pointId < vtkVoxel::NumberOfPoints && "pointId too large");
  edgeIds = pointToIncidentEdges[pointId];
  return vtkVoxel::MaximumValence;
}

//----------------------------------------------------------------------------
vtkIdType vtkVoxel::GetFaceToAdjacentFaces(vtkIdType faceId, const vtkIdType*& faceIds)
{
  assert(faceId < vtkVoxel::NumberOfFaces && "faceId too large");
  faceIds = faceToAdjacentFaces[faceId];
  return vtkVoxel::MaximumFaceSize;
}

//----------------------------------------------------------------------------
void vtkVoxel::GetEdgeToAdjacentFaces(vtkIdType edgeId, const vtkIdType*& pts)
{
  assert(edgeId < vtkVoxel::NumberOfEdges && "edgeId too large");
  pts = edgeToAdjacentFaces[edgeId];
}

#ifndef VTK_LEGACY_REMOVE
//----------------------------------------------------------------------------
void vtkVoxel::GetEdgePoints(int edgeId, int*& pts)
{
  VTK_LEGACY_REPLACED_BODY(vtkVoxel::GetEdgePoints(int, int*&), "VTK 9.0",
    vtkVoxel::GetEdgePoints(vtkIdType, const vtkIdType*&));
  static std::vector<int> tmp(std::begin(faces[edgeId]), std::end(faces[edgeId]));
  pts = tmp.data();
}

//----------------------------------------------------------------------------
void vtkVoxel::GetFacePoints(int faceId, int*& pts)
{
  VTK_LEGACY_REPLACED_BODY(vtkVoxel::GetFacePoints(int, int*&), "VTK 9.0",
    vtkVoxel::GetFacePoints(vtkIdType, const vtkIdType*&));
  static std::vector<int> tmp(std::begin(faces[faceId]), std::end(faces[faceId]));
  pts = tmp.data();
}
#endif

//----------------------------------------------------------------------------
const vtkIdType* vtkVoxel::GetEdgeToAdjacentFacesArray(vtkIdType edgeId)
{
  assert(edgeId < vtkVoxel::NumberOfEdges && "edgeId too large");
  return edgeToAdjacentFaces[edgeId];
}

//----------------------------------------------------------------------------
const vtkIdType* vtkVoxel::GetFaceToAdjacentFacesArray(vtkIdType faceId)
{
  assert(faceId < vtkVoxel::NumberOfFaces && "faceId too large");
  return faceToAdjacentFaces[faceId];
}

//----------------------------------------------------------------------------
const vtkIdType* vtkVoxel::GetPointToIncidentEdgesArray(vtkIdType pointId)
{
  assert(pointId < vtkVoxel::NumberOfPoints && "pointId too large");
  return pointToIncidentEdges[pointId];
}

//----------------------------------------------------------------------------
const vtkIdType* vtkVoxel::GetPointToIncidentFacesArray(vtkIdType pointId)
{
  assert(pointId < vtkVoxel::NumberOfPoints && "pointId too large");
  return pointToIncidentFaces[pointId];
}

//----------------------------------------------------------------------------
const vtkIdType* vtkVoxel::GetPointToOneRingPointsArray(vtkIdType pointId)
{
  assert(pointId < vtkVoxel::NumberOfPoints && "pointId too large");
  return pointToOneRingPoints[pointId];
}

//----------------------------------------------------------------------------
void vtkVoxel::GetEdgePoints(vtkIdType edgeId, const vtkIdType*& pts)
{
  assert(edgeId < vtkVoxel::NumberOfEdges && "edgeId too large");
  pts = this->GetEdgeArray(edgeId);
}

//----------------------------------------------------------------------------
vtkIdType vtkVoxel::GetFacePoints(vtkIdType faceId, const vtkIdType*& pts)
{
  assert(faceId < vtkVoxel::NumberOfFaces && "faceId too large");
  pts = this->GetFaceArray(faceId);
  return vtkVoxel::MaximumFaceSize;
}

static double vtkVoxelCellPCoords[24] = { 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0,
  0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 1.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0 };

//----------------------------------------------------------------------------
double* vtkVoxel::GetParametricCoords()
{
  return vtkVoxelCellPCoords;
}

//----------------------------------------------------------------------------
void vtkVoxel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Line:\n";
  if (this->Line)
  {
    this->Line->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "None\n";
  }
  os << indent << "Pixel:\n";
  if (this->Pixel)
  {
    this->Pixel->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "None\n";
  }
}
