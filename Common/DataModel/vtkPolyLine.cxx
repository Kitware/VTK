// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPolyLine.h"

#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkSMPTools.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPolyLine);

//------------------------------------------------------------------------------
vtkPolyLine::vtkPolyLine() = default;

//------------------------------------------------------------------------------
vtkPolyLine::~vtkPolyLine() = default;

//------------------------------------------------------------------------------
int vtkPolyLine::GenerateSlidingNormals(vtkPoints* pts, vtkCellArray* lines, vtkDataArray* normals)
{
  return vtkPolyLine::GenerateSlidingNormals(pts, lines, normals, nullptr, false);
}

//------------------------------------------------------------------------------
inline vtkIdType FindNextValidSegment(
  vtkPoints* points, vtkIdType npts, const vtkIdType* pointIds, vtkIdType start)
{
  vtkVector3d ps;
  points->GetPoint(pointIds[start], ps.GetData());

  vtkIdType end = start + 1;
  while (end < npts)
  {
    vtkVector3d pe;
    points->GetPoint(pointIds[end], pe.GetData());
    if (ps != pe)
    {
      return end - 1;
    }
    ++end;
  }

  return npts;
}

namespace
{ // anonymous namespace supporting sliding normal generation

void SlidingNormalsOnLine(vtkPoints* pts, vtkIdType npts, const vtkIdType* linePts,
  vtkDataArray* normals, double* firstNormal, vtkVector3d& normal)
{
  if (npts <= 0)
  {
    return;
  }
  if (npts == 1) // return arbitrary
  {
    normals->InsertTuple(linePts[0], normal.GetData());
    return;
  }

  vtkIdType sNextId = 0;
  vtkVector3d sPrev, sNext;

  sNextId = FindNextValidSegment(pts, npts, linePts, 0);
  if (sNextId != npts) // at least one valid segment
  {
    vtkVector3d pt1, pt2;
    pts->GetPoint(linePts[sNextId], pt1.GetData());
    pts->GetPoint(linePts[sNextId + 1], pt2.GetData());
    sPrev = (pt2 - pt1).Normalized();
  }
  else // no valid segments
  {
    for (vtkIdType i = 0; i < npts; ++i)
    {
      normals->InsertTuple(linePts[i], normal.GetData());
    }
    return;
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
      sNextId = FindNextValidSegment(pts, npts, linePts, sNextId);
      if (sNextId != npts)
      {
        vtkVector3d pt1, pt2;
        pts->GetPoint(linePts[sNextId], pt1.GetData());
        pts->GetPoint(linePts[sNextId + 1], pt2.GetData());
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
          normal[(i + 2) % 3] = 0.0;
          normal[(i + 1) % 3] = 1.0;
          normal[i] = -sPrev[(i + 1) % 3] / sPrev[i];
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
    sNextId = FindNextValidSegment(pts, npts, linePts, sNextId);
    if (sNextId == npts)
    {
      break;
    }

    vtkVector3d pt1, pt2;
    pts->GetPoint(linePts[sNextId], pt1.GetData());
    pts->GetPoint(linePts[sNextId + 1], pt2.GetData());
    sNext = (pt2 - pt1).Normalized();

    // compute rotation vector
    vtkVector3d w = sPrev.Cross(normal);
    if (w.Normalize() == 0.0) // can't use this segment
    {
      continue;
    }

    // compute rotation of line segment
    vtkVector3d q = sNext.Cross(sPrev);
    if (q.Normalize() == 0.0) // can't use this segment
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
      normals->InsertTuple(linePts[i], normal.GetData());
    }
    lastNormalId = sNextId;
    sPrev = sNext;

    // compute next normal
    normal = (f1 * q) + (f2 * w);
  }

  // insert last normal for the remaining points
  for (vtkIdType i = lastNormalId; i < npts; ++i)
  {
    normals->InsertTuple(linePts[i], normal.GetData());
  }
} // SlidingNormalsOnLine

} // anonymous

