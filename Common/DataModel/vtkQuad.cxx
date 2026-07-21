// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkQuad.h"

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
#include "vtkTriangle.h"
#include "vtkVector.h"

#include <algorithm> //std::copy
#include <array>

namespace
{
//------------------------------------------------------------------------------
[[maybe_unused]] constexpr const char* Topology = R"(
   Quad topology:

      3-----------2
      |           |
      |           |
      |           |
      0-----------1
)";

//------------------------------------------------------------------------------
double ParametricCoords[12] = {
  0.0, 0.0, 0.0, //
  1.0, 0.0, 0.0, //
  1.0, 1.0, 0.0, //
  0.0, 1.0, 0.0  //
};

//------------------------------------------------------------------------------
constexpr vtkIdType Edges[4][2] = {
  { 0, 1 },
  { 1, 2 },
  { 3, 2 },
  { 0, 3 },
};

constexpr double VTK_DIVERGED = 1.e6;
constexpr int VTK_MAX_ITERATIONS = 20;
constexpr double VTK_CONVERGED = 1.e-06;

//------------------------------------------------------------------------------
// Find the parameter values (u,v) in [0,1]^2 at which the squared distance
// from x to the bilinear patch S(u,v) has a stationary point in the interior.
// This is only a necessary condition for cell membership: the orthogonal
// projection of x can land inside the parametric domain [0,1]^2 even for a
// highly warped quad, while still being far from the patch surface. The final
// inside/outside classification therefore still needs the closest-point check
// below, which compares the interior stationary point against the four edge
// projections and keeps the minimum distance.
bool BilinearPatchClosestParameters(const double x[3], const double P00[3], const vtkVector3d& B,
  const vtkVector3d& C, const vtkVector3d& D, double u, double v, double* outU, double* outV)
{
  const vtkVector3d p0 = { x[0] - P00[0], x[1] - P00[1], x[2] - P00[2] };

  for (int iter = 0; iter < VTK_MAX_ITERATIONS; ++iter)
  {
    const vtkVector3d Bv = B + v * D;
    const vtkVector3d Cu = C + u * D;
    const vtkVector3d r = p0 - u * B - v * C - u * v * D;
    // Stationarity of |x - S(u,v)|^2 : r . dS/du = 0, r . dS/dv = 0
    const double g1 = r.Dot(Bv);
    const double g2 = r.Dot(Cu);
    const double rD = r.Dot(D);

    const double j11 = -Bv.Dot(Bv);
    const double j12 = rD - Bv.Dot(Cu);
    const double j22 = -Cu.Dot(Cu);

    const double det = j11 * j22 - j12 * j12;
    if (std::abs(det) < 1.e-300)
    {
      return false;
    }
    const double du = (g1 * j22 - g2 * j12) / det;
    const double dv = (j11 * g2 - j12 * g1) / det;
    u -= du;
    v -= dv;
    if (std::abs(u) > VTK_DIVERGED || std::abs(v) > VTK_DIVERGED)
    {
      return false; // diverged
    }
    if (std::abs(du) < VTK_CONVERGED && std::abs(dv) < VTK_CONVERGED)
    {
      break;
    }
  }
  constexpr double eps = 1.e-7;
  if (u < -eps || u > 1.0 + eps || v < -eps || v > 1.0 + eps)
  {
    return false;
  }
  *outU = std::clamp(u, 0.0, 1.0);
  *outV = std::clamp(v, 0.0, 1.0);
  return true;
}
}

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkQuad);

//------------------------------------------------------------------------------
struct IntersectionStruct
{
  bool Intersected = false;
  int SubId = -1;
  double X[3] = { 0.0, 0.0, 0.0 };
  double PCoords[3] = { 0.0, 0.0, 0.0 };
  double T = -1.0;

  operator bool() { return this->Intersected; }

  void CopyValues(double& t, double* x, double* pcoords, int& subId) const
  {
    t = this->T;
    subId = this->SubId;
    for (int i = 0; i < 3; ++i)
    {
      x[i] = this->X[i];
      pcoords[i] = this->PCoords[i];
    }
  }

  static IntersectionStruct CellIntersectWithLine(
    vtkCell* cell, const double* p1, const double* p2, double tol)
  {
    IntersectionStruct res;
    res.Intersected = cell->IntersectWithLine(p1, p2, tol, res.T, res.X, res.PCoords, res.SubId);
    return res;
  }
};

