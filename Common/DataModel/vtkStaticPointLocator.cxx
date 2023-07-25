// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkStaticPointLocator.h"

#include "vtkBoundingBox.h"
#include "vtkBox.h"
#include "vtkCellArray.h"
#include "vtkDataArray.h"
#include "vtkIdList.h"
#include "vtkIntArray.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkStructuredData.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkStaticPointLocator);

// There are stack-allocated bucket neighbor lists. This is the initial
// value. Too small and heap allocation kicks in.
constexpr size_t VTK_INITIAL_BUCKET_SIZE = 10000;

//------------------------------------------------------------------------------
// The following code supports threaded point locator construction. The locator
// is assumed to be constructed once (i.e., it does not allow incremental point
// insertion). The algorithm proceeds in three steps:
// 1) All points are assigned a bucket index (combined i-j-k bucket location).
// The index is computed in parallel. This requires a one time allocation of an
// index array (which is also associated with the originating point ids).
// 2) vtkSMPTools::Sort() is used to sort the index array. Note that the sort
// carries along the point ids as well. This creates contiguous runs of points
// all resident in the same bucket.
// 3) The bucket offsets are updated to refer to the right entry location into
// the sorted point ids array. This enables quick access, and an indirect count
// of the number of points in each bucket.

// Believe it or not I had to change the name because MS Visual Studio was
// mistakenly linking the hidden, scoped classes (vtkNeighborBuckets) found
// in vtkPointLocator and vtkStaticPointLocator and causing weird faults.
struct NeighborBuckets;

//------------------------------------------------------------------------------
// The bucketed points, including the sorted map. This is just a PIMPLd
// wrapper around the classes that do the real work.
struct vtkBucketList
{
  vtkStaticPointLocator* Locator; // locater
  vtkIdType NumPts;               // the number of points to bucket
  vtkIdType NumBuckets;
  int BatchSize;

  // These are internal data members used for performance reasons
  vtkDataSet* DataSet;
  int Divisions[3];
  double Bounds[6];
  double H[3];
  double hX, hY, hZ;
  double fX, fY, fZ, bX, bY, bZ;
  vtkIdType xD, yD, zD, xyD;

  // Construction
  vtkBucketList(vtkStaticPointLocator* loc, vtkIdType numPts, int numBuckets)
  {
    this->Locator = loc;
    this->NumPts = numPts;
    this->NumBuckets = numBuckets;
    this->BatchSize = 10000; // building the offset array
    this->DataSet = loc->GetDataSet();
    loc->GetDivisions(this->Divisions);

    // Setup internal data members for more efficient processing.
    double spacing[3], bounds[6];
    loc->GetDivisions(this->Divisions);
    loc->GetSpacing(spacing);
    loc->GetBounds(bounds);
    this->hX = this->H[0] = spacing[0];
    this->hY = this->H[1] = spacing[1];
    this->hZ = this->H[2] = spacing[2];
    this->fX = 1.0 / spacing[0];
    this->fY = 1.0 / spacing[1];
    this->fZ = 1.0 / spacing[2];
    this->bX = this->Bounds[0] = bounds[0];
    this->Bounds[1] = bounds[1];
    this->bY = this->Bounds[2] = bounds[2];
    this->Bounds[3] = bounds[3];
    this->bZ = this->Bounds[4] = bounds[4];
    this->Bounds[5] = bounds[5];
    this->xD = this->Divisions[0];
    this->yD = this->Divisions[1];
    this->zD = this->Divisions[2];
    this->xyD = this->Divisions[0] * this->Divisions[1];
  }

  // Virtuals for templated subclasses
  virtual ~vtkBucketList() = default;
  virtual void BuildLocator() = 0;

  // place points in appropriate buckets
  void GetBucketNeighbors(
    NeighborBuckets* buckets, const int ijk[3], const int ndivs[3], int level);
  void GenerateFace(int face, int i, int j, int k, vtkPoints* pts, vtkCellArray* polys);
  double Distance2ToBucket(const double x[3], const int nei[3]);
  double Distance2ToBounds(const double x[3], const double bounds[6]);

