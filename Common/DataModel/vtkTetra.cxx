// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkTetra.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkLine.h"
#include "vtkMarchingCellsClipCases.h"
#include "vtkMarchingCellsContourCases.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTriangle.h"
#include "vtkUnstructuredGrid.h"

#include <cassert>
#include <numeric> //std::iota

namespace
{
//------------------------------------------------------------------------------
[[maybe_unused]] constexpr const char* Topology = R"(
   Tetra topology:

            3
           /|\
          / | \
         /  |  \
        /   0   \
       /  /   \  \
      / /       \ \
     //           \\
    1---------------2
)";

//------------------------------------------------------------------------------
double ParametricCoords[12] = {
  0.0, 0.0, 0.0, //
  1.0, 0.0, 0.0, //
  0.0, 1.0, 0.0, //
  0.0, 0.0, 1.0  //
};

//------------------------------------------------------------------------------
constexpr vtkIdType Edges[vtkTetra::NumberOfEdges][2] = {
  { 0, 1 }, // 0
  { 1, 2 }, // 1
  { 2, 0 }, // 2
  { 0, 3 }, // 3
  { 1, 3 }, // 4
  { 2, 3 }, // 5
};

//------------------------------------------------------------------------------
constexpr vtkIdType Faces[vtkTetra::NumberOfFaces][vtkTetra::MaximumFaceSize + 1] = {
  { 0, 1, 3, -1 }, // 0
  { 1, 2, 3, -1 }, // 1
  { 2, 0, 3, -1 }, // 2
  { 0, 2, 1, -1 }, // 3
};

//------------------------------------------------------------------------------
constexpr vtkIdType EdgeToAdjacentFaces[vtkTetra::NumberOfEdges][2] = {
  { 0, 3 }, // 0
  { 1, 3 }, // 1
  { 2, 3 }, // 2
  { 0, 2 }, // 3
  { 0, 1 }, // 4
  { 1, 2 }, // 5
};

//------------------------------------------------------------------------------
constexpr vtkIdType FaceToAdjacentFaces[vtkTetra::NumberOfFaces][vtkTetra::MaximumFaceSize] = {
  { 3, 1, 2 }, // 0
  { 3, 2, 0 }, // 1
  { 3, 0, 1 }, // 2
  { 2, 1, 0 }, // 3
};

//------------------------------------------------------------------------------
constexpr vtkIdType PointToIncidentEdges[vtkTetra::NumberOfPoints][vtkTetra::MaximumValence] = {
  { 0, 3, 2 }, // 0
  { 0, 1, 4 }, // 1
  { 1, 2, 5 }, // 2
  { 3, 4, 5 }, // 3
};

//------------------------------------------------------------------------------
constexpr vtkIdType PointToIncidentFaces[vtkTetra::NumberOfPoints][vtkTetra::MaximumValence] = {
  { 0, 2, 3 }, // 0
  { 3, 1, 0 }, // 1
  { 3, 2, 1 }, // 2
  { 0, 1, 2 }, // 3
};

//------------------------------------------------------------------------------
constexpr vtkIdType PointToOneRingPoints[vtkTetra::NumberOfPoints][vtkTetra::MaximumValence] = {
  { 1, 3, 2 }, // 0
  { 0, 2, 3 }, // 1
  { 1, 0, 3 }, // 2
  { 0, 1, 2 }, // 3
};
}

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTetra);

//------------------------------------------------------------------------------
// Construct the tetra with four points.
vtkTetra::vtkTetra()
{
  this->Points->SetNumberOfPoints(4);
  this->PointIds->SetNumberOfIds(4);
  for (int i = 0; i < 4; i++)
  {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i, 0);
  }
  this->Line = vtkSmartPointer<vtkLine>::New();
  this->Triangle = vtkSmartPointer<vtkTriangle>::New();
}