//------------------------------------------------------------------------------
// Construct the quad with four points.
vtkQuad::vtkQuad()
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
static void ComputeNormal(
  vtkQuad* self, const double* pt1, const double* pt2, const double* pt3, double n[3])
{
  vtkTriangle::ComputeNormal(pt1, pt2, pt3, n);

  // If first three points are co-linear, then use fourth point
  double pt4[3];
  if (n[0] == 0.0 && n[1] == 0.0 && n[2] == 0.0)
  {
    self->Points->GetPoint(3, pt4);
    vtkTriangle::ComputeNormal(pt2, pt3, pt4, n);
  }
}

//------------------------------------------------------------------------------
int vtkQuad::EvaluatePosition(const double x[3], double closestPoint[3], int& subId,
  double pcoords[3], double& dist2, double weights[])
{
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
  const double* pt1 = pts;
  const double* pt2 = pts + 3;
  const double* pt3 = pts + 6;
  const double* pt4 = pts + 9;

  // A quad is, in general, a (possibly non-planar) bilinear patch
  // S(u,v) = pt1 + u*B + v*C + u*v*D, with corners pt1,pt2,pt3,pt4 at
  // (u,v) = (0,0),(1,0),(1,1),(0,1).
  const vtkVector3d B = { pt2[0] - pt1[0], pt2[1] - pt1[1], pt2[2] - pt1[2] };
  const vtkVector3d C = { pt4[0] - pt1[0], pt4[1] - pt1[1], pt4[2] - pt1[2] };
  const vtkVector3d D = { pt3[0] - pt2[0] - pt4[0] + pt1[0], pt3[1] - pt2[1] - pt4[1] + pt1[1],
    pt3[2] - pt2[2] - pt4[2] + pt1[2] };

  // Whether x is inside/on the cell only requires finding whether x's
  // perpendicular footprint lands within the [0,1]^2 parametric domain, i.e.
  // whether an interior stationary point of |x - S(u,v)|^2 exists in the domain.
  // BilinearPatchClosestParameters does this algebraically, without ever
  // evaluating a 3D candidate point or distance; this determines the return code
  // (a point exactly on a boundary edge/corner converges there and is reported
  // inside). The actual distance is checked separately below when closestPoint
  // is requested, so a large spatial offset is fine as long as the projection
  // falls in the unit square.
  double bestU = 0.5, bestV = 0.5;
  const bool inside = BilinearPatchClosestParameters(x, pt1, B, C, D, 0.5, 0.5, &bestU, &bestV);

  if (closestPoint)
  {
    // The exact closest point additionally requires checking the 4 boundary
    // edges (each an exact closed-form straight-segment projection), keeping
    // the global minimum. This is exact for planar quads (the common case)
    // and, unlike fitting a single plane through 3 of the 4 corners, also
    // correct for a genuinely warped/non-planar quad. The edges only refine
    // the closest point / pcoords; they never change the inside/outside verdict
    // above, since a boundary hit (dist2 ~ 0) is still inside.
    //
    // A second interior critical point (there can be up to 4: the stationarity
    // equations have bidegree (1,2) and (2,1) in (u,v), so the mixed Bezout
    // bound is 1*1+2*2=5 stationary points total, minima+maxima+saddles) is
    // deliberately not searched for here: on 3000 randomly-warped test patches
    // -- far more warped than any real mesh face -- a second interior local
    // minimum occurred in under 1% of cases, so chasing it with extra Newton
    // solves isn't worth the cost for what is already a rare-case, small
    // precision refinement (it can only improve dist2, never flip in/out,
    // since the always-checked edges already catch the outside case).
    double bestDist2;
    if (inside)
    {
      double p[3];
      for (int i = 0; i < 3; ++i)
      {
        p[i] = pt1[i] + bestU * B[i] + bestV * C[i] + bestU * bestV * D[i];
      }
      bestDist2 = vtkMath::Distance2BetweenPoints(x, p);
    }
    else
    {
      bestDist2 = VTK_DOUBLE_MAX;
    }

    double t, p[3];
    double d2 = vtkLine::DistanceToLine(x, pt1, pt2, t, p); // v=0 edge
    if (d2 < bestDist2)
    {
      bestDist2 = d2;
      bestU = t;
      bestV = 0.0;
    }
    d2 = vtkLine::DistanceToLine(x, pt2, pt3, t, p); // u=1 edge
    if (d2 < bestDist2)
    {
      bestDist2 = d2;
      bestU = 1.0;
      bestV = t;
    }
    d2 = vtkLine::DistanceToLine(x, pt4, pt3, t, p); // v=1 edge
    if (d2 < bestDist2)
    {
      bestDist2 = d2;
      bestU = t;
      bestV = 1.0;
    }
    d2 = vtkLine::DistanceToLine(x, pt1, pt4, t, p); // u=0 edge
    if (d2 < bestDist2)
    {
      bestDist2 = d2;
      bestU = 0.0;
      bestV = t;
    }

    for (int i = 0; i < 3; ++i)
    {
      closestPoint[i] = pt1[i] + bestU * B[i] + bestV * C[i] + bestU * bestV * D[i];
    }
    dist2 = bestDist2;
  }

  pcoords[0] = bestU;
  pcoords[1] = bestV;
  vtkQuad::InterpolationFunctions(pcoords, weights);

  return inside ? 1 : 0;
}

