// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHexahedron.h"

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
#include "vtkPoints.h"
#include "vtkPolygon.h"
#include "vtkQuad.h"

#include <algorithm> //std::copy
#include <array>
#include <cassert>

namespace
{
//------------------------------------------------------------------------------
[[maybe_unused]] constexpr const char* Topology = R"(
   Hexahedron topology:

            7-----------6
           /|          /|
          / |         / |
         4-----------5  |
         |  |        |  |
         |  3--------|--2
         | /         | /
         |/          |/
         0-----------1

   Note: unlike Voxel, vertices are ordered sequentially around each
   face using the right-hand rule.
)";

//------------------------------------------------------------------------------
double ParametricCoords[24] = {
  0.0, 0.0, 0.0, //
  1.0, 0.0, 0.0, //
  1.0, 1.0, 0.0, //
  0.0, 1.0, 0.0, //
  0.0, 0.0, 1.0, //
  1.0, 0.0, 1.0, //
  1.0, 1.0, 1.0, //
  0.0, 1.0, 1.0  //
};

//------------------------------------------------------------------------------
constexpr vtkIdType Edges[vtkHexahedron::NumberOfEdges][2] = {
  { 0, 1 }, // 0
  { 1, 2 }, // 1
  { 3, 2 }, // 2
  { 0, 3 }, // 3
  { 4, 5 }, // 4
  { 5, 6 }, // 5
  { 7, 6 }, // 6
  { 4, 7 }, // 7
  { 0, 4 }, // 8
  { 1, 5 }, // 9
  { 3, 7 }, // 10
  { 2, 6 }, // 11
};

//------------------------------------------------------------------------------
constexpr vtkIdType Faces[vtkHexahedron::NumberOfFaces][vtkHexahedron::MaximumFaceSize + 1] = {
  { 0, 4, 7, 3, -1 }, // 0
  { 1, 2, 6, 5, -1 }, // 1
  { 0, 1, 5, 4, -1 }, // 2
  { 3, 7, 6, 2, -1 }, // 3
  { 0, 3, 2, 1, -1 }, // 4
  { 4, 5, 6, 7, -1 }, // 5
};

//------------------------------------------------------------------------------
constexpr vtkIdType EdgeToAdjacentFaces[vtkHexahedron::NumberOfEdges][2] = {
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
constexpr vtkIdType FaceToAdjacentFaces[vtkHexahedron::NumberOfFaces]
                                       [vtkHexahedron::MaximumFaceSize] = {
                                         { 4, 2, 5, 3 }, // 0
                                         { 4, 3, 5, 2 }, // 1
                                         { 4, 1, 5, 0 }, // 2
                                         { 0, 5, 1, 4 }, // 3
                                         { 0, 3, 1, 2 }, // 4
                                         { 2, 1, 0, 3 }, // 5
                                       };

//------------------------------------------------------------------------------
constexpr vtkIdType PointToIncidentEdges[vtkHexahedron::NumberOfPoints]
                                        [vtkHexahedron::MaximumValence] = {
                                          { 0, 8, 3 },  // 0
                                          { 0, 1, 9 },  // 1
                                          { 1, 2, 11 }, // 2
                                          { 2, 3, 10 }, // 3
                                          { 7, 8, 4 },  // 4
                                          { 4, 9, 5 },  // 5
                                          { 5, 11, 6 }, // 6
                                          { 6, 10, 7 }, // 7
                                        };

//------------------------------------------------------------------------------
constexpr vtkIdType PointToIncidentFaces[vtkHexahedron::NumberOfPoints]
                                        [vtkHexahedron::MaximumValence] = {
                                          { 2, 0, 4 }, // 0
                                          { 4, 1, 2 }, // 1
                                          { 4, 3, 1 }, // 2
                                          { 4, 0, 3 }, // 3
                                          { 5, 2, 0 }, // 4
                                          { 2, 1, 5 }, // 5
                                          { 1, 3, 5 }, // 6
                                          { 3, 0, 5 }, // 7
                                        };

//------------------------------------------------------------------------------
constexpr vtkIdType PointToOneRingPoints[vtkHexahedron::NumberOfPoints]
                                        [vtkHexahedron::MaximumValence] = {
                                          { 1, 4, 3 }, // 0
                                          { 0, 2, 5 }, // 1
                                          { 1, 3, 6 }, // 2
                                          { 2, 0, 7 }, // 3
                                          { 5, 7, 0 }, // 4
                                          { 4, 1, 6 }, // 5
                                          { 5, 2, 7 }, // 6
                                          { 6, 3, 4 }, // 7
                                        };

constexpr double VTK_DIVERGED = 1.e6;
constexpr int VTK_MAX_ITERATIONS = 20;
constexpr double VTK_CONVERGED = 1.e-04;
constexpr double VTK_OUTSIDE_CELL_TOLERANCE = 1.e-06;
}

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHexahedron);
//------------------------------------------------------------------------------
// Construct the hexahedron with eight points.
vtkHexahedron::vtkHexahedron()
{
  this->Points->SetNumberOfPoints(8);
  this->PointIds->SetNumberOfIds(8);

  for (int i = 0; i < 8; i++)
  {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i, 0);
  }
  this->Line = vtkSmartPointer<vtkLine>::New();
  this->Quad = vtkSmartPointer<vtkQuad>::New();
}

