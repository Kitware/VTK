// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPixel.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkLine.h"
#include "vtkMarchingCellsClipCases.h"
#include "vtkMarchingCellsContourCases.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTriangle.h"

#include <algorithm>
#include <array>

namespace
{
//------------------------------------------------------------------------------
[[maybe_unused]] constexpr const char* Topology = R"(
   Pixel topology:

      2-----------3
      |           |
      |           |
      |           |
      0-----------1

   Note: unlike Quad, vertex ordering is not sequential around the
   perimeter — 2 and 3 are swapped compared to Quad.
)";

//------------------------------------------------------------------------------
double ParametricCoords[12] = {
  0.0, 0.0, 0.0, //
  1.0, 0.0, 0.0, //
  0.0, 1.0, 0.0, //
  1.0, 1.0, 0.0  //
};

//------------------------------------------------------------------------------
constexpr vtkIdType Edges[4][2] = {
  { 0, 1 }, //
  { 1, 3 }, //
  { 2, 3 }, //
  { 0, 2 }  //
};
}

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPixel);

//------------------------------------------------------------------------------
// Construct the pixel with four points.
vtkPixel::vtkPixel()
{
  this->Points->SetNumberOfPoints(4);
  this->PointIds->SetNumberOfIds(4);
  for (int i = 0; i < 4; i++)
  {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
  }
  for (int i = 0; i < 4; i++)
  {
    this->PointIds->SetId(i, 0);
  }
  this->Line = vtkSmartPointer<vtkLine>::New();
}

