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
          if ((dist2 = vtkMath::Distance2BetweenPoints(x, pt)) < minDist2)
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
      }   // if points in bucket
    }     // for each overlapping bucket
  }       // if not identical point

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
        }   // if ids
      }     // if bucket is within the current best distance
    }       // for each overlapping bucket

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

namespace // anonymous
{
//------------------------------------------------------------------------------
// These classes and methods are in support of FindNPointsInShell().

// Iterate over the boundary shell of a footprint of bins. Takes into account
// both the inner and outer radius, i.e. culling bins that are not within the
// request shell. It also uses the optional (Voronoi flower) spheres to further
// cull the iteration process.
template <typename TIds>
struct ShellIterator
{
  vtkDataSet* DataSet; // access dataset points
  double* FastPoints;  // fast path for points access
  BucketList<TIds>* Bins;
  int Divs[3];          // Locator divisions
  vtkIdType Slice;      // k-plane offsets
  double R;             // bin circumradius
  int Level;            // The level of iterator expansion, level==0 is center
  double X[3];          // The center of the iterator in physical space
  int Center[3];        // The center of the iterator in index space
  vtkIdType NumSpheres; // The number of inclusive spheres
  double* Spheres;      // The spheres, 4 tuples with (x,y,z,r2)

  // Keep track of iteration
  int I, J, K;
  int Min[3], Max[3];

  // Use to enable / disable bin culling
  int LEVEL_QUERY_THRESHOLD = 3;

  ShellIterator(vtkDataSet* ds, BucketList<TIds>* bins, int divs[3], double binCircumRadius,
    vtkDoubleArray* spheres, const double x[3], int center[3])
    : DataSet(ds)
    , FastPoints(bins->FastPoints)
    , Bins(bins)
    , Divs{ divs[0], divs[1], divs[2] }
    , R(binCircumRadius)
    , X{ x[0], x[1], x[2] }
    , Center{ center[0], center[1], center[2] }
    , NumSpheres(0)
    , Spheres(nullptr)
  {
    this->Slice = this->Divs[0] * this->Divs[1];
    if (spheres)
    {
      this->NumSpheres = spheres->GetNumberOfTuples();
      this->Spheres = spheres->GetPointer(0);
    }
  }

  // Fast path for double points.
  double* GetPoint(vtkIdType ptId) { return (this->FastPoints + 3 * ptId); }

  // Initialize iterator and return starting bin idx (also the starting bin
  // (i,j,k). Return the starting level of iteration (i.e., non-zero minR2
  // means that some inner bins may be skipped, and the level>0).
  vtkIdType Initialize(int level, int& i, int& j, int& k)
  {
    // Prepare for traversal at level >= 0
    this->Level = level;

    // Set the extents
    for (int ii = 0; ii < 3; ++ii)
    {
      this->Min[ii] = this->Center[ii] - level;
      this->Max[ii] = this->Center[ii] + level;
      this->Min[ii] = this->Min[ii] < 0 ? 0 : this->Min[ii];
      this->Max[ii] = this->Max[ii] >= this->Divs[ii] ? this->Divs[ii] - 1 : this->Max[ii];
    }

    // Initial iteration position
    this->I = this->Min[0];
    this->J = this->Min[1];
    this->K = this->Min[2];

    // Fast path for the common case level==0
    if (level <= 0)
    {
      i = this->I;
      j = this->J;
      k = this->K;
      return (this->I + (this->J * this->Divs[0]) + (this->K * this->Slice));
    }

    // Begin iteration until first bin on the shell is discovered.
    for (this->K = this->Min[2]; this->K <= this->Max[2]; ++this->K)
    {
      const vtkIdType kOffset = this->K * this->Slice;
      const int kmCenter = std::abs(this->K - this->Center[2]);
      for (this->J = this->Min[1]; this->J <= this->Max[1]; ++this->J)
      {
        const vtkIdType jOffset = this->J * this->Divs[0];
        const int jmCenter = std::abs(this->J - this->Center[1]);
        for (this->I = this->Min[0]; this->I <= this->Max[0]; ++this->I)
        {
          const int imCenter = std::abs(this->I - this->Center[0]);
          // We are iterating over the shell at current level. The shell
          // surface requires one of I,J,K to have a value == level.
          if (imCenter == this->Level || jmCenter == this->Level || kmCenter == this->Level)
          {
            i = this->I;
            j = this->J;
            k = this->K;
            return (this->I + jOffset + kOffset);
          }
        } // over I
      }   // over J
    }     // over K

    i = j = k = (-1);
    return -1;
  }

