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
#include "vtkPoints.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkStaticPointLocator.h"
#include "vtkStructuredData.h"

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
    // Compute point index. Make sure it lies within range of locator.
    vtkIdType tmp0 = static_cast<vtkIdType>(((x[0] - bX) * fX));
    vtkIdType tmp1 = static_cast<vtkIdType>(((x[1] - bY) * fY));
    vtkIdType tmp2 = static_cast<vtkIdType>(((x[2] - bZ) * fZ));

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
  bool BucketInsideSphere(int i, int j, int k, double center[3], double r2)
  {
    double min[3], max[3];
    min[0] = this->bX + i * this->hX;
    min[1] = this->bY + j * this->hY;
    min[2] = this->bZ + k * this->hZ;
    max[0] += this->hX;
    max[1] += this->hY;
    max[2] += this->hZ;
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

VTK_ABI_NAMESPACE_END
#endif // vtkStaticPointLocatorPrivate_h
// VTK-HeaderTest-Exclude: vtkStaticPointLocatorPrivate.h