//------------------------------------------------------------------------------
//  Method to calculate parametric coordinates in an eight noded
//  linear hexahedron element from global coordinates.
//
int vtkHexahedron::EvaluatePosition(const double x[3], double closestPoint[3], int& subId,
  double pcoords[3], double& dist2, double weights[])
{
  double params[3];
  double derivs[24];

  // Efficient point access
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return 0;
  }
  const double* pts = pointsArray->GetPointer(0);

  // compute a bound on the volume to get a scale for an acceptable determinant
  constexpr vtkIdType diagonals[4][2] = { { 0, 6 }, { 1, 7 }, { 2, 4 }, { 3, 5 } };
  double longestDiagonal = 0;
  for (int i = 0; i < 4; i++)
  {
    const double* pt0 = pts + 3 * diagonals[i][0];
    const double* pt1 = pts + 3 * diagonals[i][1];
    double d2 = vtkMath::Distance2BetweenPoints(pt0, pt1);
    longestDiagonal = std::max(longestDiagonal, d2);
  }
  // longestDiagonal value is already squared
  double volumeBound = longestDiagonal * std::sqrt(longestDiagonal);
  double determinantTolerance = 1e-20 < .00001 * volumeBound ? 1e-20 : .00001 * volumeBound;

  //  set initial position for Newton's method: seed it with the exact solution of
  //  the affine (parallelepiped) approximation defined by the edges out of vertex 0.
  //  For a non-warped or mildly-warped hex this lands very close to the true answer
  //  and noticeably cuts the number of Newton iterations below; for a degenerate
  //  triple of edges, fall back to the cell center.
  subId = 0;
  {
    const double* p0 = pts;
    double c1[3], c2[3], c3[3], p[3];
    vtkMath::Subtract(pts + 3, p0, c1);
    vtkMath::Subtract(pts + 9, p0, c2);
    vtkMath::Subtract(pts + 12, p0, c3);
    vtkMath::Subtract(x, p0, p);
    const double detGuess = vtkMath::Determinant3x3(c1, c2, c3);
    if (detGuess != 0.0)
    {
      pcoords[0] = vtkMath::Determinant3x3(p, c2, c3) / detGuess;
      pcoords[1] = vtkMath::Determinant3x3(c1, p, c3) / detGuess;
      pcoords[2] = vtkMath::Determinant3x3(c1, c2, p) / detGuess;
    }
    else
    {
      pcoords[0] = pcoords[1] = pcoords[2] = 0.5;
    }
  }
  params[0] = pcoords[0];
  params[1] = pcoords[1];
  params[2] = pcoords[2];

  //  enter iteration loop
  int converged = 0;
  for (int iteration = 0; !converged && iteration < VTK_MAX_ITERATIONS; ++iteration)
  {
    //  calculate element interpolation functions and derivatives
    vtkHexahedron::InterpolationFunctions(pcoords, weights);
    vtkHexahedron::InterpolationDerivs(pcoords, derivs);

    //  calculate newton functions
    double fcol[3] = { 0, 0, 0 }, rcol[3] = { 0, 0, 0 }, scol[3] = { 0, 0, 0 },
           tcol[3] = { 0, 0, 0 };
    for (int i = 0; i < 8; i++)
    {
      for (int j = 0; j < 3; j++)
      {
        const double coord = pts[3 * i + j];
        fcol[j] += coord * weights[i];
        rcol[j] += coord * derivs[i];
        scol[j] += coord * derivs[i + 8];
        tcol[j] += coord * derivs[i + 16];
      }
    }

    for (int i = 0; i < 3; i++)
    {
      fcol[i] -= x[i];
    }

    //  compute determinants and generate improvements
    double d = vtkMath::Determinant3x3(rcol, scol, tcol);
    if (std::abs(d) < determinantTolerance)
    {
      return -1;
    }

    pcoords[0] = params[0] - vtkMath::Determinant3x3(fcol, scol, tcol) / d;
    pcoords[1] = params[1] - vtkMath::Determinant3x3(rcol, fcol, tcol) / d;
    pcoords[2] = params[2] - vtkMath::Determinant3x3(rcol, scol, fcol) / d;

    //  check for convergence
    if (std::abs(pcoords[0] - params[0]) < VTK_CONVERGED &&
      std::abs(pcoords[1] - params[1]) < VTK_CONVERGED &&
      std::abs(pcoords[2] - params[2]) < VTK_CONVERGED)
    {
      converged = 1;
    }
    // Test for bad divergence (S.Hirschberg 11.12.2001)
    else if (std::abs(pcoords[0]) > VTK_DIVERGED || std::abs(pcoords[1]) > VTK_DIVERGED ||
      std::abs(pcoords[2]) > VTK_DIVERGED)
    {
      return -1;
    }
    //  if not converged, repeat
    else
    {
      params[0] = pcoords[0];
      params[1] = pcoords[1];
      params[2] = pcoords[2];
    }
  }

  //  if not converged, set the parametric coordinates to arbitrary values
  //  outside of element
  if (!converged)
  {
    return -1;
  }

  vtkHexahedron::InterpolationFunctions(pcoords, weights);

  double lowerlimit = 0.0 - VTK_OUTSIDE_CELL_TOLERANCE;
  double upperlimit = 1.0 + VTK_OUTSIDE_CELL_TOLERANCE;
  if (pcoords[0] >= lowerlimit && pcoords[0] <= upperlimit && pcoords[1] >= lowerlimit &&
    pcoords[1] <= upperlimit && pcoords[2] >= lowerlimit && pcoords[2] <= upperlimit)
  {
    if (closestPoint)
    {
      closestPoint[0] = x[0];
      closestPoint[1] = x[1];
      closestPoint[2] = x[2];
      dist2 = 0.0; // inside hexahedron
    }
    return 1;
  }
  else
  {
    double pc[3], w[8];
    if (closestPoint)
    {
      for (int i = 0; i < 3; i++) // only approximate, not really true for warped hexa
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
      this->EvaluateLocation(subId, pc, closestPoint, w);
      dist2 = vtkMath::Distance2BetweenPoints(closestPoint, x);
    }
    return 0;
  }
}