  //-----------------------------------------------------------------------------
  // Inlined for performance. These function invocations must be called after
  // BuildLocator() is invoked, otherwise the output is indeterminate.
  void GetBucketIndices(const double* x, int ijk[3]) const
  {
    // Compute point index. Make sure it lies within range of locator.
    vtkIdType tmp0 = static_cast<vtkIdType>(((x[0] - bX) * fX));
    vtkIdType tmp1 = static_cast<vtkIdType>(((x[1] - bY) * fY));
    vtkIdType tmp2 = static_cast<vtkIdType>(((x[2] - bZ) * fZ));

    ijk[0] = tmp0 < 0 ? 0 : (tmp0 >= xD ? xD - 1 : tmp0);
    ijk[1] = tmp1 < 0 ? 0 : (tmp1 >= yD ? yD - 1 : tmp1);
    ijk[2] = tmp2 < 0 ? 0 : (tmp2 >= zD ? zD - 1 : tmp2);
  }

  //-----------------------------------------------------------------------------
  vtkIdType GetBucketIndex(const double* x) const
  {
    int ijk[3];
    this->GetBucketIndices(x, ijk);
    return ijk[0] + ijk[1] * xD + ijk[2] * xyD;
  }
};

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
// The following tuple is what is sorted in the map. Note that it is templated
// because depending on the number of points / buckets to process we may want
// to use vtkIdType. Otherwise for performance reasons it's best to use an int
// (or other integral type). Typically sort() is 25-30% faster on smaller
// integral types, plus it takes a heck less memory (when vtkIdType is 64-bit
// and int is 32-bit).
template <typename TTuple>
struct LocatorTuple
{
  TTuple PtId;   // originating point id
  TTuple Bucket; // i-j-k index into bucket space

  //  Operator< used to support the subsequent sort operation. There are two
  //  implementations, one gives a stable sort (points ordered by id within
  //  each bucket) and the other a little faster but less stable (in parallel
  //  sorting the order of sorted points in a bucket may vary).
  //  bool operator< (const LocatorTuple& tuple) const
  //  {return Bucket < tuple.Bucket;}
  bool operator<(const LocatorTuple& tuple) const
  {
    if (Bucket < tuple.Bucket)
      return true;
    if (tuple.Bucket < Bucket)
      return false;
    if (PtId < tuple.PtId)
      return true;
    return false;
  }
};

//------------------------------------------------------------------------------
// This templates class manages the creation of the static locator
// structures. It also implements the operator() functors which are supplied
// to vtkSMPTools for threaded processesing.
template <typename TIds>
struct BucketList : public vtkBucketList
{
  // Okay the various ivars
  LocatorTuple<TIds>* Map; // the map to be sorted
  TIds* Offsets;           // offsets for each bucket into the map

  // Construction
  BucketList(vtkStaticPointLocator* loc, vtkIdType numPts, int numBuckets)
    : vtkBucketList(loc, numPts, numBuckets)
  {
    // one extra to simplify traversal
    this->Map = new LocatorTuple<TIds>[numPts + 1];
    this->Map[numPts].Bucket = numBuckets;
    this->Offsets = new TIds[numBuckets + 1];
    this->Offsets[numBuckets] = numPts;
  }

  // Release allocated memory
  ~BucketList() override
  {
    delete[] this->Map;
    delete[] this->Offsets;
  }

  // The number of point ids in a bucket is determined by computing the
  // difference between the offsets into the sorted points array.
  vtkIdType GetNumberOfIds(vtkIdType bucketNum)
  {
    return (this->Offsets[bucketNum + 1] - this->Offsets[bucketNum]);
  }

  // Given a bucket number, return the point ids in that bucket.
  const LocatorTuple<TIds>* GetIds(vtkIdType bucketNum)
  {
    return this->Map + this->Offsets[bucketNum];
  }

  // Given a bucket number, return the point ids in that bucket.
  void GetIds(vtkIdType bucketNum, vtkIdList* bList)
  {
    const LocatorTuple<TIds>* ids = this->GetIds(bucketNum);
    vtkIdType numIds = this->GetNumberOfIds(bucketNum);
    bList->SetNumberOfIds(numIds);
    for (int i = 0; i < numIds; i++)
    {
      bList->SetId(i, ids[i].PtId);
    }
  }

