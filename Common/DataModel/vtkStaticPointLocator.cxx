// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkStaticPointLocator.h"

#include "vtkBoundingBox.h"
#include "vtkBox.h"
#include "vtkCellArray.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkLine.h"
#include "vtkLocatorInterface.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"
#include "vtkStructuredData.h"

//------------------------------------------------------------------------------
// Parts of the locator and related classes are made visible through the
// included private include file (to satisfy the one definition rule). This
// include files declares internal classes.  Definitions follow in this .cxx
// file.
#include "vtkStaticPointLocatorPrivate.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkStaticPointLocator);

// There are stack-allocated bucket neighbor lists. This is the initial
// value. Too small and heap allocation kicks in.
constexpr size_t VTK_INITIAL_BUCKET_SIZE = 10000;

//------------------------------------------------------------------------------
void vtkBucketList::GenerateFace(int face, int i, int j, int k, vtkPoints* pts, vtkCellArray* polys)
{
  vtkIdType ids[4];
  double origin[3], x[3];

  // define first corner
  origin[0] = this->bX + i * this->hX;
  origin[1] = this->bY + j * this->hY;
  origin[2] = this->bZ + k * this->hZ;
  ids[0] = pts->InsertNextPoint(origin);

  if (face == 0) // x face
  {
    x[0] = origin[0];
    x[1] = origin[1] + this->hY;
    x[2] = origin[2];
    ids[1] = pts->InsertNextPoint(x);

    x[0] = origin[0];
    x[1] = origin[1] + this->hY;
    x[2] = origin[2] + this->hZ;
    ids[2] = pts->InsertNextPoint(x);

    x[0] = origin[0];
    x[1] = origin[1];
    x[2] = origin[2] + this->hZ;
    ids[3] = pts->InsertNextPoint(x);
  }

  else if (face == 1) // y face
  {
    x[0] = origin[0] + this->hX;
    x[1] = origin[1];
    x[2] = origin[2];
    ids[1] = pts->InsertNextPoint(x);

    x[0] = origin[0] + this->hX;
    x[1] = origin[1];
    x[2] = origin[2] + this->hZ;
    ids[2] = pts->InsertNextPoint(x);

    x[0] = origin[0];
    x[1] = origin[1];
    x[2] = origin[2] + this->hZ;
    ids[3] = pts->InsertNextPoint(x);
  }

  else // z face
  {
    x[0] = origin[0] + this->hX;
    x[1] = origin[1];
    x[2] = origin[2];
    ids[1] = pts->InsertNextPoint(x);

    x[0] = origin[0] + this->hX;
    x[1] = origin[1] + this->hY;
    x[2] = origin[2];
    ids[2] = pts->InsertNextPoint(x);

    x[0] = origin[0];
    x[1] = origin[1] + this->hY;
    x[2] = origin[2];
    ids[3] = pts->InsertNextPoint(x);
  }

  polys->InsertNextCell(4, ids);
}

//------------------------------------------------------------------------------
// Calculate the distance between the point x to the bucket "nei".
//
// WARNING!!!!! Be very careful altering this routine.  Simple changes to this
// routine can make is 25% slower!!!!
//
double vtkBucketList::Distance2ToBucket(const double x[3], const int nei[3])
{
  double bounds[6];

  bounds[0] = nei[0] * this->hX + this->bX;
  bounds[1] = (nei[0] + 1) * this->hX + this->bX;
  bounds[2] = nei[1] * this->hY + this->bY;
  bounds[3] = (nei[1] + 1) * this->hY + this->bY;
  bounds[4] = nei[2] * this->hZ + this->bZ;
  bounds[5] = (nei[2] + 1) * this->hZ + this->bZ;

  return this->Distance2ToBounds(x, bounds);
}

//------------------------------------------------------------------------------
// Calculate the distance between the point x and the specified bounds
//
// WARNING!!!!! Be very careful altering this routine.  Simple changes to this
// routine can make is 25% slower!!!!
double vtkBucketList::Distance2ToBounds(const double x[3], const double bounds[6])
{
  double distance;
  double deltas[3];

  // Are we within the bounds?
  if (x[0] >= bounds[0] && x[0] <= bounds[1] && x[1] >= bounds[2] && x[1] <= bounds[3] &&
    x[2] >= bounds[4] && x[2] <= bounds[5])
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

  // dz
  //
  if (x[2] < bounds[4])
  {
    deltas[2] = bounds[4] - x[2];
  }
  else if (x[2] > bounds[5])
  {
    deltas[2] = x[2] - bounds[5];
  }

  distance = vtkMath::Dot(deltas, deltas);
  return distance;
}

//------------------------------------------------------------------------------
// Utility class to store an array of ijk values
struct NeighborBuckets
{
  NeighborBuckets()
  {
    this->Count = 0;
    this->P = this->InitialBuffer;
    this->MaxSize = VTK_INITIAL_BUCKET_SIZE;
  }
  ~NeighborBuckets()
  {
    this->Count = 0;
    if (this->P != this->InitialBuffer)
    {
      delete[] this->P;
    }
  }
  int GetNumberOfNeighbors() { return this->Count; }
  void Reset() { this->Count = 0; }

  int* GetPoint(vtkIdType i) { return this->P + 3 * i; }

  vtkIdType InsertNextBucket(const int x[3])
  {
    // Re-allocate if beyond the current max size.
    // (Increase by VTK_INITIAL_BUCKET_SIZE)
    int* tmp;
    vtkIdType offset = this->Count * 3;

    if (this->Count >= this->MaxSize)
    {
      tmp = this->P;
      this->MaxSize *= 2;
      this->P = new int[this->MaxSize * 3];

      memcpy(this->P, tmp, offset * sizeof(int));

      if (tmp != this->InitialBuffer)
      {
        delete[] tmp;
      }
    }

    tmp = this->P + offset;
    *tmp++ = *x++;
    *tmp++ = *x++;
    *tmp = *x;
    this->Count++;
    return this->Count - 1;
  }

protected:
  // Start with an array to avoid memory allocation overhead
  int InitialBuffer[VTK_INITIAL_BUCKET_SIZE * 3];
  int* P;
  vtkIdType Count;
  vtkIdType MaxSize;
};