//------------------------------------------------------------------------------
int vtkPixel::EvaluatePosition(const double x[3], double closestPoint[3], int& subId,
  double pcoords[3], double& dist2, double weights[])
{
  double p[3], p21[3], p31[3], cp[3];
  double l21, l31, n[3];

  subId = 0;
  pcoords[2] = 0.0;

  // Efficient point access
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return 0;
  }
  const double* pts = pointsArray->GetPointer(0);

  // Get normal for pixel
  const double* pt1 = pts;
  const double* pt2 = pts + 3;
  const double* pt3 = pts + 6;

  vtkTriangle::ComputeNormal(pt1, pt2, pt3, n);

  // Project point to plane
  //
  vtkPlane::ProjectPoint(x, pt1, n, cp);

  for (int i = 0; i < 3; i++)
  {
    p21[i] = pt2[i] - pt1[i];
    p31[i] = pt3[i] - pt1[i];
    p[i] = x[i] - pt1[i];
  }

  if ((l21 = vtkMath::Norm(p21)) == 0.0)
  {
    l21 = 1.0;
  }
  if ((l31 = vtkMath::Norm(p31)) == 0.0)
  {
    l31 = 1.0;
  }

  pcoords[0] = vtkMath::Dot(p21, p) / (l21 * l21);
  pcoords[1] = vtkMath::Dot(p31, p) / (l31 * l31);

  vtkPixel::InterpolationFunctions(pcoords, weights);

  if (pcoords[0] >= 0.0 && pcoords[0] <= 1.0 && pcoords[1] >= 0.0 && pcoords[1] <= 1.0)
  {
    if (closestPoint)
    {
      closestPoint[0] = cp[0];
      closestPoint[1] = cp[1];
      closestPoint[2] = cp[2];
      dist2 = vtkMath::Distance2BetweenPoints(closestPoint, x); // projection distance
    }
    return 1;
  }
  else
  {
    double pc[3], w[4];
    if (closestPoint)
    {
      for (int i = 0; i < 2; i++)
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
void vtkPixel::EvaluateLocation(int& subId, const double pcoords[3], double x[3], double* weights)
{
  subId = 0;

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

  for (int i = 0; i < 3; i++)
  {
    x[i] = pt1[i] + pcoords[0] * (pt2[i] - pt1[i]) + pcoords[1] * (pt3[i] - pt1[i]);
  }

  vtkPixel::InterpolationFunctions(pcoords, weights);
}

//------------------------------------------------------------------------------
int vtkPixel::ComputeNormal(double n[3])
{
  double p0[3], p1[3], p2[3];
  this->Points->GetPoint(0, p0);
  this->Points->GetPoint(1, p1);
  this->Points->GetPoint(2, p2);
  p1[0] -= p0[0];
  p1[1] -= p0[1];
  p1[2] -= p0[2];
  p2[0] -= p0[0];
  p2[1] -= p0[1];
  p2[2] -= p0[2];
  vtkMath::Cross(p1, p2, n);
  if (std::abs(n[0]) < VTK_DBL_EPSILON && std::abs(n[1]) < VTK_DBL_EPSILON &&
    std::abs(n[2]) < VTK_DBL_EPSILON)
  {
    return -1;
  }
  vtkMath::Normalize(n);
  return (std::abs(n[1]) > 0.5) + (std::abs(n[2]) > 0.5) * 2;
}

//----------------------------------------------------------------------------
int vtkPixel::Inflate(double dist)
{
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return 0;
  }
  using Scalar = vtkDoubleArray::ValueType;
  auto pointRange = vtk::DataArrayTupleRange<3>(pointsArray);

  const auto p0 = pointRange[0], p3 = pointRange[3];

  const int normalDirection =
    static_cast<int>(vtkMathUtilities::NearlyEqual<Scalar>(p3[0], p0[0])) |
    static_cast<int>(vtkMathUtilities::NearlyEqual<Scalar>(p3[1], p0[1])) << 1 |
    static_cast<int>(vtkMathUtilities::NearlyEqual<Scalar>(p3[2], p0[2])) << 2;
  int degeneratePixelDirection = -1;

  if (normalDirection == 0x7)
  {
    // Pixel is collapsed to a single point
    return 0;
  }
  if ((normalDirection - 1) & normalDirection)
  {
    static constexpr std::array<int, 5> myLog2{ -1, 0, 1, -1, 2 };
    // Pixel is degenerate, it is homogeneous to a 1D line.
    degeneratePixelDirection = myLog2[(~normalDirection & 0x7)];
  }
  int index = 0;
  for (auto point : pointRange)
  {
    switch (normalDirection)
    {
      case 1:
        point[1] += dist * (index % 2 ? 1.0 : -1.0);
        point[2] += dist * (index / 2 ? 1.0 : -1.0);
        break;
      case 2:
        point[0] += dist * (index % 2 ? 1.0 : -1.0);
        point[2] += dist * (index / 2 ? 1.0 : -1.0);
        break;
      case 4:
        point[0] += dist * (index % 2 ? 1.0 : -1.0);
        point[1] += dist * (index / 2 ? 1.0 : -1.0);
        break;
      default:
        point[degeneratePixelDirection] += dist * (index % 2 ? 1.0 : -1.0);
        break;
    }
    ++index;
  }
  return 1;
}

//------------------------------------------------------------------------------
double vtkPixel::ComputeBoundingSphere(double center[3]) const
{
  auto points = vtk::DataArrayTupleRange<3>(this->Points->GetData());
  auto p0 = points[0], p3 = points[3];
  center[0] = 0.5 * (p0[0] + p3[0]);
  center[1] = 0.5 * (p0[1] + p3[1]);
  center[2] = 0.5 * (p0[2] + p3[2]);
  return vtkMath::Distance2BetweenPoints(center, p0);
}

//----------------------------------------------------------------------------
int vtkPixel::CellBoundary(int vtkNotUsed(subId), const double pcoords[3], vtkIdList* pts)
{
  double t1 = pcoords[0] - pcoords[1];
  double t2 = 1.0 - pcoords[0] - pcoords[1];

  pts->SetNumberOfIds(2);

  // compare against two lines in parametric space that divide element
  // into four pieces.
  if (t1 >= 0.0 && t2 >= 0.0)
  {
    pts->SetId(0, this->PointIds->GetId(0));
    pts->SetId(1, this->PointIds->GetId(1));
  }

  else if (t1 >= 0.0 && t2 < 0.0)
  {
    pts->SetId(0, this->PointIds->GetId(1));
    pts->SetId(1, this->PointIds->GetId(3));
  }

  else if (t1 < 0.0 && t2 < 0.0)
  {
    pts->SetId(0, this->PointIds->GetId(3));
    pts->SetId(1, this->PointIds->GetId(2));
  }

  else //( t1 < 0.0 && t2 >= 0.0 )
  {
    pts->SetId(0, this->PointIds->GetId(2));
    pts->SetId(1, this->PointIds->GetId(0));
  }

  if (pcoords[0] < 0.0 || pcoords[0] > 1.0 || pcoords[1] < 0.0 || pcoords[1] > 1.0)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

//------------------------------------------------------------------------------
void vtkPixel::Contour(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
  vtkCellArray* vtkNotUsed(verts), vtkCellArray* lines, vtkCellArray* vtkNotUsed(polys),
  vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd)
{
  static const int CASE_MASK[4] = { 1, 2, 4, 8 }; // note differenceom quad!
  vtkIdType pts[2];
  double x1[3], x2[3], x[3];

  // Build the case table
  int caseIndex = 0;
  for (int i = 0; i < 4; i++)
  {
    if (cellScalars->GetComponent(i, 0) >= value)
    {
      caseIndex |= CASE_MASK[i];
    }
  }

  const int* edge = vtkMarchingCellsContourCases::GetPixelCase(caseIndex);

  for (; edge[0] > -1; edge += 2)
  {
    for (int i = 0; i < 2; i++) // insert line
    {
      const auto& vert = Edges[edge[i]];
      double t = (value - cellScalars->GetComponent(vert[0], 0)) /
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
          const vtkIdType p1 = this->PointIds->GetId(vert[0]);
          const vtkIdType p2 = this->PointIds->GetId(vert[1]);
          outPd->InterpolateEdge(inPd, pts[i], p1, p2, t);
        }
      }
    }
    // check for degenerate line
    if (pts[0] != pts[1])
    {
      const vtkIdType newCellId = lines->InsertNextCell(2, pts);
      if (outCd)
      {
        outCd->CopyData(inCd, cellId, newCellId);
      }
    }
  }
}

