// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCell.h"

#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolygon.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"

#include <limits>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
namespace
{
//----------------------------------------------------------------------------
// Strategy:
//
// We throw all edges from one cell to an other and look if they intersect.
// In the case of a cell of one point, we just check if it lies inside
// the other cell.
int IntersectWithCellImpl(vtkCell* self, vtkCell* other, double tol)
{
  if (!other->GetNumberOfPoints() || !self->GetNumberOfPoints())
  {
    return 0;
  }
  double x[3], pcoords[3];
  if (other->GetNumberOfPoints() == 1)
  {
    double closestPoint[3];
    double* point = other->GetPoints()->GetPoint(0);
    int subId;
    double dist2, *weights = new double[self->GetNumberOfPoints()];
    self->EvaluatePosition(point, closestPoint, subId, pcoords, dist2, weights);
    delete[] weights;
    return dist2 <= tol * tol;
  }
  if (self->GetNumberOfPoints() == 1)
  {
    double closestPoint[3];
    double* point = self->GetPoints()->GetPoint(0);
    int subId;
    double dist2, *weights = new double[other->GetNumberOfPoints()];
    other->EvaluatePosition(point, closestPoint, subId, pcoords, dist2, weights);
    delete[] weights;
    return dist2 <= tol * tol;
  }
  double p1[3], p2[3];
  for (vtkIdType edgeId = 0; edgeId < self->GetNumberOfEdges(); ++edgeId)
  {
    double t;
    int subId;
    vtkCell* edge = self->GetEdge(edgeId);
    vtkPoints* ends = edge->GetPoints();
    ends->GetPoint(0, p1);
    ends->GetPoint(1, p2);
    if (other->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId))
    {
      return 1;
    }
  }
  for (vtkIdType edgeId = 0; edgeId < other->GetNumberOfEdges(); ++edgeId)
  {
    double t;
    int subId;
    vtkCell* edge = other->GetEdge(edgeId);
    vtkPoints* ends = edge->GetPoints();
    ends->GetPoint(0, p1);
    ends->GetPoint(1, p2);
    if (self->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId))
    {
      return 1;
    }
  }
  return 0;
}
} // anonymous namespace

//------------------------------------------------------------------------------
// Construct cell.
vtkCell::vtkCell()
{
  this->Points = vtkPoints::New(VTK_DOUBLE);
  this->PointIds = vtkIdList::New();
  // Consistent Register/Deletes (ShallowCopy uses Register.)
  this->Points->Register(this);
  this->Points->Delete();
  this->PointIds->Register(this);
  this->PointIds->Delete();
}

//------------------------------------------------------------------------------
vtkCell::~vtkCell()
{
  this->Points->UnRegister(this);
  this->PointIds->UnRegister(this);
}

//------------------------------------------------------------------------------
// Instantiate cell from outside
//
void vtkCell::Initialize(int npts, const vtkIdType* pts, vtkPoints* p)
{
  this->PointIds->Reset();
  this->Points->Reset();

  for (int i = 0; i < npts; i++)
  {
    this->PointIds->InsertId(i, pts[i]);
    this->Points->InsertPoint(i, p->GetPoint(pts[i]));
  }
}

//------------------------------------------------------------------------------
// Instantiate cell from outside. A simplified version of
// vtkCell::Initialize() that assumes point ids are simply the index into the
// points. This is a convenience function.
//
void vtkCell::Initialize(int npts, vtkPoints* p)
{
  this->PointIds->Reset();
  this->Points->Reset();

  for (int i = 0; i < npts; i++)
  {
    this->PointIds->InsertId(i, i);
    this->Points->InsertPoint(i, p->GetPoint(i));
  }
}

//------------------------------------------------------------------------------
void vtkCell::ShallowCopy(vtkCell* c)
{
  this->Points->ShallowCopy(c->Points);
  if (this->PointIds)
  {
    this->PointIds->UnRegister(this);
    this->PointIds = c->PointIds;
    this->PointIds->Register(this);
  }
}

//------------------------------------------------------------------------------
void vtkCell::DeepCopy(vtkCell* c)
{
  this->Points->DeepCopy(c->Points);
  this->PointIds->DeepCopy(c->PointIds);
}