//------------------------------------------------------------------------------
int vtkTetra::EvaluatePosition(const double x[3], double closestPoint[3], int& subId,
  double pcoords[3], double& minDist2, double weights[])
{
  double rhs[3], c1[3], c2[3], c3[3];
  double det;

  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;

  // Efficient point access
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return 0;
  }
  const double* pts = pointsArray->GetPointer(0);
  const double* pt1 = pts + 3;
  const double* pt2 = pts + 6;
  const double* pt3 = pts + 9;
  const double* pt4 = pts;

  for (int i = 0; i < 3; i++)
  {
    rhs[i] = x[i] - pt4[i];
    c1[i] = pt1[i] - pt4[i];
    c2[i] = pt2[i] - pt4[i];
    c3[i] = pt3[i] - pt4[i];
  }

  if ((det = vtkMath::Determinant3x3(c1, c2, c3)) == 0.0)
  {
    return -1;
  }

  pcoords[0] = vtkMath::Determinant3x3(rhs, c2, c3) / det;
  pcoords[1] = vtkMath::Determinant3x3(c1, rhs, c3) / det;
  pcoords[2] = vtkMath::Determinant3x3(c1, c2, rhs) / det;
  double p4 = 1.0 - pcoords[0] - pcoords[1] - pcoords[2];

  weights[0] = p4;
  weights[1] = pcoords[0];
  weights[2] = pcoords[1];
  weights[3] = pcoords[2];

  if (pcoords[0] >= -0.001 && pcoords[0] <= 1.001 && pcoords[1] >= -0.001 && pcoords[1] <= 1.001 &&
    pcoords[2] >= -0.001 && pcoords[2] <= 1.001 && p4 >= -0.001 && p4 <= 1.001)
  {
    if (closestPoint)
    {
      closestPoint[0] = x[0];
      closestPoint[1] = x[1];
      closestPoint[2] = x[2];
      minDist2 = 0.0; // inside tetra
    }
    return 1;
  }
  else
  {
    if (closestPoint)
    {
      // could easily be sped up using parametric localization - next release
      double w[3], closest[3], pc[3], dist2;
      int sub;
      minDist2 = VTK_DOUBLE_MAX;
      for (int i = 0; i < 4; i++)
      {
        vtkTriangle* triangle = static_cast<vtkTriangle*>(this->GetFace(i));
        triangle->EvaluatePosition(x, closest, sub, pc, dist2, w);

        if (dist2 < minDist2)
        {
          closestPoint[0] = closest[0];
          closestPoint[1] = closest[1];
          closestPoint[2] = closest[2];
          minDist2 = dist2;
        }
      }
    }
    return 0;
  }
}

//------------------------------------------------------------------------------
bool vtkTetra::GetCentroid(double centroid[3]) const
{
  return vtkTetra::ComputeCentroid(this->Points, nullptr, centroid);
}

//------------------------------------------------------------------------------
bool vtkTetra::ComputeCentroid(vtkPoints* points, const vtkIdType* pointIds, double centroid[3])
{
  double p[3];
  centroid[0] = centroid[1] = centroid[2] = 0.0;
  if (!pointIds)
  {
    for (vtkIdType i = 0; i < vtkTetra::NumberOfPoints; ++i)
    {
      points->GetPoint(i, p);
      centroid[0] += p[0];
      centroid[1] += p[1];
      centroid[2] += p[2];
    }
  }
  else
  {
    for (vtkIdType i = 0; i < vtkTetra::NumberOfPoints; ++i)
    {
      points->GetPoint(pointIds[i], p);
      centroid[0] += p[0];
      centroid[1] += p[1];
      centroid[2] += p[2];
    }
  }
  centroid[0] /= vtkTetra::NumberOfPoints;
  centroid[1] /= vtkTetra::NumberOfPoints;
  centroid[2] /= vtkTetra::NumberOfPoints;
  return true;
}

//------------------------------------------------------------------------------
bool vtkTetra::IsInsideOut()
{
  double v[3], a[3], b[3], c[3], d[3], e[3];
  this->Points->GetPoint(0, a);
  this->Points->GetPoint(1, b);
  this->Points->GetPoint(2, c);
  d[0] = b[0] - a[0];
  d[1] = b[1] - a[1];
  d[2] = b[2] - a[2];
  e[0] = c[0] - a[0];
  e[1] = c[1] - a[1];
  e[2] = c[2] - a[2];
  vtkMath::Cross(d, e, v);
  this->Points->GetPoint(3, d);
  a[0] = d[0] - (a[0] + b[0] + c[0]) / 3.0;
  a[1] = d[1] - (a[1] + b[1] + c[1]) / 3.0;
  a[2] = d[2] - (a[2] + b[2] + c[2]) / 3.0;
  return vtkMath::Dot(a, v) < 0.0;
}

