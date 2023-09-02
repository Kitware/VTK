// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2001, softSurfer (www.softsurfer.com)
// SPDX-License-Identifier: BSD-3-Clause AND MIT
#include "vtkLine.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkLine);

//------------------------------------------------------------------------------
// Construct the line with two points.
vtkLine::vtkLine()
{
  this->Points->SetNumberOfPoints(2);
  this->PointIds->SetNumberOfIds(2);
  for (int i = 0; i < 2; i++)
  {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i, 0);
  }
}

//------------------------------------------------------------------------------
int vtkLine::EvaluatePosition(const double x[3], double closestPoint[3], int& subId,
  double pcoords[3], double& dist2, double weights[])
{
  const double *a1, *a2;

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

  a1 = pts;
  a2 = pts + 3;

  // DistanceToLine sets pcoords[0] to a value t
  dist2 = vtkLine::DistanceToLine(x, a1, a2, pcoords[0], closestPoint);

  // pcoords[0] == t, need weights to be 1-t and t
  weights[0] = 1.0 - pcoords[0];
  weights[1] = pcoords[0];

  return (pcoords[0] >= 0.0 && pcoords[0] <= 1.0);
}

//------------------------------------------------------------------------------
void vtkLine::EvaluateLocation(
  int& vtkNotUsed(subId), const double pcoords[3], double x[3], double* weights)
{
  int i;
  double a1[3], a2[3];
  this->Points->GetPoint(0, a1);
  this->Points->GetPoint(1, a2);

  for (i = 0; i < 3; i++)
  {
    x[i] = a1[i] + pcoords[0] * (a2[i] - a1[i]);
  }

  weights[0] = 1.0 - pcoords[0];
  weights[1] = pcoords[0];
}

//------------------------------------------------------------------------------
// Performs intersection of the projection of two finite 3D lines onto a 2D
// plane. An intersection is found if the projection of the two lines onto
// the plane perpendicular to the cross product of the two lines intersect.
// The parameters (u,v) are the parametric coordinates of the lines at the
// position of closest approach.
int vtkLine::Intersection(const double a1[3], const double a2[3], const double b1[3],
  const double b2[3], double& u, double& v, double tolerance, int tolType)
{
  double a21[3], b21[3], b1a1[3];
  double c[2];
  double *A[2], row1[2], row2[2];

  //  Initialize
  u = v = 0.0;

  //   Determine line vectors.
  vtkMath::Subtract(a2, a1, a21);
  vtkMath::Subtract(b2, b1, b21);
  vtkMath::Subtract(b1, a1, b1a1);

  //   Compute the system (least squares) matrix.
  A[0] = row1;
  A[1] = row2;
  row1[0] = vtkMath::Dot(a21, a21);
  row1[1] = -vtkMath::Dot(a21, b21);
  row2[0] = row1[1];
  row2[1] = vtkMath::Dot(b21, b21);

  //   Compute the least squares system constant term.
  c[0] = vtkMath::Dot(a21, b1a1);
  c[1] = -vtkMath::Dot(b21, b1a1);

  //  Solve the system of equations. Check for colinearity.
  if (vtkMath::SolveLinearSystem(A, c, 2) == 0)
  {
    // The lines are colinear. Therefore, one of the four endpoints is the
    // point of closest approach
    double minDist = VTK_DOUBLE_MAX;
    const double* p[4] = { a1, a2, b1, b2 };
    const double* l1[4] = { b1, b1, a1, a1 };
    const double* l2[4] = { b2, b2, a2, a2 };
    double* uv1[4] = { &v, &v, &u, &u };
    double* uv2[4] = { &u, &u, &v, &v };
    double t = 0;
    for (unsigned i = 0; i < 4; i++)
    {
      double dist = vtkLine::DistanceToLine(p[i], l1[i], l2[i], t);
      if (dist < minDist)
      {
        minDist = dist;
        *(uv1[i]) = t;
        *(uv2[i]) = static_cast<double>(i % 2); // the corresponding extremum
      }
    }
    return OnLine;
  } // if colinear

  // The lines are not colinear, check for intersection.
  // However if they are nearly parallel then the solution
  // found by vtkMath::SolveLinearSystem may be very inaccurate.
  // We hence need to check the solution against a tolerance criterion.
  u = c[0];
  v = c[1];
  // calculate intersection point using u
  double ptu[] = { a21[0], a21[1], a21[2] };
  vtkMath::MultiplyScalar(ptu, u);
  vtkMath::Add(ptu, a1, ptu);
  // calculate intersection point using v
  double ptv[] = { b21[0], b21[1], b21[2] };
  vtkMath::MultiplyScalar(ptv, v);
  vtkMath::Add(ptv, b1, ptv);
  // difference between the two intersection points should ideally be zero
  double diff[3];
  vtkMath::Subtract(ptu, ptv, diff);
  double diff2 = vtkMath::SquaredNorm(diff);

  double tol2 = 0.0;
  if (std::isfinite(tolerance))
  {
    // compare either absolute or relative diff; hence either
    // tolerance*tolerance or diff > tolerance * max(nrm(ptu),nrm(ptv))
    // but without taking square roots.
    tol2 = ((tolType == Absolute || tolType == AbsoluteFuzzy)
        ? tolerance * tolerance
        : tolerance * tolerance * std::max(vtkMath::SquaredNorm(ptv), vtkMath::SquaredNorm(ptu)));
    if (diff2 > tol2)
    {
      return NoIntersect;
    }
  } // valid tolerance

  // Check parametric coordinates for intersection within the two finite line
  // segments specified. Special treatment is required to make sure that
  // intersections near the line end points are not within tolerance.  Most
  // intersections will occur within the 0<=u,v<=1 range, handle them as
  // quickly as possible.
  if ((0.0 <= u) && (u <= 1.0) && (0.0 <= v) && (v <= 1.0))
  {
    return Intersect;
  }
  // Otherwise the intersection may be within tolerance at one or both of the
  // line end points. Note that we already know from previous calculations
  // that the two points of intersection are within tol of each other; here
  // we are checking whether they are on the line within the range
  // (-tol <= u,v <= 1+tol).
  else if (tolType >= RelativeFuzzy && tol2 > 0.0)
  {
    double uTol = sqrt(tol2 / vtkMath::SquaredNorm(a21));
    double vTol = sqrt(tol2 / vtkMath::SquaredNorm(b21));
    if ((-uTol <= u) && (u <= (1.0 + uTol)) && (-vTol <= v) && (v <= (1.0 + vTol)))
    {
      return Intersect;
    }
  }

  return NoIntersect;
}