  // Return the next bin in the iteration sequence over the shell at the
  // current level. Also returns the (i,j,k) of the bin.
  vtkIdType NextBin(int& i, int& j, int& k)
  {
    // There is no next bin at level 0
    if (this->Level <= 0)
    {
      i = j = k = (-1);
      return (-1);
    }

    // Begin iteration until a bin on the shell is discovered. Note that
    // I,J,K should have been previously set. However, we need to move to the
    // next possible bin, meaning incrementing I,J,K.
    while (this->K <= this->Max[2])
    {
      // Forward increment
      this->I++;
      if (this->I > this->Max[0])
      {
        this->I = this->Min[0];
        this->J++;
        if (this->J > this->Max[1])
        {
          this->J = this->Min[1];
          this->K++;
        }
      }
      // Check if on shell boundary
      if (this->K <= this->Max[2] &&
        (this->I == (this->Center[0] + this->Level) || this->I == (this->Center[0] - this->Level) ||
          this->J == (this->Center[1] + this->Level) ||
          this->J == (this->Center[1] - this->Level) ||
          this->K == (this->Center[2] + this->Level) || this->K == (this->Center[2] - this->Level)))
      {
        i = this->I;
        j = this->J;
        k = this->K;
        return (this->I + (this->J * this->Divs[0]) + (this->K * this->Slice));
      }
    }

    // Completed traversal
    i = j = k = (-1);
    return (-1);
  }

  // Return true if the bin can be culled: if the bin specified by
  // (i,j,k) is completely outside of the shell request; and completely
  // outside any of the optional sphere petals, then the bin can be
  // eliminated from further processing. Otherwise, false is returned.
  bool CanCullBin(bool gathering, double minR2, double maxR2, int i, int j, int k, int level)
  {
    // Bin culling is generally not worth it for smaller levels.
    if (level < this->LEVEL_QUERY_THRESHOLD || gathering)
    {
      return false;
    }

    // Obtain the bucket bounding box
    double min[3], max[3];
    this->Bins->GetBucketBounds(i, j, k, min, max);

    // Cull the bin if fully outside the (minR2,maxR2] footprint.
    // Greater than the shell request outer radius, and maxR2 determined.
    if (!vtkBoundingBox::IntersectsSphere(min, max, this->X, maxR2))
    {
      return true;
    }

    // Strictly less than the shell request inner radius, minR2 is always known.
    if (vtkBoundingBox::InsideSphere(min, max, this->X, minR2))
    {
      return true;
    }

    // At this point, the bin overlaps the shell request. Cull the bin if not
    // in any of the provided Voronoi hull spheres (petals).
    if (this->NumSpheres > 0)
    {
      const double* sphere = this->Spheres;
      for (int sNum = 0; sNum < this->NumSpheres; ++sNum, sphere += 4)
      {
        if (vtkBoundingBox::IntersectsSphere(min, max, sphere, sphere[3]))
        {
          return false;
        }
      }
      return true; // not in any Voronoi flower petal
    }

    // The bin cannot be culled.
    return false;
  }

  // Gather nearby points in the bin binIdx. Initially, we gather N points
  // in order to define the sphere S with center x and radius**2 maxR2.
  // Then, after N points are defined, switch the gathering mode to all points
  // in S (inclusive).  Points are placed into the res results vector, maxR2
  // is updated and returned.
  double GatherPoints(int i, int j, int k, vtkIdType binIdx, int level, int N, double minR2,
    double maxR2, vtkDist2TupleArray& res)
  {
    bool gathering = true;

    vtkIdType numIds = this->Bins->GetNumberOfIds(binIdx);
    if (numIds <= 0 || this->CanCullBin(gathering, minR2, maxR2, i, j, k, level))
    {
      return maxR2;
    }

    const vtkLocatorTuple<TIds>* ids = this->Bins->GetIds(binIdx);
    double* pt;
    for (vtkIdType ii = 0; ii < numIds; ++ii)
    {
      vtkIdType ptId = ids[ii].PtId;
      pt = this->GetPoint(ptId);
      double d2 = vtkMath::Distance2BetweenPoints(this->X, pt);
      if (d2 > minR2) // not culled by minimum shell radius
      {
        // If not yet gathered N points, maxR2 may still be increasing
        if (static_cast<int>(res.size()) < N)
        {
          res.emplace_back(ptId, d2);
          maxR2 = (d2 > maxR2 ? d2 : maxR2);
        }
        // maxR2 is determined, so gather points in sphere
        else if (d2 <= maxR2)
        {
          gathering = false;
          res.emplace_back(ptId, d2);
        }
      } // if potential candidate
    }   // for all points in this bin

    return maxR2;
  }