//------------------------------------------------------------------------------
double vtkCell::ComputeBoundingSphere(double center[3]) const
{
  // We do easy cases first for number of points <= 4
  switch (this->Points->GetNumberOfPoints())
  {
    case 0:
      center[0] = std::numeric_limits<double>::quiet_NaN();
      center[1] = std::numeric_limits<double>::quiet_NaN();
      center[2] = std::numeric_limits<double>::quiet_NaN();
      return std::numeric_limits<double>::quiet_NaN();
    case 1:
      this->Points->GetPoint(0, center);
      return 0.0;
    case 2:
    {
      auto points = vtk::DataArrayTupleRange(this->Points->GetData());
      auto p0 = points[0], p1 = points[1];
      center[0] = 0.5 * (p0[0] + p1[0]);
      center[1] = 0.5 * (p0[1] + p1[1]);
      center[2] = 0.5 * (p0[2] + p1[2]);
      return vtkMath::Distance2BetweenPoints(center, p0);
    }
    case 3:
    {
      if (!vtkTriangle::ComputeCentroid(this->Points, nullptr, center))
      {
        break;
      }
      auto points = vtk::DataArrayTupleRange(this->Points->GetData());
      return vtkMath::Distance2BetweenPoints(center, points[0]);
    }
    case 4:
    {
      if (!vtkTetra::ComputeCentroid(this->Points, nullptr, center))
      {
        break;
      }
      auto points = vtk::DataArrayTupleRange(this->Points->GetData());
      return vtkMath::Distance2BetweenPoints(center, points[0]);
    }
    default:
      break;
  }

  // For more complex cells, we follow Ritter's bounding sphere algorithm
  // 1. Pick a point x (first point in our case) in the cell, and look for
  //    a point y the furthest from x.
  // 2. Look for a point z the furthest from y.
  // 3. Create a sphere centered at [z,y] with appropriate radius
  // 4. Until all points are not in the sphere, take a point outside the sphere,
  //    and update the sphere to include former sphere + this point

  auto points = vtk::DataArrayTupleRange(this->Points->GetData());
  using ConstRefType = decltype(points)::ConstTupleReferenceType;

  ConstRefType x = points[0];
  vtkIdType yid = 1, zid = 0;

  double dist2 = 0.0;
  for (vtkIdType id = 1; id < points.size(); ++id)
  {
    double tmpdist2 = vtkMath::Distance2BetweenPoints(points[id], x);
    if (tmpdist2 > dist2)
    {
      dist2 = tmpdist2;
      yid = id;
    }
  }

  ConstRefType y = points[yid];

  dist2 = 0.0;
  for (vtkIdType id = 0; id < points.size(); ++id)
  {
    double tmpdist2 = vtkMath::Distance2BetweenPoints(points[id], y);
    if (tmpdist2 > dist2)
    {
      dist2 = tmpdist2;
      zid = id;
    }
  }

  ConstRefType z = points[zid];
  center[0] = 0.5 * (y[0] + z[0]);
  center[1] = 0.5 * (y[1] + z[1]);
  center[2] = 0.5 * (y[2] + z[2]);
  dist2 = vtkMath::Distance2BetweenPoints(y, center);

  double v[3];
  vtkIdType pointId;
  do
  {
    for (pointId = 0; pointId < points.size(); ++pointId)
    {
      if (vtkMath::Distance2BetweenPoints(points[pointId], center) > dist2)
      {
        break;
      }
    }
    if (pointId != points.size())
    {
      ConstRefType p = points[pointId];
      v[0] = p[0] - center[0];
      v[1] = p[1] - center[1];
      v[2] = p[2] - center[2];
      double d = 0.5 * (vtkMath::Norm(v) - std::sqrt(dist2));
      vtkMath::Normalize(v);
      center[0] += d * v[0];
      center[1] += d * v[1];
      center[2] += d * v[2];

      // If dist2 was going to decrease, it means that we have some numeric imprecision, so we
      // slightly increase dist2.
      // There is numeric precision problem when 2 points are almost equidistant
      // to center, off by center's numeric precision in at least one dimension.
      // When that happens, given maxCenterEpsilon this numeric precision, we
      // need to shift dist2 by maxCenterEpsilon^2. Since this might be lower
      // than dist2's numeric precision, we shift it by the max between dist2's
      // numeric precision and maxCenterEpsilon^2.
      // Then, we take the max between this shifted dist2 and the new sphere
      // radius that we just caught.
      double maxCenterEpsilon = VTK_DBL_EPSILON *
        std::max({ std::abs(center[0]), std::abs(center[1]), std::abs(center[2]) });
      dist2 += std::max(dist2 * VTK_DBL_EPSILON, maxCenterEpsilon * maxCenterEpsilon);
      dist2 = std::max(dist2, vtkMath::Distance2BetweenPoints(p, center));
    }
  } while (pointId != points.size());
  return dist2;
}