//------------------------------------------------------------------------------
// Given points and lines, compute normals to the lines. These are not true
// normals, they are "orientation" normals used by classes like vtkTubeFilter
// that control the rotation around the line. The normals try to stay pointing
// in the same direction as much as possible (i.e., minimal rotation) w.r.t the
// firstNormal (computed if nullptr). Always returns 1 (success). It is possible
// to thread the generation of normals on multiple polylines, just make sure
// that points are not used by more than one line or a data race will result.
int vtkPolyLine::GenerateSlidingNormals(
  vtkPoints* pts, vtkCellArray* lines, vtkDataArray* normals, double* firstNormal, bool threading)
{
  vtkIdType numLines = lines->GetNumberOfCells();

  // Use threading to compute normals on each line independently.
  // If more than one polyline uses the same point, a data race
  // will occur.
  if (threading)
  {
    vtkSMPTools::For(0, numLines, [&](vtkIdType lineId, vtkIdType endLineId) {
      vtkSmartPointer<vtkCellArrayIterator> cellIter;
      cellIter.TakeReference(lines->NewIterator());
      vtkIdType npts;
      const vtkIdType* linePts;

      vtkVector3d normal(0.0, 0.0, 1.0); // arbitrary default value
      for (; lineId < endLineId; ++lineId)
      {
        cellIter->GetCellAtId(lineId, npts, linePts);
        SlidingNormalsOnLine(pts, npts, linePts, normals, firstNormal, normal);
      }
    }); // lambda
  }
  else
  {
    vtkSmartPointer<vtkCellArrayIterator> cellIter;
    cellIter.TakeReference(lines->NewIterator());
    vtkIdType npts;
    const vtkIdType* linePts;

    vtkVector3d normal(0.0, 0.0, 1.0); // arbitrary default value
    for (vtkIdType lineId = 0; lineId < numLines; ++lineId)
    {
      cellIter->GetCellAtId(lineId, npts, linePts);
      SlidingNormalsOnLine(pts, npts, linePts, normals, firstNormal, normal);
    }
  } // not threaded

  return 1;
}

//------------------------------------------------------------------------------
int vtkPolyLine::EvaluatePosition(const double x[3], double closestPoint[3], int& subId,
  double pcoords[3], double& minDist2, double weights[])
{
  double closest[3];
  double pc[3], dist2;
  int ignoreId, i, returnStatus, status;
  double lineWeights[2], closestWeights[2];

  // Efficient point access
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return 0;
  }
  const double* pts = pointsArray->GetPointer(0);

  pcoords[1] = pcoords[2] = 0.0;

  returnStatus = 0;
  subId = -1;
  closestWeights[0] = closestWeights[1] = 0.0; // Shut up, compiler
  for (minDist2 = VTK_DOUBLE_MAX, i = 0; i < this->Points->GetNumberOfPoints() - 1; i++)
  {
    this->Line->Points->SetPoint(0, pts + 3 * i);
    this->Line->Points->SetPoint(1, pts + 3 * (i + 1));
    status = this->Line->EvaluatePosition(x, closest, ignoreId, pc, dist2, lineWeights);
    if (status != -1 && ((dist2 < minDist2) || ((dist2 == minDist2) && (returnStatus == 0))))
    {
      returnStatus = status;
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
    weights[subId + 1] = closestWeights[1];
  }

  return returnStatus;
}

//------------------------------------------------------------------------------
void vtkPolyLine::EvaluateLocation(
  int& subId, const double pcoords[3], double x[3], double* weights)
{
  int i;
  double a1[3];
  double a2[3];
  this->Points->GetPoint(subId, a1);
  this->Points->GetPoint(subId + 1, a2);

  for (i = 0; i < 3; i++)
  {
    x[i] = a1[i] + pcoords[0] * (a2[i] - a1[i]);
  }

  weights[0] = 1.0 - pcoords[0];
  weights[1] = pcoords[0];
}

//------------------------------------------------------------------------------
int vtkPolyLine::CellBoundary(int subId, const double pcoords[3], vtkIdList* pts)
{
  pts->SetNumberOfIds(1);

  if (pcoords[0] >= 0.5)
  {
    pts->SetId(0, this->PointIds->GetId(subId + 1));
    if (pcoords[0] > 1.0)
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
    pts->SetId(0, this->PointIds->GetId(subId));
    if (pcoords[0] < 0.0)
    {
      return 0;
    }
    else
    {
      return 1;
    }
  }
}

//------------------------------------------------------------------------------
void vtkPolyLine::Contour(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* verts, vtkCellArray* lines,
  vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd)
{
  const vtkIdType numLines = this->Points->GetNumberOfPoints() - 1;
  vtkNew<vtkDoubleArray> lineScalars;
  lineScalars->SetNumberOfComponents(cellScalars->GetNumberOfComponents());
  lineScalars->SetNumberOfTuples(2);

  for (vtkIdType i = 0; i < numLines; i++)
  {
    this->Line->Points->SetPoint(0, this->Points->GetPoint(i));
    this->Line->Points->SetPoint(1, this->Points->GetPoint(i + 1));

    if (outPd)
    {
      this->Line->PointIds->SetId(0, this->PointIds->GetId(i));
      this->Line->PointIds->SetId(1, this->PointIds->GetId(i + 1));
    }

    lineScalars->SetTuple(0, cellScalars->GetTuple(i));
    lineScalars->SetTuple(1, cellScalars->GetTuple(i + 1));

    this->Line->Contour(
      value, lineScalars, locator, verts, lines, polys, inPd, outPd, inCd, cellId, outCd);
  }
}