  // Add points in the bin binIdx. Like GatherPoints(), except at this point
  // maxR2 has been determined. Also will cull entire bins if they are not
  // within the (minR2,maxR2] query footprint.
  void AddPoints(int i, int j, int k, vtkIdType binIdx, int level, double minR2, double maxR2,
    vtkDist2TupleArray& res)
  {
    bool gathering = false;

    // If there is nothing in the bin, or the bin is outside of the search space,
    // skip processing the bin.
    vtkIdType numIds = this->Bins->GetNumberOfIds(binIdx);
    if (numIds <= 0 || this->CanCullBin(gathering, minR2, maxR2, i, j, k, level))
    {
      return;
    }

    // Okay, process the points in the bin.
    const vtkLocatorTuple<TIds>* ids = this->Bins->GetIds(binIdx);
    double* pt;
    for (vtkIdType ii = 0; ii < numIds; ++ii)
    {
      vtkIdType ptId = ids[ii].PtId;
      pt = this->GetPoint(ptId);
      double d2 = vtkMath::Distance2BetweenPoints(this->X, pt);
      if (d2 > minR2 && d2 <= maxR2)
      {
        res.emplace_back(ptId, d2);
      } // if within shell footprint
    }   // for all points in this bin
  }
}; // ShellIterator

} // anonymous namespace