  // Templated implementations of the locator
  vtkIdType FindClosestPoint(const double x[3]);
  vtkIdType FindClosestPointWithinRadius(
    double radius, const double x[3], double inputDataLength, double& dist2);
  void FindClosestNPoints(int N, const double x[3], vtkIdList* result);
  void FindPointsWithinRadius(double R, const double x[3], vtkIdList* result);
  int IntersectWithLine(double a0[3], double a1[3], double tol, double& t, double lineX[3],
    double ptX[3], vtkIdType& ptId);
  void MergePoints(double tol, vtkIdType* pointMap, int orderingMode);
  void MergePointsWithData(vtkDataArray* data, vtkIdType* pointMap);
  void GenerateRepresentation(int vtkNotUsed(level), vtkPolyData* pd);

  // Internal methods
  void GetOverlappingBuckets(
    NeighborBuckets* buckets, const double x[3], const int ijk[3], double dist, int level);
  void GetOverlappingBuckets(NeighborBuckets* buckets, const double x[3], double dist,
    int prevMinLevel[3], int prevMaxLevel[3]);

  // Implicit point representation, slower path
  template <typename T>
  struct MapDataSet
  {
    BucketList<T>* BList;
    vtkDataSet* DataSet;

    MapDataSet(BucketList<T>* blist, vtkDataSet* ds)
      : BList(blist)
      , DataSet(ds)
    {
    }

    void operator()(vtkIdType ptId, vtkIdType end)
    {
      double p[3];
      LocatorTuple<T>* t = this->BList->Map + ptId;

      for (; ptId < end; ++ptId, ++t)
      {
        this->DataSet->GetPoint(ptId, p);
        t->Bucket = this->BList->GetBucketIndex(p);
        t->PtId = ptId;
      } // for all points in this batch
    }
  };

  // Explicit point representation (e.g., vtkPointSet), faster path
  template <typename T, typename TPts>
  struct MapPointsArray
  {
    BucketList<T>* BList;
    const TPts* Points;

    MapPointsArray(BucketList<T>* blist, const TPts* pts)
      : BList(blist)
      , Points(pts)
    {
    }

    void operator()(vtkIdType ptId, vtkIdType end)
    {
      double p[3];
      const TPts* x = this->Points + 3 * ptId;
      LocatorTuple<T>* t = this->BList->Map + ptId;

      for (; ptId < end; ++ptId, x += 3, ++t)
      {
        p[0] = static_cast<double>(x[0]);
        p[1] = static_cast<double>(x[1]);
        p[2] = static_cast<double>(x[2]);
        t->Bucket = this->BList->GetBucketIndex(p);
        t->PtId = ptId;
      } // for all points in this batch
    }
  };

  // A clever way to build offsets in parallel. Basically each thread builds
  // offsets across a range of the sorted map. Recall that offsets are an
  // integral value referring to the locations of the sorted points that
  // reside in each bucket.
  template <typename T>
  struct MapOffsets
  {
    BucketList<T>* BList;
    int NumBuckets;
    vtkIdType NumPts;

    MapOffsets(BucketList<T>* blist)
      : BList(blist)
    {
      this->NumBuckets = this->BList->NumBuckets;
      this->NumPts = this->BList->NumPts;
    }