//------------------------------------------------------------------------------
int vtkCell::Inflate(double dist)
{
  if (this->GetNumberOfFaces() != 0)
  {
    vtkWarningMacro(<< "Base version of vtkCell::Inflate only implements cell inflation"
                    << " for linear non 3D cells. Class " << this->GetClassName()
                    << " needs to overload this method. Ignoring this cell.");
    return 0;
  }

  // -----------------------------------------------------------------------
  // Setup
  // -----------------------------------------------------------------------

  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return 0;
  }
  using Scalar = vtkDoubleArray::ValueType;
  auto pointRange = vtk::DataArrayTupleRange<3>(pointsArray);

  const vtkIdType numPoints = pointsArray->GetNumberOfTuples();

  // Buffer to accumulate new positions before writing back to Points.
  // We can't update Points in-place — later iterations still need original positions.
  std::vector<Scalar> buf(3 * pointRange.size());

  // -----------------------------------------------------------------------
  // Build a 2D projection basis aligned with this cell's plane.
  //
  // Solving two line intersections in 3D is awkward. Instead, we project
  // into a 2D coordinate system where each line equation becomes n·x = d.
  // The basis is derived from the cell normal and the first edge direction.
  // -----------------------------------------------------------------------

  Scalar cellNormal[3];
  vtkPolygon::ComputeNormal(this->Points, cellNormal);

  // basis is a 2x3 matrix whose rows are two orthogonal in-plane unit vectors
  Scalar basis[6];

  // -----------------------------------------------------------------------
  // Bootstrap: compute the incoming edge normal for the first point.
  //
  // The loop processes one edge at a time (the outgoing edge at point[i]).
  // Each point needs both its incoming and outgoing edge normals to solve
  // for its new position. We prime the rolling buffer with the last edge
  // (point[N-1] -> point[0]) before the loop starts.
  // -----------------------------------------------------------------------

  Scalar edgeNormal3D[3];

  // edgeNormals2D holds two consecutive edge normals in 2D (a 2x2 matrix).
  // Slots alternate via baseId so we never recompute the previous edge.
  Scalar edgeNormals2D[2][2];

  Scalar y[2]; // right-hand side of each line equation: n·x = dot(n, p) + dist

  {
    const auto p1 = pointRange[numPoints - 1];
    const auto p2 = pointRange[0];

    // We do not support the case of collapsed edges
    if (vtkMathUtilities::NearlyEqual<Scalar>(p1[0], p2[0]) &&
      vtkMathUtilities::NearlyEqual<Scalar>(p1[1], p2[1]) &&
      vtkMathUtilities::NearlyEqual<Scalar>(p1[2], p2[2]))
    {
      return 0; // degenerate: collapsed edge
    }
    Scalar edgeDirection[3] = { p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2] };
    vtkMath::Normalize(edgeDirection);
    vtkMath::Cross(edgeDirection, cellNormal, edgeNormal3D);

    // Project the outgoing edge normal into 2D and store in the current slot.
    // In this basis the first edge's normal projects to (1.0, 0.0)
    edgeNormals2D[0][0] = 1.0;
    edgeNormals2D[0][1] = 0.0;

    // Shifted line offset
    y[0] = dist;

    // Build the 2D basis from this first edge: basis[0..2] = edge outward normal,
    // basis[3..5] = perpendicular in-plane vector
    std::copy_n(edgeNormal3D, 3, basis);
    vtkMath::Cross(cellNormal, basis, basis + 3);
  }

  // -----------------------------------------------------------------------
  // Main loop: for each point, compute its displaced position
  // -----------------------------------------------------------------------

  // Current index in edgeNormals2D and y. At each iteration, it binary swaps
  int baseId = 1;
  Scalar x[2]; // solved 2D intersection coordinates

  Scalar* newPosition = buf.data();
  for (vtkIdType pointId = 0; pointId < numPoints; ++pointId, ++baseId %= 2, newPosition += 3)
  {
    const auto p1 = pointRange[pointId];
    const auto p2 = pointRange[(pointId + 1) % numPoints];

    // We do not support the case of collapsed edges
    if (vtkMathUtilities::NearlyEqual<Scalar>(p1[0], p2[0]) &&
      vtkMathUtilities::NearlyEqual<Scalar>(p1[1], p2[1]) &&
      vtkMathUtilities::NearlyEqual<Scalar>(p1[2], p2[2]))
    {
      return 0; // degenerate: collapsed edge
    }

    // Compute the outgoing edge normal at p1 and store in the current slot
    Scalar edgeDirection[3] = { p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2] };
    vtkMath::Normalize(edgeDirection);
    vtkMath::Cross(edgeDirection, cellNormal, edgeNormal3D);

    // Project the outgoing edge normal into 2D and store in the current slot.
    // In this basis this edge's normal projects to edgeNormals2D[baseId].
    vtkMath::MultiplyMatrixWithVector<2, 3>(basis, edgeNormal3D, edgeNormals2D[baseId]);

    // Shifted line offset
    y[baseId] = vtkMath::Dot(edgeNormal3D, p1) + dist;

    // Check if the incoming and outgoing edges are collinear (parallel normals).
    // If so, the 2x2 system is singular — just displace directly along the normal.
    const bool edgesAreCollinear =
      std::abs(vtkMath::Dot<Scalar, 2>(edgeNormals2D[0], edgeNormals2D[1]) - 1.0) <
      std::numeric_limits<Scalar>::epsilon();
    if (edgesAreCollinear)
    {
      // Incident edges are collinear, we handle that differently.
      newPosition[0] = p1[0] + dist * edgeNormal3D[0];
      newPosition[1] = p1[1] + dist * edgeNormal3D[1];
      newPosition[2] = p1[2] + dist * edgeNormal3D[2];
    }
    else
    {
      // Solve n0·x = y[0], n1·x = y[1] in 2D, then project back to 3D
      vtkMath::LinearSolve<2, 2>(edgeNormals2D, y, x);
      vtkMath::MultiplyMatrixWithVector<3, 2, vtkMatrixUtilities::Layout::Transpose>(
        basis, x, newPosition);
    }
  }

  // -----------------------------------------------------------------------
  // Write-back: apply all buffered positions to Points
  // -----------------------------------------------------------------------
  std::copy_n(buf.data(), buf.size(), pointsArray->GetPointer(0));
  return 1;
}