int vtkLine::Inflate(double dist)
{
  auto pointRange = vtk::DataArrayTupleRange<3>(this->Points->GetData());
  using TupleRef = decltype(pointRange)::TupleReferenceType;
  using Scalar = typename TupleRef::value_type;
  TupleRef p0 = pointRange[0], p1 = pointRange[1];
  if (vtkMathUtilities::NearlyEqual<Scalar>(p0[0], p1[0]) &&
    vtkMathUtilities::NearlyEqual<Scalar>(p0[1], p1[1]) &&
    vtkMathUtilities::NearlyEqual<Scalar>(p0[2], p1[2]))
  {
    // line is degenerate
    return 0;
  }
  double v[3] = { p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2] };
  vtkMath::Normalize(v);
  p0[0] -= v[0] * dist;
  p0[1] -= v[1] * dist;
  p0[2] -= v[2] * dist;
  p1[0] += v[0] * dist;
  p1[1] += v[1] * dist;
  p1[2] += v[2] * dist;
  return 1;
}

//------------------------------------------------------------------------------
int vtkLine::CellBoundary(int vtkNotUsed(subId), const double pcoords[3], vtkIdList* pts)
{
  pts->SetNumberOfIds(1);

  if (pcoords[0] >= 0.5)
  {
    pts->SetId(0, this->PointIds->GetId(1));
    return (pcoords[0] <= 1.0);
  }
  else
  {
    pts->SetId(0, this->PointIds->GetId(0));
    return (pcoords[0] >= 0.0);
  }
}

//------------------------------------------------------------------------------
//
// marching lines case table
//
typedef int VERT_LIST;

struct VERT_CASES_t
{
  VERT_LIST verts[2];
};
using VERT_CASES = struct VERT_CASES_t;

static VERT_CASES vertCases[4] = {
  { { -1, -1 } },
  { { 1, 0 } },
  { { 0, 1 } },
  { { -1, -1 } },
};