//------------------------------------------------------------------------------
vtkCell* vtkPixel::GetEdge(int edgeId)
{
  edgeId = std::clamp(edgeId, 0, 3);

  // load point id's
  this->Line->PointIds->SetId(0, this->PointIds->GetId(Edges[edgeId][0]));
  this->Line->PointIds->SetId(1, this->PointIds->GetId(Edges[edgeId][1]));

  // load coordinates
  this->Line->Points->SetPoint(0, this->Points->GetPoint(Edges[edgeId][0]));
  this->Line->Points->SetPoint(1, this->Points->GetPoint(Edges[edgeId][1]));

  return this->Line;
}

//------------------------------------------------------------------------------
const vtkIdType* vtkPixel::GetEdgeArray(vtkIdType edgeId)
{
  return Edges[edgeId];
}

//------------------------------------------------------------------------------
//
// Compute interpolation functions (similar but different than Quad interpolation
// functions)
//
void vtkPixel::InterpolationFunctions(const double pcoords[3], double sf[4])
{
  double rm = 1. - pcoords[0];
  double sm = 1. - pcoords[1];

  sf[0] = rm * sm;
  sf[1] = pcoords[0] * sm;
  sf[2] = rm * pcoords[1];
  sf[3] = pcoords[0] * pcoords[1];
}
//------------------------------------------------------------------------------
//
// Compute derivatives of interpolation functions.
//
void vtkPixel::InterpolationDerivs(const double pcoords[3], double derivs[8])
{
  double rm = 1. - pcoords[0];
  double sm = 1. - pcoords[1];

  // r derivatives
  derivs[0] = -sm;
  derivs[1] = sm;
  derivs[2] = -pcoords[1];
  derivs[3] = pcoords[1];

  // s derivatives
  derivs[4] = -rm;
  derivs[5] = -pcoords[0];
  derivs[6] = rm;
  derivs[7] = pcoords[0];
}

