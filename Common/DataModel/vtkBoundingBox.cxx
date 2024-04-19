// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkBoundingBox.h"
#include "vtkArrayDispatch.h"
#include "vtkDataArrayRange.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkPlane.h"
#include "vtkPoints.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"

#include <array>
#include <cassert>
#include <cmath>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
namespace
{
inline double Sign(const double& a)
{
  return a > 0.0 ? 1.0 : (a < 0 ? -1.0 : 0.0);
}
inline bool OppSign(const double& a, const double& b)
{
  return (a <= 0 && b >= 0) || (a >= 0 && b <= 0);
}
}

//------------------------------------------------------------------------------
void vtkBoundingBox::AddPoint(double px, double py, double pz)
{
  double p[3];
  p[0] = px;
  p[1] = py;
  p[2] = pz;
  this->AddPoint(p);
}

//------------------------------------------------------------------------------
void vtkBoundingBox::AddPoint(double p[3])
{
  int i;
  for (i = 0; i < 3; i++)
  {
    if (p[i] < this->MinPnt[i])
    {
      this->MinPnt[i] = p[i];
    }

    if (p[i] > this->MaxPnt[i])
    {
      this->MaxPnt[i] = p[i];
    }
  }
}

//------------------------------------------------------------------------------
void vtkBoundingBox::AddBox(const vtkBoundingBox& bbox)
{
  double bds[6];
  bbox.GetBounds(bds);
  this->AddBounds(bds);
}

//------------------------------------------------------------------------------
void vtkBoundingBox::AddBounds(const double bounds[6])
{
  bool this_valid = (this->IsValid() != 0);
  bool other_valid = (vtkBoundingBox::IsValid(bounds) != 0);

  if (!other_valid)
  {
    return;
  }

  if (other_valid && !this_valid)
  {
    this->SetBounds(bounds);
    return;
  }

  if (bounds[0] < this->MinPnt[0])
  {
    this->MinPnt[0] = bounds[0];
  }

  if (bounds[1] > this->MaxPnt[0])
  {
    this->MaxPnt[0] = bounds[1];
  }

  if (bounds[2] < this->MinPnt[1])
  {
    this->MinPnt[1] = bounds[2];
  }

  if (bounds[3] > this->MaxPnt[1])
  {
    this->MaxPnt[1] = bounds[3];
  }

  if (bounds[4] < this->MinPnt[2])
  {
    this->MinPnt[2] = bounds[4];
  }

  if (bounds[5] > this->MaxPnt[2])
  {
    this->MaxPnt[2] = bounds[5];
  }
}

//------------------------------------------------------------------------------
void vtkBoundingBox::SetBounds(
  double xMin, double xMax, double yMin, double yMax, double zMin, double zMax)
{
  this->MinPnt[0] = xMin;
  this->MaxPnt[0] = xMax;
  this->MinPnt[1] = yMin;
  this->MaxPnt[1] = yMax;
  this->MinPnt[2] = zMin;
  this->MaxPnt[2] = zMax;
}

//------------------------------------------------------------------------------
void vtkBoundingBox::SetMinPoint(double x, double y, double z)
{
  this->MinPnt[0] = x;
  if (x > this->MaxPnt[0])
  {
    this->MaxPnt[0] = x;
  }

  this->MinPnt[1] = y;
  if (y > this->MaxPnt[1])
  {
    this->MaxPnt[1] = y;
  }

  this->MinPnt[2] = z;
  if (z > this->MaxPnt[2])
  {
    this->MaxPnt[2] = z;
  }
}

//------------------------------------------------------------------------------
void vtkBoundingBox::SetMaxPoint(double x, double y, double z)
{
  this->MaxPnt[0] = x;
  if (x < this->MinPnt[0])
  {
    this->MinPnt[0] = x;
  }

  this->MaxPnt[1] = y;
  if (y < this->MinPnt[1])
  {
    this->MinPnt[1] = y;
  }

  this->MaxPnt[2] = z;
  if (z < this->MinPnt[2])
  {
    this->MinPnt[2] = z;
  }
}

//------------------------------------------------------------------------------
void vtkBoundingBox::Inflate(double deltaX, double deltaY, double deltaZ)
{
  this->MinPnt[0] -= deltaX;
  this->MaxPnt[0] += deltaX;
  this->MinPnt[1] -= deltaY;
  this->MaxPnt[1] += deltaY;
  this->MinPnt[2] -= deltaZ;
  this->MaxPnt[2] += deltaZ;
}

//------------------------------------------------------------------------------
void vtkBoundingBox::Inflate(double delta)
{
  this->Inflate(delta, delta, delta);
}

//------------------------------------------------------------------------------
// Adjust bounding box so that it contains a non-zero volume.  Note that zero
// widths are expanded by the arbitrary 1% of the maximum width. If all
// edge widths are zero, then the box is expanded by 0.5 in each direction.
void vtkBoundingBox::Inflate()
{
  // First determine the maximum length of the side of the bounds. Keep track
  // of zero width sides of the bounding box.
  int nonZero[3], maxIdx = (-1);
  double w, max = 0.0;
  for (int i = 0; i < 3; ++i)
  {
    if ((w = (this->MaxPnt[i] - this->MinPnt[i])) > max)
    {
      max = w;
      maxIdx = i;
    }
    nonZero[i] = (w > 0.0 ? 1 : 0);
  }

  // If the bounding box is degenerate, then bump out to arbitrary size.
  if (maxIdx < 0)
  {
    this->Inflate(0.5);
  }
  else // any zero width sides are bumped out 1% of max side
  {
    double delta;
    for (int i = 0; i < 3; ++i)
    {
      if (!nonZero[i])
      {
        delta = 0.005 * max;
        this->MinPnt[i] -= delta;
        this->MaxPnt[i] += delta;
      }
    }
  }
}