//------------------------------------------------------------------------------
void vtkLine::Contour(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
  vtkCellArray* verts, vtkCellArray* vtkNotUsed(lines), vtkCellArray* vtkNotUsed(polys),
  vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd)
{
  static const int CASE_MASK[2] = { 1, 2 };
  int index, i, newCellId;
  VERT_CASES* vertCase;
  VERT_LIST* vert;
  double t, x[3], x1[3], x2[3];
  vtkIdType pts[1];

  //
  // Build the case table
  //
  for (i = 0, index = 0; i < 2; i++)
  {
    if (cellScalars->GetComponent(i, 0) >= value)
    {
      index |= CASE_MASK[i];
    }
  }

  vertCase = vertCases + index;
  vert = vertCase->verts;

  if (vert[0] > -1)
  {
    t = (value - cellScalars->GetComponent(vert[0], 0)) /
      (cellScalars->GetComponent(vert[1], 0) - cellScalars->GetComponent(vert[0], 0));
    this->Points->GetPoint(vert[0], x1);
    this->Points->GetPoint(vert[1], x2);
    for (i = 0; i < 3; i++)
    {
      x[i] = x1[i] + t * (x2[i] - x1[i]);
    }

    if (locator->InsertUniquePoint(x, pts[0]))
    {
      if (outPd)
      {
        vtkIdType p1 = this->PointIds->GetId(vert[0]);
        vtkIdType p2 = this->PointIds->GetId(vert[1]);
        outPd->InterpolateEdge(inPd, pts[0], p1, p2, t);
      }
    }
    newCellId = verts->InsertNextCell(1, pts);
    if (outCd)
    {
      outCd->CopyData(inCd, cellId, newCellId);
    }
  }
}

//------------------------------------------------------------------------------
double vtkLine::DistanceBetweenLines(double l0[3], double l1[3], // line 1
  double m0[3], double m1[3],                                    // line 2
  double closestPt1[3], double closestPt2[3],                    // closest points
  double& t1, double& t2) // parametric coords of the closest points
{
  // Part of this function was adapted from "GeometryAlgorithms.com"
  const double u[3] = { l1[0] - l0[0], l1[1] - l0[1], l1[2] - l0[2] };
  const double v[3] = { m1[0] - m0[0], m1[1] - m0[1], m1[2] - m0[2] };
  const double w[3] = { l0[0] - m0[0], l0[1] - m0[1], l0[2] - m0[2] };
  const double a = vtkMath::Dot(u, u);
  const double b = vtkMath::Dot(u, v);
  const double c = vtkMath::Dot(v, v); // always >= 0
  const double d = vtkMath::Dot(u, w);
  const double e = vtkMath::Dot(v, w);
  const double D = a * c - b * b; // always >= 0

  // compute the line parameters of the two closest points
  if (D < 1e-6)
  { // the lines are almost parallel
    t1 = 0.0;
    t2 = (b > c ? d / b : e / c); // use the largest denominator
  }
  else
  {
    t1 = (b * e - c * d) / D;
    t2 = (a * e - b * d) / D;
  }

  for (unsigned int i = 0; i < 3; i++)
  {
    closestPt1[i] = l0[i] + t1 * u[i];
    closestPt2[i] = m0[i] + t2 * v[i];
  }

  // Return the distance squared between the lines =
  // the mag squared of the distance between the two closest points
  // = L1(t1) - L2(t2)
  return vtkMath::Distance2BetweenPoints(closestPt1, closestPt2);
}