//-----------------------------------------------------------------------------
// This algorithm works by grabbing the first N points it finds (using an
// expanding wave across nearby bins so this initial set of points is
// reasonably close to the query point). This operation also determines a
// maximum maxR2 defining the radius of the initial nearby set around the query
// point. Then, resuming the traversal after grabbing this initial initial
// set / N points, all remaining points whose dist2 <= maxR2 are added to
// the results list.  A radial sort operation of the points is performed if
// requested. Optional spheres can be used to that only data within them
// are processed.
template <typename TIds>
double BucketList<TIds>::FindNPointsInShell(int N, const double x[3], vtkDist2TupleArray& results,
  double minR2, bool sort, vtkDoubleArray* spheres)
{
  // Clear out any previous results.
  results.clear();

  // Find the bucket/bin the point is in. This is the center of the request
  // footprint.
  int center[3];
  this->GetBucketIndices(x, center);

  // Traverse and gather points in the bucket/bins contained in the shell
  // request (minR,maxR].
  double minR = sqrt(minR2);
  double maxR = 0, maxR2 = 0;

  // Determine absolute limits of iteration (based on possible number of points).
  vtkIdType numPts = this->DataSet->GetNumberOfPoints();
  N = (numPts < N ? numPts : N);

  // Gather N points if possible and determine maxR2. Make sure all points within maxR2
  // have been found. We use a shell iterator to grow a "rectangular" shell from the center bin.
  // Skip over bins inside the inner radius minR2. The bin index is updated during iteration,
  // a binIdx<0 is returned when the iteration is exhausted.
  vtkIdType binIdx;
  int currentLevel = (minR <= 0 ? 0 : static_cast<int>(std::floor(minR / (2.0 * this->BinRadius))));
  ShellIterator<TIds> siter(
    this->DataSet, this, this->Divisions, this->BinRadius, spheres, x, center);

  // Loop across levels, accruing points as we go. This will determine the maxR2. It also
  // carves out some inner levels (based on current level) that do not have to be revisited.
  while (static_cast<int>(results.size()) < N && currentLevel < this->MaxLevel)
  {
    int i, j, k;
    binIdx = siter.Initialize(currentLevel, i, j, k);
    // Basically iterating over a "rectangular" footprint defined from
    // the current level.
    while (binIdx >= 0)
    {
      maxR2 = siter.GatherPoints(i, j, k, binIdx, currentLevel, N, minR2, maxR2, results);
      binIdx = siter.NextBin(i, j, k);
    }
    ++currentLevel;
  }
  int level = currentLevel - 1; // reset to the last level processed

  // We have determined maxR2 and ~N points in the request annulus
  // (minR2 < p_d2 <= maxR2). Now gather any other remaining points
  // within the request. It's typical that the number of points
  // returned is >N.
  maxR = sqrt(maxR2);

  // Determine the range of indices in each direction based on radius maxR.
  // This block of bins is processed to gather any additional points with
  // radius <=maxR2.
  double xMin[3], xMax[3];
  xMin[0] = x[0] - maxR;
  xMin[1] = x[1] - maxR;
  xMin[2] = x[2] - maxR;
  xMax[0] = x[0] + maxR;
  xMax[1] = x[1] + maxR;
  xMax[2] = x[2] + maxR;

  // Find the rectangular footprint in the locator
  int ijkMin[3], ijkMax[3];
  this->GetBucketIndices(xMin, ijkMin);
  this->GetBucketIndices(xMax, ijkMax);

  // Add points within the footprint (defined by (center,level) and the
  // spherical shell request (minR2,maxR2] possibly cropped by the
  // Voronoi flower petals.  Points within the (center+level)
  // footprint have already been processed, so don't add them again.
  vtkIdType jOffset, kOffset;
  vtkIdType icmLevel = (center[0] - level), icpLevel = (center[0] + level);
  vtkIdType jcmLevel = (center[1] - level), jcpLevel = (center[1] + level);
  vtkIdType kcmLevel = (center[2] - level), kcpLevel = (center[2] + level);
  for (int k = ijkMin[2]; k <= ijkMax[2]; ++k)
  {
    kOffset = k * this->xyD;
    for (int j = ijkMin[1]; j <= ijkMax[1]; ++j)
    {
      jOffset = j * this->xD;
      for (int i = ijkMin[0]; i <= ijkMax[0]; ++i)
      {
        // Any bin outside the level processed earlier should be visited.
        if ((i > icpLevel || i < icmLevel) || (j > jcpLevel || j < jcmLevel) ||
          (k > kcpLevel || k < kcmLevel))
        {
          binIdx = i + jOffset + kOffset;
          siter.AddPoints(i, j, k, binIdx, level, minR2, maxR2, results);
        }
      } // i-footprint
    }   // j-footprint
  }     // k-footprint

  // Sort if requested
  if (sort)
  {
    std::sort(results.begin(), results.end());
  }

  return maxR2;
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
  int i, j, k, ii, jOffset, kOffset, ijkMin[3], ijkMax[3];

  // Determine the range of indices in each direction based on radius R
  xMin[0] = x[0] - R;
  xMin[1] = x[1] - R;
  xMin[2] = x[2] - R;
  xMax[0] = x[0] + R;
  xMax[1] = x[1] + R;
  xMax[2] = x[2] + R;

  //  Find the footprint in the locator
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
          ids = this->GetIds(cno);
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
        }   // if points in bucket
      }     // i-footprint
    }       // j-footprint
  }         // k-footprint
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
                }   // over all points in bin
              }     // if points in bin
            }       // bucket not visited
          }         // i bins
        }           // j bins
      }             // k bins

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
  }   // if (vtkBox::IntersectBox(...))

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
  pts->Allocate(5000);
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
      }   // over i divisions
    }     // over j divisions
  }       // over k divisions

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
  // Short circuit mtime query process in tight loops
  if (this->Static)
  {
    return;
  }
  // don't rebuild if build time is newer than modified and dataset modified time
  if (this->Buckets && this->BuildTime > this->MTime && this->BuildTime > this->DataSet->GetMTime())
  {
    return;
  }
  // don't rebuild if UseExistingSearchStructure is ON and a search structure already exists
  if (this->Buckets && this->UseExistingSearchStructure)
  {
    this->BuildTime.Modified();
    vtkDebugMacro(<< "BuildLocator exited - UseExistingSearchStructure");
    return;
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