//----------------------------------------------------------------------------
int vtkCell::IntersectWithCell(vtkCell* other, const vtkBoundingBox& boundingBox,
  const vtkBoundingBox& otherBoundingBox, double tol)
{
  if (!boundingBox.Intersects(otherBoundingBox))
  {
    return 0;
  }
  /**
   * Given the strategy of IntersectWithCellImpl,
   * the intersection detection is likely to be speeded up
   * if exchanging other given this condition.
   * The implementation first throws edges from first cell
   * to look if it intersects with second cell, then it checks
   * the other way.
   * Since when one intersection is found, algorithm stops,
   * we'd rather check embedded bounding box's cell's edges first.
   */
  if (otherBoundingBox.IsSubsetOf(boundingBox))
  {
    return IntersectWithCellImpl(other, this, tol);
  }
  return IntersectWithCellImpl(this, other, tol);
}

//----------------------------------------------------------------------------
int vtkCell::IntersectWithCell(vtkCell* other, double tol)
{
  return this->IntersectWithCell(
    other, vtkBoundingBox(this->GetBounds()), vtkBoundingBox(other->GetBounds()), tol);
}

//----------------------------------------------------------------------------
int vtkCell::Triangulate(int index, vtkIdList* ptIds, vtkPoints* pts)
{
  // Convert the local ids to the global ones, plus collect the points
  if (!this->TriangulateLocalIds(index, ptIds))
  {
    return 0;
  }
  pts->SetNumberOfPoints(ptIds->GetNumberOfIds());
  for (int i = 0; i < ptIds->GetNumberOfIds(); i++)
  {
    pts->SetPoint(i, this->Points->GetPoint(ptIds->GetId(i)));
    ptIds->SetId(i, this->PointIds->GetId(ptIds->GetId(i)));
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkCell::TriangulateIds(int index, vtkIdList* ptIds)
{
  // Convert the local ids to the global ones
  if (!this->TriangulateLocalIds(index, ptIds))
  {
    return 0;
  }
  for (int i = 0; i < ptIds->GetNumberOfIds(); i++)
  {
    ptIds->SetId(i, this->PointIds->GetId(ptIds->GetId(i)));
  }
  return 1;
}

//------------------------------------------------------------------------------
// Compute cell bounding box (xmin,xmax,ymin,ymax,zmin,zmax). Copy result into
// user provided array.
void vtkCell::GetBounds(double bounds[6])
{
  vtkBoundingBox::ComputeBounds(this->Points, bounds);
}

//----------------------------------------------------------------------------
// Compute cell bounding box (xmin,xmax,ymin,ymax,zmin,zmax). Return pointer
// to array of six double values.
double* vtkCell::GetBounds()
{
  this->GetBounds(this->Bounds);
  return this->Bounds;
}

//------------------------------------------------------------------------------
// Compute Length squared of cell (i.e., bounding box diagonal squared).
double vtkCell::GetLength2()
{
  double l = 0.0;

  this->GetBounds();
  for (int i = 0; i < 3; i++)
  {
    double diff = this->Bounds[2 * i + 1] - this->Bounds[2 * i];
    l += diff * diff;
  }
  return l;
}

//------------------------------------------------------------------------------
// Return center of the cell in parametric coordinates.
// Note that the parametric center is not always located
// at (0.5,0.5,0.5). The return value is the subId that
// the center is in (if a composite cell). If you want the
// center in x-y-z space, invoke the EvaluateLocation() method.
int vtkCell::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = pcoords[2] = 0.5;
  return 0;
}

//------------------------------------------------------------------------------
// This method works fine for all "rectangular" cells, not triangular
// and tetrahedral topologies.
double vtkCell::GetParametricDistance(const double pcoords[3])
{
  double pDist, pDistMax = 0.0;

  for (int i = 0; i < 3; i++)
  {
    if (pcoords[i] < 0.0)
    {
      pDist = -pcoords[i];
    }
    else if (pcoords[i] > 1.0)
    {
      pDist = pcoords[i] - 1.0;
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
void vtkCell::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  int numIds = this->PointIds->GetNumberOfIds();

  os << indent << "Number Of Points: " << numIds << "\n";

  if (numIds > 0)
  {
    const double* bounds = this->GetBounds();

    os << indent << "Bounds: \n";
    os << indent << "  Xmin,Xmax: (" << bounds[0] << ", " << bounds[1] << ")\n";
    os << indent << "  Ymin,Ymax: (" << bounds[2] << ", " << bounds[3] << ")\n";
    os << indent << "  Zmin,Zmax: (" << bounds[4] << ", " << bounds[5] << ")\n";

    os << indent << "  Point ids are: ";
    for (int i = 0; i < numIds; i++)
    {
      os << this->PointIds->GetId(i);
      if (i && !(i % 12))
      {
        os << "\n\t";
      }
      else
      {
        if (i != (numIds - 1))
        {
          os << ", ";
        }
      }
    }
    os << indent << "\n";
  }
}

// Usually overridden. Only composite cells do not override this.
double* vtkCell::GetParametricCoords()
{
  return nullptr;
}
VTK_ABI_NAMESPACE_END