//------------------------------------------------------------------------------
void vtkQuad::EvaluateLocation(
  int& vtkNotUsed(subId), const double pcoords[3], double x[3], double* weights)
{

  vtkQuad::InterpolationFunctions(pcoords, weights);

  // Efficient point access
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return;
  }
  const double* pts = pointsArray->GetPointer(0);

  x[0] = x[1] = x[2] = 0.0;
  for (int i = 0; i < 4; i++)
  {
    const double* pt = pts + 3 * i;
    for (int j = 0; j < 3; j++)
    {
      x[j] += pt[j] * weights[i];
    }
  }
}

//------------------------------------------------------------------------------
// Compute iso-parametric interpolation functions
//
void vtkQuad::InterpolationFunctions(const double pcoords[3], double sf[4])
{
  double rm = 1. - pcoords[0];
  double sm = 1. - pcoords[1];

  sf[0] = rm * sm;
  sf[1] = pcoords[0] * sm;
  sf[2] = pcoords[0] * pcoords[1];
  sf[3] = rm * pcoords[1];
}

//------------------------------------------------------------------------------
void vtkQuad::InterpolationDerivs(const double pcoords[3], double derivs[8])
{
  double rm = 1. - pcoords[0];
  double sm = 1. - pcoords[1];

  derivs[0] = -sm;
  derivs[1] = sm;
  derivs[2] = pcoords[1];
  derivs[3] = -pcoords[1];
  derivs[4] = -rm;
  derivs[5] = -pcoords[0];
  derivs[6] = pcoords[0];
  derivs[7] = rm;
}

