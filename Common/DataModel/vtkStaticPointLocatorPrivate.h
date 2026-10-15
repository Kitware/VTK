// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2011 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkBucketList
 * @brief   Private declarations for 3D binned spatial locator
 *
 * The main purpose of this class is to enable access to the internals
 * of vtkStaticPointLocator, allowing complex iteration over locator bins.
 * For example, see vtkShellBinIterator.
 */

#ifndef vtkStaticPointLocatorPrivate_h
#define vtkStaticPointLocatorPrivate_h

#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchDataSetArrayList.h"
#include "vtkCellArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h" // for SafeCastFromDouble
#include "vtkPoints.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkStaticPointLocator.h"
#include "vtkStructuredData.h"

#include <cassert>
#include <cstdlib>

VTK_ABI_NAMESPACE_BEGIN

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

// Forward declaration of lists of neighboring buckets.
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
  double hX, hY, hZ, hX2, hY2, hZ2;
  double fX, fY, fZ, bX, bY, bZ;
  vtkIdType xD, yD, zD, xyD;

  // Used for accelerated performance for certain methods
  double* FastPoints; // fast path for accessing points
  double BinRadius;   // circumradius of a single bin/bucket
  int MaxLevel;       // the maximum possible level searches can proceed

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
    this->hX2 = this->hX / 2.0;
    this->hY2 = this->hY / 2.0;
    this->hZ2 = this->hZ / 2.0;
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

    this->FastPoints = nullptr;
    this->BinRadius = sqrt(hX * hX + hY * hY + hZ * hZ) / 2.0;
    this->MaxLevel = std::max({ this->xD, this->yD, this->zD });
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
    // Compute point index. SafeCastFromDouble clamps to the integer type limits
    // (mapping NaN to 0) so casting a coordinate far outside the locator bounds
    // (e.g. VTK_DOUBLE_MAX) is not undefined behavior. Make sure it then lies
    // within the range of the locator.
    vtkIdType tmp0 = vtkMathUtilities::SafeCastFromDouble<vtkIdType>((x[0] - bX) * fX);
    vtkIdType tmp1 = vtkMathUtilities::SafeCastFromDouble<vtkIdType>((x[1] - bY) * fY);
    vtkIdType tmp2 = vtkMathUtilities::SafeCastFromDouble<vtkIdType>((x[2] - bZ) * fZ);

    ijk[0] = tmp0 < 0 ? 0 : std::min(xD - 1, tmp0);
    ijk[1] = tmp1 < 0 ? 0 : std::min(yD - 1, tmp1);
    ijk[2] = tmp2 < 0 ? 0 : std::min(zD - 1, tmp2);
  }

  //-----------------------------------------------------------------------------
  vtkIdType GetBucketIndex(const double* x) const
  {
    int ijk[3];
    this->GetBucketIndices(x, ijk);
    return ijk[0] + ijk[1] * xD + ijk[2] * xyD;
  }

  //-----------------------------------------------------------------------------
  // Return the center of the bucket/bin at (i,j,k).
  void GetBucketCenter(int i, int j, int k, double center[3])
  {
    center[0] = this->bX + this->hX2 + i * this->hX;
    center[1] = this->bY + this->hY2 + j * this->hY;
    center[2] = this->bZ + this->hZ2 + k * this->hZ;
  }

  //-----------------------------------------------------------------------------
  // Return the bounding box (min,max) of a specified bucket (i,j,k).
  void GetBucketBounds(int i, int j, int k, double min[3], double max[3])
  {
    min[0] = this->bX + i * this->hX;
    min[1] = this->bY + j * this->hY;
    min[2] = this->bZ + k * this->hZ;
    max[0] = min[0] + this->hX;
    max[1] = min[1] + this->hY;
    max[2] = min[2] + this->hZ;
  }

  //-----------------------------------------------------------------------------
  // Determine whether a bin/bucket specified by i,j,k is completely contained
  // inside the sphere (center,r2). Return true if contained; false otherwise.
  bool BucketInsideSphere(int i, int j, int k, const double center[3], double r2)
  {
    double min[3], max[3];
    min[0] = this->bX + i * this->hX;
    min[1] = this->bY + j * this->hY;
    min[2] = this->bZ + k * this->hZ;
    max[0] = min[0] + this->hX;
    max[1] = min[1] + this->hY;
    max[2] = min[2] + this->hZ;
    return vtkBoundingBox::InsideSphere(min, max, center, r2);
  }
}; // vtkBucketList

//------------------------------------------------------------------------------
// This templates class manages the creation of the static locator
// structures. It also implements the operator() functors which are supplied
// to vtkSMPTools for threaded processesing.
template <typename TIds>
struct BucketList : public vtkBucketList
{
  // Okay the various ivars
  vtkLocatorTuple<TIds>* Map; // the map to be sorted
  TIds* Offsets;              // offsets for each bucket into the map