//------------------------------------------------------------------------------
double vtkLine::DistanceBetweenLineSegments(double l0[3], double l1[3], // line segment 1
  double m0[3], double m1[3],                                           // line segment 2
  double closestPt1[3], double closestPt2[3],                           // closest points
  double& t1, double& t2)                                               // parametric coords
                                                                        // of the closest points
{
  // Part of this function was adapted from "GeometryAlgorithms.com"
  const double u[3] = { l1[0] - l0[0], l1[1] - l0[1], l1[2] - l0[2] };
  const double v[3] = { m1[0] - m0[0], m1[1] - m0[1], m1[2] - m0[2] };
  const double w[3] = { l0[0] - m0[0], l0[1] - m0[1], l0[2] - m0[2] };
  const double a = vtkMath::Dot(u, u);
  const double b = vtkMath::Dot(u, v);
  const double c = vtkMath::Dot(v, v); // always >= 0
  const double d = vtkMath::Dot(u, w);
  const double e = vtkMath::Dot(v, w);
  const double D = a * c - b * b; // always >= 0
  double sN, sD = D;              // sc = sN / sD, default sD = D >= 0
  double tN, tD = D;              // tc = tN / tD, default tD = D >= 0

  // compute the line parameters of the two closest points

  if (D < 1e-6)
  {
    // The lines are colinear. Therefore, one of the four endpoints is the
    // point of closest approach
    double minDist = VTK_DOUBLE_MAX;
    double* p[4] = { l0, l1, m0, m1 };
    double* a1[4] = { m0, m0, l0, l0 };
    double* a2[4] = { m1, m1, l1, l1 };
    double* uv1[4] = { &t2, &t2, &t1, &t1 };
    double* uv2[4] = { &t1, &t1, &t2, &t2 };
    double* pn1[4] = { closestPt2, closestPt2, closestPt1, closestPt1 };
    double* pn2[4] = { closestPt1, closestPt1, closestPt2, closestPt2 };
    double dist, pn_[3];
    for (unsigned i = 0; i < 4; i++)
    {
      double t = 0;
      dist = vtkLine::DistanceToLine(p[i], a1[i], a2[i], t, pn_);
      if (dist < minDist)
      {
        minDist = dist;
        *(uv1[i]) = (t < 0. ? 0. : (t > 1. ? 1. : t));
        *(uv2[i]) = static_cast<double>(i % 2); // the corresponding extremum
        for (unsigned j = 0; j < 3; j++)
        {
          pn1[i][j] = pn_[j];
          pn2[i][j] = p[i][j];
        }
      }
    }
    return minDist;
  }

  // The lines aren't parallel.

  else
  { // get the closest points on the infinite lines
    sN = (b * e - c * d);
    tN = (a * e - b * d);
    if (sN < 0.0)
    { // sc < 0 => the s=0 edge is visible
      sN = 0.0;
      tN = e;
      tD = c;
    }
    else if (sN > sD)
    { // sc > 1 => the s=1 edge is visible
      sN = sD;
      tN = e + b;
      tD = c;
    }
  }

  if (tN < 0.0)
  { // tc < 0 => the t=0 edge is visible
    tN = 0.0;

    // recompute sc for this edge
    if (-d < 0.0)
    {
      sN = 0.0;
    }
    else if (-d > a)
    {
      sN = sD;
    }
    else
    {
      sN = -d;
      sD = a;
    }
  }
  else if (tN > tD)
  { // tc > 1 => the t=1 edge is visible
    tN = tD;

    // recompute sc for this edge
    if ((-d + b) < 0.0)
    {
      sN = 0;
    }
    else if ((-d + b) > a)
    {
      sN = sD;
    }
    else
    {
      sN = (-d + b);
      sD = a;
    }
  }

  // finally do the division to get sc and tc
  t1 = (fabs(sN) < 1e-6 ? 0.0 : sN / sD);
  t2 = (fabs(tN) < 1e-6 ? 0.0 : tN / tD);

  // Closest Point on segment1 = S1(t1) = l0 + t1*u
  // Closest Point on segment2 = S1(t2) = m0 + t2*v

  for (unsigned int i = 0; i < 3; i++)
  {
    closestPt1[i] = l0[i] + t1 * u[i];
    closestPt2[i] = m0[i] + t2 * v[i];
  }

  // Return the distance squared between the lines =
  // the mag squared of the distance between the two closest points
  // = S1(t1) - S2(t2)
  return vtkMath::Distance2BetweenPoints(closestPt1, closestPt2);
}