//------------------------------------------------------------------------------
//  Internal function to get bucket neighbors at specified level
//
void vtkBucketList::GetBucketNeighbors(
  NeighborBuckets* buckets, const int ijk[3], const int ndivs[3], int level)
{
  int i, j, k, min, max, minLevel[3], maxLevel[3];
  int nei[3];

  //  Initialize
  //
  buckets->Reset();

  //  If at this bucket, just place into list
  //
  if (level == 0)
  {
    buckets->InsertNextBucket(ijk);
    return;
  }

  //  Create permutations of the ijk indices that are at the level
  //  required. If these are legal buckets, add to list for searching.
  //
  for (i = 0; i < 3; i++)
  {
    min = ijk[i] - level;
    max = ijk[i] + level;
    minLevel[i] = (min > 0 ? min : 0);
    maxLevel[i] = (max < (ndivs[i] - 1) ? max : (ndivs[i] - 1));
  }

  for (i = minLevel[0]; i <= maxLevel[0]; i++)
  {
    for (j = minLevel[1]; j <= maxLevel[1]; j++)
    {
      for (k = minLevel[2]; k <= maxLevel[2]; k++)
      {
        if (i == (ijk[0] + level) || i == (ijk[0] - level) || j == (ijk[1] + level) ||
          j == (ijk[1] - level) || k == (ijk[2] + level) || k == (ijk[2] - level))
        {
          nei[0] = i;
          nei[1] = j;
          nei[2] = k;
          buckets->InsertNextBucket(nei);
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
// Given a position x, return the id of the point closest to it.
template <typename TIds>
vtkIdType BucketList<TIds>::FindClosestPoint(const double x[3])
{
  int i, j;
  double minDist2;
  double dist2 = VTK_DOUBLE_MAX;
  double pt[3];
  int closest, level;
  vtkIdType ptId, cno, numIds;
  int ijk[3], *nei;
  NeighborBuckets buckets;
  const vtkLocatorTuple<TIds>* ids;

  //  Find bucket point is in.
  //
  this->GetBucketIndices(x, ijk);

  //  Need to search this bucket for the closest point.  If there are no
  //  points in this bucket, search 1st level neighbors, and so on, until
  //  closest point found.
  //
  for (closest = (-1), minDist2 = VTK_DOUBLE_MAX, level = 0; (closest == -1) &&
       (level < this->Divisions[0] || level < this->Divisions[1] || level < this->Divisions[2]);
       level++)
  {
    this->GetBucketNeighbors(&buckets, ijk, this->Divisions, level);

    for (i = 0; i < buckets.GetNumberOfNeighbors(); i++)
    {
      nei = buckets.GetPoint(i);
      cno = nei[0] + nei[1] * this->xD + nei[2] * this->xyD;

      if ((numIds = this->GetNumberOfIds(cno)) > 0)
      {
        ids = this->GetIds(cno);
        for (j = 0; j < numIds; j++)
        {
          ptId = ids[j].PtId;
          this->DataSet->GetPoint(ptId, pt);
          dist2 = vtkMath::Distance2BetweenPoints(x, pt);
          // Always accept the first candidate found: for a query far outside
          // the locator bounds (e.g. VTK_DOUBLE_MAX) the squared distance
          // overflows to infinity, which would never compare less than minDist2.
          if (closest < 0 || dist2 < minDist2)
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
  // Skip this refinement when minDist2 is not finite (degenerate query far
  // outside the bounds): the closest point is already arbitrary and the
  // search radius would be infinite.
  //
  if (minDist2 > 0.0 && vtkMath::IsFinite(minDist2))
  {
    this->GetOverlappingBuckets(&buckets, x, ijk, sqrt(minDist2), 0);
    for (i = 0; i < buckets.GetNumberOfNeighbors(); i++)
    {
      nei = buckets.GetPoint(i);
      cno = nei[0] + nei[1] * this->xD + nei[2] * this->xyD;

      if ((numIds = this->GetNumberOfIds(cno)) > 0)
      {
        ids = this->GetIds(cno);
        for (j = 0; j < numIds; j++)
        {
          ptId = ids[j].PtId;
          this->DataSet->GetPoint(ptId, pt);
          if ((dist2 = vtkMath::Distance2BetweenPoints(x, pt)) < minDist2)
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
vtkIdType BucketList<TIds>::FindClosestPointWithinRadius(
  double radius, const double x[3], double inputDataLength, double& dist2)
{
  int i, j;
  double pt[3];
  vtkIdType ptId, closest = -1;
  int ijk[3], *nei;
  double minDist2;

  double refinedRadius, radius2, refinedRadius2;
  double currentRadius;
  double distance2ToDataBounds, maxDistance;
  int ii, radiusLevels[3], radiusLevel, prevMinLevel[3], prevMaxLevel[3];
  NeighborBuckets buckets;
  const vtkLocatorTuple<TIds>* ids;

  // Initialize
  dist2 = -1.0;
  radius2 = radius * radius;
  minDist2 = 1.01 * radius2; // something slightly bigger....

  vtkDataArray* pointData = static_cast<vtkPointSet*>(this->DataSet)->GetPoints()->GetData();

  //  Find the bucket the point is in.
  //
  this->GetBucketIndices(x, ijk);

  // Start by searching the bucket that the point is in.
  //
  vtkIdType numIds;
  vtkIdType cno = ijk[0] + ijk[1] * this->xD + ijk[2] * this->xyD;
  if ((numIds = this->GetNumberOfIds(cno)) > 0)
  {
    ids = this->GetIds(cno);
    for (j = 0; j < numIds; j++)
    {
      ptId = ids[j].PtId;
      pointData->GetTuple(ptId, pt);
      if ((dist2 = vtkMath::Distance2BetweenPoints(x, pt)) < minDist2)
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

  for (i = 0; i < 3; i++)
  {
    radiusLevels[i] = static_cast<int>(refinedRadius / this->H[i]);
    if (radiusLevels[i] > this->Divisions[i] / 2)
    {
      radiusLevels[i] = this->Divisions[i] / 2;
    }
  }

  radiusLevel = radiusLevels[0];
  radiusLevel = radiusLevels[1] > radiusLevel ? radiusLevels[1] : radiusLevel;
  radiusLevel = radiusLevels[2] > radiusLevel ? radiusLevels[2] : radiusLevel;
  if (radiusLevel == 0)
  {
    radiusLevel = 1;
  }

  // radius schedule increases the radius each iteration, this is currently
  // implemented by decreasing ii by 1 each iteration.  another alternative
  // is to double the radius each iteration, i.e. ii = ii >> 1
  // In practice, reducing ii by one has been found to be more efficient.
  prevMinLevel[0] = prevMaxLevel[0] = ijk[0];
  prevMinLevel[1] = prevMaxLevel[1] = ijk[1];
  prevMinLevel[2] = prevMaxLevel[2] = ijk[2];
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
        cno = nei[0] + nei[1] * this->xD + nei[2] * this->xyD;
        if ((numIds = this->GetNumberOfIds(cno)) > 0)
        {
          ids = this->GetIds(cno);
          for (j = 0; j < numIds; j++)
          {
            ptId = ids[j].PtId;
            pointData->GetTuple(ptId, pt);
            if ((dist2 = vtkMath::Distance2BetweenPoints(x, pt)) < minDist2)
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
void BucketList<TIds>::FindClosestNPoints(int N, const double x[3], vtkIdList* result)
{
  int i, j;
  double dist2;
  double pt[3];
  int level;
  vtkIdType ptId, cno, numIds;
  int ijk[3], *nei;
  NeighborBuckets buckets;
  const vtkLocatorTuple<TIds>* ids;

  // Clear out any previous results
  result->Reset();

  //  Find the bucket the point is in.
  //
  this->GetBucketIndices(x, ijk);

  // There are two steps, first a simple expanding wave of buckets until
  // we have enough points. Then a refinement to make sure we have the
  // N closest points.
  level = 0;
  double maxDistance = 0.0;
  int currentCount = 0;
  vtkDist2TupleType res(N);

  this->GetBucketNeighbors(&buckets, ijk, this->Divisions, level);
  while (buckets.GetNumberOfNeighbors() && currentCount < N)
  {
    for (i = 0; i < buckets.GetNumberOfNeighbors(); i++)
    {
      nei = buckets.GetPoint(i);
      cno = nei[0] + nei[1] * this->xD + nei[2] * this->xyD;

      if ((numIds = this->GetNumberOfIds(cno)) > 0)
      {
        ids = this->GetIds(cno);
        for (j = 0; j < numIds; j++)
        {
          ptId = ids[j].PtId;
          this->DataSet->GetPoint(ptId, pt);
          dist2 = vtkMath::Distance2BetweenPoints(x, pt);
          if (currentCount < N)
          {
            res[currentCount].Dist2 = dist2;
            res[currentCount].Id = ptId;
            maxDistance = std::max(dist2, maxDistance);
            currentCount++;
            if (currentCount == N)
            {
              std::sort(res.begin(), res.begin() + currentCount);
            }
          }
          else if (dist2 < maxDistance)
          {
            res[N - 1].Dist2 = dist2;
            res[N - 1].Id = ptId;
            std::sort(res.begin(), res.begin() + N);
            maxDistance = res[N - 1].Dist2;
          }
        }
      }
    }
    level++;
    this->GetBucketNeighbors(&buckets, ijk, this->Divisions, level);
  }

  // do a sort
  std::sort(res.begin(), res.begin() + currentCount);

  // Now do the refinement
  this->GetOverlappingBuckets(&buckets, x, ijk, sqrt(maxDistance), level - 1);

  for (i = 0; i < buckets.GetNumberOfNeighbors(); i++)
  {
    nei = buckets.GetPoint(i);
    cno = nei[0] + nei[1] * this->xD + nei[2] * this->xyD;

    if ((numIds = this->GetNumberOfIds(cno)) > 0)
    {
      ids = this->GetIds(cno);
      for (j = 0; j < numIds; j++)
      {
        ptId = ids[j].PtId;
        this->DataSet->GetPoint(ptId, pt);
        dist2 = vtkMath::Distance2BetweenPoints(x, pt);
        if (dist2 < maxDistance)
        {
          res[N - 1].Dist2 = dist2;
          res[N - 1].Id = ptId;
          std::sort(res.begin(), res.begin() + N);
          maxDistance = res[N - 1].Dist2;
        }
      }
    }
  }

  // Fill in the IdList
  result->SetNumberOfIds(currentCount);
  for (i = 0; i < currentCount; i++)
  {
    result->SetId(i, res[i].Id);
  }
}

//------------------------------------------------------------------------------
namespace
{
// The buckets overlapping a shell (MinDist2, maxDist2] around X, and at least
// one of the petals, visited in blocks of increasing levels around the bucket
// of X (see vtkBucketBlockIterator), from the first block not inside the inner
// sphere. The points are assigned to the buckets with GetBucketIndices(), which
// may round differently from the bucket bounds: the buckets are tested
// enlarged by Delta, and with a relative slack on the squared distances.
template <typename TIds>
class vtkBucketShell
{
public:
  vtkBucketShell(BucketList<TIds>* bins, const double x[3], double minDist2, vtkDoubleArray* petals)
    : Bins(bins)
    , X(x)
    , MinDist2(minDist2)
    , NumPetals(petals ? petals->GetNumberOfTuples() : 0)
    , Petals(this->NumPetals > 0 ? petals->GetPointer(0) : nullptr)
  {
    bins->GetBucketIndices(x, this->Center);
    for (int i = 0; i < 3; ++i)
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
        const double* petal = this->Petals + 4 * m;
        reach = std::max(
          reach, std::sqrt(vtkMath::Distance2BetweenPoints(x, petal)) + std::sqrt(petal[3]));
      }
      this->Reach2 = reach * reach;
    }

    // Skip the blocks inside the inner sphere: all their points are at most
    // MinDist2 away.
    double lo[3], hi[3];
    for (; this->Level <= bins->MaxLevel; ++this->Level)
    {
      this->GetBlockBounds(this->Level, true, lo, hi);
      if (FarDist2(lo, hi, x) * (1.0 + Slack) > minDist2)
      {
        break;
      }
    }
    this->Blocks.Initialize(bins->Divisions, bins->MaxLevel);
  }

  // Add to results the valid points (in the shell up to maxDist2, and inside
  // or on a petal) of the next levels, until the next level is entirely beyond
  // maxDist2 or the petals, or until results holds at least minSize points.
  // The next call continues with the next level.
  void Gather(double maxDist2, std::size_t minSize, vtkDist2TupleArray& results)
  {
    BucketList<TIds>* bins = this->Bins;
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

      for (vtkIdType bucket = this->Blocks.Begin(this->Level, this->Center); bucket >= 0;
           bucket = this->Blocks.Next())
      {
        const vtkIdType numIds = bins->GetNumberOfIds(bucket);
        if (numIds <= 0)
        {
          continue;
        }

        // Skip the buckets outside of the shell, or outside of all the petals.
        bins->GetBucketBounds(
          this->Blocks.IJK[0], this->Blocks.IJK[1], this->Blocks.IJK[2], lo, hi);
        for (int c = 0; c < 3; ++c)
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
        for (vtkIdType m = 0; m < numPetals && !inPetals; ++m, petal += 4)
        {
          inPetals = (NearDist2(lo, hi, petal) * (1.0 - Slack) <= petal[3]);
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
          const double d2 = vtkMath::Distance2BetweenPoints(pt, x);
          if (d2 <= minDist2 || d2 > maxDist2)
          {
            continue;
          }
          bool inPetal = (numPetals == 0);
          petal = petals;
          for (vtkIdType m = 0; m < numPetals && !inPetal; ++m, petal += 4)
          {
            inPetal = (vtkMath::Distance2BetweenPoints(pt, petal) <= petal[3]);
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

  // The bounds of the block of buckets of a level around Center, enlarged by
  // Delta (grow), or shrunk by it.
  void GetBlockBounds(int level, bool grow, double lo[3], double hi[3]) const
  {
    for (int i = 0; i < 3; ++i)
    {
      const double margin = grow ? this->Delta[i] : -this->Delta[i];
      lo[i] = this->Bins->Bounds[2 * i] + (this->Center[i] - level) * this->Bins->H[i] - margin;
      hi[i] = this->Bins->Bounds[2 * i] + (this->Center[i] + level + 1) * this->Bins->H[i] + margin;
    }
  }

  // The squared distance from c to the farthest point of a box.
  static double FarDist2(const double lo[3], const double hi[3], const double* c)
  {
    double d2 = 0.0;
    for (int i = 0; i < 3; ++i)
    {
      const double d = std::max(c[i] - lo[i], hi[i] - c[i]);
      d2 += d * d;
    }
    return d2;
  }

  // Whether some bucket of the current level can be within dist2 of X: they
  // are outside of the block of the previous level, on the sides that still
  // have buckets. Their points may be up to Delta inside of it: the block is
  // shrunk.
  bool Reaches(double dist2) const
  {
    if (this->Level == 0)
    {
      return true;
    }
    double lo[3], hi[3];
    this->GetBlockBounds(this->Level - 1, false, lo, hi);
    double minD2 = VTK_DOUBLE_MAX;
    for (int i = 0; i < 3; ++i)
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
  static double NearDist2(const double lo[3], const double hi[3], const double* c)
  {
    double d2 = 0.0;
    for (int i = 0; i < 3; ++i)
    {
      const double d = std::max({ lo[i] - c[i], c[i] - hi[i], 0.0 });
      d2 += d * d;
    }
    return d2;
  }

  BucketList<TIds>* Bins;
  const double* X;
  double MinDist2;
  vtkIdType NumPetals;
  const double* Petals;
  double Reach2 = VTK_DOUBLE_MAX;
  int Center[3];
  double Delta[3];
  int Level = 0; // the next level to visit
  vtkBucketBlockIterator Blocks;
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
double BucketList<TIds>::FindNPointsInShell(int N, const double x[3], vtkDist2TupleArray& results,
  double minDist2, bool sort, vtkDoubleArray* petals)
{
  results.clear();
  if (N <= 0)
  {
    return 0.0;
  }

  vtkBucketShell<TIds> shell(this, x, minDist2, petals);
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
// The radius R defines a block of buckets which the sphere of radius R may
// touch.
template <typename TIds>
void BucketList<TIds>::FindPointsWithinRadius(double R, const double x[3], vtkIdList* result)
{
  double dist2;
  double pt[3];
  vtkIdType ptId, cno, numIds;
  double R2 = R * R;
  const vtkLocatorTuple<TIds>* ids;
  double xMin[3], xMax[3];
  int i, j, k, ii, jOffset, kOffset, ijkMin[3], ijkMax[3], xijk[3];

  // Determine the range of indices in each direction based on radius R
  xMin[0] = x[0] - R;
  xMin[1] = x[1] - R;
  xMin[2] = x[2] - R;
  xMax[0] = x[0] + R;
  xMax[1] = x[1] + R;
  xMax[2] = x[2] + R;

  //  Find the footprint in the locator
  this->GetBucketIndices(x, xijk);
  this->GetBucketIndices(xMin, ijkMin);
  this->GetBucketIndices(xMax, ijkMax);

  // Clear out previous results
  result->Reset();

  // Add points within footprint and radius
  for (k = ijkMin[2]; k <= ijkMax[2]; ++k)
  {
    kOffset = k * this->xyD;
    for (j = ijkMin[1]; j <= ijkMax[1]; ++j)
    {
      jOffset = j * this->xD;
      for (i = ijkMin[0]; i <= ijkMax[0]; ++i)
      {
        cno = i + jOffset + kOffset;

        if ((numIds = this->GetNumberOfIds(cno)) > 0)
        {
          // Reject buckets entirely outside the sphere, but never reject the
          // bucket that contains the query point (floating-point inconsistency
          // between GetBucketIndices and Distance2ToBucket can make that bucket
          // appear to have a small but nonzero distance from x).
          const bool isXBucket = i == xijk[0] && j == xijk[1] && k == xijk[2];
          const int nei[3] = { i, j, k };
          if (!isXBucket && this->Distance2ToBucket(x, nei) > R2)
          {
            continue;
          }

          ids = this->GetIds(cno);

          // Accept all points if the bucket is entirely inside the sphere
          if (this->BucketInsideSphere(i, j, k, x, R2))
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
              dist2 = vtkMath::Distance2BetweenPoints(x, pt);
              if (dist2 <= R2)
              {
                result->InsertNextId(ptId);
              }
            } // for all points in bucket
          }
        } // if points in bucket
      } // i-footprint
    } // j-footprint
  } // k-footprint
}

//------------------------------------------------------------------------------
// Gather the points of the buckets overlapping the shell (minDist2, maxDist2]
// around x, and inside at least one of the petals (see vtkBucketShell).
template <typename TIds>
void BucketList<TIds>::FindPointsInShell(const double x[3], vtkDist2TupleArray& results,
  double minDist2, double maxDist2, bool sort, vtkDoubleArray* petals)
{
  results.clear();
  if (maxDist2 < 0.0 || maxDist2 <= minDist2)
  {
    return;
  }

  vtkBucketShell<TIds> shell(this, x, minDist2, petals);
  shell.Gather(maxDist2, std::numeric_limits<std::size_t>::max(), results);
  if (sort)
  {
    std::sort(results.begin(), results.end());
  }
}

//------------------------------------------------------------------------------
// Find the point within tol of the finite line, and closest to the starting
// point of the line (i.e., min parametric coordinate t).
//
// Note that we have to traverse more than just the buckets (aka bins)
// containing the line since the closest point could be in a neighboring
// bin. To keep the code simple here's the straightforward approach used in
// the code below. Imagine tracing a sphere of radius tol along the finite
// line, and processing all bins (and of course the points in the bins) which
// intersect the sphere. We use a typical ray tracing approach (see
// vtkStaticCellLocator for references) and update the current voxels/bins at
// boundaries, including intersecting the sphere with neighboring bins. Since
// this simple approach may visit bins multiple times, we keep an array that
// marks whether the bin has been visited previously and skip it if we have.
template <typename TIds>
int BucketList<TIds>::IntersectWithLine(double a0[3], double a1[3], double tol, double& t,
  double lineX[3], double ptX[3], vtkIdType& ptId)
{
  double* bounds = this->Bounds;
  int* ndivs = this->Divisions;
  vtkIdType prod = ndivs[0] * ndivs[1];
  double* h = this->H;
  TIds ii, numPtsInBin;
  double x[3], xl[3], rayDir[3], xmin[3], xmax[3];
  vtkMath::Subtract(a1, a0, rayDir);
  double curPos[3], curT, tHit, tMin = VTK_FLOAT_MAX;
  int i, j, k, enterExitCount;
  int ijk[3], ijkMin[3], ijkMax[3];
  vtkIdType idx, pId, bestPtId = (-1);
  double step[3], next[3], tMax[3], tDelta[3];
  double tol2 = tol * tol;

  // Make sure the bounding box of the locator is hit
  if (vtkBox::IntersectBox(bounds, a0, rayDir, curPos, curT))
  {
    // Initialize intersection query array if necessary. This is done
    // locally to ensure thread safety.
    std::vector<unsigned char> bucketHasBeenVisited(this->NumBuckets, 0);

    // Get the i-j-k point of intersection and bin index. This is
    // clamped to the boundary of the locator.
    this->GetBucketIndices(curPos, ijk);

    // Set up some parameters for traversing through bins
    step[0] = (rayDir[0] >= 0.0) ? 1.0 : -1.0;
    step[1] = (rayDir[1] >= 0.0) ? 1.0 : -1.0;
    step[2] = (rayDir[2] >= 0.0) ? 1.0 : -1.0;

    // If the ray is going in the negative direction, then the next voxel boundary
    // is on the "-" direction so we stay in the current voxel.
    next[0] = bounds[0] + h[0] * (rayDir[0] >= 0.0 ? (ijk[0] + step[0]) : ijk[0]);
    next[1] = bounds[2] + h[1] * (rayDir[1] >= 0.0 ? (ijk[1] + step[1]) : ijk[1]);
    next[2] = bounds[4] + h[2] * (rayDir[2] >= 0.0 ? (ijk[2] + step[2]) : ijk[2]);

    tMax[0] = (rayDir[0] != 0.0) ? (next[0] - curPos[0]) / rayDir[0] : VTK_FLOAT_MAX;
    tMax[1] = (rayDir[1] != 0.0) ? (next[1] - curPos[1]) / rayDir[1] : VTK_FLOAT_MAX;
    tMax[2] = (rayDir[2] != 0.0) ? (next[2] - curPos[2]) / rayDir[2] : VTK_FLOAT_MAX;

    tDelta[0] = (rayDir[0] != 0.0) ? (h[0] / rayDir[0]) * step[0] : VTK_FLOAT_MAX;
    tDelta[1] = (rayDir[1] != 0.0) ? (h[1] / rayDir[1]) * step[1] : VTK_FLOAT_MAX;
    tDelta[2] = (rayDir[2] != 0.0) ? (h[2] / rayDir[2]) * step[2] : VTK_FLOAT_MAX;

    // Process current position including the bins in the sphere
    // footprint. Note there is a rare pathological case where the footprint
    // on voxel exit must also be considered.
    for (bestPtId = (-1), enterExitCount = 0; bestPtId < 0 || enterExitCount < 2;)
    {
      // Get the "footprint" of bins containing the sphere defined by the
      // current position and a radius of tol.
      xmin[0] = curPos[0] - tol;
      xmin[1] = curPos[1] - tol;
      xmin[2] = curPos[2] - tol;
      xmax[0] = curPos[0] + tol;
      xmax[1] = curPos[1] + tol;
      xmax[2] = curPos[2] + tol;
      this->GetBucketIndices(xmin, ijkMin);
      this->GetBucketIndices(xmax, ijkMax);

      // Start walking through the bins, find the best point of
      // intersection. Note that the ray may not penetrate all of the way
      // through the locator so may terminate when (t > 1.0).
      for (k = ijkMin[2]; k <= ijkMax[2]; ++k)
      {
        for (j = ijkMin[1]; j <= ijkMax[1]; ++j)
        {
          for (i = ijkMin[0]; i <= ijkMax[0]; ++i)
          {
            // Current bin index
            idx = i + j * ndivs[0] + k * prod;

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
                  if (vtkLine::DistanceToLine(x, a0, a1, tHit, xl) <= tol2 && tHit < tMin)
                  {
                    tMin = tHit;
                    bestPtId = pId;
                  } // point is within tolerance and closer
                } // over all points in bin
              } // if points in bin
            } // bucket not visited
          } // i bins
        } // j bins
      } // k bins

      // Make sure to evaluate exit footprint as well. Must evaluate entrance
      // and exit of current voxel.
      if (bestPtId >= 0)
      {
        enterExitCount++;
      }

      // Advance to next voxel / bin
      if (tMax[0] < tMax[1])
      {
        if (tMax[0] < tMax[2])
        {
          ijk[0] += static_cast<int>(step[0]);
          tMax[0] += tDelta[0];
          curT = tMax[0];
        }
        else
        {
          ijk[2] += static_cast<int>(step[2]);
          tMax[2] += tDelta[2];
          curT = tMax[2];
        }
      }
      else
      {
        if (tMax[1] < tMax[2])
        {
          ijk[1] += static_cast<int>(step[1]);
          tMax[1] += tDelta[1];
          curT = tMax[1];
        }
        else
        {
          ijk[2] += static_cast<int>(step[2]);
          tMax[2] += tDelta[2];
          curT = tMax[2];
        }
      }

      // Check exit conditions
      if (curT > 1.0 || ijk[0] < 0 || ijk[0] >= ndivs[0] || ijk[1] < 0 || ijk[1] >= ndivs[1] ||
        ijk[2] < 0 || ijk[2] >= ndivs[2])
      {
        break;
      }
      else
      {
        curPos[0] = a0[0] + curT * rayDir[0];
        curPos[1] = a0[1] + curT * rayDir[1];
        curPos[2] = a0[2] + curT * rayDir[2];
      }

    } // for looking for valid intersected point
  } // if (vtkBox::IntersectBox(...))

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
// Merge points based on tolerance. Return a point map. The map (which is
// provided by the user of length numPts where numPts is the number of points
// that the locator was built with) simply indicates, for a particular point
// id, what point it was merged to. There are two separate paths: when the
// tolerance is precisely 0.0, and when tol > 0.0. Both are executed in
// parallel, although the second uses a checkerboard approach to avoid write
// collisions.  The ordering mode applies when the tolerance!=0, and controls
// how the points are processed. BIN_ORDERING is threaded and
// faster.
template <typename TIds>
void BucketList<TIds>::MergePoints(double tol, vtkIdType* mergeMap, int orderingMode)
{
  // First mark all points as uninitialized
  std::fill_n(mergeMap, this->NumPts, (-1));

  // If tol=0, then just process points bucket by bucket. Don't have to worry
  // about points in other buckets.
  if (tol <= 0.0)
  {
    MergePrecise<TIds> merge(this, mergeMap);
    vtkSMPTools::For(0, this->NumBuckets, merge);
    return;
  }

  // Merge within a tolerance. Different algorithms are used
  // depending on how points are merged / ordering mode. BTW, TBB is
  // much faster than std::thread due to the work stealing / load
  // balancing features of TBB.
  if (orderingMode == vtkStaticPointLocator::POINT_ORDER)
  {
    MergePointOrder<TIds> merge(this, tol, mergeMap);
    merge(this->NumPts); // this is sequential to avoid race conditions
  }
  else // orderingMode == vtkStaticPointLocator::BIN_ORDER
  {
    MergeBinOrder<TIds> merge(this, tol, mergeMap);
    merge.Execute(); // this is checkerboard threaded
  }
}

//------------------------------------------------------------------------------
// Merge points with precisely equal position and data values.
template <typename TIds>
void BucketList<TIds>::MergePointsWithData(vtkDataArray* data, vtkIdType* mergeMap)
{
  // First mark all points as uninitialized
  std::fill_n(mergeMap, this->NumPts, (-1));

  MergePointsAndData<TIds> merge(this, data, mergeMap);
  vtkSMPTools::For(0, this->NumBuckets, merge);
}

//------------------------------------------------------------------------------
// Internal method to find those buckets that are within distance specified
// only those buckets outside of level radiuses of ijk are returned
template <typename TIds>
void BucketList<TIds>::GetOverlappingBuckets(
  NeighborBuckets* buckets, const double x[3], const int ijk[3], double dist, int level)
{
  int i, j, k, nei[3], minLevel[3], maxLevel[3];
  double xMin[3], xMax[3];

  // Initialize
  buckets->Reset();

  // Determine the range of indices in each direction
  xMin[0] = x[0] - dist;
  xMin[1] = x[1] - dist;
  xMin[2] = x[2] - dist;
  xMax[0] = x[0] + dist;
  xMax[1] = x[1] + dist;
  xMax[2] = x[2] + dist;

  this->GetBucketIndices(xMin, minLevel);
  this->GetBucketIndices(xMax, maxLevel);

  for (i = minLevel[0]; i <= maxLevel[0]; i++)
  {
    for (j = minLevel[1]; j <= maxLevel[1]; j++)
    {
      for (k = minLevel[2]; k <= maxLevel[2]; k++)
      {
        if (i < (ijk[0] - level) || i > (ijk[0] + level) || j < (ijk[1] - level) ||
          j > (ijk[1] + level) || k < (ijk[2] - level) || k > (ijk[2] + level))
        {
          nei[0] = i;
          nei[1] = j;
          nei[2] = k;
          buckets->InsertNextBucket(nei);
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
// Internal method to find those buckets that are within distance specified
// only those buckets outside of level radiuses of ijk are returned
template <typename TIds>
void BucketList<TIds>::GetOverlappingBuckets(NeighborBuckets* buckets, const double x[3],
  double dist, int prevMinLevel[3], int prevMaxLevel[3])
{
  int i, j, k, nei[3], minLevel[3], maxLevel[3];
  int kFactor, jFactor;
  int jkSkipFlag, kSkipFlag;
  double xMin[3], xMax[3];

  // Initialize
  buckets->Reset();

  // Determine the range of indices in each direction
  xMin[0] = x[0] - dist;
  xMin[1] = x[1] - dist;
  xMin[2] = x[2] - dist;
  xMax[0] = x[0] + dist;
  xMax[1] = x[1] + dist;
  xMax[2] = x[2] + dist;

  this->GetBucketIndices(xMin, minLevel);
  this->GetBucketIndices(xMax, maxLevel);

  if (minLevel[0] == prevMinLevel[0] && maxLevel[0] == prevMaxLevel[0] &&
    minLevel[1] == prevMinLevel[1] && maxLevel[1] == prevMaxLevel[1] &&
    minLevel[2] == prevMinLevel[2] && maxLevel[2] == prevMaxLevel[2])
  {
    return;
  }

  for (k = minLevel[2]; k <= maxLevel[2]; k++)
  {
    kFactor = k * this->xyD;
    if (k >= prevMinLevel[2] && k <= prevMaxLevel[2])
    {
      kSkipFlag = 1;
    }
    else
    {
      kSkipFlag = 0;
    }
    for (j = minLevel[1]; j <= maxLevel[1]; j++)
    {
      if (kSkipFlag && j >= prevMinLevel[1] && j <= prevMaxLevel[1])
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
        if (this->GetNumberOfIds(i + jFactor + kFactor) > 0)
        {
          nei[0] = i;
          nei[1] = j;
          nei[2] = k;
          buckets->InsertNextBucket(nei);
        }
      }
    }
  }

  prevMinLevel[0] = minLevel[0];
  prevMinLevel[1] = minLevel[1];
  prevMinLevel[2] = minLevel[2];
  prevMaxLevel[0] = maxLevel[0];
  prevMaxLevel[1] = maxLevel[1];
  prevMaxLevel[2] = maxLevel[2];
}

//------------------------------------------------------------------------------
// Build polygonal representation of locator. Create faces that separate
// inside/outside buckets, or separate inside/boundary of locator.
template <typename TIds>
void BucketList<TIds>::GenerateRepresentation(int vtkNotUsed(level), vtkPolyData* pd)
{
  vtkNew<vtkPoints> pts;
  pts->Reserve(5000);
  vtkNew<vtkCellArray> polys;
  polys->AllocateEstimate(2048, 3);
  int ii, i, j, k, idx, offset[3], minusOffset[3], inside, sliceSize;

  // loop over all buckets, creating appropriate faces
  sliceSize = this->Divisions[0] * this->Divisions[1];
  for (k = 0; k < this->Divisions[2]; k++)
  {
    offset[2] = k * sliceSize;
    minusOffset[2] = (k - 1) * sliceSize;
    for (j = 0; j < this->Divisions[1]; j++)
    {
      offset[1] = j * this->Divisions[0];
      minusOffset[1] = (j - 1) * this->Divisions[0];
      for (i = 0; i < this->Divisions[0]; i++)
      {
        offset[0] = i;
        minusOffset[0] = i - 1;
        idx = offset[0] + offset[1] + offset[2];
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
              this->GenerateFace(ii, i, j, k, pts, polys);
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
              this->GenerateFace(ii, i, j, k, pts, polys);
            }
          }
          // those buckets on "positive" boundaries can generate faces specially
          if ((i + 1) >= this->Divisions[0] && inside)
          {
            this->GenerateFace(0, i + 1, j, k, pts, polys);
          }
          if ((j + 1) >= this->Divisions[1] && inside)
          {
            this->GenerateFace(1, i, j + 1, k, pts, polys);
          }
          if ((k + 1) >= this->Divisions[2] && inside)
          {
            this->GenerateFace(2, i, j, k + 1, pts, polys);
          }

        } // over negative faces
      } // over i divisions
    } // over j divisions
  } // over k divisions

  pd->SetPoints(pts);
  pd->SetPolys(polys);
  pd->Squeeze();
}

//==============================================================================
// Here is the VTK class proper. It's implemented with the templated
// BucketList class. This declares internal classes. Definitions follow in
// this .cxx file.

//------------------------------------------------------------------------------
// Construct with automatic computation of divisions, averaging
// 1 points per bucket.
vtkStaticPointLocator::vtkStaticPointLocator()
{
  this->NumberOfPointsPerBucket = 1;
  this->Divisions[0] = this->Divisions[1] = this->Divisions[2] = 50;
  this->H[0] = this->H[1] = this->H[2] = 0.0;
  this->Buckets = nullptr;
  this->MaxNumberOfBuckets = VTK_INT_MAX;
  this->LargeIds = false;
  this->TraversalOrder = BIN_ORDER;
  this->Padding = 0.0;
  this->Static = false;
}

//------------------------------------------------------------------------------
vtkStaticPointLocator::~vtkStaticPointLocator()
{
  this->FreeSearchStructure();
}

//------------------------------------------------------------------------------
void vtkStaticPointLocator::Initialize()
{
  this->FreeSearchStructure();
}

//------------------------------------------------------------------------------
void vtkStaticPointLocator::FreeSearchStructure()
{
  if (this->Buckets)
  {
    delete this->Buckets;
    this->Buckets = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkStaticPointLocator::BuildLocator()
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
void vtkStaticPointLocator::ForceBuildLocator()
{
  this->BuildLocatorInternal();
}

//------------------------------------------------------------------------------
//  Method to form subdivision of space based on the points provided and
//  subject to the constraints of levels and NumberOfPointsPerBucket.
//  The result is directly addressable and of uniform subdivision.
//
void vtkStaticPointLocator::BuildLocatorInternal()
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
  this->FreeSearchStructure();

  // Size the root bucket.  Initialize bucket data structure, compute
  // level and divisions. The GetBounds() method below can be very slow;
  // hopefully it is cached or otherwise accelerated.
  //
  const double* bounds = this->DataSet->GetBounds();
  vtkIdType numBuckets = static_cast<vtkIdType>(
    static_cast<double>(numPts) / static_cast<double>(this->NumberOfPointsPerBucket));
  numBuckets = (numBuckets > this->MaxNumberOfBuckets ? this->MaxNumberOfBuckets : numBuckets);

  vtkBoundingBox bbox(bounds);
  // If bounds padding is specified, inflate it
  if (this->Padding > 0.)
  {
    bbox.Inflate(this->Padding);
  }

  if (this->Automatic)
  {
    bbox.ComputeDivisions(numBuckets, this->Bounds, ndivs);
  }
  else
  {
    bbox.Inflate(); // make sure non-zero volume
    bbox.GetBounds(this->Bounds);
    for (i = 0; i < 3; i++)
    {
      ndivs[i] = (this->Divisions[i] < 1 ? 1 : this->Divisions[i]);
    }
  }

  this->Divisions[0] = ndivs[0];
  this->Divisions[1] = ndivs[1];
  this->Divisions[2] = ndivs[2];

  this->NumberOfBuckets = numBuckets = static_cast<vtkIdType>(ndivs[0]) *
    static_cast<vtkIdType>(ndivs[1]) * static_cast<vtkIdType>(ndivs[2]);

  //  Compute width of bucket in three directions
  for (i = 0; i < 3; i++)
  {
    this->H[i] = (this->Bounds[2 * i + 1] - this->Bounds[2 * i]) / static_cast<double>(ndivs[i]);
  }

  // Instantiate the locator. The type is related to the maximum point id.
  // This is done for performance (e.g., the sort is faster) and significant
  // memory savings.
  if (numPts >= VTK_INT_MAX || numBuckets >= VTK_INT_MAX)
  {
    this->LargeIds = true;
    this->Buckets = new BucketList<vtkIdType>(this, numPts, numBuckets);
  }
  else
  {
    this->LargeIds = false;
    this->Buckets = new BucketList<int>(this, numPts, numBuckets);
  }

  // Actually construct the locator
  this->Buckets->BuildLocator();

  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
//  Method to form subdivision of space based on the points provided and
//  subject to the constraints of levels and NumberOfPointsPerBucket.
//  The result is directly addressable and of uniform subdivision.

void vtkStaticPointLocator::BuildLocator(const double* inBounds)
{
  int ndivs[3];
  int i;
  vtkIdType numPts;

  // don't rebuild if build time is newer than modified and dataset modified time
  if (this->Buckets &&
    (this->UseExistingSearchStructure ||
      (this->BuildTime > this->MTime && this->BuildTime > this->DataSet->GetMTime())))
  {
    return;
  }

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
  const double* bounds = (inBounds == nullptr ? this->DataSet->GetBounds() : inBounds);
  vtkIdType numBuckets = static_cast<vtkIdType>(
    static_cast<double>(numPts) / static_cast<double>(this->NumberOfPointsPerBucket));
  numBuckets = (numBuckets > this->MaxNumberOfBuckets ? this->MaxNumberOfBuckets : numBuckets);

  vtkBoundingBox bbox(bounds);
  // If bounds padding is specified, inflate it
  if (this->Padding > 0.)
  {
    bbox.Inflate(this->Padding);
  }

  if (this->Automatic)
  {
    bbox.ComputeDivisions(numBuckets, this->Bounds, ndivs);
  }
  else
  {
    bbox.Inflate(); // make sure non-zero volume
    bbox.GetBounds(this->Bounds);
    for (i = 0; i < 3; i++)
    {
      ndivs[i] = (this->Divisions[i] < 1 ? 1 : this->Divisions[i]);
    }
  }

  this->Divisions[0] = ndivs[0];
  this->Divisions[1] = ndivs[1];
  this->Divisions[2] = ndivs[2];
  this->NumberOfBuckets = numBuckets = static_cast<vtkIdType>(ndivs[0]) *
    static_cast<vtkIdType>(ndivs[1]) * static_cast<vtkIdType>(ndivs[2]);

  //  Compute width of bucket in three directions
  //
  for (i = 0; i < 3; i++)
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
    this->Buckets = new BucketList<vtkIdType>(this, numPts, numBuckets);
  }
  else
  {
    this->LargeIds = false;
    this->Buckets = new BucketList<int>(this, numPts, numBuckets);
  }

  // Actually construct the locator
  this->Buckets->BuildLocator();

  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
// These methods satisfy the vtkStaticPointLocator API. The implementation is
// with the templated BucketList class. Note that a lot of the complexity here
// is due to the desire to use different id types (int versus vtkIdType) for the
// purposes of increasing speed and reducing memory.
//
// You're probably wondering why an if check (on LargeIds) is used to
// static_cast on BukcetList<T> type, when virtual inheritance could be
// used. Benchmarking shows a small speed difference due to inlining, which
// the use of virtual methods short circuits.

//------------------------------------------------------------------------------
// Given a position x, return the id of the point closest to it.
vtkIdType vtkStaticPointLocator::FindClosestPoint(const double x[3])
{
  this->BuildLocator(); // will subdivide if modified; otherwise returns
  if (!this->Buckets)
  {
    return -1;
  }

  if (this->LargeIds)
  {
    return static_cast<BucketList<vtkIdType>*>(this->Buckets)->FindClosestPoint(x);
  }
  else
  {
    return static_cast<BucketList<int>*>(this->Buckets)->FindClosestPoint(x);
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkStaticPointLocator::FindClosestPointWithinRadius(
  double radius, const double x[3], double inputDataLength, double& dist2)
{
  this->BuildLocator(); // will subdivide if modified; otherwise returns
  if (!this->Buckets)
  {
    return -1;
  }

  if (this->LargeIds)
  {
    return static_cast<BucketList<vtkIdType>*>(this->Buckets)
      ->FindClosestPointWithinRadius(radius, x, inputDataLength, dist2);
  }
  else
  {
    return static_cast<BucketList<int>*>(this->Buckets)
      ->FindClosestPointWithinRadius(radius, x, inputDataLength, dist2);
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkStaticPointLocator::FindClosestPointWithinRadius(
  double radius, const double x[3], double& dist2)
{
  return this->FindClosestPointWithinRadius(radius, x, this->DataSet->GetLength(), dist2);
}

//------------------------------------------------------------------------------
void vtkStaticPointLocator::FindClosestNPoints(int N, const double x[3], vtkIdList* result)
{
  this->BuildLocator(); // will subdivide if modified; otherwise returns
  if (!this->Buckets)
  {
    return;
  }

  if (this->LargeIds)
  {
    static_cast<BucketList<vtkIdType>*>(this->Buckets)->FindClosestNPoints(N, x, result);
  }
  else
  {
    static_cast<BucketList<int>*>(this->Buckets)->FindClosestNPoints(N, x, result);
  }
}

//------------------------------------------------------------------------------
double vtkStaticPointLocator::FindNPointsInShell(int N, const double x[3],
  vtkDist2TupleArray& results, double minDist2, bool sort, vtkDoubleArray* petals)
{
  this->BuildLocator(); // will subdivide if modified; otherwise returns
  if (!this->Buckets)
  {
    return 0.0;
  }

  if (this->LargeIds)
  {
    return static_cast<BucketList<vtkIdType>*>(this->Buckets)
      ->FindNPointsInShell(N, x, results, minDist2, sort, petals);
  }
  else
  {
    return static_cast<BucketList<int>*>(this->Buckets)
      ->FindNPointsInShell(N, x, results, minDist2, sort, petals);
  }
}

//------------------------------------------------------------------------------
void vtkStaticPointLocator::FindPointsInShell(const double x[3], vtkDist2TupleArray& results,
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
    static_cast<BucketList<vtkIdType>*>(this->Buckets)
      ->FindPointsInShell(x, results, minDist2, maxDist2, sort, petals);
  }
  else
  {
    static_cast<BucketList<int>*>(this->Buckets)
      ->FindPointsInShell(x, results, minDist2, maxDist2, sort, petals);
  }
}

//------------------------------------------------------------------------------
void vtkStaticPointLocator::FindPointsWithinRadius(double R, const double x[3], vtkIdList* result)
{
  this->BuildLocator(); // will subdivide if modified; otherwise returns
  if (!this->Buckets)
  {
    return;
  }

  if (this->LargeIds)
  {
    static_cast<BucketList<vtkIdType>*>(this->Buckets)->FindPointsWithinRadius(R, x, result);
  }
  else
  {
    static_cast<BucketList<int>*>(this->Buckets)->FindPointsWithinRadius(R, x, result);
  }
}

//------------------------------------------------------------------------------
// This method traverses the locator along the defined ray, finding the
// closest point to a0 when projected onto the line (a0,a1) (i.e., min
// parametric coordinate t) and within the tolerance tol (measured in the
// world coordinate system).
int vtkStaticPointLocator::IntersectWithLine(double a0[3], double a1[3], double tol, double& t,
  double lineX[3], double ptX[3], vtkIdType& ptId)
{
  this->BuildLocator(); // will subdivide if modified; otherwise returns
  if (!this->Buckets)
  {
    return 0;
  }

  if (this->LargeIds)
  {
    return static_cast<BucketList<vtkIdType>*>(this->Buckets)
      ->IntersectWithLine(a0, a1, tol, t, lineX, ptX, ptId);
  }
  else
  {
    return static_cast<BucketList<int>*>(this->Buckets)
      ->IntersectWithLine(a0, a1, tol, t, lineX, ptX, ptId);
  }
}

//------------------------------------------------------------------------------
// Build a representation for the locator.
void vtkStaticPointLocator::GenerateRepresentation(int level, vtkPolyData* pd)
{
  this->BuildLocator(); // will subdivide if modified; otherwise returns
  if (!this->Buckets)
  {
    return;
  }

  if (this->LargeIds)
  {
    static_cast<BucketList<vtkIdType>*>(this->Buckets)->GenerateRepresentation(level, pd);
  }
  else
  {
    static_cast<BucketList<int>*>(this->Buckets)->GenerateRepresentation(level, pd);
  }
}

//------------------------------------------------------------------------------
// Given a bucket, return the number of points inside of it.
vtkIdType vtkStaticPointLocator::GetNumberOfPointsInBucket(vtkIdType bNum)
{
  this->BuildLocator(); // will subdivide if modified; otherwise returns
  if (!this->Buckets)
  {
    return 0;
  }

  if (this->LargeIds)
  {
    return static_cast<BucketList<vtkIdType>*>(this->Buckets)->GetNumberOfIds(bNum);
  }
  else
  {
    return static_cast<BucketList<int>*>(this->Buckets)->GetNumberOfIds(bNum);
  }
}

//------------------------------------------------------------------------------
// Given a bucket, return the ids in the bucket.
void vtkStaticPointLocator::GetBucketIds(vtkIdType bNum, vtkIdList* bList)
{
  this->BuildLocator(); // will subdivide if modified; otherwise returns
  if (!this->Buckets)
  {
    bList->Reset();
    return;
  }

  if (this->LargeIds)
  {
    static_cast<BucketList<vtkIdType>*>(this->Buckets)->GetIds(bNum, bList);
  }
  else
  {
    static_cast<BucketList<int>*>(this->Buckets)->GetIds(bNum, bList);
  }
}

//------------------------------------------------------------------------------
// Return the center a specified bucket/bin.
void vtkStaticPointLocator::GetBucketCenter(int i, int j, int k, double center[3])
{
  if (this->LargeIds)
  {
    static_cast<BucketList<vtkIdType>*>(this->Buckets)->GetBucketCenter(i, j, k, center);
  }
  else
  {
    static_cast<BucketList<int>*>(this->Buckets)->GetBucketCenter(i, j, k, center);
  }
}

//------------------------------------------------------------------------------
// Merge the points in the locator, return a merge map.
void vtkStaticPointLocator::MergePoints(double tol, vtkIdType* pointMap)
{
  this->BuildLocator(); // will subdivide if modified; otherwise returns
  if (!this->Buckets)
  {
    return;
  }

  if (this->LargeIds)
  {
    static_cast<BucketList<vtkIdType>*>(this->Buckets)
      ->MergePoints(tol, pointMap, this->TraversalOrder);
  }
  else
  {
    static_cast<BucketList<int>*>(this->Buckets)->MergePoints(tol, pointMap, this->TraversalOrder);
  }
}

//------------------------------------------------------------------------------
// Merge the points and data in the locator, return a merge map.
void vtkStaticPointLocator::MergePointsWithData(vtkDataArray* data, vtkIdType* pointMap)
{
  this->BuildLocator(); // will subdivide if modified; otherwise returns
  if (!this->Buckets)
  {
    return;
  }

  if (this->LargeIds)
  {
    static_cast<BucketList<vtkIdType>*>(this->Buckets)->MergePointsWithData(data, pointMap);
  }
  else
  {
    static_cast<BucketList<int>*>(this->Buckets)->MergePointsWithData(data, pointMap);
  }
}

//------------------------------------------------------------------------------
void vtkStaticPointLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number of Points Per Bucket: " << this->NumberOfPointsPerBucket << "\n";

  os << indent << "Divisions: (" << this->Divisions[0] << ", " << this->Divisions[1] << ", "
     << this->Divisions[2] << ")\n";

  os << indent << "Max Number Of Buckets: " << this->MaxNumberOfBuckets << "\n";

  os << indent << "Large IDs: " << this->LargeIds << "\n";

  os << indent << "Traversal Order: " << (this->TraversalOrder ? "On\n" : "Off\n");

  os << indent << "Padding: " << this->Padding << "\n";

  os << indent << "Static: " << (this->Static ? "On\n" : "Off\n");
}
VTK_ABI_NAMESPACE_END