    // Traverse sorted points (i.e., tuples) and update bucket offsets.
    void operator()(vtkIdType batch, vtkIdType batchEnd)
    {
      T* offsets = this->BList->Offsets;
      const LocatorTuple<T>* curPt = this->BList->Map + batch * this->BList->BatchSize;
      const LocatorTuple<T>* endBatchPt = this->BList->Map + batchEnd * this->BList->BatchSize;
      const LocatorTuple<T>* endPt = this->BList->Map + this->NumPts;
      const LocatorTuple<T>* prevPt;
      endBatchPt = (endBatchPt > endPt ? endPt : endBatchPt);

      // Special case at the very beginning of the mapped points array.  If
      // the first point is in bucket# N, then all buckets up and including
      // N must refer to the first point.
      if (curPt == this->BList->Map)
      {
        prevPt = this->BList->Map;
        std::fill_n(offsets, curPt->Bucket + 1, 0); // point to the first points
      } // at the very beginning of the map (sorted points array)

      // We are entering this functor somewhere in the interior of the
      // mapped points array. All we need to do is point to the entry
      // position because we are interested only in prevPt->Bucket.
      else
      {
        prevPt = curPt;
      } // else in the middle of a batch

      // Okay we have a starting point for a bucket run. Now we can begin
      // filling in the offsets in this batch. A previous thread should
      // have/will have completed the previous and subsequent runs outside
      // of the [batch,batchEnd) range
      for (curPt = prevPt; curPt < endBatchPt;)
      {
        for (; curPt->Bucket == prevPt->Bucket && curPt <= endBatchPt; ++curPt)
        {
          // advance
        }
        // Fill in any gaps in the offset array
        std::fill_n(
          offsets + prevPt->Bucket + 1, curPt->Bucket - prevPt->Bucket, curPt - this->BList->Map);
        prevPt = curPt;
      } // for all batches in this range
    }   // operator()
  };

  // Merge points that are pecisely coincident. Operates in parallel on
  // locator buckets. Does not need to check neighbor buckets.
  template <typename T>
  struct MergePrecise
  {
    BucketList<T>* BList;
    vtkDataSet* DataSet;
    vtkIdType* MergeMap;

    MergePrecise(BucketList<T>* blist, vtkIdType* mergeMap)
      : BList(blist)
      , MergeMap(mergeMap)
    {
      this->DataSet = blist->DataSet;
    }

    void operator()(vtkIdType bucket, vtkIdType endBucket)
    {
      BucketList<T>* bList = this->BList;
      vtkIdType* mergeMap = this->MergeMap;
      int i, j;
      const LocatorTuple<TIds>* ids;
      double p[3], p2[3];
      vtkIdType ptId, ptId2, numIds;

      for (; bucket < endBucket; ++bucket)
      {
        if ((numIds = bList->GetNumberOfIds(bucket)) > 0)
        {
          ids = bList->GetIds(bucket);
          for (i = 0; i < numIds; i++)
          {
            ptId = ids[i].PtId;
            if (mergeMap[ptId] < 0)
            {
              mergeMap[ptId] = ptId;
              this->DataSet->GetPoint(ptId, p);
              for (j = i + 1; j < numIds; j++)
              {
                ptId2 = ids[j].PtId;
                if (mergeMap[ptId2] < 0)
                {
                  this->DataSet->GetPoint(ptId2, p2);
                  if (p[0] == p2[0] && p[1] == p2[1] && p[2] == p2[2])
                  {
                    mergeMap[ptId2] = ptId;
                  }
                }
              }
            } // if point not yet visited
          }
        }
      }
    }
  };

  // Merge points that are coincident within a specified tolerance. Depending
  // on the orderingMode, either a serialized ordering process is used (i.e.,
  // POINT_ORDER) or a threaded ordering process is used (i.e.,
  // BIN_ORDER).  Note that due to the tolerance, the merging tolerance
  // needs to check neighbor buckets which slows the algorithm down
  // considerably. Note that merging is in one direction: larger ids are
  // merged to lower ids.
  template <typename T>
  struct MergeClose
  {
    BucketList<T>* BList;
    vtkDataSet* DataSet;
    vtkIdType* MergeMap;
    double Tol;

    vtkSMPThreadLocalObject<vtkIdList> PIds;

    MergeClose(BucketList<T>* blist, double tol, vtkIdType* mergeMap)
      : BList(blist)
      , MergeMap(mergeMap)
      , Tol(tol)
    {
      this->DataSet = blist->DataSet;
    }

