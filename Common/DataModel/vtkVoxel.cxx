// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkVoxel.h"

#include "vtkBox.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkLine.h"
#include "vtkMarchingCellsClipCases.h"
#include "vtkMarchingCellsContourCases.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPixel.h"
#include "vtkPointData.h"
#include "vtkPoints.h"

#include <algorithm> //std::copy
#include <cassert>

namespace
{
//------------------------------------------------------------------------------
[[maybe_unused]] constexpr const char* VoxelTopology = R"(
   Voxel topology:

            6-----------7
           /|          /|
          / |         / |
         4-----------5  |
         |  |        |  |
         |  2--------|--3
         | /         | /
         |/          |/
         0-----------1

   Note: unlike Hexahedron, vertex ordering is not sequential around
   each face — x varies fastest, then y, then z.
)";

//------------------------------------------------------------------------------
double ParametricCoords[24] = {
  0.0, 0.0, 0.0, //
  1.0, 0.0, 0.0, //
  0.0, 1.0, 0.0, //
  1.0, 1.0, 0.0, //
  0.0, 0.0, 1.0, //
  1.0, 0.0, 1.0, //
  0.0, 1.0, 1.0, //
  1.0, 1.0, 1.0  //
};

//------------------------------------------------------------------------------
constexpr vtkIdType Edges[vtkVoxel::NumberOfEdges][2] = {
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

//------------------------------------------------------------------------------
// define in terms vtkPixel understands
constexpr vtkIdType Faces[vtkVoxel::NumberOfFaces][vtkVoxel::MaximumFaceSize + 1] = {
  { 2, 0, 6, 4, -1 }, // 0
  { 1, 3, 5, 7, -1 }, // 1
  { 0, 1, 4, 5, -1 }, // 2
  { 3, 2, 7, 6, -1 }, // 3
  { 1, 0, 3, 2, -1 }, // 4
  { 4, 5, 6, 7, -1 }, // 5
};

//------------------------------------------------------------------------------
constexpr vtkIdType EdgeToAdjacentFaces[vtkVoxel::NumberOfEdges][2] = {
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

//------------------------------------------------------------------------------
constexpr vtkIdType FaceToAdjacentFaces[vtkVoxel::NumberOfFaces][vtkVoxel::MaximumFaceSize] = {
  { 5, 3, 4, 2 }, // 0
  { 4, 3, 5, 2 }, // 1
  { 4, 1, 5, 0 }, // 2
  { 4, 0, 5, 1 }, // 3
  { 2, 0, 3, 1 }, // 4
  { 2, 1, 3, 0 }, // 5
};

//------------------------------------------------------------------------------
constexpr vtkIdType PointToIncidentEdges[vtkVoxel::NumberOfPoints][vtkVoxel::MaximumValence] = {
  { 0, 8, 3 },  // 0
  { 0, 1, 9 },  // 1
  { 2, 3, 10 }, // 2
  { 1, 2, 11 }, // 3
  { 4, 7, 8 },  // 4
  { 4, 9, 5 },  // 5
  { 6, 10, 7 }, // 6
  { 5, 11, 6 }, // 7
};

//------------------------------------------------------------------------------
constexpr vtkIdType PointToIncidentFaces[vtkVoxel::NumberOfPoints][vtkVoxel::MaximumValence] = {
  { 2, 0, 4 }, // 0
  { 4, 1, 2 }, // 1
  { 4, 0, 3 }, // 2
  { 4, 3, 1 }, // 3
  { 5, 0, 2 }, // 4
  { 2, 1, 5 }, // 5
  { 3, 0, 5 }, // 6
  { 1, 3, 5 }, // 7
};

//------------------------------------------------------------------------------
constexpr vtkIdType PointToOneRingPoints[vtkVoxel::NumberOfPoints][vtkVoxel::MaximumValence] = {
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

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkVoxel);

//------------------------------------------------------------------------------
// Construct the voxel with eight points.
vtkVoxel::vtkVoxel()
{
  this->Points->SetNumberOfPoints(8);
  this->PointIds->SetNumberOfIds(8);
  for (int i = 0; i < 8; i++)
  {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
  }
  for (int i = 0; i < 8; i++)
  {
    this->PointIds->SetId(i, 0);
  }
  this->Line = vtkSmartPointer<vtkLine>::New();
  this->Pixel = vtkSmartPointer<vtkPixel>::New();
}

//------------------------------------------------------------------------------
bool vtkVoxel::GetCentroid(double centroid[3]) const
{
  return vtkVoxel::ComputeCentroid(this->Points, nullptr, centroid);
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
bool vtkVoxel::IsInsideOut()
{
  double pt1[3], pt2[3];
  this->Points->GetPoint(0, pt1);
  this->Points->GetPoint(7, pt2);
  return (pt2[0] - pt1[0]) * (pt2[1] - pt1[1]) * (pt2[2] - pt1[2]) < 0.0;
}

//------------------------------------------------------------------------------
double vtkVoxel::ComputeBoundingSphere(double center[3]) const
{
  auto points = vtk::DataArrayTupleRange(this->Points->GetData());
  auto p0 = points[0], p7 = points[7];
  center[0] = 0.5 * (p0[0] + p7[0]);
  center[1] = 0.5 * (p0[1] + p7[1]);
  center[2] = 0.5 * (p0[2] + p7[2]);
  return vtkMath::Distance2BetweenPoints(center, p0);
}

//------------------------------------------------------------------------------
int vtkVoxel::EvaluatePosition(const double x[3], double closestPoint[3], int& subId,
  double pcoords[3], double& dist2, double weights[])
{
  subId = 0;
  // Efficient point access
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return 0;
  }
  const double* pts = pointsArray->GetPointer(0);

  //
  // Get coordinate system
  //
  const double* pt1 = pts;
  const double* pt2 = pts + 3;
  const double* pt3 = pts + 6;
  const double* pt4 = pts + 12;
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
    vtkVoxel::InterpolationFunctions(pcoords, weights);
    return 1;
  }
  else
  {
    double pc[3], w[8];
    if (closestPoint)
    {
      for (int i = 0; i < 3; i++)
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

//------------------------------------------------------------------------------
void vtkVoxel::EvaluateLocation(
  int& vtkNotUsed(subId), const double pcoords[3], double x[3], double* weights)
{
  // Efficient point access
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return;
  }
  const double* pts = pointsArray->GetPointer(0);

  const double* pt1 = pts;
  const double* pt2 = pts + 3;
  const double* pt3 = pts + 6;
  const double* pt4 = pts + 12;

  for (int i = 0; i < 3; i++)
  {
    x[i] = pt1[i] + pcoords[0] * (pt2[i] - pt1[i]) + pcoords[1] * (pt3[i] - pt1[i]) +
      pcoords[2] * (pt4[i] - pt1[i]);
  }

  vtkVoxel::InterpolationFunctions(pcoords, weights);
}

//------------------------------------------------------------------------------
int vtkVoxel::Inflate(double dist)
{
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return 0;
  }
  auto range = vtk::DataArrayTupleRange<3>(pointsArray);
  int index = 0;
  for (auto point : range)
  {
    point[0] += dist * (index % 2 ? 1.0 : -1.0);
    point[1] += dist * ((index / 2) % 2 ? 1.0 : -1.0);
    point[2] += dist * (index / 4 ? 1.0 : -1.0);
    ++index;
  }
  return 1;
}

//----------------------------------------------------------------------------
//
// Compute Interpolation functions
//
void vtkVoxel::InterpolationFunctions(const double pcoords[3], double sf[8])
{
  double r = pcoords[0], s = pcoords[1], t = pcoords[2];

  double rm = 1. - r;
  double sm = 1. - s;
  double tm = 1. - t;

  sf[0] = rm * sm * tm;
  sf[1] = r * sm * tm;
  sf[2] = rm * s * tm;
  sf[3] = r * s * tm;
  sf[4] = rm * sm * t;
  sf[5] = r * sm * t;
  sf[6] = rm * s * t;
  sf[7] = r * s * t;
}

//------------------------------------------------------------------------------
void vtkVoxel::InterpolationDerivs(const double pcoords[3], double derivs[24])
{
  double rm = 1. - pcoords[0];
  double sm = 1. - pcoords[1];
  double tm = 1. - pcoords[2];

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

//------------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkVoxel::Contour(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
  vtkCellArray* verts, vtkCellArray* lines, vtkCellArray* polys, vtkPointData* inPd,
  vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd)
{
  static const int CASE_MASK[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };
  vtkIdType pts[3];
  double x1[3], x2[3], x[3];
  vtkIdType offset = verts->GetNumberOfCells() + lines->GetNumberOfCells();

  // Build the case table
  int caseIndex = 0;
  for (int i = 0; i < 8; i++)
  {
    if (cellScalars->GetComponent(i, 0) >= value)
    {
      caseIndex |= CASE_MASK[i];
    }
  }

  const int* edge = vtkMarchingCellsContourCases::GetVoxelCase(caseIndex);

  for (; edge[0] > -1; edge += 3)
  {
    for (int i = 0; i < 3; i++) // insert triangle
    {
      const vtkIdType* vert = Edges[edge[i]];
      const double t = (value - cellScalars->GetComponent(vert[0], 0)) /
        (cellScalars->GetComponent(vert[1], 0) - cellScalars->GetComponent(vert[0], 0));
      this->Points->GetPoint(vert[0], x1);
      this->Points->GetPoint(vert[1], x2);
      for (int j = 0; j < 3; j++)
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
      const vtkIdType newCellId = offset + polys->InsertNextCell(3, pts);
      if (outCd)
      {
        outCd->CopyData(inCd, cellId, newCellId);
      }
    }
  }
}

//------------------------------------------------------------------------------
// Return the case table for table-based isocontouring (aka marching cubes
// style implementations). A linear 3D cell with N vertices will have 2**N
// cases. The cases list three Edges in order to produce one output triangle.
int* vtkVoxel::GetTriangleCases(int caseId)
{
  return const_cast<int*>(vtkMarchingCellsContourCases::GetVoxelCase(caseId));
}

//------------------------------------------------------------------------------
void vtkVoxel::Clip(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
  vtkCellArray* connectivity, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd,
  vtkIdType cellId, vtkCellData* outCd, vtkTypeBool insideOut)
{
  vtkIdType pts[8];
  double coords[8][3];
  double grdDiffs[8];
  double x[3], x1[3], x2[3];
  vtkIdType centroidIndex = 0;

  // Build the case table
  uint8_t caseIndex = 0;
  for (int pointId = 0; pointId < 8; ++pointId)
  {
    grdDiffs[pointId] = cellScalars->GetComponent(pointId, 0) - value;
    caseIndex |= (grdDiffs[pointId] >= 0.0) << pointId;
  }
  const uint8_t* thisCase = insideOut
    ? vtkMarchingCellsClipCases<true>::GetCellCase(VTK_VOXEL, caseIndex)
    : vtkMarchingCellsClipCases<false>::GetCellCase(VTK_VOXEL, caseIndex);
  using MCCases = vtkMarchingCellsClipCasesBase;
  const MCCases::EDGEIDXS* edgeVertices = MCCases::GetCellEdges(VTK_VOXEL);
  const uint8_t numberOfOutputCells = *thisCase++;

  for (uint8_t outputCellId = 0; outputCellId < numberOfOutputCells; ++outputCellId)
  {
    const uint8_t shape = *thisCase++;
    const uint8_t numberOfCellPoints = *thisCase++;
    for (int i = 0; i < numberOfCellPoints; ++i)
    {
      const uint8_t pointIndex = *thisCase++;
      if (pointIndex <= MCCases::P7) // Input Point
      {
        this->Points->GetPoint(pointIndex, coords[i]);
        if (locator->InsertUniquePoint(coords[i], pts[i]))
        {
          if (outPd)
          {
            outPd->CopyData(inPd, this->PointIds->GetId(pointIndex), pts[i]);
          }
        }
      }
      else if (pointIndex <= MCCases::EL) // Edge point
      {
        const auto& edgePoints = edgeVertices[pointIndex - MCCases::EA];
        uint8_t point1Index = edgePoints[0];
        uint8_t point2Index = edgePoints[1];
        double point1ToPoint2 = grdDiffs[point2Index] - grdDiffs[point1Index];
        if (point1ToPoint2 < 0)
        {
          std::swap(point1Index, point2Index);
          point1ToPoint2 = -point1ToPoint2;
        }
        const double point1ToIso = 0.0 - grdDiffs[point1Index];
        const double t = point1ToPoint2 != 0 ? point1ToIso / point1ToPoint2 : 0;
        this->Points->GetPoint(point1Index, x1);
        this->Points->GetPoint(point2Index, x2);
        for (int j = 0; j < 3; j++)
        {
          coords[i][j] = x1[j] + t * (x2[j] - x1[j]);
        }

        if (locator->InsertUniquePoint(coords[i], pts[i]))
        {
          if (outPd)
          {
            vtkIdType pointIndex1 = this->PointIds->GetId(point1Index);
            vtkIdType pointIndex2 = this->PointIds->GetId(point2Index);
            outPd->InterpolateEdge(inPd, pts[i], pointIndex1, pointIndex2, t);
          }
        }
      }
      else // centroid point
      {
        pts[i] = centroidIndex;
      }
    }
    if (shape != VTK_EMPTY_CELL) // normal cell
    {
      const vtkIdType newCellId = connectivity->InsertNextCell(numberOfCellPoints, pts);
      if (outCd)
      {
        outCd->CopyData(inCd, cellId, newCellId);
      }
    }
    else // centroid
    {
      x[0] = x[1] = x[2] = 0.0;
      double weightFactor = 1.0 / numberOfCellPoints;
      double weights[8];
      for (int i = 0; i < numberOfCellPoints; ++i)
      {
        x[0] += coords[i][0];
        x[1] += coords[i][1];
        x[2] += coords[i][2];
        weights[i] = weightFactor;
      }
      x[0] *= weightFactor;
      x[1] *= weightFactor;
      x[2] *= weightFactor;
      if (locator->InsertUniquePoint(x, centroidIndex))
      {
        if (outPd)
        {
          vtkNew<vtkIdList> idList;
          idList->SetList(pts, numberOfCellPoints, /*save*/ true);
          outPd->InterpolatePoint(outPd, centroidIndex, idList, weights);
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
vtkCell* vtkVoxel::GetEdge(int edgeId)
{
  const vtkIdType* verts = Edges[edgeId];

  // load point id's
  this->Line->PointIds->SetId(0, this->PointIds->GetId(verts[0]));
  this->Line->PointIds->SetId(1, this->PointIds->GetId(verts[1]));

  // load coordinates
  this->Line->Points->SetPoint(0, this->Points->GetPoint(verts[0]));
  this->Line->Points->SetPoint(1, this->Points->GetPoint(verts[1]));

  return this->Line;
}

//------------------------------------------------------------------------------
vtkCell* vtkVoxel::GetFace(int faceId)
{
  const vtkIdType* verts = Faces[faceId];

  for (int i = 0; i < 4; i++)
  {
    this->Pixel->PointIds->SetId(i, this->PointIds->GetId(verts[i]));
    this->Pixel->Points->SetPoint(i, this->Points->GetPoint(verts[i]));
  }

  return this->Pixel;
}

//------------------------------------------------------------------------------
const vtkIdType* vtkVoxel::GetEdgeArray(vtkIdType edgeId)
{
  return Edges[edgeId];
}

//------------------------------------------------------------------------------
const vtkIdType* vtkVoxel::GetFaceArray(vtkIdType faceId)
{
  return Faces[faceId];
}

//------------------------------------------------------------------------------
const vtkIdType* vtkVoxel::GetEdgeToAdjacentFacesArray(vtkIdType edgeId)
{
  assert(edgeId < vtkVoxel::NumberOfEdges && "edgeId too large");
  return EdgeToAdjacentFaces[edgeId];
}

//------------------------------------------------------------------------------
const vtkIdType* vtkVoxel::GetFaceToAdjacentFacesArray(vtkIdType faceId)
{
  assert(faceId < vtkVoxel::NumberOfFaces && "faceId too large");
  return FaceToAdjacentFaces[faceId];
}

//------------------------------------------------------------------------------
const vtkIdType* vtkVoxel::GetPointToIncidentEdgesArray(vtkIdType pointId)
{
  assert(pointId < vtkVoxel::NumberOfPoints && "pointId too large");
  return PointToIncidentEdges[pointId];
}

//------------------------------------------------------------------------------
const vtkIdType* vtkVoxel::GetPointToIncidentFacesArray(vtkIdType pointId)
{
  assert(pointId < vtkVoxel::NumberOfPoints && "pointId too large");
  return PointToIncidentFaces[pointId];
}

//------------------------------------------------------------------------------
const vtkIdType* vtkVoxel::GetPointToOneRingPointsArray(vtkIdType pointId)
{
  assert(pointId < vtkVoxel::NumberOfPoints && "pointId too large");
  return PointToOneRingPoints[pointId];
}

//------------------------------------------------------------------------------
void vtkVoxel::GetEdgePoints(vtkIdType edgeId, const vtkIdType*& pts)
{
  assert(edgeId < vtkVoxel::NumberOfEdges && "edgeId too large");
  pts = this->GetEdgeArray(edgeId);
}

//------------------------------------------------------------------------------
vtkIdType vtkVoxel::GetFacePoints(vtkIdType faceId, const vtkIdType*& pts)
{
  assert(faceId < vtkVoxel::NumberOfFaces && "faceId too large");
  pts = this->GetFaceArray(faceId);
  return vtkVoxel::MaximumFaceSize;
}

//------------------------------------------------------------------------------
void vtkVoxel::GetEdgeToAdjacentFaces(vtkIdType edgeId, const vtkIdType*& pts)
{
  assert(edgeId < vtkVoxel::NumberOfEdges && "edgeId too large");
  pts = EdgeToAdjacentFaces[edgeId];
}

//------------------------------------------------------------------------------
vtkIdType vtkVoxel::GetFaceToAdjacentFaces(vtkIdType faceId, const vtkIdType*& faceIds)
{
  assert(faceId < vtkVoxel::NumberOfFaces && "faceId too large");
  faceIds = FaceToAdjacentFaces[faceId];
  return vtkVoxel::MaximumFaceSize;
}

//------------------------------------------------------------------------------
vtkIdType vtkVoxel::GetPointToIncidentEdges(vtkIdType pointId, const vtkIdType*& edgeIds)
{
  assert(pointId < vtkVoxel::NumberOfPoints && "pointId too large");
  edgeIds = PointToIncidentEdges[pointId];
  return vtkVoxel::MaximumValence;
}

//------------------------------------------------------------------------------
vtkIdType vtkVoxel::GetPointToIncidentFaces(vtkIdType pointId, const vtkIdType*& faceIds)
{
  assert(pointId < vtkVoxel::NumberOfPoints && "pointId too large");
  faceIds = PointToIncidentFaces[pointId];
  return vtkVoxel::MaximumValence;
}

//------------------------------------------------------------------------------
vtkIdType vtkVoxel::GetPointToOneRingPoints(vtkIdType pointId, const vtkIdType*& pts)
{
  assert(pointId < vtkVoxel::NumberOfPoints && "pointId too large");
  pts = PointToOneRingPoints[pointId];
  return vtkVoxel::MaximumValence;
}

//------------------------------------------------------------------------------
//
// Intersect voxel with line using "bounding box" intersection.
//
int vtkVoxel::IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t,
  double x[3], double pcoords[3], int& subId)
{
  double minPt[3], maxPt[3];
  double bounds[6];
  double p21[3];

  subId = 0;

  this->Points->GetPoint(0, minPt);
  this->Points->GetPoint(7, maxPt);

  for (int i = 0; i < 3; i++)
  {
    p21[i] = p2[i] - p1[i];
    bounds[2 * i] = minPt[i];
    bounds[2 * i + 1] = maxPt[i];
  }

  if (!vtkBox::IntersectBox(bounds, p1, p21, x, t, tol))
  {
    return 0;
  }

  //
  // Evaluate intersection
  //
  for (int i = 0; i < 3; i++)
  {
    pcoords[i] = (x[i] - minPt[i]) / (maxPt[i] - minPt[i]);
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkVoxel::TriangulateLocalIds(int index, vtkIdList* ptIds)
{
  // Create five tetrahedron. Triangulation varies depending upon index. This
  // is necessary to ensure compatible voxel triangulations.
  ptIds->SetNumberOfIds(5 * 4);

  if ((index % 2))
  {
    constexpr vtkIdType ids[5][4] = { { 0, 1, 2, 4 }, { 1, 4, 5, 7 }, { 1, 4, 7, 2 },
      { 1, 2, 7, 3 }, { 2, 7, 6, 4 } };
    std::copy_n(&ids[0][0], 20, ptIds->begin());
  }
  else
  {
    constexpr vtkIdType ids[5][4] = { { 3, 1, 5, 0 }, { 0, 3, 2, 6 }, { 3, 5, 7, 6 },
      { 0, 6, 4, 5 }, { 0, 3, 6, 5 } };
    std::copy_n(&ids[0][0], 20, ptIds->begin());
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkVoxel::Derivatives(
  int vtkNotUsed(subId), const double pcoords[3], const double* values, int dim, double* derivs)
{
  double functionDerivs[24];
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
  for (int k = 0; k < dim; k++) // loop over values per point
  {
    for (int j = 0; j < 3; j++) // loop over derivative directions
    {
      double sum = 0.0;
      for (int i = 0; i < 8; i++) // loop over interp. function derivatives
      {
        sum += functionDerivs[8 * j + i] * values[dim * i + k];
      }
      derivs[3 * k + j] = sum / spacing[j];
    }
  }
}

//------------------------------------------------------------------------------
double* vtkVoxel::GetParametricCoords()
{
  return ParametricCoords;
}

//------------------------------------------------------------------------------
void vtkVoxel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Line:\n";
  this->Line->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Pixel:\n";
  this->Pixel->PrintSelf(os, indent.GetNextIndent());
}
VTK_ABI_NAMESPACE_END