//------------------------------------------------------------------------------
// Compute distance to finite line. Returns parametric coordinate t
// and point location on line.
double vtkLine::DistanceToLine(
  const double x[3], const double p1[3], const double p2[3], double& t, double closestPoint[3])
{
  const double* closest = nullptr;
  //
  //   Determine appropriate vectors
  //
  double p21[3] = { p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2] };

  //
  //   Get parametric location
  //
  double num = p21[0] * (x[0] - p1[0]) + p21[1] * (x[1] - p1[1]) + p21[2] * (x[2] - p1[2]);
  if (num == 0.0)
  {
    t = 0;
    closest = p1;
  }
  else
  {
    double denom = vtkMath::Dot(p21, p21);

    // trying to avoid an expensive fabs
    double tolerance = VTK_TOL * num;
    if (tolerance < 0.0)
    {
      tolerance = -tolerance;
    }
    if (denom < tolerance) // numerically bad!
    {
      if (num > 0)
      {
        closest = p2;
        t = VTK_DOUBLE_MAX;
      }
      else
      {
        closest = p1;
        t = VTK_DOUBLE_MIN;
      }
    }
    //
    // If parametric coordinate is within 0<=p<=1, then the point is closest to
    // the line.  Otherwise, it's closest to a point at the end of the line.
    //
    else if ((t = num / denom) < 0.0)
    {
      closest = p1;
    }
    else if (t > 1.0)
    {
      closest = p2;
    }
    else
    {
      closest = p21;
      p21[0] = p1[0] + t * p21[0];
      p21[1] = p1[1] + t * p21[1];
      p21[2] = p1[2] + t * p21[2];
    }
  }

  if (closestPoint)
  {
    closestPoint[0] = closest[0];
    closestPoint[1] = closest[1];
    closestPoint[2] = closest[2];
  }
  return vtkMath::Distance2BetweenPoints(closest, x);
}

//------------------------------------------------------------------------------
//
// Determine the distance of the current vertex to the edge defined by
// the vertices provided.  Returns distance squared. Note: line is assumed
// infinite in extent.
//
double vtkLine::DistanceToLine(const double x[3], const double p1[3], const double p2[3])
{
  int i;
  double np1[3], p1p2[3], proj, den;

  for (i = 0; i < 3; i++)
  {
    np1[i] = x[i] - p1[i];
    p1p2[i] = p1[i] - p2[i];
  }

  if ((den = vtkMath::Norm(p1p2)) != 0.0)
  {
    for (i = 0; i < 3; i++)
    {
      p1p2[i] /= den;
    }
  }
  else
  {
    return vtkMath::Dot(np1, np1);
  }

  proj = vtkMath::Dot(np1, p1p2);

  return (vtkMath::Dot(np1, np1) - proj * proj);
}

//------------------------------------------------------------------------------
// Line-line intersection. Intersection has to occur within [0,1] parametric
// coordinates and with specified tolerance.
int vtkLine::IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t,
  double x[3], double pcoords[3], int& subId)
{
  double a1[3], a2[3];
  int i;

  subId = 0;
  pcoords[1] = pcoords[2] = 0.0;

  this->Points->GetPoint(0, a1);
  this->Points->GetPoint(1, a2);

  // check line-line intersection; use inf tolerance which will force
  // vtkLine::Intersection() to only check parametric intersection
  // we then perform the tolerance check here using the absolute tolerance tol
  if (this->Intersection(p1, p2, a1, a2, t, pcoords[0], vtkMath::Inf()) == Intersect)
  {
    double projXYZ[3];
    // make sure we are within tolerance
    for (i = 0; i < 3; i++)
    {
      x[i] = a1[i] + pcoords[0] * (a2[i] - a1[i]);
      projXYZ[i] = p1[i] + t * (p2[i] - p1[i]);
    }
    return (vtkMath::Distance2BetweenPoints(x, projXYZ) <= tol * tol);
  }

  else // check to see if it lies within tolerance
  {
    // one of the parametric coords must be outside 0-1
    if (t < 0.0)
    {
      t = 0.0;
      return (vtkLine::DistanceToLine(p1, a1, a2, pcoords[0], x) <= tol * tol);
    }
    if (t > 1.0)
    {
      t = 1.0;
      return (vtkLine::DistanceToLine(p2, a1, a2, pcoords[0], x) <= tol * tol);
    }
    if (pcoords[0] < 0.0)
    {
      pcoords[0] = 0.0;
      return (vtkLine::DistanceToLine(a1, p1, p2, t, x) <= tol * tol);
    }
    if (pcoords[0] > 1.0)
    {
      pcoords[0] = 1.0;
      return (vtkLine::DistanceToLine(a2, p1, p2, t, x) <= tol * tol);
    }
  }
  return 0;
}

//------------------------------------------------------------------------------
int vtkLine::TriangulateLocalIds(int vtkNotUsed(index), vtkIdList* ptIds)
{
  ptIds->SetNumberOfIds(2);
  ptIds->SetId(0, 0);
  ptIds->SetId(1, 1);
  return 1;
}