  // Construction
  BucketList(vtkStaticPointLocator* loc, vtkIdType numPts, int numBuckets)
    : vtkBucketList(loc, numPts, numBuckets)
  {
    // one extra to simplify traversal
    this->Map = new vtkLocatorTuple<TIds>[numPts + 1];
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
  const vtkLocatorTuple<TIds>* GetIds(vtkIdType bucketNum)
  {
    return this->Map + this->Offsets[bucketNum];
  }

  // Given a bucket number, return the point ids in that bucket.
  void GetIds(vtkIdType bucketNum, vtkIdList* bList)
  {
    const vtkLocatorTuple<TIds>* ids = this->GetIds(bucketNum);
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
  double FindNPointsInShell(int N, const double x[3], vtkDist2TupleArray& results,
    double minDist2 = (-0.1), bool sort = true, vtkDoubleArray* petals = nullptr);
  void FindPointsInShell(const double x[3], vtkDist2TupleArray& results, double minDist2,
    double maxDist2, bool sort, vtkDoubleArray* petals);
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
      vtkLocatorTuple<T>* t = this->BList->Map + ptId;

      for (; ptId < end; ++ptId, ++t)
      {
        this->DataSet->GetPoint(ptId, p);
        t->Bucket = this->BList->GetBucketIndex(p);
        t->PtId = ptId;
      } // for all points in this batch
    }
  };

  template <typename T, typename TPointsArray>
  struct MapPointsArray
  {
    BucketList<T>* BList;
    TPointsArray* Points;

    MapPointsArray(BucketList<T>* blist, TPointsArray* pts)
      : BList(blist)
      , Points(pts)
    {
    }

    void operator()(vtkIdType ptId, vtkIdType end)
    {
      double p[3];
      auto x = vtk::DataArrayTupleRange<3>(this->Points, ptId, end).begin();
      vtkLocatorTuple<T>* t = this->BList->Map + ptId;

      for (; ptId < end; ++ptId, ++x, ++t)
      {
        x->GetTuple(p);
        t->Bucket = this->BList->GetBucketIndex(p);
        t->PtId = ptId;
      } // for all points in this batch
    }
  };