//------------------------------------------------------------------------------
// Make sure the length of all sides of the bounding box are non-zero, and at
// least 2*delta thick.
void vtkBoundingBox::InflateSlice(double delta)
{
  for (auto i = 0; i < 3; ++i)
  {
    double w = this->MaxPnt[i] - this->MinPnt[i];
    if (w < (2.0 * delta))
    {
      this->MinPnt[i] -= delta;
      this->MaxPnt[i] += delta;
    }
  }
}

//------------------------------------------------------------------------------
int vtkBoundingBox::IntersectBox(const vtkBoundingBox& bbox)
{
  // if either box is not valid don't do the operation
  if (!(this->IsValid() && bbox.IsValid()))
  {
    return 0;
  }

  bool intersects;
  double pMin[3], pMax[3];
  for (unsigned i = 0; i < 3; i++)
  {
    intersects = false;
    if ((bbox.MinPnt[i] >= this->MinPnt[i]) && (bbox.MinPnt[i] <= this->MaxPnt[i]))
    {
      intersects = true;
      pMin[i] = bbox.MinPnt[i];
    }
    else if ((this->MinPnt[i] >= bbox.MinPnt[i]) && (this->MinPnt[i] <= bbox.MaxPnt[i]))
    {
      intersects = true;
      pMin[i] = this->MinPnt[i];
    }
    if ((bbox.MaxPnt[i] >= this->MinPnt[i]) && (bbox.MaxPnt[i] <= this->MaxPnt[i]))
    {
      intersects = true;
      pMax[i] = bbox.MaxPnt[i];
    }
    else if ((this->MaxPnt[i] >= bbox.MinPnt[i]) && (this->MaxPnt[i] <= bbox.MaxPnt[i]))
    {
      intersects = true;
      pMax[i] = this->MaxPnt[i];
    }
    if (!intersects)
    {
      return 0;
    }
  }

  // OK they did intersect - set the box to be the result
  for (unsigned i = 0; i < 3; i++)
  {
    this->MinPnt[i] = pMin[i];
    this->MaxPnt[i] = pMax[i];
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkBoundingBox::Intersects(const vtkBoundingBox& bbox) const
{
  // if either box is not valid they don't intersect
  if (!(this->IsValid() && bbox.IsValid()))
  {
    return 0;
  }
  int i;
  for (i = 0; i < 3; i++)
  {
    if ((bbox.MinPnt[i] >= this->MinPnt[i]) && (bbox.MinPnt[i] <= this->MaxPnt[i]))
    {
      continue;
    }
    if ((this->MinPnt[i] >= bbox.MinPnt[i]) && (this->MinPnt[i] <= bbox.MaxPnt[i]))
    {
      continue;
    }
    if ((bbox.MaxPnt[i] >= this->MinPnt[i]) && (bbox.MaxPnt[i] <= this->MaxPnt[i]))
    {
      continue;
    }
    if ((this->MaxPnt[i] >= bbox.MinPnt[i]) && (this->MaxPnt[i] <= bbox.MaxPnt[i]))
    {
      continue;
    }
    return 0;
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkBoundingBox::Contains(const vtkBoundingBox& bbox) const
{
  // if either box is not valid or they don't intersect
  if (!this->Intersects(bbox))
  {
    return 0;
  }
  const double* pt = bbox.GetMinPoint();
  if (!this->ContainsPoint(pt[0], pt[1], pt[2]))
  {
    return 0;
  }
  pt = bbox.GetMaxPoint();
  if (!this->ContainsPoint(pt[0], pt[1], pt[2]))
  {
    return 0;
  }
  return 1;
}

//------------------------------------------------------------------------------
bool vtkBoundingBox::ContainsLine(const double x[3], const double s[3], const double lineEnd[3],
  double& t, double xInt[3], int& plane)
{
  double v[3], tMin = VTK_DOUBLE_MAX;
  double halfBox[3] = { s[0] / 2.0, s[1] / 2.0, s[2] / 2.0 };
  for (auto i = 0; i < 3; ++i)
  {
    v[i] = (lineEnd[i] - x[i]);

    // Compare to - plane
    if (v[i] < -halfBox[i])
    {
      if ((t = (-halfBox[i] / v[i])) < tMin)
      {
        tMin = t;
        plane = 2 * i;
      }
    }
    // else compare to + plane
    else if (v[i] > halfBox[i])
    {
      if ((t = (halfBox[i] / v[i])) < tMin)
      {
        tMin = t;
        plane = 2 * i + 1;
      }
    }
  } // for all 3 pairs of +/- planes

  // See if there are any intersections, and if so compute intersection point.
  if (tMin == VTK_DOUBLE_MAX)
  {
    return true; // line is contained by box
  }
  else
  {
    t = tMin;
    xInt[0] = x[0] + t * v[0];
    xInt[1] = x[1] + t * v[1];
    xInt[2] = x[2] + t * v[2];
    return false; // line is not contained by box
  }
}

//------------------------------------------------------------------------------
double vtkBoundingBox::GetMaxLength() const
{
  double l[3];
  this->GetLengths(l);
  if (l[0] > l[1])
  {
    if (l[0] > l[2])
    {
      return l[0];
    }
    return l[2];
  }
  else if (l[1] > l[2])
  {
    return l[1];
  }
  return l[2];
}

//------------------------------------------------------------------------------
double vtkBoundingBox::GetDiagonalLength2() const
{
  assert("pre: not_empty" && this->IsValid());

  double l[3];
  this->GetLengths(l);

  return l[0] * l[0] + l[1] * l[1] + l[2] * l[2];
}

//------------------------------------------------------------------------------
double vtkBoundingBox::GetDiagonalLength() const
{
  return std::sqrt(this->GetDiagonalLength2());
}

//------------------------------------------------------------------------------
// Description:
// Scale each dimension of the box by some given factor.
// If the box is not valid, it stays unchanged.
// If the scalar factor is negative, bounds are flipped: for example,
// if (xMin,xMax)=(-2,4) and sx=-3, (xMin,xMax) becomes (-12,6).
void vtkBoundingBox::Scale(double sx, double sy, double sz)
{
  if (this->IsValid())
  {
    if (sx >= 0.0)
    {
      this->MinPnt[0] *= sx;
      this->MaxPnt[0] *= sx;
    }
    else
    {
      double tmp = this->MinPnt[0];
      this->MinPnt[0] = sx * this->MaxPnt[0];
      this->MaxPnt[0] = sx * tmp;
    }

    if (sy >= 0.0)
    {
      this->MinPnt[1] *= sy;
      this->MaxPnt[1] *= sy;
    }
    else
    {
      double tmp = this->MinPnt[1];
      this->MinPnt[1] = sy * this->MaxPnt[1];
      this->MaxPnt[1] = sy * tmp;
    }

    if (sz >= 0.0)
    {
      this->MinPnt[2] *= sz;
      this->MaxPnt[2] *= sz;
    }
    else
    {
      double tmp = this->MinPnt[2];
      this->MinPnt[2] = sz * this->MaxPnt[2];
      this->MaxPnt[2] = sz * tmp;
    }
  }
}

//------------------------------------------------------------------------------
void vtkBoundingBox::Scale(double s[3])
{
  this->Scale(s[0], s[1], s[2]);
}

//------------------------------------------------------------------------------
void vtkBoundingBox::ScaleAboutCenter(double s)
{
  this->ScaleAboutCenter(s, s, s);
}

//------------------------------------------------------------------------------
// Scale the box around the bounding box center point.
void vtkBoundingBox::ScaleAboutCenter(double sx, double sy, double sz)
{
  if (this->IsValid())
  {
    double center[3];
    this->GetCenter(center);

    this->MinPnt[0] = center[0] + sx * (this->MinPnt[0] - center[0]);
    this->MaxPnt[0] = center[0] + sx * (this->MaxPnt[0] - center[0]);

    this->MinPnt[1] = center[1] + sy * (this->MinPnt[1] - center[1]);
    this->MaxPnt[1] = center[1] + sy * (this->MaxPnt[1] - center[1]);

    this->MinPnt[2] = center[2] + sz * (this->MinPnt[2] - center[2]);
    this->MaxPnt[2] = center[2] + sz * (this->MaxPnt[2] - center[2]);
  }
}

//------------------------------------------------------------------------------
void vtkBoundingBox::ScaleAboutCenter(double s[3])
{
  this->ScaleAboutCenter(s[0], s[1], s[2]);
}

//------------------------------------------------------------------------------
// Compute the number of divisions given the current bounding box and a
// target number of buckets/bins. Note that degenerate bounding boxes (i.e.,
// one or more of the edges are zero length) are handled properly.
vtkIdType vtkBoundingBox::ComputeDivisions(vtkIdType totalBins, double bounds[6], int divs[3]) const
{
  // This will always produce at least one bin
  totalBins = (totalBins <= 0 ? 1 : totalBins);

  // First determine the maximum length of the side of the bounds. Keep track
  // of zero width sides of the bounding box.
  int numNonZero = 0, nonZero[3], maxIdx = (-1);
  double max = 0.0, lengths[3];
  this->GetLengths(lengths);

  // Use a finite tolerance when detecting zero width sides to ensure that
  // numerical noise doesn't cause an explosion later on. We'll consider any
  // length that's less than 0.1% of the average length to be zero:
  double totLen = lengths[0] + lengths[1] + lengths[2];
  const double zeroDetectionTolerance = totLen * (0.001 / 3.);

  for (int i = 0; i < 3; ++i)
  {
    if (lengths[i] > max)
    {
      maxIdx = i;
      max = lengths[i];
    }
    if (lengths[i] > zeroDetectionTolerance)
    {
      nonZero[i] = 1;
      numNonZero++;
    }
    else
    {
      nonZero[i] = 0;
    }
  }

  // If the bounding box is degenerate, then one bin of arbitrary size
  if (numNonZero < 1)
  {
    divs[0] = divs[1] = divs[2] = 1;
    bounds[0] = this->MinPnt[0] - 0.5;
    bounds[1] = this->MaxPnt[0] + 0.5;
    bounds[2] = this->MinPnt[1] - 0.5;
    bounds[3] = this->MaxPnt[1] + 0.5;
    bounds[4] = this->MinPnt[2] - 0.5;
    bounds[5] = this->MaxPnt[2] + 0.5;
    return 1;
  }

  // Okay we need to compute the divisions roughly in proportion to the
  // bounding box edge lengths.  The idea is to make the bins as close to a
  // cube as possible. Ensure that the number of divisions is valid.
  double f = static_cast<double>(totalBins);
  f /= (nonZero[0] ? (lengths[0] / totLen) : 1.0);
  f /= (nonZero[1] ? (lengths[1] / totLen) : 1.0);
  f /= (nonZero[2] ? (lengths[2] / totLen) : 1.0);
  f = pow(f, (1.0 / static_cast<double>(numNonZero)));

  for (int i = 0; i < 3; ++i)
  {
    divs[i] = (nonZero[i] ? vtkMath::Floor(f * lengths[i] / totLen) : 1);
    divs[i] = (divs[i] < 1 ? 1 : divs[i]);
  }

  // Make sure that we do not exceed the totalBins, brute force the divs[]
  // as necessary.
  vtkBoundingBox::ClampDivisions(totalBins, divs);

  // Now compute the final bounds, making sure it is a non-zero volume.
  double delta = 0.5 * lengths[maxIdx] / static_cast<double>(divs[maxIdx]);
  for (int i = 0; i < 3; ++i)
  {
    if (nonZero[i])
    {
      bounds[2 * i] = this->MinPnt[i];
      bounds[2 * i + 1] = this->MaxPnt[i];
    }
    else
    {
      bounds[2 * i] = this->MinPnt[i] - delta;
      bounds[2 * i + 1] = this->MaxPnt[i] + delta;
    }
  }
  return static_cast<vtkIdType>(divs[0]) * divs[1] * divs[2];
}

//------------------------------------------------------------------------------
void vtkBoundingBox::ClampDivisions(vtkIdType targetBins, int divs[3])
{
  for (int i = 0; i < 3; ++i)
  {
    divs[i] = ((divs[i] < 1) ? 1 : divs[i]);
  }
  vtkIdType numBins = divs[0] * divs[1] * divs[2];
  while (numBins > targetBins)
  {
    for (int i = 0; i < 3; ++i)
    {
      divs[i] = ((divs[i] > 1) ? (divs[i] - 1) : 1);
    }
    numBins = divs[0] * divs[1] * divs[2];
  }
}

// ---------------------------------------------------------------------------
// Description:
// Intersect this box with the half space defined by plane.
// Returns 1 if there is intersection---which implies that the box has been modified
// Returns 0 otherwise
// The algorithm:
//   Because the change can only happens in one axis aligned direction,
//   we first figure out which direction it is (stored in dir), then
//   update the bounding interval in that direction based on intersection
//   of the plane with the four edges

bool vtkBoundingBox::IntersectPlane(double origin[3], double normal[3])
{
  double* bounds[2] = { this->MinPnt, this->MaxPnt };
  assert(this->IsValid());

  // Index[0..2] represents the order of traversing the corners of a cube
  //  in (x,y,z), (y,x,z) and (z,x,y) ordering, respectively
  static const int Index[3][8] = {
    { 0, 1, 2, 3, 4, 5, 6, 7 },
    { 0, 1, 4, 5, 2, 3, 6, 7 },
    { 0, 2, 4, 6, 1, 3, 5, 7 },
  };

  double d[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // stores the signed distance to a plane
  {
    int index(-1);
    for (int ix = 0; ix <= 1; ix++)
    {
      for (int iy = 0; iy <= 1; iy++)
      {
        for (int iz = 0; iz <= 1; iz++)
        {
          double x[3] = { bounds[ix][0], bounds[iy][1], bounds[iz][2] };
          d[++index] = vtkPlane::Evaluate(normal, origin, x);
        }
      }
    }
  }

  int dir(-1);
  for (dir = 2; dir >= 0; dir--)
  {
    // in each direction, we test if the vertices of two orthogonal faces
    // are on either side of the plane
    if (OppSign(d[Index[dir][0]], d[Index[dir][4]]) &&
      OppSign(d[Index[dir][1]], d[Index[dir][5]]) && OppSign(d[Index[dir][2]], d[Index[dir][6]]) &&
      OppSign(d[Index[dir][3]], d[Index[dir][7]]))
    {
      break;
    }
  }
  if (dir < 0)
  {
    return false;
  }

  double sign = Sign(normal[dir]);
  double size = fabs((bounds[1][dir] - bounds[0][dir]) * normal[dir]);
  double t = sign > 0 ? 1 : 0;
  for (int i = 0; i < 4; i++)
  {
    if (size == 0)
      continue; // shouldn't happen
    double ti = fabs(d[Index[dir][i]]) / size;
    if (sign > 0 && ti < t)
    {
      t = ti;
    }
    if (sign < 0 && ti > t)
    {
      t = ti;
    }
  }
  double bound = (1.0 - t) * bounds[0][dir] + t * bounds[1][dir];

  if (sign > 0)
  {
    bounds[0][dir] = bound;
  }
  else
  {
    bounds[1][dir] = bound;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkBoundingBox::IntersectsSphere(double center[3], double radius) const
{
  return center[0] >= this->MinPnt[0] - radius && center[0] <= this->MaxPnt[0] + radius &&
    center[1] >= this->MinPnt[1] - radius && center[1] <= this->MaxPnt[1] + radius &&
    center[2] >= this->MinPnt[2] - radius && center[2] <= this->MaxPnt[2] + radius;
}

// ---------------------------------------------------------------------------
bool vtkBoundingBox::IntersectsLine(const double p1[3], const double p2[3]) const
{
  if (this->ContainsPoint(p1) || this->ContainsPoint(p2))
  {
    return true;
  }

  if (vtkMathUtilities::NearlyEqual(p1[0], p2[0]) && vtkMathUtilities::NearlyEqual(p1[1], p2[1]) &&
    vtkMathUtilities::NearlyEqual(p1[2], p2[2]))
  {
    return false;
  }

  double line[3];
  vtkMath::Subtract(p2, p1, line);
  vtkMath::Normalize(line);

  const double* points[2] = { p1, p2 };
  const double* bbPoints[2] = { this->MinPnt, this->MaxPnt };

  for (int dim = 0; dim < 3; ++dim)
  {
    if (std::abs(line[dim]) > VTK_DBL_EPSILON)
    {
      for (int pointId = 0; pointId < 2; ++pointId)
      {
        const double* p = points[pointId];
        const double* bbp = bbPoints[pointId];
        double t = (bbp[dim] - p[dim]) / line[dim];
        int orthdimx = (dim + 1) % 3;
        int orthdimy = (dim + 2) % 3;
        double x = p[orthdimx] + t * line[orthdimx];
        double y = p[orthdimy] + t * line[orthdimy];
        if (x - this->MinPnt[orthdimx] >=
            -VTK_DBL_EPSILON * std::max(std::abs(x), std::abs(this->MinPnt[orthdimx])) &&
          x - this->MaxPnt[orthdimx] <=
            VTK_DBL_EPSILON * std::max(std::abs(x), std::abs(this->MaxPnt[orthdimx])) &&
          y - this->MinPnt[orthdimy] >=
            -VTK_DBL_EPSILON * std::max(std::abs(x), std::abs(this->MinPnt[orthdimy])) &&
          y - this->MaxPnt[orthdimy] <=
            VTK_DBL_EPSILON * std::max(std::abs(x), std::abs(this->MaxPnt[orthdimy])))
        {
          return true;
        }
      }
    }
  }
  return false;
}

// ---------------------------------------------------------------------------
int vtkBoundingBox::ComputeInnerDimension() const
{
  double thickness = this->MaxPnt[0] - this->MinPnt[0];
  int dim = 3;
  if (std::abs(thickness) <=
    std::max(std::fabs(this->MaxPnt[0]), std::fabs(this->MinPnt[0])) * VTK_DBL_EPSILON)
  {
    --dim;
  }
  thickness = this->MaxPnt[1] - this->MinPnt[1];
  if (std::abs(thickness) <=
    std::max(std::fabs(this->MaxPnt[1]), std::fabs(this->MinPnt[1])) * VTK_DBL_EPSILON)
  {
    --dim;
  }
  thickness = this->MaxPnt[2] - this->MinPnt[2];
  if (std::fabs(thickness) <=
    std::max(std::fabs(this->MaxPnt[2]), std::fabs(this->MinPnt[2])) * VTK_DBL_EPSILON)
  {
    --dim;
  }
  return dim;
}

// Support ComputeBounds()
namespace
{
// ---------------------------------------------------------------------------
// Base Bounds Functor
template <typename TPointsArray>
struct BaseBoundsFunctor
{
  TPointsArray* PointsArray;
  double* Bounds;

  BaseBoundsFunctor(TPointsArray* pointsArray, double* bounds)
    : PointsArray(pointsArray)
    , Bounds(bounds)
  {
  }
  virtual ~BaseBoundsFunctor() = default;
};

// ---------------------------------------------------------------------------
// Threaded Base Bounds Functor
template <typename TPointsArray>
struct ThreadedBaseBoundsFunctor : public BaseBoundsFunctor<TPointsArray>
{
  vtkSMPThreadLocal<std::array<double, 6>> LocalBounds;

  ThreadedBaseBoundsFunctor(TPointsArray* pts, double* bds)
    : BaseBoundsFunctor<TPointsArray>(pts, bds)
  {
  }
  ~ThreadedBaseBoundsFunctor() override = default;
  virtual void Initialize()
  {
    std::array<double, 6>& localBds = this->LocalBounds.Local();
    localBds[0] = localBds[2] = localBds[4] = VTK_DOUBLE_MAX;
    localBds[1] = localBds[3] = localBds[5] = VTK_DOUBLE_MIN;
  }
  virtual void operator()(vtkIdType begin, vtkIdType end) = 0;
  virtual void Reduce()
  {
    // Composite bounds from all threads
    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = VTK_DOUBLE_MIN;
    for (const auto& localBds : this->LocalBounds)
    {
      this->Bounds[0] = std::min(this->Bounds[0], localBds[0]);
      this->Bounds[1] = std::max(this->Bounds[1], localBds[1]);
      this->Bounds[2] = std::min(this->Bounds[2], localBds[2]);
      this->Bounds[3] = std::max(this->Bounds[3], localBds[3]);
      this->Bounds[4] = std::min(this->Bounds[4], localBds[4]);
      this->Bounds[5] = std::max(this->Bounds[5], localBds[5]);
    }
  }
};

// ---------------------------------------------------------------------------
// Serial bounds functor
template <typename TPointsArray>
struct SerialBoundsFunctor : public BaseBoundsFunctor<TPointsArray>
{
  SerialBoundsFunctor(TPointsArray* pts, double* bds)
    : BaseBoundsFunctor<TPointsArray>(pts, bds)
  {
  }
  ~SerialBoundsFunctor() override = default;

  void operator()(vtkIdType numberOfPoints)
  {
    if (numberOfPoints == 0)
    {
      vtkMath::UninitializeBounds(this->Bounds);
      return;
    }
    const auto points = vtk::DataArrayTupleRange<3>(this->PointsArray);
    double point[3];
    // Initialize bounds to first point:
    {
      // Explicitly reusing a local will improve performance when virtual
      // calls are involved in the iterator read:
      points.GetTuple(0, point);

      this->Bounds[0] = point[0];
      this->Bounds[1] = point[0];
      this->Bounds[2] = point[1];
      this->Bounds[3] = point[1];
      this->Bounds[4] = point[2];
      this->Bounds[5] = point[2];
    }
    // Reduce bounds with the rest of the ids:
    for (vtkIdType i = 1; i < numberOfPoints; ++i)
    {
      // Explicitly reusing a local will improve performance when virtual
      // calls are involved in the iterator read:
      points.GetTuple(i, point);

      this->Bounds[0] = std::min(this->Bounds[0], point[0]);
      this->Bounds[1] = std::max(this->Bounds[1], point[0]);
      this->Bounds[2] = std::min(this->Bounds[2], point[1]);
      this->Bounds[3] = std::max(this->Bounds[3], point[1]);
      this->Bounds[4] = std::min(this->Bounds[4], point[2]);
      this->Bounds[5] = std::max(this->Bounds[5], point[2]);
    }
  }
};

// ---------------------------------------------------------------------------
// Threaded bounds functor
template <typename TPointsArray>
struct ThreadedBoundsFunctor : public ThreadedBaseBoundsFunctor<TPointsArray>
{
  ThreadedBoundsFunctor(TPointsArray* pts, double* bds)
    : ThreadedBaseBoundsFunctor<TPointsArray>(pts, bds)
  {
  }
  ~ThreadedBoundsFunctor() override = default;

  void Initialize() override { this->ThreadedBaseBoundsFunctor<TPointsArray>::Initialize(); }

  void operator()(vtkIdType beginPtId, vtkIdType endPtId) override
  {
    std::array<double, 6>& localBds = this->LocalBounds.Local();
    const auto points = vtk::DataArrayTupleRange<3>(this->PointsArray);
    double point[3];

    for (vtkIdType i = beginPtId; i < endPtId; ++i)
    {
      // Explicitly reusing a local will improve performance when virtual
      // calls are involved in the iterator read:
      points.GetTuple(i, point);

      localBds[0] = std::min(localBds[0], point[0]);
      localBds[1] = std::max(localBds[1], point[0]);
      localBds[2] = std::min(localBds[2], point[1]);
      localBds[3] = std::max(localBds[3], point[1]);
      localBds[4] = std::min(localBds[4], point[2]);
      localBds[5] = std::max(localBds[5], point[2]);
    }
  }

  void Reduce() override { this->ThreadedBaseBoundsFunctor<TPointsArray>::Reduce(); }
};

// ---------------------------------------------------------------------------
// Serial bounds with point uses
template <typename TPointsArray, typename TUsed>
struct SerialBoundsPointUsesFunctor : public BaseBoundsFunctor<TPointsArray>
{
  const TUsed* PointUses;
  SerialBoundsPointUsesFunctor(TPointsArray* pts, const TUsed* ptUses, double* bds)
    : BaseBoundsFunctor<TPointsArray>(pts, bds)
    , PointUses(ptUses)
  {
  }
  ~SerialBoundsPointUsesFunctor() override = default;

  void operator()(vtkIdType numberOfPoints)
  {
    if (numberOfPoints == 0)
    {
      vtkMath::UninitializeBounds(this->Bounds);
      return;
    }

    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = VTK_DOUBLE_MIN;

    const auto points = vtk::DataArrayTupleRange<3>(this->PointsArray);
    const TUsed* used = static_cast<const TUsed*>(this->PointUses);
    double point[3];

    for (vtkIdType i = 0; i < numberOfPoints; ++i)
    {
      if (*used)
      {
        // Explicitly reusing a local will improve performance when virtual
        // calls are involved in the iterator read:
        points.GetTuple(i, point);

        this->Bounds[0] = std::min(this->Bounds[0], point[0]);
        this->Bounds[1] = std::max(this->Bounds[1], point[0]);
        this->Bounds[2] = std::min(this->Bounds[2], point[1]);
        this->Bounds[3] = std::max(this->Bounds[3], point[1]);
        this->Bounds[4] = std::min(this->Bounds[4], point[2]);
        this->Bounds[5] = std::max(this->Bounds[5], point[2]);
      }
      ++used;
    }
  }
};

// ---------------------------------------------------------------------------
// Threaded bounds with point uses
template <typename TPointsArray, typename TUsed>
struct ThreadedBoundsPointUsesFunctor : public ThreadedBaseBoundsFunctor<TPointsArray>
{
  const TUsed* PointUses;

  ThreadedBoundsPointUsesFunctor(TPointsArray* pts, const TUsed* ptUses, double* bds)
    : ThreadedBaseBoundsFunctor<TPointsArray>(pts, bds)
    , PointUses(ptUses)
  {
  }
  ~ThreadedBoundsPointUsesFunctor() override = default;

  void Initialize() override { this->ThreadedBaseBoundsFunctor<TPointsArray>::Initialize(); }

  void operator()(vtkIdType beginPtId, vtkIdType endPtId) override
  {
    std::array<double, 6>& localBds = this->LocalBounds.Local();
    const auto points = vtk::DataArrayTupleRange<3>(this->PointsArray);
    const TUsed* used = static_cast<const TUsed*>(this->PointUses + beginPtId);
    double point[3];

    for (vtkIdType i = beginPtId; i < endPtId; ++i)
    {
      if (*used)
      {
        // Explicitly reusing a local will improve performance when virtual
        // calls are involved in the iterator read:
        points.GetTuple(i, point);

        localBds[0] = std::min(localBds[0], point[0]);
        localBds[1] = std::max(localBds[1], point[0]);
        localBds[2] = std::min(localBds[2], point[1]);
        localBds[3] = std::max(localBds[3], point[1]);
        localBds[4] = std::min(localBds[4], point[2]);
        localBds[5] = std::max(localBds[5], point[2]);
      }
      ++used;
    }
  }

  void Reduce() override { this->ThreadedBaseBoundsFunctor<TPointsArray>::Reduce(); }
};

// ---------------------------------------------------------------------------
// Serial bounds with point ids
template <typename TPointsArray, typename TId>
struct SerialBoundsPointIdsFunctor : public BaseBoundsFunctor<TPointsArray>
{
  const TId* PointIds;

  SerialBoundsPointIdsFunctor(TPointsArray* pts, const TId* ptIds, double* bds)
    : BaseBoundsFunctor<TPointsArray>(pts, bds)
    , PointIds(ptIds)
  {
  }
  ~SerialBoundsPointIdsFunctor() override = default;

  void operator()(vtkIdType numberOfPoints)
  {
    if (numberOfPoints == 0)
    {
      vtkMath::UninitializeBounds(this->Bounds);
      return;
    }
    const auto points = vtk::DataArrayTupleRange<3>(this->PointsArray);
    double point[3];
    // Initialize bounds to first point:
    {
      // Explicitly reusing a local will improve performance when virtual
      // calls are involved in the iterator read:
      points.GetTuple(this->PointIds[0], point);

      this->Bounds[0] = point[0];
      this->Bounds[1] = point[0];
      this->Bounds[2] = point[1];
      this->Bounds[3] = point[1];
      this->Bounds[4] = point[2];
      this->Bounds[5] = point[2];
    }
    // Reduce bounds with the rest of the ids:
    for (vtkIdType i = 1; i < numberOfPoints; ++i)
    {
      // Explicitly reusing a local will improve performance when virtual
      // calls are involved in the iterator read:
      points.GetTuple(this->PointIds[i], point);

      this->Bounds[0] = std::min(this->Bounds[0], point[0]);
      this->Bounds[1] = std::max(this->Bounds[1], point[0]);
      this->Bounds[2] = std::min(this->Bounds[2], point[1]);
      this->Bounds[3] = std::max(this->Bounds[3], point[1]);
      this->Bounds[4] = std::min(this->Bounds[4], point[2]);
      this->Bounds[5] = std::max(this->Bounds[5], point[2]);
    }
  }
};

// ---------------------------------------------------------------------------
// Threaded bounds with point ids
template <typename TPointsArray, typename TId>
struct ThreadedBoundsPointIdsFunctor : public ThreadedBaseBoundsFunctor<TPointsArray>
{
  const TId* PointIds;

  ThreadedBoundsPointIdsFunctor(TPointsArray* pts, const TId* ptIds, double* bds)
    : ThreadedBaseBoundsFunctor<TPointsArray>(pts, bds)
    , PointIds(ptIds)
  {
  }
  ~ThreadedBoundsPointIdsFunctor() override = default;

  void Initialize() override { this->ThreadedBaseBoundsFunctor<TPointsArray>::Initialize(); }

  void operator()(vtkIdType beginPtId, vtkIdType endPtId) override
  {
    std::array<double, 6>& localBds = this->LocalBounds.Local();
    const auto points = vtk::DataArrayTupleRange<3>(this->PointsArray);
    double point[3];
    // Reduce bounds with the rest of the ids:
    for (vtkIdType i = beginPtId; i < endPtId; ++i)
    {
      // Explicitly reusing a local will improve performance when virtual
      // calls are involved in the iterator read:
      points.GetTuple(this->PointIds[i], point);

      localBds[0] = std::min(localBds[0], point[0]);
      localBds[1] = std::max(localBds[1], point[0]);
      localBds[2] = std::min(localBds[2], point[1]);
      localBds[3] = std::max(localBds[3], point[1]);
      localBds[4] = std::min(localBds[4], point[2]);
      localBds[5] = std::max(localBds[5], point[2]);
    }
  }

  void Reduce() override { this->ThreadedBaseBoundsFunctor<TPointsArray>::Reduce(); }
};

// ---------------------------------------------------------------------------
struct BoundsWorker
{
  template <typename TPointsArray>
  void operator()(TPointsArray* pts, double* bds)
  {
    const vtkIdType numPts = pts->GetNumberOfTuples();
    // We use THRESHOLD to test if the data size is small enough
    // to execute the functor serially. This is faster.
    // and also potentially avoids nested multithreading which creates race conditions.
    if (numPts <= vtkSMPTools::THRESHOLD)
    {
      SerialBoundsFunctor<TPointsArray> serialBds(pts, bds);
      serialBds(numPts);
    }
    else
    {
      ThreadedBoundsFunctor<TPointsArray> threadedBds(pts, bds);
      vtkSMPTools::For(0, numPts, threadedBds);
    }
  }
};

// ---------------------------------------------------------------------------
struct BoundsPointUsesWorker
{
  template <typename TPointsArray, typename TUsed>
  void operator()(TPointsArray* pts, const TUsed* ptUses, double* bds)
  {
    const vtkIdType numPts = pts->GetNumberOfTuples();
    // We use THRESHOLD to test if the data size is small enough
    // to execute the functor serially. This is faster.
    // and also potentially avoids nested multithreading which creates race conditions.
    if (numPts <= vtkSMPTools::THRESHOLD)
    {
      SerialBoundsPointUsesFunctor<TPointsArray, TUsed> serialBds(pts, ptUses, bds);
      serialBds(numPts);
    }
    else
    {
      ThreadedBoundsPointUsesFunctor<TPointsArray, TUsed> threadedBds(pts, ptUses, bds);
      vtkSMPTools::For(0, numPts, threadedBds);
    }
  }
};

// ---------------------------------------------------------------------------
struct BoundsPointIdsWorker
{
  template <typename TPointsArray, typename TId>
  void operator()(TPointsArray* pts, const TId* ptIds, TId numberOfPointsIds, double* bds)
  {
    // We use THRESHOLD to test if the data size is small enough
    // to execute the functor serially. This is faster.
    // and also potentially avoids nested multithreading which creates race conditions.
    if (numberOfPointsIds <= vtkSMPTools::THRESHOLD)
    {
      SerialBoundsPointIdsFunctor<TPointsArray, TId> serialBds(pts, ptIds, bds);
      serialBds(numberOfPointsIds);
    }
    else
    {
      ThreadedBoundsPointIdsFunctor<TPointsArray, TId> threadedBds(pts, ptIds, bds);
      vtkSMPTools::For(0, numberOfPointsIds, threadedBds);
    }
  }
};
} // anonymous namespace

//------------------------------------------------------------------------------
void vtkBoundingBox::ComputeBounds(vtkPoints* pts, double bounds[6])
{
  // Compute bounds: dispatch to real types, fallback for other types.
  using Dispatcher = vtkArrayDispatch::DispatchByValueTypeUsingArrays<vtkArrayDispatch::AllArrays,
    vtkArrayDispatch::Reals>;
  BoundsWorker worker;

  if (!Dispatcher::Execute(pts->GetData(), worker, bounds))
  { // Fallback to slowpath for other point types
    worker(pts->GetData(), bounds);
  }
}

//------------------------------------------------------------------------------
void vtkBoundingBox::ComputeBounds(vtkPoints* pts, const unsigned char* ptUses, double bounds[6])
{
  // Compute bounds: dispatch to real types, fallback for other types.
  using Dispatcher = vtkArrayDispatch::DispatchByValueTypeUsingArrays<vtkArrayDispatch::AllArrays,
    vtkArrayDispatch::Reals>;
  BoundsPointUsesWorker worker;

  if (!Dispatcher::Execute(pts->GetData(), worker, ptUses, bounds))
  { // Fallback to slowpath for other point types
    worker(pts->GetData(), ptUses, bounds);
  }
}

//------------------------------------------------------------------------------
void vtkBoundingBox::ComputeBounds(
  vtkPoints* pts, const std::atomic<unsigned char>* ptUses, double bounds[6])
{
  // Compute bounds: dispatch to real types, fallback for other types.
  using Dispatcher = vtkArrayDispatch::DispatchByValueTypeUsingArrays<vtkArrayDispatch::AllArrays,
    vtkArrayDispatch::Reals>;
  BoundsPointUsesWorker worker;

  if (!Dispatcher::Execute(pts->GetData(), worker, ptUses, bounds))
  { // Fallback to slowpath for other point types
    worker(pts->GetData(), ptUses, bounds);
  }
}

//------------------------------------------------------------------------------
void vtkBoundingBox::ComputeBounds(
  vtkPoints* pts, const long long* ptIds, long long numberOfPointsIds, double bounds[6])
{
  // Compute bounds: dispatch to real types, fallback for other types.
  using Dispatcher = vtkArrayDispatch::DispatchByValueTypeUsingArrays<vtkArrayDispatch::AllArrays,
    vtkArrayDispatch::Reals>;
  BoundsPointIdsWorker worker;

  if (!Dispatcher::Execute(pts->GetData(), worker, ptIds, numberOfPointsIds, bounds))
  { // Fallback to slowpath for other point types
    worker(pts->GetData(), ptIds, numberOfPointsIds, bounds);
  }
}

//------------------------------------------------------------------------------
void vtkBoundingBox::ComputeBounds(
  vtkPoints* pts, const long* ptIds, long numberOfPointsIds, double bounds[6])
{
  // Compute bounds: dispatch to real types, fallback for other types.
  using Dispatcher = vtkArrayDispatch::DispatchByValueTypeUsingArrays<vtkArrayDispatch::AllArrays,
    vtkArrayDispatch::Reals>;
  BoundsPointIdsWorker worker;

  if (!Dispatcher::Execute(pts->GetData(), worker, ptIds, numberOfPointsIds, bounds))
  { // Fallback to slowpath for other point types
    worker(pts->GetData(), ptIds, numberOfPointsIds, bounds);
  }
}

//------------------------------------------------------------------------------
void vtkBoundingBox::ComputeBounds(
  vtkPoints* pts, const int* ptIds, int numberOfPointsIds, double bounds[6])
{
  // Compute bounds: dispatch to real types, fallback for other types.
  using Dispatcher = vtkArrayDispatch::DispatchByValueTypeUsingArrays<vtkArrayDispatch::AllArrays,
    vtkArrayDispatch::Reals>;
  BoundsPointIdsWorker worker;

  if (!Dispatcher::Execute(pts->GetData(), worker, ptIds, numberOfPointsIds, bounds))
  { // Fallback to slowpath for other point types
    worker(pts->GetData(), ptIds, numberOfPointsIds, bounds);
  }
}

// ---------------------------------------------------------------------------
void vtkBoundingBox::ComputeLocalBounds(
  vtkPoints* points, double u[3], double v[3], double w[3], double outputBounds[6])
{
  vtkBoundingBox bbox;

  for (vtkIdType i = 0; i < points->GetNumberOfPoints(); i++)
  {
    double* point = points->GetPoint(i);
    double du = vtkMath::Dot(point, u);
    double dv = vtkMath::Dot(point, v);
    double dw = vtkMath::Dot(point, w);
    bbox.AddPoint(du, dv, dw);
  }
  bbox.GetBounds(outputBounds);
}
VTK_ABI_NAMESPACE_END