//------------------------------------------------------------------------------
// Compute iso-parametric interpolation functions
//
void vtkHexahedron::InterpolationFunctions(const double pcoords[3], double sf[8])
{
  double rm = 1. - pcoords[0];
  double sm = 1. - pcoords[1];
  double tm = 1. - pcoords[2];

  const auto rmXsm = rm * sm;
  const auto p0Xsm = pcoords[0] * sm;
  const auto p0Xp1 = pcoords[0] * pcoords[1];
  const auto rmXp1 = rm * pcoords[1];

  sf[0] = rmXsm * tm;
  sf[1] = p0Xsm * tm;
  sf[2] = p0Xp1 * tm;
  sf[3] = rmXp1 * tm;
  sf[4] = rmXsm * pcoords[2];
  sf[5] = p0Xsm * pcoords[2];
  sf[6] = p0Xp1 * pcoords[2];
  sf[7] = rmXp1 * pcoords[2];
}

//------------------------------------------------------------------------------
void vtkHexahedron::InterpolationDerivs(const double pcoords[3], double derivs[24])
{

  double rm = 1. - pcoords[0];
  double sm = 1. - pcoords[1];
  double tm = 1. - pcoords[2];

  // r-derivatives
  derivs[0] = -sm * tm;
  derivs[1] = -derivs[0];
  derivs[2] = pcoords[1] * tm;
  derivs[3] = -derivs[2];
  derivs[4] = -sm * pcoords[2];
  derivs[5] = -derivs[4];
  derivs[6] = pcoords[1] * pcoords[2];
  derivs[7] = -derivs[6];

  // s-derivatives
  derivs[8] = -rm * tm;
  derivs[9] = -pcoords[0] * tm;
  derivs[10] = -derivs[9];
  derivs[11] = -derivs[8];
  derivs[12] = -rm * pcoords[2];
  derivs[13] = -pcoords[0] * pcoords[2];
  derivs[14] = -derivs[13];
  derivs[15] = -derivs[12];

  // t-derivatives
  derivs[16] = -rm * sm;
  derivs[17] = -pcoords[0] * sm;
  derivs[18] = -pcoords[0] * pcoords[1];
  derivs[19] = -rm * pcoords[1];
  derivs[20] = -derivs[16];
  derivs[21] = -derivs[17];
  derivs[22] = -derivs[18];
  derivs[23] = -derivs[19];
}