  struct MapPointsArrayWorker
  {
    template <typename TPointsArray>
    void operator()(TPointsArray* points, BucketList* blist)
    {
      MapPointsArray<TIds, TPointsArray> mapper(blist, points);
      vtkSMPTools::For(0, blist->NumPts, mapper);
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
      const vtkLocatorTuple<T>* curPt = this->BList->Map + batch * this->BList->BatchSize;
      const vtkLocatorTuple<T>* endBatchPt = this->BList->Map + batchEnd * this->BList->BatchSize;
      const vtkLocatorTuple<T>* endPt = this->BList->Map + this->NumPts;
      const vtkLocatorTuple<T>* prevPt;
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
    } // operator()
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
      const vtkLocatorTuple<TIds>* ids;
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
    void MergePoint(vtkIdType ptId, vtkIdList* nearby)
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
          } // for all nearby points
        } // if nearby points exist
      } // if point not yet merged
    } // MergePoint

    // Just allocate a little bit of memory to get started.
    void Initialize()
    {
      vtkIdList*& pIds = this->PIds.Local();
      pIds->Reserve(128); // allocate some memory
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
    } // operator()

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
      double hMin = std::min({ bl->hX, bl->hY, bl->hZ });
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
      vtkIdList*& nearby = this->PIds.Local();
      for (; blockId < endBlockId; ++blockId)
      {
        vtkIdType bin = this->GetCurrentBin(blockId, this->CheckerboardIndex);
        vtkIdType numIds;

        if (bin >= 0 && (numIds = this->BList->GetNumberOfIds(bin)) > 0)
        {
          const vtkLocatorTuple<TIds>* ids = this->BList->GetIds(bin);
          for (auto i = 0; i < numIds; ++i)
          {
            vtkIdType ptId = ids[i].PtId;
            this->MergePoint(ptId, nearby);
          } // for all points in bin/bucket
        } // if points exist in bin/bucket
      } // for all blocks
    } // operator()

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
      const vtkLocatorTuple<TIds>* ids;
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
                  } // if points geometrically coincident
                } // if point not yet visited
              } // for the remaining points in the bin
            } // if point not yet merged
          } // for all points in bucket
        } // if bucket contains points
      } // for all buckets
    } // operator()

    void Reduce() {}
  }; // MergePointsWithData

  // Build the map and other structures to support locator operations
  void BuildLocator() override
  {
    // Place each point in a bucket
    auto points = this->DataSet->GetPoints()->GetData();
    MapPointsArrayWorker worker;
    if (!vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::AllPointArrays>::Execute(
          points, worker, this))
    {
      worker(points, this);
    }

    // Provide accelerated access to points. Needed for Voronoi bin iterators.
    if (vtkDoubleArray::SafeDownCast(points))
    {
      this->FastPoints = static_cast<double*>(vtkDoubleArray::SafeDownCast(points)->GetPointer(0));
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
// Iteration over the bins of a vtkStaticPointLocator in blocks of increasing
// levels around a center bin: the block of level L is the cube of bins
// (2L+1)^3 around the center bin, and only its boundary bins (the bins not in
// the block of level L-1) are visited, each once. Used by
// vtkStaticPointLocator::FindPointsInShell() and vtkShellBinIterator.
//------------------------------------------------------------------------------
// Iterate over a *specified face* of a block of bins. Iteration occurs by
// looping around the face center point at increasing face levels. Looping
// continues until the face level is equal to the block level. This struct is
// used in conjunction with vtkBucketBlockIterator to visit bins closer to a
// generating point, and to ping-pong iterate over -/+ face points. The block
// size, and therefore the sizes of the x, y, z faces, is specified by
// blockLevel.
struct vtkBucketBlockFaceIterator
{
  // The current state of iteration.
  int BlockLevel;    // the current block over which iteration is occuring
  int FaceCenter[3]; // the center of iteration of the current block face
  int FaceNum;       // the face being iterated over: 0-xface,1-yface,2-zface
  int FaceLevel;     // the current face level over which we are iterating
  int Span;          // the topological width of the block at the current face level
  int EdgeNum;       // four edges of each face form a loop, the current edge
  int NumIncs;       // the number of iteration steps over the current edge
  int IncNum;        // the current position along a block face edge
  int Inc0[3];       // the index increment in the face "i" direction
  int Inc1[3];       // the index increment in the face "j" direction
  int IJK[3];        // the current IJK index of iteration

  // Constructor requires block face type: i, j, or k.
  vtkBucketBlockFaceIterator(int faceNum)
  {
    // The face being iterated over: -i, -j, or -k. Ping-ponging
    // is used to iterate over the +i, +j, or +k block faces.
    this->FaceNum = faceNum;

    // Specify the increments to move to the next bin during face looping.
    if (this->FaceNum == 0) // -i face
    {
      this->Inc0[0] = 0;
      this->Inc0[1] = 1;
      this->Inc0[2] = 0;
      this->Inc1[0] = 0;
      this->Inc1[1] = 0;
      this->Inc1[2] = 1;
    }
    else if (this->FaceNum == 1) // -j face
    {
      this->Inc0[0] = 1;
      this->Inc0[1] = 0;
      this->Inc0[2] = 0;
      this->Inc1[0] = 0;
      this->Inc1[1] = 0;
      this->Inc1[2] = 1;
    }
    else // if ( this->FaceNum == 0 ) // -k face
    {
      this->Inc0[0] = 1;
      this->Inc0[1] = 0;
      this->Inc0[2] = 0;
      this->Inc1[0] = 0;
      this->Inc1[1] = 1;
      this->Inc1[2] = 0;
    }
  }

  // Update the IJK index.
  void GetIJK(int ijk[3])
  {
    ijk[0] = this->IJK[0];
    ijk[1] = this->IJK[1];
    ijk[2] = this->IJK[2];
  }

  // The following methods are used for looping around the face center point in
  // a counterclockwise direction.
  void MoveRight()
  {
    this->IJK[0] += this->Inc0[0];
    this->IJK[1] += this->Inc0[1];
    this->IJK[2] += this->Inc0[2];
  }
  void MoveUp()
  {
    this->IJK[0] += this->Inc1[0];
    this->IJK[1] += this->Inc1[1];
    this->IJK[2] += this->Inc1[2];
  }
  void MoveLeft()
  {
    this->IJK[0] -= this->Inc0[0];
    this->IJK[1] -= this->Inc0[1];
    this->IJK[2] -= this->Inc0[2];
  }
  void MoveDown()
  {
    this->IJK[0] -= this->Inc1[0];
    this->IJK[1] -= this->Inc1[1];
    this->IJK[2] -= this->Inc1[2];
  }

  // The i,j,k faces of the block overlap along edges and at vertices. To
  // prevent retrieval of bins more than once, we make sure that potentially
  // overlapping vertices are only retrieved once.
  bool IsValid()
  {
    // The i-face always produces a valid IJK. Any i,j,k face loops not
    // touching the block boundary edges are always valid as well.
    if (this->FaceNum == 0 || this->FaceLevel < this->BlockLevel)
    {
      return true;
    }

    // The j-face may revisit some edge bins (due to overlap with the
    // +/- i face edges). So check the +/- i extremes for overlap.
    else if (this->FaceNum == 1)
    {
      if (this->IJK[0] == (this->FaceCenter[0] - this->FaceLevel) ||
        this->IJK[0] == (this->FaceCenter[0] + this->FaceLevel))
      {
        return false;
      }
    }

    // The k-face may revisit some edge bins (due to overlap with the
    // +/- i and +/- j face edges). Note: this code should not be
    // visited since during iteration we prevent processing the outer
    // k-face loop altogether, it's included for debugging purposes.
    else // if ( this->FaceNum == 2 )
    {
      if (this->IJK[0] == (this->FaceCenter[0] - this->FaceLevel) ||
        this->IJK[0] == (this->FaceCenter[0] + this->FaceLevel) ||
        this->IJK[1] == (this->FaceCenter[1] - this->FaceLevel) ||
        this->IJK[1] == (this->FaceCenter[1] + this->FaceLevel))
      {
        return false;
      }
    }

    return true;
  }

  // Initialize the block face iteration process. Indicate the
  // center of the block, and the block level. This is used to
  // specify the face center of iteration.
  void Initialize(int blockLevel, int blockCenter[3])
  {
    this->BlockLevel = blockLevel;
    if (this->FaceNum == 0) // -i face
    {
      this->FaceCenter[0] = blockCenter[0] - blockLevel;
      this->FaceCenter[1] = blockCenter[1];
      this->FaceCenter[2] = blockCenter[2];
    }
    else if (this->FaceNum == 1) // -j face
    {
      this->FaceCenter[0] = blockCenter[0];
      this->FaceCenter[1] = blockCenter[1] - blockLevel;
      this->FaceCenter[2] = blockCenter[2];
    }
    else // if ( this->FaceNum == 0 ) // -k face
    {
      this->FaceCenter[0] = blockCenter[0];
      this->FaceCenter[1] = blockCenter[1];
      this->FaceCenter[2] = blockCenter[2] - blockLevel;
    }
  }

  // Internal methods for the face iteration process follow below. These just
  // visit the bins on the face of a block without considering topological
  // validity.  The iteration starts at the face center (level==0), and then
  // grows outward one level at a time until the all bins associated with the
  // face are visited. This method updates IJK[3] which is the current bin.
  void BeginBin(int faceLevel)
  {
    // The current level of face iteration
    this->FaceLevel = faceLevel;

    // Special case: starting at the center of the current face.
    if (faceLevel == 0)
    {
      this->IJK[0] = this->FaceCenter[0];
      this->IJK[1] = this->FaceCenter[1];
      this->IJK[2] = this->FaceCenter[2];
      return;
    }

    // General case: iterate over the four edges of the face (basically
    // traverse loops around the face center). We start at the lower left of the
    // current level, moving horizontally NumIncs steps before turning vertically
    // along the next edge.
    this->Span = 2 * faceLevel + 1;

    // Specify various iteration parameters, and the initial starting position
    this->EdgeNum = 0;
    this->IncNum = 0;
    this->NumIncs = this->Span - 1;

    if (this->FaceNum == 0) // -i face
    {
      this->IJK[0] = this->FaceCenter[0];
      this->IJK[1] = this->FaceCenter[1] - faceLevel + 1;
      this->IJK[2] = this->FaceCenter[2] - faceLevel;
    }
    else if (this->FaceNum == 1) // -j face
    {
      this->IJK[0] = this->FaceCenter[0] - faceLevel + 1;
      this->IJK[1] = this->FaceCenter[1];
      this->IJK[2] = this->FaceCenter[2] - faceLevel;
    }
    else // if ( this->FaceNum == 2 ) // -k face
    {
      this->IJK[0] = this->FaceCenter[0] - faceLevel + 1;
      this->IJK[1] = this->FaceCenter[1] - faceLevel;
      this->IJK[2] = this->FaceCenter[2];
    }
  }

  // Return false when iteration over the current faceLevel loop is complete.
  bool NextBin()
  {
    // Special case for face level == 0.
    if (this->FaceLevel == 0)
    {
      return false;
    }

    // The k-face special case for skipping the whole outer loop on
    // the k block face.
    if (this->FaceNum == 2 && this->FaceLevel == this->BlockLevel)
    {
      return false;
    }

    // Now for the general case. We traverse the four edges that
    // form a loop of bins around the face center point.
    this->IncNum++; // advance along current edge
    switch (this->EdgeNum)
    {
      case 0: // lower edge
        if (this->IncNum < this->NumIncs)
        {
          this->MoveRight();
        }
        else // move to next edge
        {
          this->EdgeNum = 1;
          this->IncNum = 0;
          this->MoveUp();
        }
        break;
      case 1: // right edge
        if (this->IncNum < this->NumIncs)
        {
          this->MoveUp();
        }
        else // move to next edge
        {
          this->EdgeNum = 2;
          this->IncNum = 0;
          this->MoveLeft();
        }
        break;
      case 2: // upper edge
        if (this->IncNum < this->NumIncs)
        {
          this->MoveLeft();
        }
        else // move to next edge
        {
          this->EdgeNum = 3;
          this->IncNum = 0;
          this->MoveDown();
        }
        break;
      case 3: // left edge
        if (this->IncNum < this->NumIncs)
        {
          this->MoveDown();
        }
        else // move to next edge
        {
          return false;
        }
        break;
      default: // terminate loop
        return false;
    }

    return true;
  }

  // Basically just sets the iteration starting point for the face loops
  // at the i face level==0.
  void Begin(int ijk[3])
  {
    this->BeginBin(0);
    this->GetIJK(ijk);
  }

  // Return false when iteration over the entirety of the current face
  // completes.  Rectangular rings of nested block face loops are processed
  // until all bins on the face are visited. When all face loops have been
  // processed, then return false;
  bool Next(int ijk[3])
  {
    // Loop until valid IJK is produced
    while (this->NextBin()) // returns false when level is complete
    {
      if (this->IsValid())
      {
        this->GetIJK(ijk);
        return true;
      }
    }

    // If here, the current loop at face level has been exhausted. Start the next
    // face loop at the next level, or check and see if all face loops have been
    // visited.
    if (this->FaceLevel < this->BlockLevel)
    {
      this->BeginBin(this->FaceLevel + 1);
      this->GetIJK(ijk);
      return true;
    }
    else
    {
      return false;
    }
  } // Next()
}; // vtkBucketBlockFaceIterator

//------------------------------------------------------------------------------
// Hard coded block traversals for lower levels. Note the ping-pong
// order (traversal of opposite bins).
inline constexpr int vtkBucketBlockLevel1[26][3] = { { -1, 0, 0 }, { 1, 0, 0 }, { 0, -1, 0 },
  { 0, 1, 0 }, { 0, 0, -1 }, { 0, 0, 1 },                     // six face centers
  { 0, -1, -1 }, { 0, 1, 1 }, { 0, 1, -1 }, { 0, -1, 1 },     // centers of x-edges
  { -1, 0, -1 }, { 1, 0, 1 }, { 1, 0, -1 }, { -1, 0, 1 },     // centers of y-edges
  { -1, -1, 0 }, { 1, 1, 0 }, { 1, -1, 0 }, { -1, 1, 0 },     // centers of z-edges
  { -1, -1, -1 }, { 1, 1, 1 }, { 1, -1, -1 }, { -1, 1, 1 },   // eight corners
  { -1, 1, -1 }, { 1, -1, 1 }, { 1, 1, -1 }, { -1, -1, 1 } }; // Level1

// Face centers, spiraling out with ping-pong traversal.
inline constexpr int vtkBucketBlockLevel2[98][3] = { { -2, 0, 0 }, { 2, 0, 0 }, { 0, -2, 0 },
  { 0, 2, 0 }, { 0, 0, -2 }, { 0, 0, 2 }, { -2, 0, -1 }, { 2, 0, -1 }, { 0, -2, -1 }, { 0, 2, -1 },
  { 0, -1, -2 }, { 0, -1, 2 }, { -2, 1, -1 }, { 2, 1, -1 }, { 1, -2, -1 }, { 1, 2, -1 },
  { 1, -1, -2 }, { 1, -1, 2 }, { -2, 1, 0 }, { 2, 1, 0 }, { 1, -2, 0 }, { 1, 2, 0 }, { 1, 0, -2 },
  { 1, 0, 2 }, { -2, 1, 1 }, { 2, 1, 1 }, { 1, -2, 1 }, { 1, 2, 1 }, { 1, 1, -2 }, { 1, 1, 2 },
  { -2, 0, 1 }, { 2, 0, 1 }, { 0, -2, 1 }, { 0, 2, 1 }, { 0, 1, -2 }, { 0, 1, 2 }, { -2, -1, 1 },
  { 2, -1, 1 }, { -1, -2, 1 }, { -1, 2, 1 }, { -1, 1, -2 }, { -1, 1, 2 }, { -2, -1, 0 },
  { 2, -1, 0 }, { -1, -2, 0 }, { -1, 2, 0 }, { -1, 0, -2 }, { -1, 0, 2 }, { -2, -1, -1 },
  { 2, -1, -1 }, { -1, -2, -1 }, { -1, 2, -1 }, { -1, -1, -2 }, { -1, -1, 2 }, { -1, -2, -2 },
  { -1, 2, -2 }, { -2, -1, -2 }, { 2, -1, -2 }, { 0, -2, -2 }, { 0, 2, -2 }, { -2, 0, -2 },
  { 2, 0, -2 }, { 1, -2, -2 }, { 1, 2, -2 }, { -2, 1, -2 }, { 2, 1, -2 }, { 1, -2, 2 }, { 1, 2, 2 },
  { -2, 2, -2 }, { 2, 2, -2 }, { 0, -2, 2 }, { 0, 2, 2 }, { -2, 2, -1 }, { 2, 2, -1 },
  { -1, -2, 2 }, { -1, 2, 2 }, { -2, 2, 0 }, { 2, 2, 0 }, { -2, 2, 1 }, { 2, 2, 1 }, { -2, 2, 2 },
  { 2, 2, 2 }, { -2, 1, 2 }, { 2, 1, 2 }, { -2, 0, 2 }, { 2, 0, 2 }, { -2, -1, 2 }, { 2, -1, 2 },
  { -2, -2, 2 }, { 2, -2, 2 }, { -2, -2, 1 }, { 2, -2, 1 }, { -2, -2, 0 }, { 2, -2, 0 },
  { -2, -2, -1 }, { 2, -2, -1 }, { -2, -2, -2 }, { 2, -2, -2 } }; // Level2

inline constexpr int vtkBucketBlockLevel3[218][3] = { { -3, 0, 0 }, { 3, 0, 0 }, { 0, -3, 0 },
  { 0, 3, 0 }, { 0, 0, -3 }, { 0, 0, 3 }, { -3, 0, -1 }, { 3, 0, -1 }, { 0, -3, -1 }, { 0, 3, -1 },
  { 0, -1, -3 }, { 0, -1, 3 }, { -3, 1, -1 }, { 3, 1, -1 }, { 1, -3, -1 }, { 1, 3, -1 },
  { 1, -1, -3 }, { 1, -1, 3 }, { -3, 1, 0 }, { 3, 1, 0 }, { 1, -3, 0 }, { 1, 3, 0 }, { 1, 0, -3 },
  { 1, 0, 3 }, { -3, 1, 1 }, { 3, 1, 1 }, { 1, -3, 1 }, { 1, 3, 1 }, { 1, 1, -3 }, { 1, 1, 3 },
  { -3, 0, 1 }, { 3, 0, 1 }, { 0, -3, 1 }, { 0, 3, 1 }, { 0, 1, -3 }, { 0, 1, 3 }, { -3, -1, 1 },
  { 3, -1, 1 }, { -1, -3, 1 }, { -1, 3, 1 }, { -1, 1, -3 }, { -1, 1, 3 }, { -3, -1, 0 },
  { 3, -1, 0 }, { -1, -3, 0 }, { -1, 3, 0 }, { -1, 0, -3 }, { -1, 0, 3 }, { -3, -1, -1 },
  { 3, -1, -1 }, { -1, -3, -1 }, { -1, 3, -1 }, { -1, -1, -3 }, { -1, -1, 3 }, { -3, -1, -2 },
  { 3, -1, -2 }, { -1, -3, -2 }, { -1, 3, -2 }, { -1, -2, -3 }, { -1, -2, 3 }, { -3, 0, -2 },
  { 3, 0, -2 }, { 0, -3, -2 }, { 0, 3, -2 }, { 0, -2, -3 }, { 0, -2, 3 }, { -3, 1, -2 },
  { 3, 1, -2 }, { 1, -3, -2 }, { 1, 3, -2 }, { 1, -2, -3 }, { 1, -2, 3 }, { -3, 2, -2 },
  { 3, 2, -2 }, { 2, -3, -2 }, { 2, 3, -2 }, { 2, -2, -3 }, { 2, -2, 3 }, { -3, 2, -1 },
  { 3, 2, -1 }, { 2, -3, -1 }, { 2, 3, -1 }, { 2, -1, -3 }, { 2, -1, 3 }, { -3, 2, 0 }, { 3, 2, 0 },
  { 2, -3, 0 }, { 2, 3, 0 }, { 2, 0, -3 }, { 2, 0, 3 }, { -3, 2, 1 }, { 3, 2, 1 }, { 2, -3, 1 },
  { 2, 3, 1 }, { 2, 1, -3 }, { 2, 1, 3 }, { -3, 2, 2 }, { 3, 2, 2 }, { 2, -3, 2 }, { 2, 3, 2 },
  { 2, 2, -3 }, { 2, 2, 3 }, { -3, 1, 2 }, { 3, 1, 2 }, { 1, -3, 2 }, { 1, 3, 2 }, { 1, 2, -3 },
  { 1, 2, 3 }, { -3, 0, 2 }, { 3, 0, 2 }, { 0, -3, 2 }, { 0, 3, 2 }, { 0, 2, -3 }, { 0, 2, 3 },
  { -3, -1, 2 }, { 3, -1, 2 }, { -1, -3, 2 }, { -1, 3, 2 }, { -1, 2, -3 }, { -1, 2, 3 },
  { -3, -2, 2 }, { 3, -2, 2 }, { -2, -3, 2 }, { -2, 3, 2 }, { -2, 2, -3 }, { -2, 2, 3 },
  { -3, -2, 1 }, { 3, -2, 1 }, { -2, -3, 1 }, { -2, 3, 1 }, { -2, 1, -3 }, { -2, 1, 3 },
  { -3, -2, 0 }, { 3, -2, 0 }, { -2, -3, 0 }, { -2, 3, 0 }, { -2, 0, -3 }, { -2, 0, 3 },
  { -3, -2, -1 }, { 3, -2, -1 }, { -2, -3, -1 }, { -2, 3, -1 }, { -2, -1, -3 }, { -2, -1, 3 },
  { -3, -2, -2 }, { 3, -2, -2 }, { -2, -3, -2 }, { -2, 3, -2 }, { -2, -2, -3 }, { -2, -2, 3 },
  { -2, -3, -3 }, { -2, 3, -3 }, { -3, -2, -3 }, { 3, -2, -3 }, { -1, -3, -3 }, { -1, 3, -3 },
  { -3, -1, -3 }, { 3, -1, -3 }, { 0, -3, -3 }, { 0, 3, -3 }, { -3, 0, -3 }, { 3, 0, -3 },
  { 1, -3, -3 }, { 1, 3, -3 }, { -3, 1, -3 }, { 3, 1, -3 }, { 2, -3, -3 }, { 2, 3, -3 },
  { -3, 2, -3 }, { 3, 2, -3 }, { 2, -3, 3 }, { 2, 3, 3 }, { -3, 3, -3 }, { 3, 3, -3 }, { 1, -3, 3 },
  { 1, 3, 3 }, { -3, 3, -2 }, { 3, 3, -2 }, { 0, -3, 3 }, { 0, 3, 3 }, { -3, 3, -1 }, { 3, 3, -1 },
  { -1, -3, 3 }, { -1, 3, 3 }, { -3, 3, 0 }, { 3, 3, 0 }, { -2, -3, 3 }, { -2, 3, 3 }, { -3, 3, 1 },
  { 3, 3, 1 }, { -3, 3, 2 }, { 3, 3, 2 }, { -3, 3, 3 }, { 3, 3, 3 }, { -3, 2, 3 }, { 3, 2, 3 },
  { -3, 1, 3 }, { 3, 1, 3 }, { -3, 0, 3 }, { 3, 0, 3 }, { -3, -1, 3 }, { 3, -1, 3 }, { -3, -2, 3 },
  { 3, -2, 3 }, { -3, -3, 3 }, { 3, -3, 3 }, { -3, -3, 2 }, { 3, -3, 2 }, { -3, -3, 1 },
  { 3, -3, 1 }, { -3, -3, 0 }, { 3, -3, 0 }, { -3, -3, -1 }, { 3, -3, -1 }, { -3, -3, -2 },
  { 3, -3, -2 }, { -3, -3, -3 }, { 3, -3, -3 } }; // Level3

//------------------------------------------------------------------------------
// Iterate over the boundary bins of a block of bins. The block is centered
// at Center, with width/height/depth of 2*Level+1. Note that this iterator
// deals strictly with topological concerns (iteration, and inclusion in
// topological space). For performance, block levels==0 and ==1 are hard
// coded to return the bins in near-optimal order (i.e., bins closer to the
// block center are returned first). The general iteration case (level>1) is
// performed using the vtkBucketBlockFaceIterator.
struct vtkBucketBlockIterator
{
  int Divs[3];        // the topology of a regular (locator) binning
  vtkIdType BinSlice; // the size of a slice of bins
  int MaxLevel;       // maximum level of iteration for the block+center

  // The current state of iteration.
  int Level;     // the current level over which we are iterating
  int Center[3]; // the center of iteration of the block
  int Span;      // the topological width of the block at current level
  int NumBins;   // total number of bins to iterate over at current level
  int BinNum;    // the current iteration position
  int Interior;  // indicate whether (current bin+level) is interior to the block
  int IJK[3];    // the current IJK index of iteration

  // Traversal info to support level>1 general iteration over bins on the
  // faces of the block. We need only the -i, -j, and -k block faces; using
  // ping-ponging (to opposite face) we visit the +i, +j, and +k faces as well.
  int BinNumRange[4];
  vtkBucketBlockFaceIterator IFace, JFace, KFace;

  // Constructor defines three face types (-i, -j, -k).
  vtkBucketBlockIterator()
    : IFace(0)
    , JFace(1)
    , KFace(2)
  {
  }

  // Convenience method to get the current bin id from the current bin IJK.
  vtkIdType GetBinId()
  {
    return (this->IJK[0] + this->IJK[1] * this->Divs[0] + this->IJK[2] * this->BinSlice);
  }

  // Determine if the current bin is within the locator binning.
  bool IsValid()
  {
    if (this->Interior)
    {
      return true;
    }

    if (this->IJK[0] < 0 || this->IJK[0] >= this->Divs[0] || this->IJK[1] < 0 ||
      this->IJK[1] >= this->Divs[1] || this->IJK[2] < 0 || this->IJK[2] >= this->Divs[2])
    {
      return false;
    }
    return true;
  }

  // Initialize the iterator. It is required that the center is
  // within the topological binning specified by divs[3].
  void Initialize(int divs[3], int maxLevel)
  {
    this->Divs[0] = divs[0];
    this->Divs[1] = divs[1];
    this->Divs[2] = divs[2];
    this->BinSlice = divs[0] * divs[1];
    this->MaxLevel = maxLevel;
  }

  // Internal methods for the iteration process follow below. These just
  // visit the surface of the block without considering topological validity.
  // Begin processing the block at the given level. Bins are returned in
  // this->IJK[3]. The level must be >=0, and the center within the specified
  // Divs[3].
  void BeginBin(int level, int center[3])
  {
    // The level being iterated over
    this->Level = level;

    // The center of iteration in topological coordinates.
    this->Center[0] = center[0];
    this->Center[1] = center[1];
    this->Center[2] = center[2];

    // Starting iteration bin is always 0
    this->BinNum = 0;

    // Special case for level 0.
    if (level == 0)
    {
      this->NumBins = 1;
      this->IJK[0] = this->Center[0];
      this->IJK[1] = this->Center[1];
      this->IJK[2] = this->Center[2];
      return;
    }

    // The span. This comes into play later to process edge and face bins
    // when the level>1.
    this->Span = 2 * level + 1;

    // The total number of bins to process
    this->NumBins = (level == 0 ? 1
                                : (this->Span * this->Span * this->Span) -
          ((2 * (level - 1) + 1) * (2 * (level - 1) + 1) * (2 * (level - 1) + 1)));

    // Determine whether the iteration region is inside of the locator. If
    // the iteration region overlaps the boundary (i.e., is not interior),
    // then bin validity checks have to be performed.
    this->Interior = true;
    if ((this->Center[0] - level) < 0 || (this->Center[0] + level) >= this->Divs[0] ||
      (this->Center[1] - level) < 0 || (this->Center[1] + level) >= this->Divs[1] ||
      (this->Center[2] - level) < 0 || (this->Center[2] + level) >= this->Divs[2])
    {
      this->Interior = false;
    }

    // Special case for levels 1-3. The bins are traversed in ping-pong order
    // (-/+ face centers, edges, and then corner points).
    if (level == 1)
    {
      // Iteration always begins at the center of the (-i)-face.
      this->IJK[0] = this->Center[0] + vtkBucketBlockLevel1[0][0];
      this->IJK[1] = this->Center[1] + vtkBucketBlockLevel1[0][1];
      this->IJK[2] = this->Center[2] + vtkBucketBlockLevel1[0][2];
      return;
    }
    else if (level == 2)
    {
      // Iteration always begins at the center of the (-i)-face.
      this->IJK[0] = this->Center[0] + vtkBucketBlockLevel2[0][0];
      this->IJK[1] = this->Center[1] + vtkBucketBlockLevel2[0][1];
      this->IJK[2] = this->Center[2] + vtkBucketBlockLevel2[0][2];
      return;
    }
    else if (level == 3)
    {
      // Iteration always begins at the center of the (-i)-face.
      this->IJK[0] = this->Center[0] + vtkBucketBlockLevel3[0][0];
      this->IJK[1] = this->Center[1] + vtkBucketBlockLevel3[0][1];
      this->IJK[2] = this->Center[2] + vtkBucketBlockLevel3[0][2];
      return;
    }

    // Otherwise general case for level>3. Process the three i, j, k boundary
    // bins of the current cuboid iteration sublock using ping-pong
    // traversal.  Begin by setting up some iteration ranges for each of the
    // i, j, k boundary planes.
    this->BinNumRange[0] = 0;
    this->BinNumRange[1] = 2 * (this->Span * this->Span);
    this->BinNumRange[2] = this->BinNumRange[1] + (2 * this->Span * (this->Span - 2));
    this->BinNumRange[3] = this->BinNumRange[2] + (2 * (this->Span - 2) * (this->Span - 2));
    assert(this->BinNumRange[3] == this->NumBins);

    // Initialize the block face iteration process (for the general case).
    this->IFace.Initialize(level, center);
    this->JFace.Initialize(level, center);
    this->KFace.Initialize(level, center);

    // Start in the center of the i-face.
    this->IFace.Begin(this->IJK);
  } // BeginBin

  // Return false when iteration over the entire block is complete. We do not worry
  // at this point if the returned IJK is valid, this will be checked by the calling
  // Next() method.
  bool NextBin()
  {
    // Advance the current bin number
    this->BinNum++;

    // Special case for level 0, or if visited all bins in the current level
    int level = this->Level;
    if (level == 0 || this->BinNum >= this->NumBins || level > this->MaxLevel)
    {
      return false;
    }

    // Special case for levels 1-3
    if (level == 1)
    {
      this->IJK[0] = this->Center[0] + vtkBucketBlockLevel1[this->BinNum][0];
      this->IJK[1] = this->Center[1] + vtkBucketBlockLevel1[this->BinNum][1];
      this->IJK[2] = this->Center[2] + vtkBucketBlockLevel1[this->BinNum][2];
      return true;
    }
    else if (level == 2)
    {
      this->IJK[0] = this->Center[0] + vtkBucketBlockLevel2[this->BinNum][0];
      this->IJK[1] = this->Center[1] + vtkBucketBlockLevel2[this->BinNum][1];
      this->IJK[2] = this->Center[2] + vtkBucketBlockLevel2[this->BinNum][2];
      return true;
    }
    else if (level == 3)
    {
      this->IJK[0] = this->Center[0] + vtkBucketBlockLevel3[this->BinNum][0];
      this->IJK[1] = this->Center[1] + vtkBucketBlockLevel3[this->BinNum][1];
      this->IJK[2] = this->Center[2] + vtkBucketBlockLevel3[this->BinNum][2];
      return true;
    }

    // Otherwise The general case with level>3: process the points on the
    // boundary of the block.  We visit the -/+i, -/+j, -/+k planes in "round
    // robin" order, at the same time ping ponging from bins on the "-" plane
    // to opposite bins on the "+" plane.

    // The total number of bins on i+j+k faces; and the i+j faces.
    vtkIdType nijk = 3 * (this->BinNumRange[3] - this->BinNumRange[2]);
    vtkIdType nij = 2 * (2 * this->BinNumRange[2] - this->BinNumRange[1] - this->BinNumRange[3]);

    // Process the interior face bins on the +/- i,j, and k faces.
    // These are bins strictly interior to the i,j,k faces.
    const int binNum = this->BinNum;
    if (binNum < nijk)
    {
      const int idx = binNum % 6;
      switch (idx)
      {
        case 0: // The Begin() method has initialized IFace traversal
          this->IFace.Next(this->IJK);
          break;
        case 1:
          this->IJK[0] = this->Center[0] + level; // i-pong
          break;
        case 2:
          if (binNum == 2)
          {
            this->JFace.Begin(this->IJK); // Initialize JFace traversal
          }
          else
          {
            this->JFace.Next(this->IJK);
          }
          break;
        case 3:
          this->IJK[1] = this->Center[1] + level; // j-pong
          break;
        case 4:
          if (binNum == 4)
          {
            this->KFace.Begin(this->IJK); // Initialize KFace traversal
          }
          else
          {
            this->KFace.Next(this->IJK);
          }
          break;
        case 5:
          this->IJK[2] = this->Center[2] + level; // k-pong
          break;
      }
    }

    // Process the remaining bins on +/- i and j faces. These
    // remaining faces are along i,j face edges.
    else if (binNum < (nij + nijk))
    {
      const int idx = binNum % 4;
      switch (idx)
      {
        case 0:
          this->IFace.Next(this->IJK);
          break;
        case 1:
          this->IJK[0] = this->Center[0] + level; // i-pong
          break;
        case 2:
          this->JFace.Next(this->IJK);
          break;
        case 3:
          this->IJK[1] = this->Center[1] + level; // j-pong
          break;
      }
    }

    // Process the remaining bins on +/- i faces. These remaining bins
    // are along i face edges.
    else // if ( this->BinNum < (ni+nij+nijk) )
    {
      if (!(binNum % 2))
      {
        this->IFace.Next(this->IJK);
      }
      else
      {
        this->IJK[0] = this->Center[0] + level; // i-pong
      }
    }
    return true;
  } // NextBin

  // Return a valid binId at the iteration starting point.
  vtkIdType Begin(int blockLevel, int blockCenter[3])
  {
    // Now iterate over this block
    this->BeginBin(blockLevel, blockCenter);
    while (!this->IsValid())
    {
      this->NextBin();
    }
    return this->GetBinId();
  }

  // Return a valid binId, or a -1 if the iteration for this level is
  // complete. Note that the iteration occurs in a prescribed order, so
  // intermediate information is computed incrementally as needed.
  vtkIdType Next()
  {
    // Get the next bin and check whether it is valid. If so,
    // return it. Otherwise, get the next valid bin.
    while (this->BinNum < this->NumBins)
    {
      if (!this->NextBin())
      {
        return (-1);
      }
      if (this->IsValid())
      {
        return this->GetBinId();
      }
    }
    return (-1); // end of bins at current level
  }

}; // vtkBucketBlockIterator

VTK_ABI_NAMESPACE_END
#endif // vtkStaticPointLocatorPrivate_h
// VTK-HeaderTest-Exclude: vtkStaticPointLocatorPrivate.h