//------------------------------------------------------------------------------
// Intersect with sub-lines
//
int vtkPolyLine::IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t,
  double x[3], double pcoords[3], int& subId)
{
  int subTest, numLines = this->Points->GetNumberOfPoints() - 1;

  for (subId = 0; subId < numLines; subId++)
  {
    this->Line->Points->SetPoint(0, this->Points->GetPoint(subId));
    this->Line->Points->SetPoint(1, this->Points->GetPoint(subId + 1));

    if (this->Line->IntersectWithLine(p1, p2, tol, t, x, pcoords, subTest))
    {
      return 1;
    }
  }

  return 0;
}

//------------------------------------------------------------------------------
int vtkPolyLine::TriangulateLocalIds(int vtkNotUsed(index), vtkIdList* ptIds)
{
  int numLines = this->Points->GetNumberOfPoints() - 1;
  ptIds->SetNumberOfIds(2 * numLines);
  for (int subId = 0; subId < numLines; subId++)
  {
    ptIds->SetId(subId * 2, subId);
    ptIds->SetId(subId * 2 + 1, subId + 1);
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkPolyLine::Derivatives(
  int subId, const double pcoords[3], const double* values, int dim, double* derivs)
{
  this->Line->PointIds->SetNumberOfIds(2);

  this->Line->Points->SetPoint(0, this->Points->GetPoint(subId));
  this->Line->Points->SetPoint(1, this->Points->GetPoint(subId + 1));

  this->Line->Derivatives(0, pcoords, values + dim * subId, dim, derivs);
}

//------------------------------------------------------------------------------
void vtkPolyLine::Clip(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
  vtkCellArray* polyLines, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd,
  vtkIdType cellId, vtkCellData* outCd, int insideOut)
{
  const vtkIdType numLines = this->Points->GetNumberOfPoints() - 1;
  vtkNew<vtkDoubleArray> lineScalars;
  lineScalars->SetNumberOfTuples(2);
  vtkNew<vtkCellArray> lines;
  vtkIdType numberOfCurrentLines, numberOfPreviousLines = 0;

  const auto appendLines = [&]() {
    // copy the previous lines to the output
    const auto numberOfPointsOfPolyLine = numberOfCurrentLines + 1;
#ifdef VTK_USE_64BIT_IDS
    const auto linesConnectivity = lines->GetConnectivityArray64()->GetPointer(0);
#else  // VTK_USE_64BIT_IDS
    const auto linesConnectivity = lines->GetConnectivityArray32()->GetPointer(0);
#endif // VTK_USE_64BIT_IDS
    const auto newCellId = polyLines->InsertNextCell(numberOfPointsOfPolyLine);
    polyLines->InsertCellPoint(linesConnectivity[0]);
    for (vtkIdType j = 0; j < numberOfPointsOfPolyLine - 1; ++j)
    {
      polyLines->InsertCellPoint(linesConnectivity[2 * j + 1]);
    }
    // copy the cell data
    outCd->CopyData(inCd, cellId, newCellId);
    // reset the number of previous lines
    numberOfPreviousLines = 0;
    lines->Reset();
  };

  for (vtkIdType i = 0; i < numLines; i++)
  {
    this->Line->Points->SetPoint(0, this->Points->GetPoint(i));
    this->Line->Points->SetPoint(1, this->Points->GetPoint(i + 1));

    this->Line->PointIds->SetId(0, this->PointIds->GetId(i));
    this->Line->PointIds->SetId(1, this->PointIds->GetId(i + 1));

    lineScalars->SetComponent(0, 0, cellScalars->GetComponent(i, 0));
    lineScalars->SetComponent(1, 0, cellScalars->GetComponent(i + 1, 0));

    this->Line->Clip(
      value, lineScalars, locator, lines, inPd, outPd, inCd, cellId, nullptr, insideOut);
    // if the line is clipped, we need to add the number of lines
    numberOfCurrentLines = lines->GetNumberOfCells();
    if (numberOfCurrentLines != numberOfPreviousLines)
    {
      numberOfPreviousLines = numberOfCurrentLines;
    }
    // if the line is not clipped, we need to combine the previous lines (if any) to the output
    else if (numberOfPreviousLines > 0)
    {
      appendLines();
    }
  }
  // if there are any lines left, we need to add them to the output
  if (numberOfCurrentLines > 0)
  {
    appendLines();
  }
}

//------------------------------------------------------------------------------
// Return the center of the point cloud in parametric coordinates.
int vtkPolyLine::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = 0.5;
  pcoords[1] = pcoords[2] = 0.0;
  return ((this->Points->GetNumberOfPoints() - 1) / 2);
}

//------------------------------------------------------------------------------
void vtkPolyLine::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Line:\n";
  this->Line->PrintSelf(os, indent.GetNextIndent());
}
VTK_ABI_NAMESPACE_END
