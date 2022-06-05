/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCell.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCell.h"

#include "vtkDataArrayRange.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolygon.h"
#include "vtkSmartPointer.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"

#include <vector>

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
  using ConstRefType = typename decltype(points)::ConstTupleReferenceType;

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
        std::max({ std::fabs(center[0]), std::fabs(center[1]), std::fabs(center[2]) });
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

  // Strategy:
  // For each point, store in a buffer its inflated position by moving each
  // incident edge their normal direction by a distance of dist. This new
  // position is done by solving a linear system of equation (intersection of 2
  // lines).

  auto pointRange = vtk::DataArrayTupleRange<3>(this->Points->GetData());
  using ConstTupleRef = typename decltype(pointRange)::ConstTupleReferenceType;
  using TupleRef = typename decltype(pointRange)::TupleReferenceType;
  using ConstScalar = typename ConstTupleRef::value_type;
  using Scalar = typename TupleRef::value_type;

  std::vector<Scalar> buf(3 * pointRange.size());

  Scalar normal[3];
  vtkPolygon::ComputeNormal(this->Points, normal);

  // Matrix transforming the 3D world into a 2D space
  // used for solving line intersection.
  // 2x3 matrix
  Scalar basis[6];

  // This will be used to store consecutive edge line equations
  // 2x2 matrix
  Scalar normals2D[4];

  Scalar edgeNormal3D[3];

  // Offset of the corresponding edge line equations in normals2D, shifted by
  // dist
  Scalar y[2];

  // Intersection coordinates in 2D basis normals2D of the intersection between
  // edges
  Scalar x[2];

  // Current index in normals2D and y. At each iteration, it binary swaps
  int baseId = 1;

  {
    ConstTupleRef p1 = pointRange[this->Points->GetNumberOfPoints() - 1], p2 = pointRange[0];

    // We do not support the case of collapsed edges
    if (vtkMathUtilities::NearlyEqual<ConstScalar>(p1[0], p2[0]) &&
      vtkMathUtilities::NearlyEqual<ConstScalar>(p1[1], p2[1]) &&
      vtkMathUtilities::NearlyEqual<ConstScalar>(p1[2], p2[2]))
    {
      return 0;
    }
    Scalar v[3] = { p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2] };
    vtkMath::Normalize(v);

    // We create a 2D basis with normal to first edge
    vtkMath::Cross(v, normal, basis);
    vtkMath::Cross(normal, basis, basis + 3);

    // In this basis the normal of first edge is (1.0, 0.0)
    normals2D[0] = 1.0;
    normals2D[1] = 0.0;

    // Shifted line offset
    y[0] = dist;
  }

  double* bufIt = buf.data();
  for (vtkIdType pointId = 0; pointId < this->Points->GetNumberOfPoints();
       ++pointId, ++baseId %= 2, bufIt += 3)
  {
    ConstTupleRef p1 = pointRange[pointId];
    ConstTupleRef p2 = pointRange[(pointId + 1) % this->Points->GetNumberOfPoints()];

    // We do not support the case of collapsed edges
    if (vtkMathUtilities::NearlyEqual<ConstScalar>(p1[0], p2[0]) &&
      vtkMathUtilities::NearlyEqual<ConstScalar>(p1[1], p2[1]) &&
      vtkMathUtilities::NearlyEqual<ConstScalar>(p1[2], p2[2]))
    {
      return 0;
    }
    Scalar v[3] = { p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2] };
    vtkMath::Normalize(v);
    vtkMath::Cross(v, normal, edgeNormal3D);
    vtkMath::MultiplyMatrixWithVector<2, 3>(basis, edgeNormal3D, normals2D + 2 * baseId);
    y[baseId] = vtkMath::Dot(edgeNormal3D, p1) + dist;
    if (std::fabs(vtkMath::Dot<Scalar, 2>(normals2D, normals2D + 2) - 1.0) <
      std::numeric_limits<Scalar>::epsilon())
    {
      // Incident edges are colinear, we handle that differently.
      bufIt[0] = p1[0] + dist * edgeNormal3D[0];
      bufIt[1] = p1[1] + dist * edgeNormal3D[1];
      bufIt[2] = p1[2] + dist * edgeNormal3D[2];
    }
    else
    {
      vtkMath::LinearSolve<2, 2>(normals2D, y, x);
      vtkMath::MultiplyMatrixWithVector<3, 2, vtkMatrixUtilities::Layout::Transpose>(
        basis, x, bufIt);
    }
  }

  bufIt = buf.data();
  for (TupleRef point : pointRange)
  {
    point[0] = bufIt[0];
    point[1] = bufIt[1];
    point[2] = bufIt[2];
    bufIt += 3;
  }
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
// Compute cell bounding box (xmin,xmax,ymin,ymax,zmin,zmax). Return pointer
// to array of six double values.
double* vtkCell::GetBounds()
{
  double x[3];
  int i, numPts = this->Points->GetNumberOfPoints();

  if (numPts)
  {
    this->Points->GetPoint(0, x);
    this->Bounds[0] = x[0];
    this->Bounds[2] = x[1];
    this->Bounds[4] = x[2];
    this->Bounds[1] = x[0];
    this->Bounds[3] = x[1];
    this->Bounds[5] = x[2];
    for (i = 1; i < numPts; i++)
    {
      this->Points->GetPoint(i, x);
      this->Bounds[0] = (x[0] < this->Bounds[0] ? x[0] : this->Bounds[0]);
      this->Bounds[1] = (x[0] > this->Bounds[1] ? x[0] : this->Bounds[1]);
      this->Bounds[2] = (x[1] < this->Bounds[2] ? x[1] : this->Bounds[2]);
      this->Bounds[3] = (x[1] > this->Bounds[3] ? x[1] : this->Bounds[3]);
      this->Bounds[4] = (x[2] < this->Bounds[4] ? x[2] : this->Bounds[4]);
      this->Bounds[5] = (x[2] > this->Bounds[5] ? x[2] : this->Bounds[5]);
    }
  }
  else
  {
    vtkMath::UninitializeBounds(this->Bounds);
  }
  return this->Bounds;
}

//------------------------------------------------------------------------------
// Compute cell bounding box (xmin,xmax,ymin,ymax,zmin,zmax). Copy result into
// user provided array.
void vtkCell::GetBounds(double bounds[6])
{
  this->GetBounds();
  for (int i = 0; i < 6; i++)
  {
    bounds[i] = this->Bounds[i];
  }
}

//------------------------------------------------------------------------------
// Compute Length squared of cell (i.e., bounding box diagonal squared).
double vtkCell::GetLength2()
{
  double diff, l = 0.0;
  int i;

  this->GetBounds();
  for (i = 0; i < 3; i++)
  {
    diff = this->Bounds[2 * i + 1] - this->Bounds[2 * i];
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
  int i;
  double pDist, pDistMax = 0.0;

  for (i = 0; i < 3; i++)
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
    if (pDist > pDistMax)
    {
      pDistMax = pDist;
    }
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