    // The core merging process around the point ptId.
    inline void MergePoint(vtkIdType ptId, vtkIdList* nearby)
    {
      vtkIdType* mergeMap = this->MergeMap;

      // Make sure the point is not already merged
      if (mergeMap[ptId] < 0)
      {
        mergeMap[ptId] = ptId;
        double p[3];
        this->DataSet->GetPoint(ptId, p);
        this->BList->FindPointsWithinRadius(this->Tol, p, nearby);
        vtkIdType numIds = nearby->GetNumberOfIds();
        if (numIds > 0)
        {
          for (auto i = 0; i < numIds; ++i)
          {
            vtkIdType nearId = nearby->GetId(i);
            if (mergeMap[nearId] < 0)
            {
              mergeMap[nearId] = ptId;
            } // if eligible for merging and not yet merged
          }   // for all nearby points
        }     // if nearby points exist
      }       // if point not yet merged
    }         // MergePoint

    // Just allocate a little bit of memory to get started.
    void Initialize()
    {
      vtkIdList*& pIds = this->PIds.Local();
      pIds->Allocate(128); // allocate some memory
    }

    void Reduce() {}
  };

  // Merge points with non-zero tolerance. Order of point merging guarantees
  // that any two merged point ids (p0,p1) are such that p0<p1. Consequently
  // this is a completely serial algorithm.
  template <typename T>
  struct MergePointOrder : public MergeClose<T>
  {
    MergePointOrder(BucketList<T>* blist, double tol, vtkIdType* mergeMap)
      : MergeClose<T>(blist, tol, mergeMap)
    {
    }

    void Initialize() { this->MergeClose<T>::Initialize(); }

    // Process serially, point by point.
    void operator()(vtkIdType numPts)
    {
      vtkIdList*& nearby = this->PIds.Local();

      // Serial operation over all points in the locator.
      for (vtkIdType ptId = 0; ptId < numPts; ++ptId)
      {
        this->MergePoint(ptId, nearby);
      } // for all points in the locator
    }   // operator()

    void Reduce() { this->MergeClose<T>::Reduce(); }
  }; // Merge points in point ordering

  // Merge points with non-zero tolerance. The order of point merging depends
  // on the order in which the bins are traversed (using a checkerboard
  // pattern).  While the algorithm is threaded, the checkerboarding acts as
  // a barrier to full threading so the performance is not optimal (but at
  // least deterministic / reproducible).
  //
  // Checkerboarding works as follows. The locator bin volume of dimensions
  // Divisions[3] is divided into a collection of "blocks" which are
  // subvolumes of bins of dimensions d^3. The algorithm makes multiple,
  // threaded passes over the blocks (a total of d^3 threaded traversals),
  // choosing one of the bins in each block to process via the current
  // checkerboard index.  The dimension d of the blocks is determined by the
  // tolerance and locator bin size, and is chosen in such a way as to
  // separate the point merging computation so as to avoid threading data
  // races / write contention.
  template <typename T>
  struct MergeBinOrder : public MergeClose<T>
  {
    int CheckerboardDimension; // the dimension of the checkerboard block/subvolume
    int NumBlocks;             // how many blocks/subvolumes are in the binned locator
    int BlockDims[3];          // the number of blocks in each coordinate direction
    int CheckerboardIndex[3];  // which bin is being processed in the blocks

    // The main function of the constructor is the setup the checkerboard
    // traversal. This means configuring the checkerboard subvolume, and
    // set up the traversal indices.
    MergeBinOrder(BucketList<T>* blist, double tol, vtkIdType* mergeMap)
      : MergeClose<T>(blist, tol, mergeMap)
    {
      BucketList<T>* bl = this->BList;
      double hMin = (bl->hX < bl->hY ? (bl->hX < bl->hZ ? bl->hX : bl->hZ)
                                     : (bl->hY < bl->hZ ? bl->hY : bl->hZ));
      this->CheckerboardDimension =
        1 + (hMin == 0.0 ? 1 : (1 + vtkMath::Floor(tol / (hMin / 2.0))));

      // Determine how many blocks there are in the locater, and determine the
      // dimensions of the blocks.
      this->NumBlocks = 1;
      for (auto i = 0; i < 3; ++i)
      {
        double numBlocks =
          static_cast<double>(bl->Divisions[i]) / static_cast<double>(this->CheckerboardDimension);
        this->BlockDims[i] = (bl->Divisions[i] <= 1 ? 1 : vtkMath::Ceil(numBlocks));
        this->NumBlocks *= this->BlockDims[i];
      }
      this->InitializeCheckerboardIndex();
    }