//------------------------------------------------------------------------------
void vtkHexahedron::EvaluateLocation(
  int& vtkNotUsed(subId), const double pcoords[3], double x[3], double* weights)
{

  this->InterpolationFunctions(pcoords, weights);

  // Efficient point access
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return;
  }
  const double* pts = pointsArray->GetPointer(0);

  x[0] = x[1] = x[2] = 0.0;
  for (int i = 0; i < 8; i++)
  {
    const double* pt = pts + 3 * i;
    for (int j = 0; j < 3; j++)
    {
      x[j] += pt[j] * weights[i];
    }
  }
}

//------------------------------------------------------------------------------
int vtkHexahedron::CellBoundary(int vtkNotUsed(subId), const double pcoords[3], vtkIdList* pts)
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
    pts->SetId(2, this->PointIds->GetId(2));
    pts->SetId(3, this->PointIds->GetId(3));
  }

  else if (t1 >= 0.0 && t2 < 0.0 && t5 < 0.0 && t6 < 0.0)
  {
    pts->SetId(0, this->PointIds->GetId(1));
    pts->SetId(1, this->PointIds->GetId(2));
    pts->SetId(2, this->PointIds->GetId(6));
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
    pts->SetId(2, this->PointIds->GetId(6));
    pts->SetId(3, this->PointIds->GetId(7));
  }

  else if (t1 < 0.0 && t2 >= 0.0 && t5 >= 0.0 && t6 >= 0.0)
  {
    pts->SetId(0, this->PointIds->GetId(0));
    pts->SetId(1, this->PointIds->GetId(4));
    pts->SetId(2, this->PointIds->GetId(7));
    pts->SetId(3, this->PointIds->GetId(3));
  }

  else // if ( t1 < 0.0 && t2 < 0.0 && t3 >= 0.0 && t6 < 0.0 )
  {
    pts->SetId(0, this->PointIds->GetId(2));
    pts->SetId(1, this->PointIds->GetId(3));
    pts->SetId(2, this->PointIds->GetId(7));
    pts->SetId(3, this->PointIds->GetId(6));
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

//------------------------------------------------------------------------------
bool vtkHexahedron::GetCentroid(double centroid[3]) const
{
  return vtkHexahedron::ComputeCentroid(this->Points, nullptr, centroid);
}

//------------------------------------------------------------------------------
bool vtkHexahedron::ComputeCentroid(
  vtkPoints* points, const vtkIdType* pointIds, double centroid[3])
{
  double p[3];
  if (pointIds)
  {
    vtkIdType facePointIds[4] = { pointIds[Faces[0][0]], pointIds[Faces[0][1]],
      pointIds[Faces[0][2]], pointIds[Faces[0][3]] };
    vtkPolygon::ComputeCentroid(points, vtkHexahedron::MaximumFaceSize, facePointIds, centroid);
    facePointIds[0] = pointIds[Faces[1][0]];
    facePointIds[1] = pointIds[Faces[1][1]];
    facePointIds[2] = pointIds[Faces[1][2]];
    facePointIds[3] = pointIds[Faces[1][3]];
    vtkPolygon::ComputeCentroid(points, vtkHexahedron::MaximumFaceSize, facePointIds, p);
  }
  else
  {
    vtkPolygon::ComputeCentroid(points, vtkHexahedron::MaximumFaceSize, Faces[0], centroid);
    vtkPolygon::ComputeCentroid(points, vtkHexahedron::MaximumFaceSize, Faces[1], p);
  }
  centroid[0] += p[0];
  centroid[1] += p[1];
  centroid[2] += p[2];
  centroid[0] *= 0.5;
  centroid[1] *= 0.5;
  centroid[2] *= 0.5;
  return true;
}

void vtkHexahedron::Contour(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* verts, vtkCellArray* lines,
  vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd)
{
  static const int CASE_MASK[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };
  int v1, v2;
  vtkIdType pts[3];
  double x1[3], x2[3], x[3];
  vtkIdType offset = verts->GetNumberOfCells() + lines->GetNumberOfCells();

  // Build the case table
  int index = 0;
  for (int i = 0; i < 8; i++)
  {
    if (cellScalars->GetComponent(i, 0) >= value)
    {
      index |= CASE_MASK[i];
    }
  }

  const int* edge = vtkMarchingCellsContourCases::GetHexahedronCase(index);

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

      // linear interpolation
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
int* vtkHexahedron::GetTriangleCases(int caseId)
{
  return const_cast<int*>(vtkMarchingCellsContourCases::GetHexahedronCase(caseId));
}

//------------------------------------------------------------------------------
void vtkHexahedron::Clip(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* connectivity, vtkPointData* inPd,
  vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd, int insideOut)
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
    ? vtkMarchingCellsClipCases<true>::GetCellCase(VTK_HEXAHEDRON, caseIndex)
    : vtkMarchingCellsClipCases<false>::GetCellCase(VTK_HEXAHEDRON, caseIndex);
  using MCCases = vtkMarchingCellsClipCasesBase;
  const MCCases::EDGEIDXS* edgeVertices = MCCases::GetCellEdges(VTK_HEXAHEDRON);
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
vtkCell* vtkHexahedron::GetEdge(int edgeId)
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
vtkCell* vtkHexahedron::GetFace(int faceId)
{
  const vtkIdType* verts = Faces[faceId];

  for (int i = 0; i < 4; i++)
  {
    this->Quad->PointIds->SetId(i, this->PointIds->GetId(verts[i]));
    this->Quad->Points->SetPoint(i, this->Points->GetPoint(verts[i]));
  }

  return this->Quad;
}

//------------------------------------------------------------------------------
const vtkIdType* vtkHexahedron::GetEdgeArray(vtkIdType edgeId)
{
  assert(edgeId < vtkHexahedron::NumberOfEdges && "edgeId too large");
  return Edges[edgeId];
}

//------------------------------------------------------------------------------
const vtkIdType* vtkHexahedron::GetFaceArray(vtkIdType faceId)
{
  assert(faceId < vtkHexahedron::NumberOfFaces && "faceId too large");
  return Faces[faceId];
}

//------------------------------------------------------------------------------
const vtkIdType* vtkHexahedron::GetEdgeToAdjacentFacesArray(vtkIdType edgeId)
{
  assert(edgeId < vtkHexahedron::NumberOfEdges && "edgeId too large");
  return EdgeToAdjacentFaces[edgeId];
}

//------------------------------------------------------------------------------
const vtkIdType* vtkHexahedron::GetFaceToAdjacentFacesArray(vtkIdType faceId)
{
  assert(faceId < vtkHexahedron::NumberOfFaces && "faceId too large");
  return FaceToAdjacentFaces[faceId];
}

//------------------------------------------------------------------------------
const vtkIdType* vtkHexahedron::GetPointToIncidentEdgesArray(vtkIdType pointId)
{
  assert(pointId < vtkHexahedron::NumberOfPoints && "pointId too large");
  return PointToIncidentEdges[pointId];
}

//------------------------------------------------------------------------------
const vtkIdType* vtkHexahedron::GetPointToIncidentFacesArray(vtkIdType pointId)
{
  assert(pointId < vtkHexahedron::NumberOfPoints && "pointId too large");
  return PointToIncidentFaces[pointId];
}

//------------------------------------------------------------------------------
const vtkIdType* vtkHexahedron::GetPointToOneRingPointsArray(vtkIdType pointId)
{
  assert(pointId < vtkHexahedron::NumberOfPoints && "pointId too large");
  return PointToOneRingPoints[pointId];
}

//------------------------------------------------------------------------------
void vtkHexahedron::GetEdgePoints(vtkIdType edgeId, const vtkIdType*& pts)
{
  assert(edgeId < vtkHexahedron::NumberOfEdges && "edgeId too large");
  pts = this->GetEdgeArray(edgeId);
}

//------------------------------------------------------------------------------
vtkIdType vtkHexahedron::GetFacePoints(vtkIdType faceId, const vtkIdType*& pts)
{
  assert(faceId < vtkHexahedron::NumberOfFaces && "faceId too large");
  pts = this->GetFaceArray(faceId);
  return vtkHexahedron::MaximumFaceSize;
}

//------------------------------------------------------------------------------
void vtkHexahedron::GetEdgeToAdjacentFaces(vtkIdType edgeId, const vtkIdType*& pts)
{
  assert(edgeId < vtkHexahedron::NumberOfEdges && "edgeId too large");
  pts = EdgeToAdjacentFaces[edgeId];
}

//------------------------------------------------------------------------------
vtkIdType vtkHexahedron::GetFaceToAdjacentFaces(vtkIdType faceId, const vtkIdType*& faceIds)
{
  assert(faceId < vtkHexahedron::NumberOfFaces && "faceId too large");
  faceIds = FaceToAdjacentFaces[faceId];
  return vtkHexahedron::MaximumFaceSize;
}

//------------------------------------------------------------------------------
vtkIdType vtkHexahedron::GetPointToIncidentEdges(vtkIdType pointId, const vtkIdType*& edgeIds)
{
  assert(pointId < vtkHexahedron::NumberOfPoints && "pointId too large");
  edgeIds = PointToIncidentEdges[pointId];
  return vtkHexahedron::MaximumValence;
}

//------------------------------------------------------------------------------
vtkIdType vtkHexahedron::GetPointToIncidentFaces(vtkIdType pointId, const vtkIdType*& faceIds)
{
  assert(pointId < vtkHexahedron::NumberOfPoints && "pointId too large");
  faceIds = PointToIncidentFaces[pointId];
  return vtkHexahedron::MaximumValence;
}

//------------------------------------------------------------------------------
vtkIdType vtkHexahedron::GetPointToOneRingPoints(vtkIdType pointId, const vtkIdType*& pts)
{
  assert(pointId < vtkHexahedron::NumberOfPoints && "pointId too large");
  pts = PointToOneRingPoints[pointId];
  return vtkHexahedron::MaximumValence;
}

//------------------------------------------------------------------------------
//
// Intersect hexa faces against line. Each hexa face is a quadrilateral.
//
int vtkHexahedron::IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t,
  double x[3], double pcoords[3], int& subId)
{
  int intersection = 0;
  double pt1[3], pt2[3], pt3[3], pt4[3];
  double tTemp;
  double pc[3], xTemp[3];

  t = VTK_DOUBLE_MAX;
  for (int faceNum = 0; faceNum < 6; faceNum++)
  {
    this->Points->GetPoint(Faces[faceNum][0], pt1);
    this->Points->GetPoint(Faces[faceNum][1], pt2);
    this->Points->GetPoint(Faces[faceNum][2], pt3);
    this->Points->GetPoint(Faces[faceNum][3], pt4);

    this->Quad->Points->SetPoint(0, pt1);
    this->Quad->Points->SetPoint(1, pt2);
    this->Quad->Points->SetPoint(2, pt3);
    this->Quad->Points->SetPoint(3, pt4);

    if (this->Quad->IntersectWithLine(p1, p2, tol, tTemp, xTemp, pc, subId))
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
            pcoords[0] = 0.0;
            pcoords[1] = pc[0];
            pcoords[2] = 0.0;
            break;

          case 1:
            pcoords[0] = 1.0;
            pcoords[1] = pc[0];
            pcoords[2] = 0.0;
            break;

          case 2:
            pcoords[0] = pc[0];
            pcoords[1] = 0.0;
            pcoords[2] = pc[1];
            break;

          case 3:
            pcoords[0] = pc[0];
            pcoords[1] = 1.0;
            pcoords[2] = pc[1];
            break;

          case 4:
            pcoords[0] = pc[0];
            pcoords[1] = pc[1];
            pcoords[2] = 0.0;
            break;

          case 5:
            pcoords[0] = pc[0];
            pcoords[1] = pc[1];
            pcoords[2] = 1.0;
            break;
        }
      }
    }
  }
  return intersection;
}

