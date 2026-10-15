// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkStaticPointLocator2D.h"

#include "vtkBoundingBox.h"
#include "vtkBox.h"
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkLine.h"
#include "vtkLocatorInterface.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"

//------------------------------------------------------------------------------
// Parts of the locator and related classes are made visible through the
// included private include file (to satisfy the one definition rule). This
// include files declares internal classes.  Definitions follow in this .cxx
// file.
#include "vtkStaticPointLocator2DPrivate.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkStaticPointLocator2D);

// There are stack-allocated bucket neighbor lists. This is the initial
// value. Too small and heap allocation kicks in.
constexpr size_t VTK_INITIAL_BUCKET_SIZE = 10000;

//------------------------------------------------------------------------------
// Utility class to store an array of ij values
struct NeighborBuckets2D
{
  // Start with an array to avoid memory allocation overhead.
  // Initially, P will alias InitialBuffer, but could later
  // be assigned dynamically allocated memory.
  int InitialBuffer[VTK_INITIAL_BUCKET_SIZE * 2];
  int* P;
  vtkIdType Count;
  vtkIdType MaxSize;

  NeighborBuckets2D()
  {
    this->P = this->InitialBuffer;
    this->Count = 0;
    this->MaxSize = VTK_INITIAL_BUCKET_SIZE;
  }

  ~NeighborBuckets2D()
  {
    this->Count = 0;
    if (this->P != this->InitialBuffer)
    {
      delete[] this->P;
    }
  }

  int GetNumberOfNeighbors() { return this->Count; }
  void Reset() { this->Count = 0; }

  int* GetPoint(vtkIdType i) { return this->P + 2 * i; }

  vtkIdType InsertNextBucket(const int x[2])
  {
    // Re-allocate if beyond the current max size.
    // (Increase by VTK_INITIAL_BUCKET_SIZE)
    int* tmp;
    vtkIdType offset = this->Count * 2;

    if (this->Count >= this->MaxSize)
    {
      tmp = this->P;
      this->MaxSize *= 2;
      this->P = new int[this->MaxSize * 2];

      memcpy(this->P, tmp, offset * sizeof(int));

      if (tmp != this->InitialBuffer)
      {
        delete[] tmp;
      }
    }

    tmp = this->P + offset;
    *tmp++ = *x++;
    *tmp = *x;
    this->Count++;
    return this->Count - 1;
  }
}; // NeighborBuckets2D