    // Initialize the checkerboard traversal process. A pointer to the
    // current traversal state (within the checkerboard region) is returned.
    int* InitializeCheckerboardIndex()
    {
      // Control checkerboard traversal
      this->CheckerboardIndex[0] = 0;
      this->CheckerboardIndex[1] = 0;
      this->CheckerboardIndex[2] = 0;
      return this->CheckerboardIndex;
    }

    // Given a blockId and the current checkerboard index, compute the
    // current locator bin/bucket id. May return <0 if no bin exists.
    vtkIdType GetCurrentBin(int blockId, int cIdx[3])
    {
      // Which checkerboard block are we in?
      int ijk[3];
      vtkStructuredData::ComputePointStructuredCoords(blockId, this->BlockDims, ijk);

      // Combine the block index with the checkerboard index. Make sure that
      // we are still inside the locator bins (partial blocks may exist at
      // the boundary). Recall that the blocks are composed of d^3 bins.
      for (auto i = 0; i < 3; ++i)
      {
        ijk[i] = ijk[i] * this->CheckerboardDimension + cIdx[i];
        if (ijk[i] >= this->BList->Divisions[i])
        {
          return (-1);
        }
      }

      // Okay return the bin index
      return (ijk[0] + ijk[1] * this->BList->Divisions[0] +
        ijk[2] * this->BList->Divisions[0] * this->BList->Divisions[1]);
    }

    void Initialize() { this->MergeClose<T>::Initialize(); }

    // Process locator blocks/subvolumes.
    void operator()(vtkIdType blockId, vtkIdType endBlockId)
    {

      for (; blockId < endBlockId; ++blockId)
      {
        vtkIdType bin = this->GetCurrentBin(blockId, this->CheckerboardIndex);
        vtkIdType numIds;

        if (bin >= 0 && (numIds = this->BList->GetNumberOfIds(bin)) > 0)
        {
          const LocatorTuple<TIds>* ids = this->BList->GetIds(bin);
          for (auto i = 0; i < numIds; ++i)
          {
            vtkIdType ptId = ids[i].PtId;
            vtkIdList*& nearby = this->PIds.Local();
            this->MergePoint(ptId, nearby);
          } // for all points in bin/bucket
        }   // if points exist in bin/bucket
      }     // for all blocks
    }       // operator()

    void Reduce() { this->MergeClose<T>::Reduce(); }

    // Coordinate the checkerboard threading process. Checkerboarding simply
    // processes a subset of the locator bins to avoid write contention. The
    // checkerboard footprint (its subvolume size) is a function of the
    // tolerance, and is effectively a d^3 subvolume that is traversed
    // (across all subvolumes) in a synchronized fashion. Hence there are d^3
    // separate SMP traversals - if d becomes too large, the fallback is
    // simply a serial (MergePointOrder()) to avoid thread thrashing.
    void Execute()
    {
      int cDim = this->CheckerboardDimension;
      int* cIdx = this->InitializeCheckerboardIndex();

      // Coordinate the checkerboarding by synchronized traversal of the
      // the checkerboard subblocks.
      for (cIdx[2] = 0; cIdx[2] < cDim; ++cIdx[2])
      {
        for (cIdx[1] = 0; cIdx[1] < cDim; ++cIdx[1])
        {
          for (cIdx[0] = 0; cIdx[0] < cDim; ++cIdx[0])
          {
            vtkSMPTools::For(0, this->NumBlocks, *this);
          }
        }
      }
    } // Execute()

  }; // MergeBinOrder

  // Merge points that are geometrically coincident and have matching data
  // values. Operates in parallel on locator buckets. Does not need to check
  // neighbor buckets.
  template <typename T>
  struct MergePointsAndData
  {
    BucketList<T>* BList;
    vtkDataSet* DataSet;
    vtkDataArray* DataArray;
    vtkIdType* MergeMap;
    vtkSMPThreadLocal<std::vector<double>> Tuple;
    vtkSMPThreadLocal<std::vector<double>> Tuple2;