//------------------------------------------------------------------------------
//
// Intersect plane; see whether point is inside.
//
int vtkPixel::IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t,
  double x[3], double pcoords[3], int& subId)
{
  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;
  double pt1[3], pt4[3];
  this->Points->GetPoint(0, pt1);
  this->Points->GetPoint(3, pt4);

  //
  // Get normal for triangle
  //
  double n[3] = { 0.0, 0.0, 0.0 };
  for (int i = 0; i < 3; i++)
  {
    if ((pt4[i] - pt1[i]) <= 0.0)
    {
      n[i] = 1.0;
      break;
    }
  }

  // Because vtkPlane::IntersectWithLine cannot handle intersection with finite
  // plane, we need to handle the coplanar case ourself and find the closest x/t possible.
  // EvaluatePosition will take care of filling values for subId and pcoords.
  const double v1[3] = { p1[0] - pt1[0], p1[1] - pt1[1], p1[2] - pt1[2] };
  const double v2[3] = { p2[0] - pt1[0], p2[1] - pt1[1], p2[2] - pt1[2] };
  bool isCoplanar = std::abs(vtkMath::Dot(v1, n)) < tol && (std::abs(vtkMath::Dot(v2, n)) < tol);
  if (isCoplanar)
  {
    // if p1 is inside the pixel then return p1.
    if (p1[0] <= pt4[0] && p1[0] >= pt1[0] && p1[1] <= pt4[1] && p1[1] >= pt1[1] &&
      p1[2] <= pt4[2] && p1[2] >= pt1[2])
    {
      t = 0;
      x[0] = p1[0];
      x[1] = p1[1];
      x[2] = p1[2];
    }
    // Else we check if we intersect any edges. If we dont that means we do not intersect the pixel.
    else
    {
      double mint = VTK_DOUBLE_MAX;
      double tmpt, tmpx[3], tmppcoords[3];
      int tmpid;
      for (int i = 0; i < 4; ++i)
      {
        bool res = this->GetEdge(i)->IntersectWithLine(p1, p2, tol, tmpt, tmpx, tmppcoords, tmpid);
        if (res && (tmpt < mint))
        {
          mint = tmpt;
          t = tmpt;
          x[0] = tmpx[0];
          x[1] = tmpx[1];
          x[2] = tmpx[2];
        }
      }

      if (mint == VTK_DOUBLE_MAX)
      {
        return 0;
      }
    }
  }
  else if (!vtkPlane::IntersectWithLine(p1, p2, n, pt1, t, x))
  {
    return 0;
  }

  //
  // Use evaluate position
  //
  double closestPoint[3], dist2, weights[4];
  return this->EvaluatePosition(x, closestPoint, subId, pcoords, dist2, weights) &&
    (dist2 <= (tol * tol));
}