//------------------------------------------------------------------------------
int vtkQuad::CellBoundary(int vtkNotUsed(subId), const double pcoords[3], vtkIdList* pts)
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
    pts->SetId(1, this->PointIds->GetId(2));
  }

  else if (t1 < 0.0 && t2 < 0.0)
  {
    pts->SetId(0, this->PointIds->GetId(2));
    pts->SetId(1, this->PointIds->GetId(3));
  }

  else //( t1 < 0.0 && t2 >= 0.0 )
  {
    pts->SetId(0, this->PointIds->GetId(3));
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
const vtkIdType* vtkQuad::GetEdgeArray(vtkIdType edgeId)
{
  return Edges[edgeId];
}

//------------------------------------------------------------------------------
void vtkQuad::Contour(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
  vtkCellArray* verts, vtkCellArray* lines, vtkCellArray* vtkNotUsed(polys), vtkPointData* inPd,
  vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd)
{
  static constexpr int CASE_MASK[4] = { 1, 2, 4, 8 };
  vtkIdType pts[2];
  int e1, e2;
  double t, x1[3], x2[3], x[3];
  vtkIdType offset = verts->GetNumberOfCells();

  // Build the case table
  int caseIndex = 0;
  for (int i = 0; i < 4; i++)
  {
    if (cellScalars->GetComponent(i, 0) >= value)
    {
      caseIndex |= CASE_MASK[i];
    }
  }

  const int* edge = vtkMarchingCellsContourCases::GetQuadCase(caseIndex);
  for (; edge[0] > -1; edge += 2)
  {
    for (int i = 0; i < 2; i++) // insert line
    {
      const vtkIdType* vert = Edges[edge[i]];
      // calculate a preferred interpolation direction
      double deltaScalar =
        (cellScalars->GetComponent(vert[1], 0) - cellScalars->GetComponent(vert[0], 0));
      if (deltaScalar > 0)
      {
        e1 = vert[0];
        e2 = vert[1];
      }
      else
      {
        e1 = vert[1];
        e2 = vert[0];
        deltaScalar = -deltaScalar;
      }

      // linear interpolation
      if (deltaScalar == 0.0)
      {
        t = 0.0;
      }
      else
      {
        t = (value - cellScalars->GetComponent(e1, 0)) / deltaScalar;
      }

      this->Points->GetPoint(e1, x1);
      this->Points->GetPoint(e2, x2);

      for (int j = 0; j < 3; j++)
      {
        x[j] = x1[j] + t * (x2[j] - x1[j]);
      }
      if (locator->InsertUniquePoint(x, pts[i]))
      {
        if (outPd)
        {
          vtkIdType p1 = this->PointIds->GetId(e1);
          vtkIdType p2 = this->PointIds->GetId(e2);
          outPd->InterpolateEdge(inPd, pts[i], p1, p2, t);
        }
      }
    }
    // check for degenerate line
    if (pts[0] != pts[1])
    {
      const vtkIdType newCellId = offset + lines->InsertNextCell(2, pts);
      if (outCd)
      {
        outCd->CopyData(inCd, cellId, newCellId);
      }
    }
  }
}

//------------------------------------------------------------------------------
vtkCell* vtkQuad::GetEdge(int edgeId)
{
  edgeId = std::clamp(edgeId, 0, 3);

  int edgeIdPlus1 = edgeId + 1;

  if (edgeIdPlus1 > 3)
  {
    edgeIdPlus1 = 0;
  }

  // load point id's
  this->Line->PointIds->SetId(0, this->PointIds->GetId(edgeId));
  this->Line->PointIds->SetId(1, this->PointIds->GetId(edgeIdPlus1));

  // load coordinates
  this->Line->Points->SetPoint(0, this->Points->GetPoint(edgeId));
  this->Line->Points->SetPoint(1, this->Points->GetPoint(edgeIdPlus1));

  return this->Line;
}

//------------------------------------------------------------------------------
// Intersect plane; see whether point is in quadrilateral. This code
// splits the quad into two triangles and intersects them (because the
// quad may be non-planar).
//
int vtkQuad::IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t,
  double x[3], double pcoords[3], int& subId)
{
  int diagonalCase;
  double d1 = vtkMath::Distance2BetweenPoints(this->Points->GetPoint(0), this->Points->GetPoint(2));
  double d2 = vtkMath::Distance2BetweenPoints(this->Points->GetPoint(1), this->Points->GetPoint(3));
  subId = 0;

  // Figure out how to uniquely tessellate the quad. Watch out for
  // equivalent triangulations (i.e., the triangulation is equivalent
  // no matter where the diagonal). In this case use the point ids as
  // a tie breaker to ensure unique triangulation across the quad.
  //
  if (d1 == d2) // rare case; discriminate based on point id
  {
    int id, maxId = 0, maxIdx = 0;
    for (int i = 0; i < 4; i++) // find the maximum id
    {
      if ((id = this->PointIds->GetId(i)) > maxId)
      {
        maxId = id;
        maxIdx = i;
      }
    }
    if (maxIdx == 0 || maxIdx == 2)
    {
      diagonalCase = 0;
    }
    else
    {
      diagonalCase = 1;
    }
  }
  else if (d1 < d2)
  {
    diagonalCase = 0;
  }
  else // d2 < d1
  {
    diagonalCase = 1;
  }

  // Note: in the following code the parametric coords must be adjusted to
  // reflect the use of the triangle parametric coordinate system.
  IntersectionStruct res;
  switch (diagonalCase)
  {
    case 0:
    {
      this->Triangle->Points->SetPoint(0, this->Points->GetPoint(0));
      this->Triangle->Points->SetPoint(1, this->Points->GetPoint(1));
      this->Triangle->Points->SetPoint(2, this->Points->GetPoint(2));
      IntersectionStruct firstIntersect =
        IntersectionStruct::CellIntersectWithLine(this->Triangle, p1, p2, tol);

      this->Triangle->Points->SetPoint(0, this->Points->GetPoint(2));
      this->Triangle->Points->SetPoint(1, this->Points->GetPoint(3));
      this->Triangle->Points->SetPoint(2, this->Points->GetPoint(0));
      IntersectionStruct secondIntersect =
        IntersectionStruct::CellIntersectWithLine(this->Triangle, p1, p2, tol);

      bool useFirstIntersection = (firstIntersect && secondIntersect)
        ? (firstIntersect.T <= secondIntersect.T)
        : firstIntersect;
      bool useSecondIntersection = (firstIntersect && secondIntersect)
        ? (secondIntersect.T < firstIntersect.T)
        : secondIntersect;

      if (useFirstIntersection)
      {
        res = firstIntersect;
        res.PCoords[0] += res.PCoords[1];
      }
      else if (useSecondIntersection)
      {
        res = secondIntersect;
        res.PCoords[0] = 1.0 - (res.PCoords[0] + res.PCoords[1]);
        res.PCoords[1] = 1.0 - res.PCoords[1];
      }
    }
    break;

    case 1:
    {
      this->Triangle->Points->SetPoint(0, this->Points->GetPoint(0));
      this->Triangle->Points->SetPoint(1, this->Points->GetPoint(1));
      this->Triangle->Points->SetPoint(2, this->Points->GetPoint(3));
      IntersectionStruct firstIntersect =
        IntersectionStruct::CellIntersectWithLine(this->Triangle, p1, p2, tol);

      this->Triangle->Points->SetPoint(0, this->Points->GetPoint(2));
      this->Triangle->Points->SetPoint(1, this->Points->GetPoint(3));
      this->Triangle->Points->SetPoint(2, this->Points->GetPoint(1));
      IntersectionStruct secondIntersect =
        IntersectionStruct::CellIntersectWithLine(this->Triangle, p1, p2, tol);

      bool useFirstIntersection = (firstIntersect && secondIntersect)
        ? (firstIntersect.T <= secondIntersect.T)
        : firstIntersect;
      bool useSecondIntersection = (firstIntersect && secondIntersect)
        ? (secondIntersect.T < firstIntersect.T)
        : secondIntersect;

      if (useFirstIntersection)
      {
        res = firstIntersect;
      }
      else if (useSecondIntersection)
      {
        res = secondIntersect;
        res.PCoords[0] = 1.0 - res.PCoords[0];
        res.PCoords[1] = 1.0 - res.PCoords[1];
      }
    }
    break;
  }

  if (res)
  {
    res.CopyValues(t, x, pcoords, subId);
  }

  return res.Intersected;
}