//------------------------------------------------------------------------------
void vtkTetra::EvaluateLocation(
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
  const double* pt1 = pts + 3;
  const double* pt2 = pts + 6;
  const double* pt3 = pts + 9;
  const double* pt4 = pts;

  double u4 = 1.0 - pcoords[0] - pcoords[1] - pcoords[2];

  for (int i = 0; i < 3; i++)
  {
    x[i] = pt1[i] * pcoords[0] + pt2[i] * pcoords[1] + pt3[i] * pcoords[2] + pt4[i] * u4;
  }

  weights[0] = u4;
  weights[1] = pcoords[0];
  weights[2] = pcoords[1];
  weights[3] = pcoords[2];
}

//------------------------------------------------------------------------------
// Returns the set of points that are on the boundary of the tetrahedron that
// are closest parametrically to the point specified. This may include Faces,
// Edges, or vertices.
int vtkTetra::CellBoundary(int vtkNotUsed(subId), const double pcoords[3], vtkIdList* pts)
{
  double minPCoord = 1.0 - pcoords[0] - pcoords[1] - pcoords[2];
  int idx = 3;
  for (int i = 0; i < 3; i++)
  {
    if (pcoords[i] < minPCoord)
    {
      minPCoord = pcoords[i];
      idx = i;
    }
  }

  pts->SetNumberOfIds(3);
  switch (idx) // find the face closest to the point
  {
    case 0:
      pts->SetId(0, this->PointIds->GetId(0));
      pts->SetId(1, this->PointIds->GetId(2));
      pts->SetId(2, this->PointIds->GetId(3));
      break;

    case 1:
      pts->SetId(0, this->PointIds->GetId(0));
      pts->SetId(1, this->PointIds->GetId(1));
      pts->SetId(2, this->PointIds->GetId(3));
      break;

    case 2:
      pts->SetId(0, this->PointIds->GetId(0));
      pts->SetId(1, this->PointIds->GetId(1));
      pts->SetId(2, this->PointIds->GetId(2));
      break;

    case 3:
      pts->SetId(0, this->PointIds->GetId(1));
      pts->SetId(1, this->PointIds->GetId(2));
      pts->SetId(2, this->PointIds->GetId(3));
      break;
  }

  if (pcoords[0] < 0.0 || pcoords[1] < 0.0 || pcoords[2] < 0.0 || pcoords[0] > 1.0 ||
    pcoords[1] > 1.0 || pcoords[2] > 1.0 || (1.0 - pcoords[0] - pcoords[1] - pcoords[2]) < 0.0)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

//------------------------------------------------------------------------------
void vtkTetra::Contour(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
  vtkCellArray* verts, vtkCellArray* lines, vtkCellArray* polys, vtkPointData* inPd,
  vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd)
{
  static constexpr int CASE_MASK[4] = { 1, 2, 4, 8 };
  int v1, v2;
  vtkIdType pts[3];
  double x1[3], x2[3], x[3];
  vtkIdType offset = verts->GetNumberOfCells() + lines->GetNumberOfCells();

  // Build the case table
  int index = 0;
  for (int i = 0; i < 4; i++)
  {
    if (cellScalars->GetComponent(i, 0) >= value)
    {
      index |= CASE_MASK[i];
    }
  }

  const int* edge = vtkMarchingCellsContourCases::GetTetraCase(index);

  for (; edge[0] > -1; edge += 3)
  {
    for (int i = 0; i < 3; i++) // insert triangle
    {
      const vtkIdType* vert = Edges[edge[i]];

      // calculate a preferred interpolation direction
      double deltaScalar =
        (cellScalars->GetComponent(vert[1], 0) - cellScalars->GetComponent(vert[0], 0));
      if (deltaScalar > 0)
      {
        v1 = vert[0];
        v2 = vert[1];
      }
      else
      {
        v1 = vert[1];
        v2 = vert[0];
        deltaScalar = -deltaScalar;
      }

      // linear interpolation across edge
      double t =
        (deltaScalar == 0.0 ? 0.0 : (value - cellScalars->GetComponent(v1, 0)) / deltaScalar);

      this->Points->GetPoint(v1, x1);
      this->Points->GetPoint(v2, x2);

      for (int j = 0; j < 3; j++)
      {
        x[j] = x1[j] + t * (x2[j] - x1[j]);
      }
      if (locator->InsertUniquePoint(x, pts[i]))
      {
        if (outPd)
        {
          vtkIdType p1 = this->PointIds->GetId(v1);
          vtkIdType p2 = this->PointIds->GetId(v2);
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
int* vtkTetra::GetTriangleCases(int caseId)
{
  return const_cast<int*>(vtkMarchingCellsContourCases::GetTetraCase(caseId));
}

//------------------------------------------------------------------------------
vtkCell* vtkTetra::GetEdge(int edgeId)
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
vtkCell* vtkTetra::GetFace(int faceId)
{
  const vtkIdType* verts = ::Faces[faceId];
  for (int i = 0; i < 3; ++i)
  {
    this->Triangle->PointIds->SetId(i, this->PointIds->GetId(verts[i]));
    this->Triangle->Points->SetPoint(i, this->Points->GetPoint(verts[i]));
  }

  return this->Triangle;
}

//------------------------------------------------------------------------------
const vtkIdType* vtkTetra::GetEdgeArray(vtkIdType edgeId)
{
  assert(edgeId < vtkTetra::NumberOfEdges && "edgeId too large");
  return Edges[edgeId];
}

//------------------------------------------------------------------------------
const vtkIdType* vtkTetra::GetFaceArray(vtkIdType faceId)
{
  assert(faceId < vtkTetra::NumberOfFaces && "faceId too large");
  return Faces[faceId];
}

//------------------------------------------------------------------------------
const vtkIdType* vtkTetra::GetEdgeToAdjacentFacesArray(vtkIdType edgeId)
{
  assert(edgeId < vtkTetra::NumberOfEdges && "edgeId too large");
  return EdgeToAdjacentFaces[edgeId];
}

//------------------------------------------------------------------------------
const vtkIdType* vtkTetra::GetFaceToAdjacentFacesArray(vtkIdType faceId)
{
  assert(faceId < vtkTetra::NumberOfFaces && "faceId too large");
  return FaceToAdjacentFaces[faceId];
}

//------------------------------------------------------------------------------
const vtkIdType* vtkTetra::GetPointToIncidentEdgesArray(vtkIdType pointId)
{
  assert(pointId < vtkTetra::NumberOfPoints && "pointId too large");
  return PointToIncidentEdges[pointId];
}

//------------------------------------------------------------------------------
const vtkIdType* vtkTetra::GetPointToIncidentFacesArray(vtkIdType pointId)
{
  assert(pointId < vtkTetra::NumberOfPoints && "pointId too large");
  return PointToIncidentFaces[pointId];
}

//------------------------------------------------------------------------------
const vtkIdType* vtkTetra::GetPointToOneRingPointsArray(vtkIdType pointId)
{
  assert(pointId < vtkTetra::NumberOfPoints && "pointId too large");
  return PointToOneRingPoints[pointId];
}

//------------------------------------------------------------------------------
void vtkTetra::GetEdgePoints(vtkIdType edgeId, const vtkIdType*& pts)
{
  assert(edgeId < vtkTetra::NumberOfEdges && "edgeId too large");
  pts = this->GetEdgeArray(edgeId);
}

//------------------------------------------------------------------------------
vtkIdType vtkTetra::GetFacePoints(vtkIdType faceId, const vtkIdType*& pts)
{
  assert(faceId < vtkTetra::NumberOfFaces && "faceId too large");
  pts = this->GetFaceArray(faceId);
  return vtkTetra::MaximumFaceSize;
}

//------------------------------------------------------------------------------
void vtkTetra::GetEdgeToAdjacentFaces(vtkIdType edgeId, const vtkIdType*& pts)
{
  assert(edgeId < vtkTetra::NumberOfEdges && "edgeId too large");
  pts = EdgeToAdjacentFaces[edgeId];
}

//------------------------------------------------------------------------------
vtkIdType vtkTetra::GetFaceToAdjacentFaces(vtkIdType faceId, const vtkIdType*& faceIds)
{
  assert(faceId < vtkTetra::NumberOfFaces && "faceId too large");
  faceIds = FaceToAdjacentFaces[faceId];
  return vtkTetra::MaximumFaceSize;
}

//------------------------------------------------------------------------------
vtkIdType vtkTetra::GetPointToIncidentEdges(vtkIdType pointId, const vtkIdType*& edgeIds)
{
  assert(pointId < vtkTetra::NumberOfPoints && "pointId too large");
  edgeIds = PointToIncidentEdges[pointId];
  return vtkTetra::MaximumValence;
}

//------------------------------------------------------------------------------
vtkIdType vtkTetra::GetPointToIncidentFaces(vtkIdType pointId, const vtkIdType*& faceIds)
{
  assert(pointId < vtkTetra::NumberOfPoints && "pointId too large");
  faceIds = PointToIncidentFaces[pointId];
  return vtkTetra::MaximumValence;
}

//------------------------------------------------------------------------------
vtkIdType vtkTetra::GetPointToOneRingPoints(vtkIdType pointId, const vtkIdType*& pts)
{
  assert(pointId < vtkTetra::NumberOfPoints && "pointId too large");
  pts = PointToOneRingPoints[pointId];
  return vtkTetra::MaximumValence;
}

//------------------------------------------------------------------------------
//
// Intersect triangle Faces against line.
//
int vtkTetra::IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t,
  double x[3], double pcoords[3], int& subId)
{
  int intersection = 0;

  t = VTK_DOUBLE_MAX;
  for (int faceNum = 0; faceNum < 4; faceNum++)
  {
    vtkCell* face = this->GetFace(faceNum);

    double pcTemp[3], xTemp[3];
    double tTemp = VTK_DOUBLE_MAX;
    if (face->IntersectWithLine(p1, p2, tol, tTemp, xTemp, pcTemp, subId))
    {
      intersection = 1;
      if (tTemp < t)
      {
        t = tTemp;
        x[0] = xTemp[0];
        x[1] = xTemp[1];
        x[2] = xTemp[2];
        switch (faceNum)
        {
          case 0:
            pcoords[0] = pcTemp[0];
            pcoords[1] = 0.0;
            pcoords[2] = pcTemp[1];
            break;

          case 1:
            pcoords[0] = 1.0 - pcTemp[0] - pcTemp[1];
            pcoords[1] = pcTemp[0];
            pcoords[2] = pcTemp[1];
            break;

          case 2:
            pcoords[0] = 0.0;
            pcoords[1] = 1 - pcTemp[0] - pcTemp[1];
            pcoords[2] = pcTemp[1];
            break;

          case 3:
            pcoords[0] = pcTemp[0];
            pcoords[1] = pcTemp[1];
            pcoords[2] = pcTemp[2];
            break;
        }
      }
    }
  }
  return intersection;
}

//------------------------------------------------------------------------------
int vtkTetra::TriangulateLocalIds(int vtkNotUsed(index), vtkIdList* ptIds)
{
  ptIds->SetNumberOfIds(4);
  std::iota(ptIds->begin(), ptIds->end(), 0);
  return 1;
}

//------------------------------------------------------------------------------
void vtkTetra::Derivatives(int vtkNotUsed(subId), const double vtkNotUsed(pcoords)[3],
  const double* values, int dim, double* derivs)
{
  double *jI[3], j0[3], j1[3], j2[3];
  double functionDerivs[12], sum[3];

  // compute inverse Jacobian and interpolation function derivatives
  jI[0] = j0;
  jI[1] = j1;
  jI[2] = j2;
  this->JacobianInverse(jI, functionDerivs);

  // now compute derivates of values provided
  for (int k = 0; k < dim; k++) // loop over values per point
  {
    sum[0] = sum[1] = sum[2] = 0.0;
    for (int i = 0; i < 4; i++) // loop over interp. function derivatives
    {
      double value = values[dim * i + k];
      sum[0] += functionDerivs[i] * value;
      sum[1] += functionDerivs[4 + i] * value;
      sum[2] += functionDerivs[8 + i] * value;
    }

    for (int j = 0; j < 3; j++) // loop over derivative directions
    {
      derivs[3 * k + j] = sum[0] * jI[j][0] + sum[1] * jI[j][1] + sum[2] * jI[j][2];
    }
  }
}

//------------------------------------------------------------------------------
// Compute the center of the tetrahedron,
void vtkTetra::TetraCenter(double p1[3], double p2[3], double p3[3], double p4[3], double center[3])
{
  center[0] = (p1[0] + p2[0] + p3[0] + p4[0]) / 4.0;
  center[1] = (p1[1] + p2[1] + p3[1] + p4[1]) / 4.0;
  center[2] = (p1[2] + p2[2] + p3[2] + p4[2]) / 4.0;
}

//------------------------------------------------------------------------------
double vtkTetra::ComputeVolume(double p1[3], double p2[3], double p3[3], double p4[3])
{
  return (vtkMath::Determinant3x3(p2[0] - p1[0], p3[0] - p1[0], p4[0] - p1[0], p2[1] - p1[1],
            p3[1] - p1[1], p4[1] - p1[1], p2[2] - p1[2], p3[2] - p1[2], p4[2] - p1[2]) /
    6.0);
}

//------------------------------------------------------------------------------
// Compute the circumcenter (center[3]) and radius squared (method
// return value) of a tetrahedron defined by the four points x1, x2,
// x3, and x4.
double vtkTetra::Circumsphere(
  double x1[3], double x2[3], double x3[3], double x4[3], double center[3])
{
  // Closed-form circumcenter via Cramer's rule (no general-purpose
  // linear-system solver).
  const double a[3] = { x2[0] - x1[0], x2[1] - x1[1], x2[2] - x1[2] };
  const double b[3] = { x3[0] - x1[0], x3[1] - x1[1], x3[2] - x1[2] };
  const double c[3] = { x4[0] - x1[0], x4[1] - x1[1], x4[2] - x1[2] };

  const double ra = a[0] * a[0] + a[1] * a[1] + a[2] * a[2];
  const double rb = b[0] * b[0] + b[1] * b[1] + b[2] * b[2];
  const double rc = c[0] * c[0] + c[1] * c[1] + c[2] * c[2];

  const double bxc[3] = { b[1] * c[2] - b[2] * c[1], b[2] * c[0] - b[0] * c[2],
    b[0] * c[1] - b[1] * c[0] };
  const double det = 2.0 * (a[0] * bxc[0] + a[1] * bxc[1] + a[2] * bxc[2]);

  if (det == 0.0)
  {
    center[0] = center[1] = center[2] = 0.0;
    return VTK_DOUBLE_MAX;
  }
  const double invDet = 1.0 / det;

  const double cxa[3] = { c[1] * a[2] - c[2] * a[1], c[2] * a[0] - c[0] * a[2],
    c[0] * a[1] - c[1] * a[0] };
  const double axb[3] = { a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
    a[0] * b[1] - a[1] * b[0] };

  double off[3];
  for (int i = 0; i < 3; i++)
  {
    off[i] = (ra * bxc[i] + rb * cxa[i] + rc * axb[i]) * invDet;
    center[i] = off[i] + x1[i];
  }

  const double r2 = off[0] * off[0] + off[1] * off[1] + off[2] * off[2];
  return (r2 > VTK_DOUBLE_MAX) ? VTK_DOUBLE_MAX : r2;
}

//------------------------------------------------------------------------------
// Compute the incenter (center[3]) and radius (method return value) of
// a tetrahedron defined by the four points p1, p2, p3, and p4.
double vtkTetra::Insphere(double p1[3], double p2[3], double p3[3], double p4[3], double center[3])
{
  double u[3], v[3], w[3];
  double p[3], q[3], r[3];
  double O1[3], O2[3];
  double y[3], s[3], t;

  u[0] = p2[0] - p1[0];
  u[1] = p2[1] - p1[1];
  u[2] = p2[2] - p1[2];

  v[0] = p3[0] - p1[0];
  v[1] = p3[1] - p1[1];
  v[2] = p3[2] - p1[2];

  w[0] = p4[0] - p1[0];
  w[1] = p4[1] - p1[1];
  w[2] = p4[2] - p1[2];

  vtkMath::Cross(u, v, p);
  vtkMath::Normalize(p);

  vtkMath::Cross(v, w, q);
  vtkMath::Normalize(q);

  vtkMath::Cross(w, u, r);
  vtkMath::Normalize(r);

  O1[0] = p[0] - q[0];
  O1[1] = p[1] - q[1];
  O1[2] = p[2] - q[2];

  O2[0] = q[0] - r[0];
  O2[1] = q[1] - r[1];
  O2[2] = q[2] - r[2];

  vtkMath::Cross(O1, O2, y);

  O1[0] = u[0] - w[0];
  O1[1] = u[1] - w[1];
  O1[2] = u[2] - w[2];

  O2[0] = v[0] - w[0];
  O2[1] = v[1] - w[1];
  O2[2] = v[2] - w[2];

  vtkMath::Cross(O1, O2, s);
  vtkMath::Normalize(s);

  s[0] = -1 * s[0];
  s[1] = -1 * s[1];
  s[2] = -1 * s[2];

  O1[0] = s[0] - p[0];
  O1[1] = s[1] - p[1];
  O1[2] = s[2] - p[2];

  t = vtkMath::Dot(w, s) / vtkMath::Dot(y, O1);
  center[0] = p1[0] + (t * y[0]);
  center[1] = p1[1] + (t * y[1]);
  center[2] = p1[2] + (t * y[2]);

  return std::abs(t * vtkMath::Dot(y, p));
}

//------------------------------------------------------------------------------
// Given a 3D point x[3], determine the barycentric coordinates of the point.
// Barycentric coordinates are a natural coordinate system for simplices that
// express a position as a linear combination of the vertices. For a
// tetrahedron, there are four barycentric coordinates (because there are
// four vertices), and the sum of the coordinates must equal 1. If a
// point x is inside a simplex, then all four coordinates will be strictly
// positive.  If three coordinates are zero (so the fourth =1), then the
// point x is on a point. If two coordinates are zero, the point x is on an
// edge (and so on). In this method, you must specify the point coordinates
// x1->x4. Returns 0 if tetrahedron is degenerate.
int vtkTetra::BarycentricCoords(
  double x[3], double x1[3], double x2[3], double x3[3], double x4[3], double bcoords[4])
{
  // Closed-form barycentric coordinates via Cramer's rule (no
  // general-purpose linear-system solver).
  const double a[3] = { x2[0] - x1[0], x2[1] - x1[1], x2[2] - x1[2] };
  const double b[3] = { x3[0] - x1[0], x3[1] - x1[1], x3[2] - x1[2] };
  const double c[3] = { x4[0] - x1[0], x4[1] - x1[1], x4[2] - x1[2] };
  const double d[3] = { x[0] - x1[0], x[1] - x1[1], x[2] - x1[2] };

  const double bxc[3] = { b[1] * c[2] - b[2] * c[1], b[2] * c[0] - b[0] * c[2],
    b[0] * c[1] - b[1] * c[0] };
  const double det = a[0] * bxc[0] + a[1] * bxc[1] + a[2] * bxc[2];

  if (det == 0.0)
  {
    return 0;
  }
  const double invDet = 1.0 / det;

  bcoords[1] = (d[0] * bxc[0] + d[1] * bxc[1] + d[2] * bxc[2]) * invDet;

  const double dxc[3] = { d[1] * c[2] - d[2] * c[1], d[2] * c[0] - d[0] * c[2],
    d[0] * c[1] - d[1] * c[0] };
  bcoords[2] = (a[0] * dxc[0] + a[1] * dxc[1] + a[2] * dxc[2]) * invDet;

  const double bxd[3] = { b[1] * d[2] - b[2] * d[1], b[2] * d[0] - b[0] * d[2],
    b[0] * d[1] - b[1] * d[0] };
  bcoords[3] = (a[0] * bxd[0] + a[1] * bxd[1] + a[2] * bxd[2]) * invDet;

  bcoords[0] = 1.0 - bcoords[1] - bcoords[2] - bcoords[3];
  return 1;
}

//------------------------------------------------------------------------------
//
// Compute iso-parametric interpolation functions
//
void vtkTetra::InterpolationFunctions(const double pcoords[3], double sf[4])
{
  sf[0] = 1.0 - pcoords[0] - pcoords[1] - pcoords[2];
  sf[1] = pcoords[0];
  sf[2] = pcoords[1];
  sf[3] = pcoords[2];
}

//------------------------------------------------------------------------------
void vtkTetra::InterpolationDerivs(const double vtkNotUsed(pcoords)[3], double derivs[12])
{
  // r-derivatives
  derivs[0] = -1.0;
  derivs[1] = 1.0;
  derivs[2] = 0.0;
  derivs[3] = 0.0;

  // s-derivatives
  derivs[4] = -1.0;
  derivs[5] = 0.0;
  derivs[6] = 1.0;
  derivs[7] = 0.0;

  // t-derivatives
  derivs[8] = -1.0;
  derivs[9] = 0.0;
  derivs[10] = 0.0;
  derivs[11] = 1.0;
}

//------------------------------------------------------------------------------
// Given parametric coordinates compute inverse Jacobian transformation
// matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
// function derivatives. Returns 0 if no inverse exists.
int vtkTetra::JacobianInverse(double** inverse, double derivs[12])
{
  double *m[3], m0[3], m1[3], m2[3];
  double x[3];

  // compute interpolation function derivatives
  this->InterpolationDerivs(nullptr, derivs);

  // create Jacobian matrix
  m[0] = m0;
  m[1] = m1;
  m[2] = m2;
  for (int i = 0; i < 3; i++) // initialize matrix
  {
    m0[i] = m1[i] = m2[i] = 0.0;
  }

  for (int j = 0; j < 4; j++)
  {
    this->Points->GetPoint(j, x);
    for (int i = 0; i < 3; i++)
    {
      m0[i] += x[i] * derivs[j];
      m1[i] += x[i] * derivs[4 + j];
      m2[i] += x[i] * derivs[8 + j];
    }
  }

  // now find the inverse
  if (vtkMath::InvertMatrix(m, inverse, 3) == 0)
  {
    vtkErrorMacro(<< "Jacobian inverse not found"
                  << "Matrix:(" << m[0][0] << "," << m[0][1] << "," << m[0][2] << " " << m[1][0]
                  << "," << m[1][1] << "," << m[1][2] << " " << m[2][0] << "," << m[2][1] << ","
                  << m[2][2] << ")");
    return 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
// Clip this tetra using scalar value provided. Like contouring, except that
// it cuts the tetra to produce other 3D cells (note that this method will
// produce a single tetrahedra or a single wedge). The table has been
// carefully designed to ensure that face neighbors--after clipping--are
// remain compatible.
void vtkTetra::Clip(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
  vtkCellArray* tets, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd, int insideOut)
{
  vtkIdType pts[6];
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
    ? vtkMarchingCellsClipCases<true>::GetCellCase(VTK_TETRA, caseIndex)
    : vtkMarchingCellsClipCases<false>::GetCellCase(VTK_TETRA, caseIndex);
  using MCCases = vtkMarchingCellsClipCasesBase;
  const MCCases::EDGEIDXS* edgeVertices = MCCases::GetCellEdges(VTK_TETRA);
  const uint8_t numberOfOutputCells = *thisCase++;
  assert(numberOfOutputCells <= 1);
  if (numberOfOutputCells == 0)
  {
    return;
  }

  // generate clipped triangle
  /*shape = */ thisCase++; // VTK_TETRA/VTK_WEDGE
  const uint8_t numberOfCellPoints = *thisCase++;
  for (uint8_t i = 0; i < numberOfCellPoints; i++) // insert tetra/wedge
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

  int numUnique = 1;
  for (int i = 0; i < numberOfCellPoints - 1; i++)
  {
    assert(i < 6 && "The point index is out-of-range.");
    int allDifferent = 1;
    for (int j = i + 1; j < numberOfCellPoints && allDifferent && j < 6; j++)
    {
      assert(j < 6 && "The point index is out-of-range.");
      if (pts[i] == pts[j])
      {
        allDifferent = 0;
      }
    }
    if (allDifferent)
    {
      numUnique++;
    }
  }

  if (numberOfCellPoints == 4 && numUnique == 4) // check for degenerate tetra
  {
    const vtkIdType newCellId = tets->InsertNextCell(numberOfCellPoints, pts);
    if (outCd)
    {
      outCd->CopyData(inCd, cellId, newCellId);
    }
  }
  else if (numberOfCellPoints == 6 && numUnique > 3) // check for degenerate wedge
  {
    const vtkIdType newCellId = tets->InsertNextCell(numberOfCellPoints, pts);
    if (outCd)
    {
      outCd->CopyData(inCd, cellId, newCellId);
    }
  }
}

//------------------------------------------------------------------------------
double* vtkTetra::GetParametricCoords()
{
  return ParametricCoords;
}

//------------------------------------------------------------------------------
double vtkTetra::GetParametricDistance(const double pcoords[3])
{
  double pDist, pDistMax = 0.0;
  double pc[4];

  pc[0] = pcoords[0];
  pc[1] = pcoords[1];
  pc[2] = pcoords[2];
  pc[3] = 1.0 - pcoords[0] - pcoords[1] - pcoords[2];

  for (int i = 0; i < 4; i++)
  {
    if (pc[i] < 0.0)
    {
      pDist = -pc[i];
    }
    else if (pc[i] > 1.0)
    {
      pDist = pc[i] - 1.0;
    }
    else // inside the cell in the parametric direction
    {
      pDist = 0.0;
    }
    pDistMax = std::max(pDist, pDistMax);
  }

  return pDistMax;
}

//------------------------------------------------------------------------------
void vtkTetra::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Line:\n";
  this->Line->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Triangle:\n";
  this->Triangle->PrintSelf(os, indent.GetNextIndent());
}
VTK_ABI_NAMESPACE_END