//------------------------------------------------------------------------------
void vtkLine::Derivatives(int vtkNotUsed(subId), const double vtkNotUsed(pcoords)[3],
  const double* values, int dim, double* derivs)
{
  double x0[3], x1[3], deltaX[3];
  int i, j;

  this->Points->GetPoint(0, x0);
  this->Points->GetPoint(1, x1);

  for (i = 0; i < 3; i++)
  {
    deltaX[i] = x1[i] - x0[i];
  }
  for (i = 0; i < dim; i++)
  {
    for (j = 0; j < 3; j++)
    {
      if (deltaX[j] != 0)
      {
        derivs[3 * i + j] = (values[i + dim] - values[i]) / deltaX[j];
      }
      else
      {
        derivs[3 * i + j] = 0;
      }
    }
  }
}

//------------------------------------------------------------------------------
// support line clipping
namespace
{ // required so we don't violate ODR
typedef int LINE_LIST;
struct LINE_CASES_t
{
  LINE_LIST lines[2];
};
using LINE_CASES = struct LINE_CASES_t;

LINE_CASES lineCases[] = {
  { { -1, -1 } },   // 0
  { { 100, 1 } },   // 1
  { { 0, 101 } },   // 2
  { { 100, 101 } }, // 3
};
}

// Clip this line using scalar value provided. Like contouring, except
// that it cuts the line to produce other lines.
void vtkLine::Clip(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
  vtkCellArray* lines, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd, int insideOut)
{
  static const int CASE_MASK[3] = { 1, 2 };
  LINE_CASES* lineCase;
  int i, j, index, *vert, newCellId;
  vtkIdType pts[3];
  int vertexId;
  double t, x1[3], x2[3], x[3];

  // Build the case table
  if (insideOut)
  {
    for (i = 0, index = 0; i < 2; i++)
    {
      if (cellScalars->GetComponent(i, 0) <= value)
      {
        index |= CASE_MASK[i];
      }
    }
  }
  else
  {
    for (i = 0, index = 0; i < 2; i++)
    {
      if (cellScalars->GetComponent(i, 0) > value)
      {
        index |= CASE_MASK[i];
      }
    }
  }

  // Select the case based on the index and get the list of lines for this case
  lineCase = lineCases + index;
  vert = lineCase->lines;

  // generate clipped line
  if (vert[0] > -1)
  {
    for (i = 0; i < 2; i++) // insert line
    {
      // vertex exists, and need not be interpolated
      if (vert[i] >= 100)
      {
        vertexId = vert[i] - 100;
        this->Points->GetPoint(vertexId, x);
        if (locator->InsertUniquePoint(x, pts[i]))
        {
          if (outPd)
          {
            outPd->CopyData(inPd, this->PointIds->GetId(vertexId), pts[i]);
          }
        }
      }

      else // new vertex, interpolate
      {
        t = (value - cellScalars->GetComponent(0, 0)) /
          (cellScalars->GetComponent(1, 0) - cellScalars->GetComponent(0, 0));

        this->Points->GetPoint(0, x1);
        this->Points->GetPoint(1, x2);
        for (j = 0; j < 3; j++)
        {
          x[j] = x1[j] + t * (x2[j] - x1[j]);
        }

        if (locator->InsertUniquePoint(x, pts[i]))
        {
          if (outPd)
          {
            outPd->InterpolateEdge(
              inPd, pts[i], this->PointIds->GetId(0), this->PointIds->GetId(1), t);
          }
        }
      }
    }
    // check for degenerate lines
    if (pts[0] != pts[1])
    {
      newCellId = lines->InsertNextCell(2, pts);
      if (outCd)
      {
        outCd->CopyData(inCd, cellId, newCellId);
      }
    }
  }
}

//------------------------------------------------------------------------------
//
// Compute interpolation functions
//
void vtkLine::InterpolationFunctions(const double pcoords[3], double weights[2])
{
  weights[0] = 1.0 - pcoords[0];
  weights[1] = pcoords[0];
}

//------------------------------------------------------------------------------
void vtkLine::InterpolationDerivs(const double vtkNotUsed(pcoords)[3], double derivs[2])
{
  derivs[0] = -1.0;
  derivs[1] = 1.0;
}

//------------------------------------------------------------------------------
static double vtkLineCellPCoords[6] = { 0.0, 0.0, 0.0, 1.0, 0.0, 0.0 };
double* vtkLine::GetParametricCoords()
{
  return vtkLineCellPCoords;
}

//------------------------------------------------------------------------------
void vtkLine::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