//------------------------------------------------------------------------------
int vtkHexahedron::TriangulateLocalIds(int index, vtkIdList* ptIds)
{
  // Create five tetrahedron. Triangulation varies depending upon index. This
  // is necessary to ensure compatible voxel triangulations.
  ptIds->SetNumberOfIds(20);
  if (index % 2)
  {
    constexpr std::array<vtkIdType, 20> localPtIds{ 0, 1, 3, 4, 1, 4, 5, 6, 1, 4, 6, 3, 1, 3, 6, 2,
      3, 6, 7, 4 };
    std::copy(localPtIds.begin(), localPtIds.end(), ptIds->begin());
  }
  else
  {
    constexpr std::array<vtkIdType, 20> localPtIds{ 2, 1, 5, 0, 0, 2, 3, 7, 2, 5, 6, 7, 0, 7, 4, 5,
      0, 2, 7, 5 };
    std::copy(localPtIds.begin(), localPtIds.end(), ptIds->begin());
  }
  return 1;
}

//------------------------------------------------------------------------------
// Compute derivatives in x-y-z directions. Use chain rule in combination
// with interpolation function derivatives.
//
void vtkHexahedron::Derivatives(
  int vtkNotUsed(subId), const double pcoords[3], const double* values, int dim, double* derivs)
{
  double *jI[3], j0[3], j1[3], j2[3];
  double functionDerivs[24], sum[3];

  // compute inverse Jacobian and interpolation function derivatives
  jI[0] = j0;
  jI[1] = j1;
  jI[2] = j2;
  this->JacobianInverse(pcoords, jI, functionDerivs);

  // now compute derivates of values provided
  for (int k = 0; k < dim; k++) // loop over values per point
  {
    sum[0] = sum[1] = sum[2] = 0.0;
    for (int i = 0; i < 8; i++) // loop over interp. function derivatives
    {
      sum[0] += functionDerivs[i] * values[dim * i + k];
      sum[1] += functionDerivs[8 + i] * values[dim * i + k];
      sum[2] += functionDerivs[16 + i] * values[dim * i + k];
    }
    for (int j = 0; j < 3; j++) // loop over derivative directions
    {
      derivs[3 * k + j] = sum[0] * jI[j][0] + sum[1] * jI[j][1] + sum[2] * jI[j][2];
    }
  }
}