//------------------------------------------------------------------------------
//  Internal function to get bucket neighbors at specified level
//
void vtkBucketList2D::GetBucketNeighbors(
  NeighborBuckets2D* buckets, const int ij[2], const int ndivs[2], int level)
{
  int i, j, min, max, minLevel[2], maxLevel[2];
  int nei[2];

  //  Initialize
  //
  buckets->Reset();

  //  If at this bucket, just place into list
  //
  if (level == 0)
  {
    buckets->InsertNextBucket(ij);
    return;
  }

  //  Create permutations of the ij indices that are at the level
  //  required. If these are legal buckets, add to list for searching.
  //
  for (i = 0; i < 2; i++)
  {
    min = ij[i] - level;
    max = ij[i] + level;
    minLevel[i] = (min > 0 ? min : 0);
    maxLevel[i] = (max < (ndivs[i] - 1) ? max : (ndivs[i] - 1));
  }

  for (i = minLevel[0]; i <= maxLevel[0]; i++)
  {
    for (j = minLevel[1]; j <= maxLevel[1]; j++)
    {
      if (i == (ij[0] + level) || i == (ij[0] - level) || j == (ij[1] + level) ||
        j == (ij[1] - level))
      {
        nei[0] = i;
        nei[1] = j;
        buckets->InsertNextBucket(nei);
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkBucketList2D::GenerateFace(
  int vtkNotUsed(face), int i, int j, int vtkNotUsed(k), vtkPoints* pts, vtkCellArray* polys)
{
  vtkIdType ids[4];
  double origin[3], x[3];

  // define first corner
  origin[0] = this->bX + i * this->hX;
  origin[1] = this->bY + j * this->hY;
  origin[2] = 0.0;
  ids[0] = pts->InsertNextPoint(origin);

  x[0] = origin[0];
  x[1] = origin[1] + this->hY;
  x[2] = origin[2];
  ids[1] = pts->InsertNextPoint(x);

  x[0] = origin[0];
  x[1] = origin[1] + this->hY;
  x[2] = origin[2];
  ids[2] = pts->InsertNextPoint(x);

  x[0] = origin[0];
  x[1] = origin[1];
  x[2] = origin[2];
  ids[3] = pts->InsertNextPoint(x);

  polys->InsertNextCell(4, ids);
}

//------------------------------------------------------------------------------
// Calculate the distance between the point x to the bucket "nei".
//
// WARNING!!!!! Be very careful altering this routine.  Simple changes to this
// routine can make it 25% slower!!!!
//
double vtkBucketList2D::Distance2ToBucket(const double x[3], const int nei[3])
{
  double bounds[6];

  bounds[0] = nei[0] * this->hX + this->bX;
  bounds[1] = (nei[0] + 1) * this->hX + this->bX;
  bounds[2] = nei[1] * this->hY + this->bY;
  bounds[3] = (nei[1] + 1) * this->hY + this->bY;
  bounds[4] = 0.0;
  bounds[5] = 0.0;

  return this->Distance2ToBounds(x, bounds);
}

//------------------------------------------------------------------------------
// Calculate the distance between the point x and the specified bounds
//
// WARNING!!!!! Be very careful altering this routine.  Simple changes to this
// routine can make is 25% slower!!!!
double vtkBucketList2D::Distance2ToBounds(const double x[3], const double bounds[6])
{
  double distance;
  double deltas[3];

  // Are we within the bounds?
  if (x[0] >= bounds[0] && x[0] <= bounds[1] && x[1] >= bounds[2] && x[1] <= bounds[3])
  {
    return 0.0;
  }

  deltas[0] = deltas[1] = deltas[2] = 0.0;

  // dx
  //
  if (x[0] < bounds[0])
  {
    deltas[0] = bounds[0] - x[0];
  }
  else if (x[0] > bounds[1])
  {
    deltas[0] = x[0] - bounds[1];
  }

  // dy
  //
  if (x[1] < bounds[2])
  {
    deltas[1] = bounds[2] - x[1];
  }
  else if (x[1] > bounds[3])
  {
    deltas[1] = x[1] - bounds[3];
  }

  distance = vtkMath::Dot(deltas, deltas);
  return distance;
}

//------------------------------------------------------------------------------
// Given a position x, return the id of the point closest to it.
template <typename TIds>
vtkIdType BucketList2D<TIds>::FindClosestPoint(const double x[3])
{
  int i, j;
  double minDist2;
  double dist2 = VTK_DOUBLE_MAX;
  double pt[3];
  int closest, level;
  vtkIdType ptId, cno, numIds;
  int ij[2], *nei;
  NeighborBuckets2D buckets;
  const vtkLocatorTuple<TIds>* ids;

  //  Find bucket point is in.
  //
  this->GetBucketIndices(x, ij);

  //  Need to search this bucket for the closest point.  If there are no
  //  points in this bucket, search 1st level neighbors, and so on, until
  //  closest point found.
  //
  for (closest = (-1), minDist2 = VTK_DOUBLE_MAX, level = 0;
       (closest == -1) && (level < this->Divisions[0] || level < this->Divisions[1]); level++)
  {
    this->GetBucketNeighbors(&buckets, ij, this->Divisions, level);

    for (i = 0; i < buckets.GetNumberOfNeighbors(); i++)
    {
      nei = buckets.GetPoint(i);
      cno = nei[0] + nei[1] * this->xD;

      if ((numIds = this->GetNumberOfIds(cno)) > 0)
      {
        ids = this->GetIds(cno);
        for (j = 0; j < numIds; j++)
        {
          ptId = ids[j].PtId;
          this->DataSet->GetPoint(ptId, pt);
          if ((dist2 = Distance2BetweenPoints2D(x, pt)) < minDist2)
          {
            closest = ptId;
            minDist2 = dist2;
          }
        }
      }
    }
  }

  //
  // Because of the relative location of the points in the buckets, the
  // point found previously may not be the closest point. We have to
  // search those bucket neighbors that might also contain the point.
  //
  if (minDist2 > 0.0)
  {
    this->GetOverlappingBuckets(&buckets, x, ij, sqrt(minDist2), 0);
    for (i = 0; i < buckets.GetNumberOfNeighbors(); i++)
    {
      nei = buckets.GetPoint(i);
      cno = nei[0] + nei[1] * this->xD;

      if ((numIds = this->GetNumberOfIds(cno)) > 0)
      {
        ids = this->GetIds(cno);
        for (j = 0; j < numIds; j++)
        {
          ptId = ids[j].PtId;
          this->DataSet->GetPoint(ptId, pt);
          if ((dist2 = Distance2BetweenPoints2D(x, pt)) < minDist2)
          {
            closest = ptId;
            minDist2 = dist2;
          }
        } // for each point
      } // if points in bucket
    } // for each overlapping bucket
  } // if not identical point

  return closest;
}

//------------------------------------------------------------------------------
template <typename TIds>
vtkIdType BucketList2D<TIds>::FindClosestPointWithinRadius(
  double radius, const double x[3], double inputDataLength, double& dist2)
{
  int i, j;
  double pt[3];
  vtkIdType ptId, closest = -1;
  int ij[2], *nei;
  double minDist2;

  double refinedRadius, radius2, refinedRadius2;
  double currentRadius;
  double distance2ToDataBounds, maxDistance;
  int ii, radiusLevels[2], radiusLevel, prevMinLevel[2], prevMaxLevel[2];
  NeighborBuckets2D buckets;
  const vtkLocatorTuple<TIds>* ids;

  // Initialize
  dist2 = -1.0;
  radius2 = radius * radius;
  minDist2 = 1.01 * radius2; // something slightly bigger....

  vtkDataArray* pointData = static_cast<vtkPointSet*>(this->DataSet)->GetPoints()->GetData();

  //  Find the bucket the point is in.
  //
  this->GetBucketIndices(x, ij);

  // Start by searching the bucket that the point is in.
  //
  vtkIdType numIds;
  vtkIdType cno = ij[0] + ij[1] * this->xD;
  if ((numIds = this->GetNumberOfIds(cno)) > 0)
  {
    ids = this->GetIds(cno);
    for (j = 0; j < numIds; j++)
    {
      ptId = ids[j].PtId;
      pointData->GetTuple(ptId, pt);
      if ((dist2 = Distance2BetweenPoints2D(x, pt)) < minDist2)
      {
        closest = ptId;
        minDist2 = dist2;
      }
    }
  }

  // Now, search only those buckets that are within a radius. The radius used
  // is the smaller of sqrt(minDist2) and the radius that is passed in. To avoid
  // checking a large number of buckets unnecessarily, if the radius is
  // larger than the dimensions of a bucket, we search outward using a
  // simple heuristic of rings.  This heuristic ends up collecting inner
  // buckets multiple times, but this only happens in the case where these
  // buckets are empty, so they are discarded quickly.
  //
  if (minDist2 < radius2)
  {
    refinedRadius = sqrt(minDist2);
    refinedRadius2 = dist2;
  }
  else
  {
    refinedRadius = radius;
    refinedRadius2 = radius2;
  }

  if (inputDataLength != 0.0)
  {
    distance2ToDataBounds = this->Distance2ToBounds(x, this->Bounds);
    maxDistance = sqrt(distance2ToDataBounds) + inputDataLength;
    if (refinedRadius > maxDistance)
    {
      refinedRadius = maxDistance;
      refinedRadius2 = maxDistance * maxDistance;
    }
  }

  for (i = 0; i < 2; i++)
  {
    radiusLevels[i] = static_cast<int>(refinedRadius / this->H[i]);
    if (radiusLevels[i] > this->Divisions[i] / 2)
    {
      radiusLevels[i] = this->Divisions[i] / 2;
    }
  }

  radiusLevel = radiusLevels[0];
  radiusLevel = radiusLevels[1] > radiusLevel ? radiusLevels[1] : radiusLevel;
  if (radiusLevel == 0)
  {
    radiusLevel = 1;
  }

  // radius schedule increases the radius each iteration, this is currently
  // implemented by decreasing ii by 1 each iteration.  another alternative
  // is to double the radius each iteration, i.e. ii = ii >> 1
  // In practice, reducing ii by one has been found to be more efficient.
  prevMinLevel[0] = prevMaxLevel[0] = ij[0];
  prevMinLevel[1] = prevMaxLevel[1] = ij[1];
  for (ii = radiusLevel; ii >= 1; ii--)
  {
    currentRadius = refinedRadius; // used in if at bottom of this for loop

    // Build up a list of buckets that are arranged in rings
    this->GetOverlappingBuckets(&buckets, x, refinedRadius / ii, prevMinLevel, prevMaxLevel);

    for (i = 0; i < buckets.GetNumberOfNeighbors(); i++)
    {
      nei = buckets.GetPoint(i);
      // do we still need to test this bucket?
      if (this->Distance2ToBucket(x, nei) < refinedRadius2)
      {
        cno = nei[0] + nei[1] * this->xD;
        if ((numIds = this->GetNumberOfIds(cno)) > 0)
        {
          ids = this->GetIds(cno);
          for (j = 0; j < numIds; j++)
          {
            ptId = ids[j].PtId;
            pointData->GetTuple(ptId, pt);
            if ((dist2 = Distance2BetweenPoints2D(x, pt)) < minDist2)
            {
              closest = ptId;
              minDist2 = dist2;
              refinedRadius = sqrt(minDist2);
              refinedRadius2 = minDist2;
            }
          } // for each pt in bucket
        } // if ids
      } // if bucket is within the current best distance
    } // for each overlapping bucket

    // Don't want to check a smaller radius than we just checked so update
    // it appropriately
    if (refinedRadius < currentRadius && ii > 2) // always check ii==1
    {
      ii = static_cast<int>(static_cast<double>(ii) * (refinedRadius / currentRadius)) + 1;
      ii = std::max(ii, 2);
    }
  } // for each radius in the radius schedule

  if ((closest != -1) && (minDist2 <= radius2))
  {
    dist2 = minDist2;
  }
  else
  {
    closest = -1;
  }

  return closest;
}

//------------------------------------------------------------------------------
template <typename TIds>
void BucketList2D<TIds>::FindClosestNPoints(int N, const double x[3], vtkIdList* result)
{
  int i = 0, j = 0;
  double dist2;
  double pt[3];
  int level, maxLevel;
  vtkIdType ptId, cno, numIds;
  int ij[2], *nei;
  NeighborBuckets2D buckets;
  const vtkLocatorTuple<TIds>* ids;

  //  Find the bucket the point is in.
  this->GetBucketIndices(x, ij);

  // Gather points keeping track of maximum radius in the first group of
  // points.
  level = 0;
  double maxDist2 = 0.0;
  vtkDist2TupleType sortedPts;
  sortedPts.reserve(128);

  // Start in the current bucket and expand out to grab the first N
  // points. Keep track of maximum distance.
  this->GetBucketNeighbors(&buckets, ij, this->Divisions, level);
  while (buckets.GetNumberOfNeighbors() > 0)
  {
    for (i = 0; i < buckets.GetNumberOfNeighbors(); i++)
    {
      nei = buckets.GetPoint(i);
      cno = nei[0] + nei[1] * this->xD;
      if ((numIds = this->GetNumberOfIds(cno)) > 0)
      {
        ids = this->GetIds(cno);
        for (j = 0; j < numIds; j++)
        {
          ptId = ids[j].PtId;
          this->DataSet->GetPoint(ptId, pt);
          dist2 = Distance2BetweenPoints2D(x, pt);
          maxDist2 = std::max(dist2, maxDist2);
          sortedPts.emplace_back(ptId, dist2);
        }
        // As soon as N points found, jump out.
        if (static_cast<int>(sortedPts.size()) >= N)
        {
          goto FOUND_N;
        }
      } // if points in bucket
    } // for unprocessed buckets

    level++;
    this->GetBucketNeighbors(&buckets, ij, this->Divisions, level);
  } // while still not found N points

// We've found N initial points (or exhausted all points). Now insert
// additional points that are closer than this original sample.
FOUND_N:
  if (static_cast<int>(sortedPts.size()) >= N)
  {
    // If here, resume processing current buckets to identify additional
    // close points. Then go out one more level and do the same thing. Watch
    // boundary condition.
    int iStart = i;
    int jStart = j + 1;
    maxLevel = level + 2; // finish current one plus one more level
    while (level < maxLevel)
    {
      for (i = iStart; i < buckets.GetNumberOfNeighbors(); i++)
      {
        nei = buckets.GetPoint(i);
        cno = nei[0] + nei[1] * this->xD;
        if ((numIds = this->GetNumberOfIds(cno)) > 0)
        {
          ids = this->GetIds(cno);
          // Start where previous loop left off
          for (j = jStart; j < numIds; j++)
          {
            ptId = ids[j].PtId;
            this->DataSet->GetPoint(ptId, pt);
            dist2 = Distance2BetweenPoints2D(x, pt);
            if (dist2 <= maxDist2)
            {
              sortedPts.emplace_back(ptId, dist2);
            }
          }
          jStart = 0;
        } // if points in bucket
      } // for unprocessed buckets
      iStart = 0;

      level++;
      this->GetBucketNeighbors(&buckets, ij, this->Divisions, level);
    }
  }

  // Now do the final sort to find N closest.
  std::sort(sortedPts.begin(), sortedPts.end());
  N = (static_cast<int>(sortedPts.size()) < N ? static_cast<int>(sortedPts.size()) : N);

  // Copy result
  result->SetNumberOfIds(N);
  for (i = 0; i < N; i++)
  {
    result->SetId(i, sortedPts.at(i).Id);
  }
}

//------------------------------------------------------------------------------
namespace
{
// The buckets overlapping an annulus (MinDist2, maxDist2] around X, and at
// least one of the petals (circles), with the distances in the x-y plane,
// visited in patches of increasing levels around the bucket of X (see
// vtkBucketPatchIterator), from the first patch not inside the inner circle.
// The points are assigned to the buckets with GetBucketIndices(), which may
// round differently from the bucket bounds: the buckets are tested enlarged by
// Delta, and with a relative slack on the squared distances.
template <typename TIds>
class vtkBucketShell2D
{
public:
  vtkBucketShell2D(
    BucketList2D<TIds>* bins, const double x[3], double minDist2, vtkDoubleArray* petals)
    : Bins(bins)
    , X(x)
    , MinDist2(minDist2)
    , NumPetals(petals ? petals->GetNumberOfTuples() : 0)
    , Petals(this->NumPetals > 0 ? petals->GetPointer(0) : nullptr)
  {
    bins->GetBucketIndices(x, this->Center);
    for (int i = 0; i < 2; ++i)
    {
      this->Delta[i] = 1.0e-6 * bins->H[i] +
        1.0e-14 * (std::abs(bins->Bounds[2 * i]) + std::abs(bins->Bounds[2 * i + 1]));
    }

    // A valid point is inside (or on) a petal, so it is at most Reach2 away.
    if (this->NumPetals > 0)
    {
      double reach = 0.0;
      for (vtkIdType m = 0; m < this->NumPetals; ++m)
      {
        const double* petal = this->Petals + 3 * m;
        reach =
          std::max(reach, std::sqrt(Distance2BetweenPoints2D(x, petal)) + std::sqrt(petal[2]));
      }
      this->Reach2 = reach * reach;
    }

    // Skip the patches inside the inner circle: all their points are at most
    // MinDist2 away.
    double lo[2], hi[2];
    for (; this->Level <= bins->MaxLevel; ++this->Level)
    {
      this->GetPatchBounds(this->Level, true, lo, hi);
      if (FarDist2(lo, hi, x) * (1.0 + Slack) > minDist2)
      {
        break;
      }
    }
    this->Patches.Initialize(bins->Divisions, bins->MaxLevel);
  }

  // Add to results the valid points (in the annulus up to maxDist2, and inside
  // or on a petal) of the next levels, until the next level is entirely beyond
  // maxDist2 or the petals, or until results holds at least minSize points.
  // The next call continues with the next level.
  void Gather(double maxDist2, std::size_t minSize, vtkDist2TupleArray& results)
  {
    BucketList2D<TIds>* bins = this->Bins;
    // The members used per point, as locals: results may alias them.
    const double* x = this->X;
    const double minDist2 = this->MinDist2;
    const vtkIdType numPetals = this->NumPetals;
    const double* petals = this->Petals;
    const double* fastPoints = bins->FastPoints;
    double lo[3], hi[3], p[3];
    for (; this->Level <= bins->MaxLevel; ++this->Level)
    {
      if (results.size() >= minSize || !this->Reaches(std::min(maxDist2, this->Reach2)))
      {
        return;
      }

      for (vtkIdType bucket = this->Patches.Begin(this->Level, this->Center); bucket >= 0;
           bucket = this->Patches.Next())
      {
        const vtkIdType numIds = bins->GetNumberOfIds(bucket);
        if (numIds <= 0)
        {
          continue;
        }

        // Skip the buckets outside of the annulus, or outside of all the petals.
        bins->GetBucketBounds(this->Patches.IJ[0], this->Patches.IJ[1], lo, hi);
        for (int c = 0; c < 2; ++c)
        {
          lo[c] -= this->Delta[c];
          hi[c] += this->Delta[c];
        }
        if (NearDist2(lo, hi, x) * (1.0 - Slack) > maxDist2 ||
          FarDist2(lo, hi, x) * (1.0 + Slack) <= minDist2)
        {
          continue;
        }
        bool inPetals = (numPetals == 0);
        const double* petal = petals;
        for (vtkIdType m = 0; m < numPetals && !inPetals; ++m, petal += 3)
        {
          inPetals = (NearDist2(lo, hi, petal) * (1.0 - Slack) <= petal[2]);
        }
        if (!inPetals)
        {
          continue;
        }

        const vtkLocatorTuple<TIds>* ids = bins->GetIds(bucket);
        for (vtkIdType n = 0; n < numIds; ++n)
        {
          const vtkIdType ptId = ids[n].PtId;
          const double* pt = p;
          if (fastPoints)
          {
            pt = fastPoints + 3 * ptId;
          }
          else
          {
            bins->DataSet->GetPoint(ptId, p);
          }
          const double d2 = Distance2BetweenPoints2D(pt, x);
          if (d2 <= minDist2 || d2 > maxDist2)
          {
            continue;
          }
          bool inPetal = (numPetals == 0);
          petal = petals;
          for (vtkIdType m = 0; m < numPetals && !inPetal; ++m, petal += 3)
          {
            inPetal = (Distance2BetweenPoints2D(pt, petal) <= petal[2]);
          }
          if (inPetal)
          {
            results.emplace_back(ptId, d2);
          }
        }
      }
    }
  }

private:
  static constexpr double Slack = 1.0e-12;

  // The bounds of the patch of buckets of a level around Center, enlarged by
  // Delta (grow), or shrunk by it.
  void GetPatchBounds(int level, bool grow, double lo[2], double hi[2]) const
  {
    for (int i = 0; i < 2; ++i)
    {
      const double margin = grow ? this->Delta[i] : -this->Delta[i];
      lo[i] = this->Bins->Bounds[2 * i] + (this->Center[i] - level) * this->Bins->H[i] - margin;
      hi[i] = this->Bins->Bounds[2 * i] + (this->Center[i] + level + 1) * this->Bins->H[i] + margin;
    }
  }

  // The squared distance from c to the farthest point of a box.
  static double FarDist2(const double lo[2], const double hi[2], const double* c)
  {
    double d2 = 0.0;
    for (int i = 0; i < 2; ++i)
    {
      const double d = std::max(c[i] - lo[i], hi[i] - c[i]);
      d2 += d * d;
    }
    return d2;
  }

  // Whether some bucket of the current level can be within dist2 of X: they
  // are outside of the patch of the previous level, on the sides that still
  // have buckets. Their points may be up to Delta inside of it: the patch is
  // shrunk.
  bool Reaches(double dist2) const
  {
    if (this->Level == 0)
    {
      return true;
    }
    double lo[2], hi[2];
    this->GetPatchBounds(this->Level - 1, false, lo, hi);
    double minD2 = VTK_DOUBLE_MAX;
    for (int i = 0; i < 2; ++i)
    {
      if (this->Center[i] - this->Level >= 0)
      {
        const double d = std::max(this->X[i] - lo[i], 0.0);
        minD2 = std::min(d * d, minD2);
      }
      if (this->Center[i] + this->Level < this->Bins->Divisions[i])
      {
        const double d = std::max(hi[i] - this->X[i], 0.0);
        minD2 = std::min(d * d, minD2);
      }
    }
    return minD2 < VTK_DOUBLE_MAX && minD2 * (1.0 - Slack) <= dist2;
  }

  // The squared distance from c to the nearest point of a box.
  static double NearDist2(const double lo[2], const double hi[2], const double* c)
  {
    double d2 = 0.0;
    for (int i = 0; i < 2; ++i)
    {
      const double d = std::max({ lo[i] - c[i], c[i] - hi[i], 0.0 });
      d2 += d * d;
    }
    return d2;
  }

  BucketList2D<TIds>* Bins;
  const double* X;
  double MinDist2;
  vtkIdType NumPetals;
  const double* Petals;
  double Reach2 = VTK_DOUBLE_MAX;
  int Center[2];
  double Delta[2];
  int Level = 0; // the next level to visit
  vtkBucketPatchIterator Patches;
};
} // anonymous namespace

//------------------------------------------------------------------------------
// The N nearest valid points (farther than minDist2, and inside (or on) one of
// the petals), and those tied with the N-th one, in a single visit of the
// buckets: the levels of buckets around x are gathered until they hold N valid
// points, whose N-th nearest bounds the distance of the N-th nearest valid
// point of all. The next levels are then only gathered up to that distance,
// and that point is the N-th nearest of all the points gathered.
template <typename TIds>
double BucketList2D<TIds>::FindNPointsInShell(int N, const double x[3], vtkDist2TupleArray& results,
  double minDist2, bool sort, vtkDoubleArray* petals)
{
  results.clear();
  if (N <= 0)
  {
    return 0.0;
  }

  vtkBucketShell2D<TIds> shell(this, x, minDist2, petals);
  const std::size_t numPoints = static_cast<std::size_t>(N);
  shell.Gather(VTK_DOUBLE_MAX, numPoints, results);
  if (results.empty())
  {
    return 0.0;
  }

  // The N-th nearest point, or the farthest if there are fewer than N.
  const std::size_t nth = std::min(numPoints, results.size()) - 1;
  std::nth_element(results.begin(), results.begin() + nth, results.end());
  if (results.size() >= numPoints)
  {
    const std::size_t numGathered = results.size();
    shell.Gather(results[nth].Dist2, std::numeric_limits<std::size_t>::max(), results);
    if (results.size() > numGathered)
    {
      std::nth_element(results.begin(), results.begin() + nth, results.end());
    }
  }
  const double maxDist2 = results[nth].Dist2;
  results.erase(std::remove_if(results.begin(), results.end(),
                  [maxDist2](const vtkDist2Tuple& t) { return t.Dist2 > maxDist2; }),
    results.end());
  if (sort)
  {
    std::sort(results.begin(), results.end());
  }
  return maxDist2;
}

//------------------------------------------------------------------------------
// Gather the points of the buckets overlapping the annulus (minDist2, maxDist2]
// around x, and inside at least one of the petals, with the distances in the
// x-y plane (see vtkBucketShell2D).
template <typename TIds>
void BucketList2D<TIds>::FindPointsInShell(const double x[3], vtkDist2TupleArray& results,
  double minDist2, double maxDist2, bool sort, vtkDoubleArray* petals)
{
  results.clear();
  if (maxDist2 < 0.0 || maxDist2 <= minDist2)
  {
    return;
  }

  vtkBucketShell2D<TIds> shell(this, x, minDist2, petals);
  shell.Gather(maxDist2, std::numeric_limits<std::size_t>::max(), results);
  if (sort)
  {
    std::sort(results.begin(), results.end());
  }
}

//------------------------------------------------------------------------------
// The radius R defines a block of buckets which the circle of radius R may
// touch.
template <typename TIds>
void BucketList2D<TIds>::FindPointsWithinRadius(double R, const double x[3], vtkIdList* result)
{
  double dist2;
  double pt[3];
  vtkIdType ptId, cno, numIds;
  double R2 = R * R;
  const vtkLocatorTuple<TIds>* ids;
  double xMin[2], xMax[2];
  int i, j, ii, jOffset, ijMin[2], ijMax[2], xij[2];

  // Determine the range of indices in each direction based on radius R
  xMin[0] = x[0] - R;
  xMin[1] = x[1] - R;
  xMax[0] = x[0] + R;
  xMax[1] = x[1] + R;

  //  Find the footprint in the locator
  this->GetBucketIndices(x, xij);
  this->GetBucketIndices(xMin, ijMin);
  this->GetBucketIndices(xMax, ijMax);

  // Clear out previous results
  result->Reset();

  // Add points within footprint and radius
  for (j = ijMin[1]; j <= ijMax[1]; ++j)
  {
    jOffset = j * this->xD;
    for (i = ijMin[0]; i <= ijMax[0]; ++i)
    {
      cno = i + jOffset;

      if ((numIds = this->GetNumberOfIds(cno)) > 0)
      {
        // Reject buckets entirely outside the sphere, but never reject the
        // bucket that contains the query point (floating-point inconsistency
        // between GetBucketIndices and Distance2ToBucket can make that bucket
        // appear to have a small but nonzero distance from x).
        const bool isXBucket = i == xij[0] && j == xij[1];
        const int nei[3] = { i, j, 0 };
        if (!isXBucket && this->Distance2ToBucket(x, nei) > R2)
        {
          continue;
        }

        ids = this->GetIds(cno);

        // Accept all points if the bucket is entirely inside the circle
        if (this->BucketInsideCircle(i, j, x, R2))
        {
          for (ii = 0; ii < numIds; ii++)
          {
            result->InsertNextId(ids[ii].PtId);
          }
        }
        else
        {
          for (ii = 0; ii < numIds; ii++)
          {
            ptId = ids[ii].PtId;
            this->DataSet->GetPoint(ptId, pt);
            dist2 = Distance2BetweenPoints2D(x, pt);
            if (dist2 <= R2)
            {
              result->InsertNextId(ptId);
            }
          } // for all points in bucket
        }
      } // if points in bucket
    } // i-footprint
  } // j-footprint
}

//------------------------------------------------------------------------------
// Find the point within tol of the finite line, and closest to the starting
// point of the line (i.e., min parametric coordinate t). This is specialized
// for 2D, so the line may either be parallel to the locator or not. If not,
// the locator plane is intersected and the closest point is found from the
// intersection point. Otherwise, the ray is traversed through the locator as
// described below.
//
// Note that we have to traverse more than just the buckets (aka bins)
// containing the line since the closest point could be in a neighboring
// bin. To keep the code simple here's the straightforward approach used in
// the code below. Imagine tracing a circle of radius tol along the finite
// line, and processing all bins (and of course the points in the bins) which
// intersect the circle. We use a typical ray tracing approach (see
// vtkStaticCellLocator for references) and update the current voxels/bins at
// boundaries, including intersecting the circle with neighboring bins. Since
// this simple approach may visit bins multiple times, we keep an array that
// marks whether the bin has been visited previously and skip it if we have.
template <typename TIds>
int BucketList2D<TIds>::IntersectWithLine(double a0[3], double a1[3], double tol, double& t,
  double lineX[3], double ptX[3], vtkIdType& ptId)
{
  double* bounds = this->Bounds;
  int* ndivs = this->Divisions;
  double* h = this->H;
  double n[3] = { 0, 0, 1 };

  // First check if this line is in a 2D plane or not. If not, intersect the
  // locator plane with the line and return closest point.
  if (a0[2] != a1[2]) // not in locator plane
  {
    double minPnt[3];
    vtkBoundingBox bbox(bounds);
    bbox.Inflate(tol, tol, 0.0);
    bbox.GetMinPoint(minPnt);
    if (vtkPlane::IntersectWithLine(a0, a1, n, minPnt, t, lineX) && bbox.ContainsPoint(lineX))
    {
      ptId = this->FindClosestPoint(lineX);
      if (ptId < 0)
      {
        return 0;
      }
      this->DataSet->GetPoint(ptId, ptX);
      return 1;
    }
    else
    {
      ptId = (-1);
      return 0;
    }
  } // line not in z-plane

  // If here then the ray is parallel to the z-plane. In this case, traversing the
  // pixels (i.e., buckets) in the locator is required.
  TIds ii, numPtsInBin;
  double x[3], xl[3], rayDir[3], xmin[3], xmax[3];
  int ij[2], ijMin[2], ijMax[2];
  int i, j, enterExitCount;
  vtkIdType idx, pId, bestPtId = (-1);
  double step[2], next[2], tMax[2], tDelta[2];
  double curPos[3], curT, tHit, tMin = VTK_FLOAT_MAX;
  double tol2 = tol * tol;
  unsigned char* bucketHasBeenVisited = nullptr;
  vtkMath::Subtract(a1, a0, rayDir);

  // Need to pad out bbox
  vtkBoundingBox bbox(bounds);
  bbox.Inflate(0.0, 0.0, tol);
  bbox.GetBounds(bounds);

  if (vtkBox::IntersectBox(bounds, a0, rayDir, curPos, curT))
  {
    // Initialize intersection query array if necessary. This is done
    // locally to ensure thread safety.
    bucketHasBeenVisited = new unsigned char[this->NumBuckets];
    memset(bucketHasBeenVisited, 0, this->NumBuckets);

    // Get the i-j-k point of intersection and bin index. This is
    // clamped to the boundary of the locator.
    this->GetBucketIndices(curPos, ij);

    // Set up some parameters for traversing through bins
    step[0] = (rayDir[0] >= 0.0) ? 1.0 : -1.0;
    step[1] = (rayDir[1] >= 0.0) ? 1.0 : -1.0;

    // If the ray is going in the negative direction, then the next voxel boundary
    // is on the "-" direction so we stay in the current voxel.
    next[0] = bounds[0] + h[0] * (rayDir[0] >= 0.0 ? (ij[0] + step[0]) : ij[0]);
    next[1] = bounds[2] + h[1] * (rayDir[1] >= 0.0 ? (ij[1] + step[1]) : ij[1]);

    tMax[0] = (rayDir[0] != 0.0) ? (next[0] - curPos[0]) / rayDir[0] : VTK_FLOAT_MAX;
    tMax[1] = (rayDir[1] != 0.0) ? (next[1] - curPos[1]) / rayDir[1] : VTK_FLOAT_MAX;

    tDelta[0] = (rayDir[0] != 0.0) ? (h[0] / rayDir[0]) * step[0] : VTK_FLOAT_MAX;
    tDelta[1] = (rayDir[1] != 0.0) ? (h[1] / rayDir[1]) * step[1] : VTK_FLOAT_MAX;

    // Process current position including the bins in the circle
    // footprint. Note there is a rare pathological case where the footprint
    // on voxel exit must also be considered.
    for (bestPtId = (-1), enterExitCount = 0; bestPtId < 0 || enterExitCount < 2;)
    {
      // Get the "footprint" of bins containing the circle defined by the
      // current position and a radius of tol.
      xmin[0] = curPos[0] - tol;
      xmin[1] = curPos[1] - tol;
      xmax[0] = curPos[0] + tol;
      xmax[1] = curPos[1] + tol;
      this->GetBucketIndices(xmin, ijMin);
      this->GetBucketIndices(xmax, ijMax);

      // Start walking through the bins, find the best point of
      // intersection. Note that the ray may not penetrate all of the way
      // through the locator so may terminate when (t > 1.0).
      for (j = ijMin[1]; j <= ijMax[1]; ++j)
      {
        for (i = ijMin[0]; i <= ijMax[0]; ++i)
        {
          // Current bin index
          idx = i + j * ndivs[0];

          if (!bucketHasBeenVisited[idx])
          {
            bucketHasBeenVisited[idx] = 1;
            if ((numPtsInBin = this->GetNumberOfIds(idx)) > 0) // there are some points here
            {
              const vtkLocatorTuple<TIds>* ptIds = this->GetIds(idx);
              for (ii = 0; ii < numPtsInBin; ii++)
              {
                pId = ptIds[ii].PtId;
                this->DataSet->GetPoint(pId, x);
                if (vtkLine::DistanceToLine(x, a0, a1, tHit, xl) <= tol2 && t < tMin)
                {
                  tMin = t;
                  bestPtId = pId;
                } // point is within tolerance and closer
              } // over all points in bin
            } // if points in bin
          } // bucket not visited
        } // i bins
      } // j bins

      // Make sure to evaluate exit footprint as well. Must evaluate entrance
      // and exit of current voxel.
      if (bestPtId >= 0)
      {
        enterExitCount++;
      }

      // Advance to next pixel / bin
      if (tMax[0] < tMax[1])
      {
        ij[0] += static_cast<int>(step[0]);
        tMax[0] += tDelta[0];
        curT = tMax[0];
      }
      else
      {
        ij[1] += static_cast<int>(step[1]);
        tMax[1] += tDelta[1];
        curT = tMax[1];
      }

      // Check exit conditions
      if (curT > 1.0 || ij[0] < 0 || ij[0] >= ndivs[0] || ij[1] < 0 || ij[1] >= ndivs[1])
      {
        break;
      }
      else
      {
        curPos[0] = a0[0] + curT * rayDir[0];
        curPos[1] = a0[1] + curT * rayDir[1];
      }

    } // for looking for valid intersected point
  } // if (vtkBox::IntersectBox(...))

  // Clean up and get out
  delete[] bucketHasBeenVisited;

  // If a point has been intersected, recover the information and return.
  // This information could be cached....
  if (bestPtId >= 0)
  {
    // update the return information
    ptId = bestPtId;
    this->DataSet->GetPoint(ptId, ptX);
    vtkLine::DistanceToLine(ptX, a0, a1, t, lineX);

    return 1;
  }

  return 0;
}

//------------------------------------------------------------------------------
template <typename TIds>
double BucketList2D<TIds>::FindCloseNBoundedPoints(int N, const double x[3], vtkIdList* result)
{
  int i, j;
  double dist2;
  double pt[3];
  int level;
  vtkIdType ptId, cno, numIds;
  int ij[2], *nei;
  NeighborBuckets2D buckets;
  const vtkLocatorTuple<TIds>* ids;

  //  Find the bucket the point is in.
  this->GetBucketIndices(x, ij);

  // Gather points keeping track of maximum radius
  level = 0;
  double maxDist2 = 0.0;
  vtkDist2TupleType sortedPts;
  sortedPts.reserve(128);

  // Start in the current bucket and expand out to grab first N points. Keep
  // track of maximum distance.
  this->GetBucketNeighbors(&buckets, ij, this->Divisions, level);
  while (buckets.GetNumberOfNeighbors() > 0)
  {
    // For all buckets in this level
    for (i = 0; i < buckets.GetNumberOfNeighbors(); i++)
    {
      nei = buckets.GetPoint(i);
      cno = nei[0] + nei[1] * this->xD;
      if ((numIds = this->GetNumberOfIds(cno)) > 0)
      {
        ids = this->GetIds(cno);
        for (j = 0; j < numIds; j++)
        {
          ptId = ids[j].PtId;
          this->DataSet->GetPoint(ptId, pt);
          dist2 = Distance2BetweenPoints2D(x, pt);
          // accumulate first N points
          if (static_cast<int>(sortedPts.size()) < N)
          {
            maxDist2 = (dist2 > maxDist2 ? dist2 : maxDist2);
            sortedPts.emplace_back(ptId, dist2);
          }
          else if (dist2 <= maxDist2)
          {
            sortedPts.emplace_back(ptId, dist2);
          }
        }
      } // if points in bucket
    } // for buckets in this level
    level++;
    // As soon as N points in this level found, jump out.
    if (static_cast<int>(sortedPts.size()) >= N)
    {
      goto FOUND_N;
    }
    this->GetBucketNeighbors(&buckets, ij, this->Divisions, level);
  } // while still not found N points

// We've found at least N initial points (or exhausted all points). Now insert
// additional points that are closer than this original sample.
FOUND_N:
  if (static_cast<int>(sortedPts.size()) >= N)
  {
    // If here, check for any overlapping buckets we might have missed.
    this->GetOverlappingBuckets(&buckets, x, ij, sqrt(maxDist2), level - 1);
    for (i = 0; i < buckets.GetNumberOfNeighbors(); i++)
    {
      nei = buckets.GetPoint(i);
      cno = nei[0] + nei[1] * this->xD;
      if ((numIds = this->GetNumberOfIds(cno)) > 0)
      {
        ids = this->GetIds(cno);
        // Start where previous loop left off
        for (j = 0; j < numIds; j++)
        {
          ptId = ids[j].PtId;
          this->DataSet->GetPoint(ptId, pt);
          dist2 = Distance2BetweenPoints2D(x, pt);
          if (dist2 <= maxDist2)
          {
            sortedPts.emplace_back(ptId, dist2);
          }
        }
      } // if points in bucket
    } // for unprocessed buckets
  } // if more than N points

  // Now do final sort and find N closest, and if there are points located at
  // the same distance as the Nth point, include them too.
  std::sort(sortedPts.begin(), sortedPts.end());
  if (static_cast<int>(sortedPts.size()) <= N)
  {
    N = static_cast<int>(sortedPts.size());
  }
  else
  {
    maxDist2 = sortedPts.at(N - 1).Dist2;
    while (N < static_cast<int>(sortedPts.size()) && sortedPts.at(N).Dist2 == maxDist2)
    {
      N++;
    }
  }

  // Now copy result
  result->SetNumberOfIds(N);
  for (i = 0; i < N; i++)
  {
    result->SetId(i, sortedPts.at(i).Id);
  }

  return maxDist2;
}

//------------------------------------------------------------------------------
// Does the circle contain the bucket? Find the closest of the four points of
// the bucket and see if it is within R2.
template <typename TIds>
bool BucketList2D<TIds>::BucketIntersectsCircle(int i, int j, const double center[3], double R2)
{
  double delX = center[0] - (bX + static_cast<double>(i) * this->hX);
  double delY = center[1] - (bY + static_cast<double>(j) * this->hY);

  int quadrant = (delX > 0.0 ? 1 : 0);
  quadrant += (delY > 0.0 ? 2 : 0);

  switch (quadrant)
  {
    case 0:
      delX += this->hX;
      delY += this->hY;
      break;
    case 1:
      delY += this->hY;
      break;
    case 2:
      delX += this->hX;
      break;
    case 3:
      break;
  }

  return (delX * delX + delY * delY) <= R2;
}

//------------------------------------------------------------------------------
// Merge points based on tolerance. Return a point map. There are two
// separate paths: when the tolerance is precisely 0.0, and when tol >
// 0.0. Both are executed in parallel, although the second uses a
// checkerboard approach to avoid write collisions.
template <typename TIds>
void BucketList2D<TIds>::MergePoints(double tol, vtkIdType* mergeMap)
{
  // First mark all points as uninitialized
  std::fill_n(mergeMap, this->NumPts, (-1));

  // If tol=0, then just process points bucket by bucket. Don't have to worry
  // about points in other buckets.
  if (tol <= 0.0)
  {
    MergePrecise<TIds> merge(this, mergeMap);
    vtkSMPTools::For(0, this->NumBuckets, merge);
  }

  // Merge within a tolerance. This is a greedy algorithm that can give
  // weird results since exactly which points to merge with is not an
  // obvious answer (without doing fancy clustering etc).
  else
  {
    MergeClose<TIds> merge(this, tol, mergeMap);
    vtkSMPTools::For(0, this->NumPts, merge);
  }
}

//------------------------------------------------------------------------------
// Internal method to find those buckets that are within distance specified
// only those buckets outside of level radiuses of ij are returned
template <typename TIds>
void BucketList2D<TIds>::GetOverlappingBuckets(
  NeighborBuckets2D* buckets, const double x[3], const int ij[2], double dist, int level)
{
  int i, j, nei[3], minLevel[2], maxLevel[2];
  double xMin[3], xMax[3];

  // Initialize
  buckets->Reset();

  // Determine the range of indices in each direction
  xMin[0] = x[0] - dist;
  xMin[1] = x[1] - dist;
  xMax[0] = x[0] + dist;
  xMax[1] = x[1] + dist;

  this->GetBucketIndices(xMin, minLevel);
  this->GetBucketIndices(xMax, maxLevel);

  for (i = minLevel[0]; i <= maxLevel[0]; i++)
  {
    for (j = minLevel[1]; j <= maxLevel[1]; j++)
    {
      if (i < (ij[0] - level) || i > (ij[0] + level) || j < (ij[1] - level) || j > (ij[1] + level))
      {
        nei[0] = i;
        nei[1] = j;
        buckets->InsertNextBucket(nei);
      }
    }
  }
}

//------------------------------------------------------------------------------
// Internal method to find those buckets that are within distance specified
// only those buckets outside of level radiuses of ij are returned
template <typename TIds>
void BucketList2D<TIds>::GetOverlappingBuckets(NeighborBuckets2D* buckets, const double x[3],
  double dist, int prevMinLevel[2], int prevMaxLevel[2])
{
  int i, j, nei[3], minLevel[2], maxLevel[2];
  int jFactor;
  int jkSkipFlag;
  double xMin[2], xMax[2];

  // Initialize
  buckets->Reset();

  // Determine the range of indices in each direction
  xMin[0] = x[0] - dist;
  xMin[1] = x[1] - dist;
  xMax[0] = x[0] + dist;
  xMax[1] = x[1] + dist;

  this->GetBucketIndices(xMin, minLevel);
  this->GetBucketIndices(xMax, maxLevel);

  if (minLevel[0] == prevMinLevel[0] && maxLevel[0] == prevMaxLevel[0] &&
    minLevel[1] == prevMinLevel[1] && maxLevel[1] == prevMaxLevel[1])
  {
    return;
  }

  for (j = minLevel[1]; j <= maxLevel[1]; j++)
  {
    if (j >= prevMinLevel[1] && j <= prevMaxLevel[1])
    {
      jkSkipFlag = 1;
    }
    else
    {
      jkSkipFlag = 0;
    }
    jFactor = j * this->xD;
    for (i = minLevel[0]; i <= maxLevel[0]; i++)
    {
      if (jkSkipFlag && i == prevMinLevel[0])
      {
        i = prevMaxLevel[0];
        continue;
      }
      // if this bucket has any cells, add it to the list
      if (this->GetNumberOfIds(i + jFactor) > 0)
      {
        nei[0] = i;
        nei[1] = j;
        buckets->InsertNextBucket(nei);
      }
    }
  }

  prevMinLevel[0] = minLevel[0];
  prevMinLevel[1] = minLevel[1];
  prevMaxLevel[0] = maxLevel[0];
  prevMaxLevel[1] = maxLevel[1];
}

//------------------------------------------------------------------------------
// Build polygonal representation of locator. Create faces that separate
// inside/outside buckets, or separate inside/boundary of locator.
template <typename TIds>
void BucketList2D<TIds>::GenerateRepresentation(int vtkNotUsed(level), vtkPolyData* pd)
{
  vtkPoints* pts;
  vtkCellArray* polys;
  int ii, i, j, idx, inside;
  int offset[3] = { 0, 0, 0 }, minusOffset[3] = { 0, 0, 0 };

  pts = vtkPoints::New();
  pts->Reserve(5000);
  polys = vtkCellArray::New();
  polys->AllocateEstimate(2048, 3);

  // loop over all buckets, creating appropriate faces
  for (j = 0; j < this->Divisions[1]; j++)
  {
    offset[1] = j * this->Divisions[0];
    minusOffset[1] = (j - 1) * this->Divisions[0];
    for (i = 0; i < this->Divisions[0]; i++)
    {
      offset[0] = i;
      minusOffset[0] = i - 1;
      idx = offset[0] + offset[1];
      if (this->GetNumberOfIds(idx) > 0)
      {
        inside = 0;
      }
      else
      {
        inside = 1;
      }

      // check "negative" neighbors
      for (ii = 0; ii < 3; ii++)
      {
        if (minusOffset[ii] < 0)
        {
          if (inside)
          {
            this->GenerateFace(ii, i, j, 0, pts, polys);
          }
        }
        else
        {
          if (ii == 0)
          {
            idx = minusOffset[0] + offset[1] + offset[2];
          }
          else if (ii == 1)
          {
            idx = offset[0] + minusOffset[1] + offset[2];
          }
          else
          {
            idx = offset[0] + offset[1] + minusOffset[2];
          }

          if ((this->GetNumberOfIds(idx) > 0 && inside) ||
            (this->GetNumberOfIds(idx) > 0 && !inside))
          {
            this->GenerateFace(ii, i, j, 0, pts, polys);
          }
        }
        // those buckets on "positive" boundaries can generate faces specially
        if ((i + 1) >= this->Divisions[0] && inside)
        {
          this->GenerateFace(0, i + 1, j, 0, pts, polys);
        }
        if ((j + 1) >= this->Divisions[1] && inside)
        {
          this->GenerateFace(1, i, j + 1, 0, pts, polys);
        }

      } // over negative faces
    } // over i divisions
  } // over j divisions

  pd->SetPoints(pts);
  pts->Delete();
  pd->SetPolys(polys);
  polys->Delete();
  pd->Squeeze();
}

//------------------------------------------------------------------------------
// Here is the VTK class proper. It's implemented with the templated
// BucketList2D class.

//------------------------------------------------------------------------------
// Construct with automatic computation of divisions, averaging
// 5 points per bucket.
vtkStaticPointLocator2D::vtkStaticPointLocator2D()
{
  this->NumberOfPointsPerBucket = 5;
  this->Divisions[0] = this->Divisions[1] = 50;
  this->H[0] = this->H[1] = 0.0;
  this->Buckets = nullptr;
  this->MaxNumberOfBuckets = VTK_INT_MAX;
  this->LargeIds = false;
  this->Static = false;
}

//------------------------------------------------------------------------------
vtkStaticPointLocator2D::~vtkStaticPointLocator2D()
{
  this->FreeSearchStructure();
}

//------------------------------------------------------------------------------
void vtkStaticPointLocator2D::Initialize()
{
  this->FreeSearchStructure();
}

//------------------------------------------------------------------------------
void vtkStaticPointLocator2D::FreeSearchStructure()
{
  if (this->Buckets)
  {
    delete this->Buckets;
    this->Buckets = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkStaticPointLocator2D::BuildLocator()
{
  // if a search structure already exists
  if (this->Buckets)
  {
    // don't rebuild if UseExistingSearchStructure is ON
    if (this->UseExistingSearchStructure)
    {
      return;
    }
    // don't rebuild if build time is newer than modified and dataset modified time
    if (this->BuildTime > this->MTime && this->BuildTime > this->DataSet->GetMTime())
    {
      return;
    }
  }
  this->BuildLocatorInternal();
}

//------------------------------------------------------------------------------
void vtkStaticPointLocator2D::ForceBuildLocator()
{
  this->BuildLocatorInternal();
}

//------------------------------------------------------------------------------
//  Method to form subdivision of space based on the points provided and
//  subject to the constraints of levels and NumberOfPointsPerBucket.
//  The result is directly addressable and of uniform subdivision.
//
void vtkStaticPointLocator2D::BuildLocatorInternal()
{
  int ndivs[3];
  int i;
  vtkIdType numPts;

  vtkDebugMacro(<< "Hashing points...");
  this->Level = 1; // only single lowest level - from superclass

  if (!this->DataSet || (numPts = this->DataSet->GetNumberOfPoints()) < 1)
  {
    vtkErrorMacro(<< "No points to locate");
    return;
  }

  //  Make sure the appropriate data is available
  //
  if (this->Buckets)
  {
    this->FreeSearchStructure();
  }

  // Size the root bucket.  Initialize bucket data structure, compute
  // level and divisions. The GetBounds() method below can be very slow;
  // hopefully it is cached or otherwise accelerated.
  //
  const double* bounds = this->DataSet->GetBounds();
  vtkIdType numBuckets = static_cast<vtkIdType>(
    static_cast<double>(numPts) / static_cast<double>(this->NumberOfPointsPerBucket));
  numBuckets = (numBuckets > this->MaxNumberOfBuckets ? this->MaxNumberOfBuckets : numBuckets);

  vtkBoundingBox bbox(bounds);
  if (this->Automatic)
  {
    bbox.ComputeDivisions(numBuckets, this->Bounds, ndivs);
  }
  else
  {
    bbox.Inflate(); // make sure non-zero volume
    bbox.GetBounds(this->Bounds);
    for (i = 0; i < 2; i++)
    {
      ndivs[i] = (this->Divisions[i] < 1 ? 1 : this->Divisions[i]);
    }
  }

  this->Divisions[0] = ndivs[0];
  this->Divisions[1] = ndivs[1];
  this->NumberOfBuckets = numBuckets =
    static_cast<vtkIdType>(ndivs[0]) * static_cast<vtkIdType>(ndivs[1]);

  //  Compute width of bucket in three directions
  //
  for (i = 0; i < 2; i++)
  {
    this->H[i] = (this->Bounds[2 * i + 1] - this->Bounds[2 * i]) / static_cast<double>(ndivs[i]);
  }

  // Instantiate the locator. The type is related to the maximum point id.
  // This is done for performance (e.g., the sort is faster) and significant
  // memory savings.
  //
  if (numPts >= VTK_INT_MAX || numBuckets >= VTK_INT_MAX)
  {
    this->LargeIds = true;
    this->Buckets = new BucketList2D<vtkIdType>(this, numPts, numBuckets);
  }
  else
  {
    this->LargeIds = false;
    this->Buckets = new BucketList2D<int>(this, numPts, numBuckets);
  }

  // Actually construct the locator
  this->Buckets->BuildLocator();

  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
// These methods satisfy the vtkStaticPointLocator2D API. The implementation is
// with the templated BucketList2D class. Note that a lot of the complexity here
// is due to the desire to use different id types (int versus vtkIdType) for the
// purposes of increasing speed and reducing memory.
//
// You're probably wondering why an if check (on LargeIds) is used to
// static_cast on BucketList2D<T> type, when virtual inheritance could be
// used. Benchmarking shows a small speed difference due to inlining, which
// the use of virtual methods short circuits.

//------------------------------------------------------------------------------
// Given a position x, return the id of the point closest to it.
vtkIdType vtkStaticPointLocator2D::FindClosestPoint(const double x[3])
{
  this->BuildLocator(); // will subdivide if modified; otherwise returns
  if (!this->Buckets)
  {
    return -1;
  }

  if (this->LargeIds)
  {
    return static_cast<BucketList2D<vtkIdType>*>(this->Buckets)->FindClosestPoint(x);
  }
  else
  {
    return static_cast<BucketList2D<int>*>(this->Buckets)->FindClosestPoint(x);
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkStaticPointLocator2D::FindClosestPointWithinRadius(
  double radius, const double x[3], double inputDataLength, double& dist2)
{
  this->BuildLocator(); // will subdivide if modified; otherwise returns
  if (!this->Buckets)
  {
    return -1;
  }

  if (this->LargeIds)
  {
    return static_cast<BucketList2D<vtkIdType>*>(this->Buckets)
      ->FindClosestPointWithinRadius(radius, x, inputDataLength, dist2);
  }
  else
  {
    return static_cast<BucketList2D<int>*>(this->Buckets)
      ->FindClosestPointWithinRadius(radius, x, inputDataLength, dist2);
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkStaticPointLocator2D::FindClosestPointWithinRadius(
  double radius, const double x[3], double& dist2)
{
  return this->FindClosestPointWithinRadius(radius, x, this->DataSet->GetLength(), dist2);
}

//------------------------------------------------------------------------------
void vtkStaticPointLocator2D::FindClosestNPoints(int N, const double x[3], vtkIdList* result)
{
  this->BuildLocator(); // will subdivide if modified; otherwise returns
  if (!this->Buckets)
  {
    return;
  }

  if (this->LargeIds)
  {
    static_cast<BucketList2D<vtkIdType>*>(this->Buckets)->FindClosestNPoints(N, x, result);
  }
  else
  {
    static_cast<BucketList2D<int>*>(this->Buckets)->FindClosestNPoints(N, x, result);
  }
}

//------------------------------------------------------------------------------
double vtkStaticPointLocator2D::FindNPointsInShell(int N, const double x[3],
  vtkDist2TupleArray& results, double minDist2, bool sort, vtkDoubleArray* petals)
{
  this->BuildLocator(); // will subdivide if modified; otherwise returns
  results.clear();
  if (!this->Buckets)
  {
    return 0.0;
  }

  if (this->LargeIds)
  {
    return static_cast<BucketList2D<vtkIdType>*>(this->Buckets)
      ->FindNPointsInShell(N, x, results, minDist2, sort, petals);
  }
  else
  {
    return static_cast<BucketList2D<int>*>(this->Buckets)
      ->FindNPointsInShell(N, x, results, minDist2, sort, petals);
  }
}

//------------------------------------------------------------------------------
double vtkStaticPointLocator2D::FindNPointsInAnnulus(int N, const double x[3],
  vtkDist2TupleArray& results, double minDist2, bool sort, vtkDoubleArray* petals)
{
  return this->FindNPointsInShell(N, x, results, minDist2, sort, petals);
}

//------------------------------------------------------------------------------
void vtkStaticPointLocator2D::FindPointsInShell(const double x[3], vtkDist2TupleArray& results,
  double minDist2, double maxDist2, bool sort, vtkDoubleArray* petals)
{
  this->BuildLocator(); // will subdivide if modified; otherwise returns
  results.clear();
  if (!this->Buckets)
  {
    return;
  }

  if (this->LargeIds)
  {
    static_cast<BucketList2D<vtkIdType>*>(this->Buckets)
      ->FindPointsInShell(x, results, minDist2, maxDist2, sort, petals);
  }
  else
  {
    static_cast<BucketList2D<int>*>(this->Buckets)
      ->FindPointsInShell(x, results, minDist2, maxDist2, sort, petals);
  }
}

//------------------------------------------------------------------------------
void vtkStaticPointLocator2D::FindPointsWithinRadius(double R, const double x[3], vtkIdList* result)
{
  this->BuildLocator(); // will subdivide if modified; otherwise returns
  if (!this->Buckets)
  {
    return;
  }

  if (this->LargeIds)
  {
    static_cast<BucketList2D<vtkIdType>*>(this->Buckets)->FindPointsWithinRadius(R, x, result);
  }
  else
  {
    static_cast<BucketList2D<int>*>(this->Buckets)->FindPointsWithinRadius(R, x, result);
  }
}

//------------------------------------------------------------------------------
// Find bounded points, approximately N, returning max radius Rmax. All
// points returned will be r <= Rmax.
double vtkStaticPointLocator2D::FindCloseNBoundedPoints(int N, const double x[3], vtkIdList* result)
{
  this->BuildLocator(); // will subdivide if modified; otherwise returns
  if (!this->Buckets)
  {
    return 0.0;
  }

  if (this->LargeIds)
  {
    return static_cast<BucketList2D<vtkIdType>*>(this->Buckets)
      ->FindCloseNBoundedPoints(N, x, result);
  }
  else
  {
    return static_cast<BucketList2D<int>*>(this->Buckets)->FindCloseNBoundedPoints(N, x, result);
  }
}

//------------------------------------------------------------------------------
// This method traverses the locator along the defined ray, finding the
// closest point to a0 when projected onto the line (a0,a1) (i.e., min
// parametric coordinate t) and within the tolerance tol (measured in the
// world coordinate system).
int vtkStaticPointLocator2D::IntersectWithLine(double a0[3], double a1[3], double tol, double& t,
  double lineX[3], double ptX[3], vtkIdType& ptId)
{
  this->BuildLocator(); // will subdivide if modified; otherwise returns
  if (!this->Buckets)
  {
    return 0;
  }

  if (this->LargeIds)
  {
    return static_cast<BucketList2D<vtkIdType>*>(this->Buckets)
      ->IntersectWithLine(a0, a1, tol, t, lineX, ptX, ptId);
  }
  else
  {
    return static_cast<BucketList2D<int>*>(this->Buckets)
      ->IntersectWithLine(a0, a1, tol, t, lineX, ptX, ptId);
  }
}

//------------------------------------------------------------------------------
void vtkStaticPointLocator2D::GenerateRepresentation(int level, vtkPolyData* pd)
{
  this->BuildLocator(); // will subdivide if modified; otherwise returns
  if (!this->Buckets)
  {
    return;
  }

  if (this->LargeIds)
  {
    static_cast<BucketList2D<vtkIdType>*>(this->Buckets)->GenerateRepresentation(level, pd);
  }
  else
  {
    static_cast<BucketList2D<int>*>(this->Buckets)->GenerateRepresentation(level, pd);
  }
}

//------------------------------------------------------------------------------
// Given a bin/bucket number, return the number of points in the bin/bucket.
vtkIdType vtkStaticPointLocator2D::GetNumberOfPointsInBucket(vtkIdType bNum)
{
  if (this->LargeIds)
  {
    return static_cast<BucketList2D<vtkIdType>*>(this->Buckets)->GetNumberOfIds(bNum);
  }
  else
  {
    return static_cast<BucketList2D<int>*>(this->Buckets)->GetNumberOfIds(bNum);
  }
}

//------------------------------------------------------------------------------
// Given a bin/bucket number, return an array of point ids in the bucket. Use
// this method in conjunction with GetNumberOfPointsInBucket().
void vtkStaticPointLocator2D::GetBucketIds(vtkIdType bNum, vtkIdList* bList)
{
  if (this->LargeIds)
  {
    static_cast<BucketList2D<vtkIdType>*>(this->Buckets)->GetIds(bNum, bList);
  }
  else
  {
    static_cast<BucketList2D<int>*>(this->Buckets)->GetIds(bNum, bList);
  }
}

//------------------------------------------------------------------------------
// Given a bucket, return the ids in the bucket.
void vtkStaticPointLocator2D::MergePoints(double tol, vtkIdType* pointMap)
{
  this->BuildLocator(); // will subdivide if modified; otherwise returns
  if (!this->Buckets)
  {
    return;
  }

  if (this->LargeIds)
  {
    static_cast<BucketList2D<vtkIdType>*>(this->Buckets)->MergePoints(tol, pointMap);
  }
  else
  {
    static_cast<BucketList2D<int>*>(this->Buckets)->MergePoints(tol, pointMap);
  }
}

//------------------------------------------------------------------------------
void vtkStaticPointLocator2D::GetBucketIndices(const double* x, int ij[2]) const
{
  this->Buckets->GetBucketIndices(x, ij);
}

//------------------------------------------------------------------------------
vtkIdType vtkStaticPointLocator2D::GetBucketIndex(const double* x) const
{
  return this->Buckets->GetBucketIndex(x);
}

//------------------------------------------------------------------------------
void vtkStaticPointLocator2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number of Points Per Bucket: " << this->NumberOfPointsPerBucket << "\n";

  os << indent << "Divisions: (" << this->Divisions[0] << ", " << this->Divisions[1] << ")\n";

  os << indent << "Max Number Of Buckets: " << this->MaxNumberOfBuckets << "\n";

  os << indent << "Large IDs: " << this->LargeIds << "\n";

  os << indent << "Static: " << (this->Static ? "On\n" : "Off\n");
}
VTK_ABI_NAMESPACE_END