    MergePointsAndData(BucketList<T>* blist, vtkDataArray* da, vtkIdType* mergeMap)
      : BList(blist)
      , DataArray(da)
      , MergeMap(mergeMap)
    {
      this->DataSet = blist->DataSet;
    }

    bool TuplesEqual(int tupleSize, double* t1, double* t2)
    {
      for (auto i = 0; i < tupleSize; ++i)
      {
        if (t1[i] != t2[i])
        {
          return false;
        }
      }
      return true;
    }

    void Initialize()
    {
      vtkIdType numComp = this->DataArray->GetNumberOfComponents();
      this->Tuple.Local().resize(numComp);
      this->Tuple2.Local().resize(numComp);
    }

    void operator()(vtkIdType bucket, vtkIdType endBucket)
    {
      BucketList<T>* bList = this->BList;
      vtkIdType* mergeMap = this->MergeMap;
      int i, j;
      const LocatorTuple<TIds>* ids;
      double p[3], p2[3];
      vtkIdType ptId, ptId2, numIds;
      int tupleSize = static_cast<int>(this->Tuple.Local().size());
      double* t = this->Tuple.Local().data();
      double* t2 = this->Tuple2.Local().data();

      for (; bucket < endBucket; ++bucket)
      {
        if ((numIds = bList->GetNumberOfIds(bucket)) > 0)
        {
          ids = bList->GetIds(bucket);
          for (i = 0; i < numIds; i++)
          {
            ptId = ids[i].PtId;
            if (mergeMap[ptId] < 0)
            {
              mergeMap[ptId] = ptId;
              this->DataSet->GetPoint(ptId, p);
              this->DataArray->GetTuple(ptId, t);
              for (j = i + 1; j < numIds; j++)
              {
                ptId2 = ids[j].PtId;
                if (mergeMap[ptId2] < 0)
                {
                  this->DataSet->GetPoint(ptId2, p2);
                  if (p[0] == p2[0] && p[1] == p2[1] && p[2] == p2[2])
                  {
                    this->DataArray->GetTuple(ptId2, t2);
                    if (this->TuplesEqual(tupleSize, t, t2))
                    {
                      mergeMap[ptId2] = ptId;
                    } // if point's data match
                  }   // if points geometrically coincident
                }     // if point not yet visited
              }       // for the remaining points in the bin
            }         // if point not yet merged
          }           // for all points in bucket
        }             // if bucket contains points
      }               // for all buckets
    }                 // operator()

    void Reduce() {}
  }; // MergePointsWithData

  // Build the map and other structures to support locator operations
  void BuildLocator() override
  {
    // Place each point in a bucket
    //
    vtkPointSet* ps = vtkPointSet::SafeDownCast(this->DataSet);
    if (ps)
    { // map points array: explicit points representation of float or double
      int dataType = ps->GetPoints()->GetDataType();
      void* pts = ps->GetPoints()->GetVoidPointer(0);
      if (dataType == VTK_FLOAT)
      {
        MapPointsArray<TIds, float> mapper(this, static_cast<float*>(pts));
        vtkSMPTools::For(0, this->NumPts, mapper);
      }
      else if (dataType == VTK_DOUBLE)
      {
        MapPointsArray<TIds, double> mapper(this, static_cast<double*>(pts));
        vtkSMPTools::For(0, this->NumPts, mapper);
      }
    }
    else // if (!mapped)
    {    // map dataset points: non-float points or implicit points representation
      MapDataSet<TIds> mapper(this, this->DataSet);
      vtkSMPTools::For(0, this->NumPts, mapper);
    }

    // Now group the points into contiguous runs within buckets (recall that
    // sorting is occurring based on bin/bucket id).
    vtkSMPTools::Sort(this->Map, this->Map + this->NumPts);

    // Build the offsets into the Map. The offsets are the positions of
    // each bucket into the sorted list. They mark the beginning of the
    // list of points in each bucket. Amazingly, this can be done in
    // parallel.
    int numBatches = static_cast<int>(ceil(static_cast<double>(this->NumPts) / this->BatchSize));
    MapOffsets<TIds> offMapper(this);
    vtkSMPTools::For(0, numBatches, offMapper);
  }
};

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
  const LocatorTuple<TIds>* ids;

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
  const LocatorTuple<TIds>* ids;

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
      if (ii < 2)
      {
        ii = 2;
      }
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

