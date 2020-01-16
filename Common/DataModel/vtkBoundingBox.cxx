/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoundingBox.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBoundingBox.h"
#include "vtkArrayDispatch.h"
#include "vtkDataArrayRange.h"
#include "vtkMath.h"
#include "vtkPlane.h"
#include "vtkPoints.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"

#include <array>
#include <cassert>
#include <cmath>

// ---------------------------------------------------------------------------
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
};

// ---------------------------------------------------------------------------
void vtkBoundingBox::AddPoint(double px, double py, double pz)
{
  double p[3];
  p[0] = px;
  p[1] = py;
  p[2] = pz;
  this->AddPoint(p);
}

// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
void vtkBoundingBox::AddBox(const vtkBoundingBox& bbox)
{
  double bds[6];
  bbox.GetBounds(bds);
  this->AddBounds(bds);
}

// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
void vtkBoundingBox::Inflate(double delX, double delY, double delZ)
{
  this->MinPnt[0] -= delX;
  this->MaxPnt[0] += delX;
  this->MinPnt[1] -= delY;
  this->MaxPnt[1] += delY;
  this->MinPnt[2] -= delZ;
  this->MaxPnt[2] += delZ;
}

// ---------------------------------------------------------------------------
void vtkBoundingBox::Inflate(double delta)
{
  this->Inflate(delta, delta, delta);
}

// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
double vtkBoundingBox::GetDiagonalLength() const
{
  assert("pre: not_empty" && this->IsValid());

  double l[3];
  this->GetLengths(l);

  return sqrt(l[0] * l[0] + l[1] * l[1] + l[2] * l[2]);
}

// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
void vtkBoundingBox::Scale(double s[3])
{
  this->Scale(s[0], s[1], s[2]);
}

// ---------------------------------------------------------------------------
void vtkBoundingBox::ScaleAboutCenter(double s)
{
  this->ScaleAboutCenter(s, s, s);
}

// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
void vtkBoundingBox::ScaleAboutCenter(double s[3])
{
  this->ScaleAboutCenter(s[0], s[1], s[2]);
}

// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Support ComputeBounds()
namespace
{

template <typename PointsT>
struct FastBounds
{
  PointsT* Points;
  const unsigned char* PointUses;
  double* Bounds;
  vtkSMPThreadLocal<std::array<double, 6> > LocalBounds;

  FastBounds(PointsT* pts, const unsigned char* ptUses, double* bds)
    : Points(pts)
    , PointUses(ptUses)
    , Bounds(bds)
  {
  }

  void Initialize()
  {
    std::array<double, 6>& localBds = this->LocalBounds.Local();

    localBds[0] = VTK_DOUBLE_MAX;
    localBds[2] = VTK_DOUBLE_MAX;
    localBds[4] = VTK_DOUBLE_MAX;

    localBds[1] = VTK_DOUBLE_MIN;
    localBds[3] = VTK_DOUBLE_MIN;
    localBds[5] = VTK_DOUBLE_MIN;
  }

  void operator()(vtkIdType beginPtId, vtkIdType endPtId)
  {
    std::array<double, 6>& localBds = this->LocalBounds.Local();
    const auto tuples = vtk::DataArrayTupleRange<3>(this->Points, beginPtId, endPtId);
    const unsigned char usedConst[1] = { 1 };
    const unsigned char* used =
      (this->PointUses != nullptr ? this->PointUses + beginPtId : usedConst);

    for (const auto tuple : tuples)
    {
      if (*used)
      {
        double x = static_cast<double>(tuple[0]);
        double y = static_cast<double>(tuple[1]);
        double z = static_cast<double>(tuple[2]);

        localBds[0] = std::min(x, localBds[0]);
        localBds[1] = std::max(x, localBds[1]);
        localBds[2] = std::min(y, localBds[2]);
        localBds[3] = std::max(y, localBds[3]);
        localBds[4] = std::min(z, localBds[4]);
        localBds[5] = std::max(z, localBds[5]);
      }
      if (this->PointUses != nullptr)
      {
        ++used;
      }
    }
  }

  void Reduce()
  {
    // Composite bounds from all threads
    double xmin = VTK_DOUBLE_MAX;
    double ymin = VTK_DOUBLE_MAX;
    double zmin = VTK_DOUBLE_MAX;
    double xmax = VTK_DOUBLE_MIN;
    double ymax = VTK_DOUBLE_MIN;
    double zmax = VTK_DOUBLE_MIN;

    for (auto iter = this->LocalBounds.begin(); iter != this->LocalBounds.end(); ++iter)
    {
      xmin = std::min((*iter)[0], xmin);
      xmax = std::max((*iter)[1], xmax);
      ymin = std::min((*iter)[2], ymin);
      ymax = std::max((*iter)[3], ymax);
      zmin = std::min((*iter)[4], zmin);
      zmax = std::max((*iter)[5], zmax);
    }

    this->Bounds[0] = xmin;
    this->Bounds[1] = xmax;
    this->Bounds[2] = ymin;
    this->Bounds[3] = ymax;
    this->Bounds[4] = zmin;
    this->Bounds[5] = zmax;
  }
};

// Hooks into dispatcher vtkArrayDispatch by providing a callable generic
struct BoundsWorker
{
  template <typename PointsT>
  void operator()(PointsT* pts, const unsigned char* ptUses, double* bds)
  {
    vtkIdType numPts = pts->GetNumberOfTuples();
    FastBounds<PointsT> fastBds(pts, ptUses, bds);
    vtkSMPTools::For(0, numPts, fastBds);
  }
};

} // anonymous namespace

// ---------------------------------------------------------------------------
// Fast computing of bounding box from vtkPoints.
void vtkBoundingBox::ComputeBounds(vtkPoints* pts, double bounds[6])
{
  return vtkBoundingBox::ComputeBounds(pts, nullptr, bounds);
}

// ---------------------------------------------------------------------------
// Fast computing of bounding box from vtkPoints and optional array that marks
// points that should be used in the computation.
void vtkBoundingBox::ComputeBounds(vtkPoints* pts, const unsigned char* ptUses, double bounds[6])
{
  // Check for valid
  vtkIdType numPts;
  if (pts == nullptr || (numPts = pts->GetNumberOfPoints()) < 1)
  {
    bounds[0] = bounds[2] = bounds[4] = VTK_DOUBLE_MAX;
    bounds[1] = bounds[3] = bounds[5] = VTK_DOUBLE_MIN;
    return;
  }

  // Compute bounds: dispatch to real types, fallback for other types.
  using vtkArrayDispatch::Reals;
  using Dispatcher = vtkArrayDispatch::DispatchByValueType<Reals>;
  BoundsWorker worker;

  if (!Dispatcher::Execute(pts->GetData(), worker, ptUses, bounds))
  { // Fallback to slowpath for other point types
    worker(pts->GetData(), ptUses, bounds);
  }
}