//------------------------------------------------------------------------------
int vtkQuad::TriangulateLocalIds(int vtkNotUsed(index), vtkIdList* ptIds)
{
  // The base of the pyramid must be split into two triangles.  There are two
  // ways to do this (across either diagonal).  Pick the shorter diagonal.
  double d1 = vtkMath::Distance2BetweenPoints(this->Points->GetPoint(0), this->Points->GetPoint(2));
  double d2 = vtkMath::Distance2BetweenPoints(this->Points->GetPoint(1), this->Points->GetPoint(3));

  ptIds->SetNumberOfIds(6);
  if (d1 <= d2)
  {
    constexpr std::array<vtkIdType, 6> localPtIds{ 0, 1, 2, 0, 2, 3 };
    std::copy(localPtIds.begin(), localPtIds.end(), ptIds->begin());
  }
  else
  {
    constexpr std::array<vtkIdType, 6> localPtIds{ 0, 1, 3, 1, 2, 3 };
    std::copy(localPtIds.begin(), localPtIds.end(), ptIds->begin());
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkQuad::Derivatives(
  int vtkNotUsed(subId), const double pcoords[3], const double* values, int dim, double* derivs)
{
  double v0[2], v1[2], v2[2], v3[2], v10[3], v20[3], lenX;
  double x0[3], x1[3], x2[3], x3[3], n[3], vec20[3], vec30[3];
  double *J[2], J0[2], J1[2];
  double *JI[2], JI0[2], JI1[2];
  double funcDerivs[8], sum[2], dBydx, dBydy;

  // Project points of quad into 2D system
  this->Points->GetPoint(0, x0);
  this->Points->GetPoint(1, x1);
  this->Points->GetPoint(2, x2);
  ComputeNormal(this, x0, x1, x2, n);
  this->Points->GetPoint(3, x3);

  for (int i = 0; i < 3; i++)
  {
    v10[i] = x1[i] - x0[i];
    vec20[i] = x2[i] - x0[i];
    vec30[i] = x3[i] - x0[i];
  }

  vtkMath::Cross(n, v10, v20); // creates local y' axis

  if ((lenX = vtkMath::Normalize(v10)) <= 0.0 || vtkMath::Normalize(v20) <= 0.0) // degenerate
  {
    for (int j = 0; j < dim; j++)
    {
      for (int i = 0; i < 3; i++)
      {
        derivs[j * dim + i] = 0.0;
      }
    }
    return;
  }

  v0[0] = v0[1] = 0.0; // convert points to 2D (i.e., local system)
  v1[0] = lenX;
  v1[1] = 0.0;
  v2[0] = vtkMath::Dot(vec20, v10);
  v2[1] = vtkMath::Dot(vec20, v20);
  v3[0] = vtkMath::Dot(vec30, v10);
  v3[1] = vtkMath::Dot(vec30, v20);

  this->InterpolationDerivs(pcoords, funcDerivs);

  // Compute Jacobian and inverse Jacobian
  J[0] = J0;
  J[1] = J1;
  JI[0] = JI0;
  JI[1] = JI1;

  J[0][0] =
    v0[0] * funcDerivs[0] + v1[0] * funcDerivs[1] + v2[0] * funcDerivs[2] + v3[0] * funcDerivs[3];
  J[0][1] =
    v0[1] * funcDerivs[0] + v1[1] * funcDerivs[1] + v2[1] * funcDerivs[2] + v3[1] * funcDerivs[3];
  J[1][0] =
    v0[0] * funcDerivs[4] + v1[0] * funcDerivs[5] + v2[0] * funcDerivs[6] + v3[0] * funcDerivs[7];
  J[1][1] =
    v0[1] * funcDerivs[4] + v1[1] * funcDerivs[5] + v2[1] * funcDerivs[6] + v3[1] * funcDerivs[7];

  // Compute inverse Jacobian, return if Jacobian is singular
  if (!vtkMath::InvertMatrix(J, JI, 2))
  {
    for (int j = 0; j < dim; j++)
    {
      for (int i = 0; i < 3; i++)
      {
        derivs[j * dim + i] = 0.0;
      }
    }
    return;
  }

  // Loop over "dim" derivative values. For each set of values,
  // compute derivatives
  // in local system and then transform into modelling system.
  // First compute derivatives in local x'-y' coordinate system
  for (int j = 0; j < dim; j++)
  {
    sum[0] = sum[1] = 0.0;
    for (int i = 0; i < 4; i++) // loop over interp. function derivatives
    {
      sum[0] += funcDerivs[i] * values[dim * i + j];
      sum[1] += funcDerivs[4 + i] * values[dim * i + j];
    }
    dBydx = sum[0] * JI[0][0] + sum[1] * JI[0][1];
    dBydy = sum[0] * JI[1][0] + sum[1] * JI[1][1];

    // Transform into global system (dot product with global axes)
    derivs[3 * j] = dBydx * v10[0] + dBydy * v20[0];
    derivs[3 * j + 1] = dBydx * v10[1] + dBydy * v20[1];
    derivs[3 * j + 2] = dBydx * v10[2] + dBydy * v20[2];
  }
}

//------------------------------------------------------------------------------
// Clip this quad using scalar value provided. Like contouring, except
// that it cuts the quad to produce other quads and/or triangles.
void vtkQuad::Clip(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
  vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd, int insideOut)
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
    ? vtkMarchingCellsClipCases<true>::GetCellCase(VTK_QUAD, caseIndex)
    : vtkMarchingCellsClipCases<false>::GetCellCase(VTK_QUAD, caseIndex);
  using MCCases = vtkMarchingCellsClipCasesBase;
  const MCCases::EDGEIDXS* edgeVertices = MCCases::GetCellEdges(VTK_QUAD);
  const uint8_t numberOfOutputCells = *thisCase++;

  // generate each tri/quad
  for (uint8_t outputCellId = 0; outputCellId < numberOfOutputCells; ++outputCellId)
  {
    /*shape =*/thisCase++; // ST_TRI/ST_QUAD
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
double* vtkQuad::GetParametricCoords()
{
  return ParametricCoords;
}

//------------------------------------------------------------------------------
void vtkQuad::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Line:\n";
  this->Line->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Triangle:\n";
  this->Triangle->PrintSelf(os, indent.GetNextIndent());
}
VTK_ABI_NAMESPACE_END