namespace
{
//------------------------------------------------------------------------------
// Obtaining closest points requires sorting nearby points
struct IdTuple
{
  vtkIdType PtId;
  double Dist2;

  bool operator<(const IdTuple& tuple) const { return Dist2 < tuple.Dist2; }
};
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
  const LocatorTuple<TIds>* ids;

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
  std::vector<IdTuple> res(N);

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
            res[currentCount].PtId = ptId;
            if (dist2 > maxDistance)
            {
              maxDistance = dist2;
            }
            currentCount++;
            if (currentCount == N)
            {
              std::sort(res.begin(), res.begin() + currentCount);
            }
          }
          else if (dist2 < maxDistance)
          {
            res[N - 1].Dist2 = dist2;
            res[N - 1].PtId = ptId;
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
          res[N - 1].PtId = ptId;
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
    result->SetId(i, res[i].PtId);
  }
}

//------------------------------------------------------------------------------
// The Radius defines a block of buckets which the sphere of radius R may
// touch.
template <typename TIds>
void BucketList<TIds>::FindPointsWithinRadius(double R, const double x[3], vtkIdList* result)
{
  double dist2;
  double pt[3];
  vtkIdType ptId, cno, numIds;
  double R2 = R * R;
  const LocatorTuple<TIds>* ids;
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
                const LocatorTuple<TIds>* ptIds = this->GetIds(idx);
                for (ii = 0; ii < numPtsInBin; ii++)
                {
                  pId = ptIds[ii].PtId;
                  this->DataSet->GetPoint(pId, x);
                  if (vtkLine::DistanceToLine(x, a0, a1, tHit, xl) <= tol2 && t < tMin)
                  {
                    tMin = t;
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

//------------------------------------------------------------------------------
// Here is the VTK class proper. It's implemented with the templated
// BucketList class.

//------------------------------------------------------------------------------
// Construct with automatic computation of divisions, averaging
// 5 points per bucket.
vtkStaticPointLocator::vtkStaticPointLocator()
{
  this->NumberOfPointsPerBucket = 1;
  this->Divisions[0] = this->Divisions[1] = this->Divisions[2] = 50;
  this->H[0] = this->H[1] = this->H[2] = 0.0;
  this->Buckets = nullptr;
  this->MaxNumberOfBuckets = VTK_INT_MAX;
  this->LargeIds = false;
  this->TraversalOrder = BIN_ORDER;
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
    return static_cast<BucketList<vtkIdType>*>(this->Buckets)->FindClosestNPoints(N, x, result);
  }
  else
  {
    return static_cast<BucketList<int>*>(this->Buckets)->FindClosestNPoints(N, x, result);
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
    return static_cast<BucketList<vtkIdType>*>(this->Buckets)->FindPointsWithinRadius(R, x, result);
  }
  else
  {
    return static_cast<BucketList<int>*>(this->Buckets)->FindPointsWithinRadius(R, x, result);
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
    return static_cast<BucketList<vtkIdType>*>(this->Buckets)->GenerateRepresentation(level, pd);
  }
  else
  {
    return static_cast<BucketList<int>*>(this->Buckets)->GenerateRepresentation(level, pd);
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
    return static_cast<BucketList<vtkIdType>*>(this->Buckets)->GetIds(bNum, bList);
  }
  else
  {
    return static_cast<BucketList<int>*>(this->Buckets)->GetIds(bNum, bList);
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
    return static_cast<BucketList<vtkIdType>*>(this->Buckets)
      ->MergePoints(tol, pointMap, this->TraversalOrder);
  }
  else
  {
    return static_cast<BucketList<int>*>(this->Buckets)
      ->MergePoints(tol, pointMap, this->TraversalOrder);
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
    return static_cast<BucketList<vtkIdType>*>(this->Buckets)->MergePointsWithData(data, pointMap);
  }
  else
  {
    return static_cast<BucketList<int>*>(this->Buckets)->MergePointsWithData(data, pointMap);
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
}
VTK_ABI_NAMESPACE_END