//------------------------------------------------------------------------------
// Given parametric coordinates compute inverse Jacobian transformation
// matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
// function derivatives.
void vtkHexahedron::JacobianInverse(const double pcoords[3], double** inverse, double derivs[24])
{
  double *m[3], m0[3], m1[3], m2[3];
  double x[3];

  // compute interpolation function derivatives
  this->InterpolationDerivs(pcoords, derivs);

  // create Jacobian matrix
  m[0] = m0;
  m[1] = m1;
  m[2] = m2;
  for (int i = 0; i < 3; i++) // initialize matrix
  {
    m0[i] = m1[i] = m2[i] = 0.0;
  }

  for (int j = 0; j < 8; j++)
  {
    this->Points->GetPoint(j, x);
    for (int i = 0; i < 3; i++)
    {
      m0[i] += x[i] * derivs[j];
      m1[i] += x[i] * derivs[8 + j];
      m2[i] += x[i] * derivs[16 + j];
    }
  }

  // now find the inverse
  if (vtkMath::InvertMatrix(m, inverse, 3) == 0)
  {
    vtkErrorMacro(<< "Jacobian inverse not found");
    return;
  }
}

//------------------------------------------------------------------------------
double* vtkHexahedron::GetParametricCoords()
{
  return ParametricCoords;
}

//------------------------------------------------------------------------------
void vtkHexahedron::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Line:\n";
  this->Line->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Quad:\n";
  this->Quad->PrintSelf(os, indent.GetNextIndent());
}
VTK_ABI_NAMESPACE_END