//------------------------------------------------------------------------------
int vtkPixel::TriangulateLocalIds(int index, vtkIdList* ptIds)
{
  ptIds->SetNumberOfIds(6);
  if ((index % 2))
  {
    constexpr std::array<vtkIdType, 6> localPtIds{ 0, 1, 2, 1, 3, 2 };
    std::copy(localPtIds.begin(), localPtIds.end(), ptIds->begin());
  }
  else
  {
    constexpr std::array<vtkIdType, 6> localPtIds{ 0, 1, 3, 0, 3, 2 };
    std::copy(localPtIds.begin(), localPtIds.end(), ptIds->begin());
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkPixel::Derivatives(
  int vtkNotUsed(subId), const double pcoords[3], const double* values, int dim, double* derivs)
{
  double functionDerivs[8];
  int plane, idx[2];
  double x0[3], x1[3], x2[3], x3[3], spacing[3];

  this->Points->GetPoint(0, x0);
  this->Points->GetPoint(1, x1);
  this->Points->GetPoint(2, x2);
  this->Points->GetPoint(3, x3);

  // figure which plane this pixel is in
  for (int i = 0; i < 3; i++)
  {
    spacing[i] = x3[i] - x0[i];
  }

  if (spacing[0] > spacing[2] && spacing[1] > spacing[2]) // z-plane
  {
    plane = 2;
    idx[0] = 0;
    idx[1] = 1;
  }
  else if (spacing[0] > spacing[1] && spacing[2] > spacing[1]) // y-plane
  {
    plane = 1;
    idx[0] = 0;
    idx[1] = 2;
  }
  else // x-plane
  {
    plane = 0;
    idx[0] = 1;
    idx[1] = 2;
  }

  // get derivatives in r-s directions
  this->InterpolationDerivs(pcoords, functionDerivs);

  // since two of the x-y-z axes are aligned with r-s axes, only need to scale
  // the derivative values by the data spacing.
  double sum;
  for (int k = 0; k < dim; k++) // loop over values per vertex
  {
    for (int jj = 0, j = 0; j < 3; j++) // loop over derivative directions
    {
      if (j == plane) // 0-derivative values in this direction
      {
        sum = 0.0;
      }
      else // compute derivatives
      {
        sum = 0.0;
        for (int i = 0; i < 4; i++) // loop over interp. function derivatives
        {
          sum += functionDerivs[4 * jj + i] * values[dim * i + k];
        }
        sum /= spacing[idx[jj++]];
      }
      derivs[3 * k + j] = sum;
    }
  }
}

//------------------------------------------------------------------------------
// Clip this pixel using scalar value provided. Like contouring, except
// that it cuts the pixel to produce quads and/or triangles.
void vtkPixel::Clip(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
  vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd, vtkTypeBool insideOut)
{
  vtkIdType pts[4];
  double grdDiffs[4];
  double x[3], x1[3], x2[3];

  // Build the case table
  uint8_t caseIndex = 0;
  for (int pointId = 0; pointId < 4; ++pointId)
  {
    grdDiffs[pointId] = cellScalars->GetComponent(pointId, 0) - value;
    caseIndex |= (grdDiffs[pointId] >= 0.0) << pointId;
  }
  const uint8_t* thisCase = insideOut
    ? vtkMarchingCellsClipCases<true>::GetCellCase(VTK_PIXEL, caseIndex)
    : vtkMarchingCellsClipCases<false>::GetCellCase(VTK_PIXEL, caseIndex);
  using MCCases = vtkMarchingCellsClipCasesBase;
  const MCCases::EDGEIDXS* edgeVertices = MCCases::GetCellEdges(VTK_PIXEL);
  const uint8_t numberOfOutputCells = *thisCase++;

  // generate each tri/quad
  for (uint8_t outputCellId = 0; outputCellId < numberOfOutputCells; ++outputCellId)
  {
    /*shape =*/thisCase++; // VTK_TRIANGLE/VTK_QUAD
    const uint8_t numberOfCellPoints = *thisCase++;
    for (int i = 0; i < numberOfCellPoints; ++i) // insert quad or triangle
    {
      const uint8_t pointIndex = *thisCase++;
      if (pointIndex <= MCCases::P7) // Input Point
      {
        this->Points->GetPoint(pointIndex, x);
        if (locator->InsertUniquePoint(x, pts[i]))
        {
          if (outPd)
          {
            outPd->CopyData(inPd, this->PointIds->GetId(pointIndex), pts[i]);
          }
        }
      }
      else // Mid-Edge Point
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
          x[j] = x1[j] + t * (x2[j] - x1[j]);
        }

        if (locator->InsertUniquePoint(x, pts[i]))
        {
          if (outPd)
          {
            vtkIdType pointIndex1 = this->PointIds->GetId(point1Index);
            vtkIdType pointIndex2 = this->PointIds->GetId(point2Index);
            outPd->InterpolateEdge(inPd, pts[i], pointIndex1, pointIndex2, t);
          }
        }
      }
    }
    // check for degenerate output
    if (numberOfCellPoints == 3) // i.e., a triangle
    {
      if (pts[0] == pts[1] || pts[0] == pts[2] || pts[1] == pts[2])
      {
        continue;
      }
    }
    else // a quad
    {
      if ((pts[0] == pts[3] && pts[1] == pts[2]) || (pts[0] == pts[1] && pts[3] == pts[2]))
      {
        continue;
      }
    }

    const vtkIdType newCellId = polys->InsertNextCell(numberOfCellPoints, pts);
    outCd->CopyData(inCd, cellId, newCellId);
  }
}

//------------------------------------------------------------------------------
double* vtkPixel::GetParametricCoords()
{
  return ParametricCoords;
}

//------------------------------------------------------------------------------
void vtkPixel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Line:\n";
  this->Line->PrintSelf(os, indent.GetNextIndent());
}
VTK_ABI_NAMESPACE_END
