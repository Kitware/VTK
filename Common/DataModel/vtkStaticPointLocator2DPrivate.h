// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2011 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkBucketList2D
 * @brief   Private declarations for 2D binned spatial locator
 *
 * The main purpose of this class is to enable access to the internals
 * of vtkStaticPointLocator2D, allowing complex iteration over locator bins.
 * For example, see vtkAnnularBinIterator.
 */

#ifndef vtkStaticPointLocator2DPrivate_h
#define vtkStaticPointLocator2DPrivate_h

#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchDataSetArrayList.h"
#include "vtkCellArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkPoints.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkStaticPointLocator2D.h"
#include "vtkStructuredData.h"

VTK_ABI_NAMESPACE_BEGIN

#define Distance2BetweenPoints2D(p1, p2)                                                           \
  ((p1[0] - p2[0]) * (p1[0] - p2[0]) + (p1[1] - p2[1]) * (p1[1] - p2[1]))

/**
 * Performant method to intersect a box with a circle. The box is defined
 * by (min,max) corners; the circle by (center,radius**2). Inspired by
 * Graphics Gems 1.
 */
inline bool IntersectsCircle(
  const double min[2], const double max[2], const double center[2], double r2)
{
  double d2 = 0.0;
  for (int i = 0; i < 2; ++i)
  {
    if (center[i] < min[i])
    {
      d2 += (center[i] - min[i]) * (center[i] - min[i]);
    }
    else if (center[i] > max[i])
    {
      d2 += (center[i] - max[i]) * (center[i] - max[i]);
    }
  }
  return (d2 <= r2);
}

/**
 * Performant method to determine if a box if fully inside a circle. The
 * box is defined by (min,max) corners; the circle by (center,radius**2).
 * Inspired by Graphics Gems 1.
 */
inline bool InsideCircle(
  const double min[2], const double max[2], const double center[2], double r2)
{
  double dmin = 0.0, dmax = 0.0;
  for (int i = 0; i < 2; ++i)
  {
    double a = (center[i] - min[i]) * (center[i] - min[i]);
    double b = (center[i] - max[i]) * (center[i] - max[i]);
    dmax += std::max(a, b);
    if (min[i] <= center[i] && center[i] <= max[i])
    {
      dmin += std::min(a, b);
    }
  }
  return (!(dmin <= r2 && r2 <= dmax));
}

//------------------------------------------------------------------------------
// The following code supports threaded point locator construction. The locator
// is assumed to be constructed once (i.e., it does not allow incremental point
// insertion). The algorithm proceeds in three steps:
// 1) All points are assigned a bucket index (combined i-j bucket location).
// The index is computed in parallel. This requires a one time allocation of an
// index array (which is also associated with the originating point ids).
// 2) vtkSMPTools::Sort() is used to sort the index array. Note that the sort
// carries along the point ids as well. This creates contiguous runs of points
// all resident in the same bucket.
// 3) The bucket offsets are updated to refer to the right entry location into
// the sorted point ids array. This enables quick access, and an indirect count
// of the number of points in each bucket.

struct NeighborBuckets2D;

//------------------------------------------------------------------------------
// The bucketed points, including the sorted map. This is just a PIMPLd
// wrapper around the classes that do the real work.
struct vtkBucketList2D
{
  vtkStaticPointLocator2D* Locator; // locater
  vtkIdType NumPts;                 // the number of points to bucket
  vtkIdType NumBuckets;
  int BatchSize;

  // These are internal data members used for performance reasons
  vtkDataSet* DataSet;
  int Divisions[3];
  double Bounds[6];
  double H[3];
  double hX, hY, hX2, hY2;
  double fX, fY, bX, bY;
  vtkIdType xD, yD, zD;

  // Used for accelerated performance for certain methods
  double* FastPoints; // fast path for accessing points
  double BinRadius;   // circumradius of a single bin/bucket
  int MaxLevel;       // the maximum possible level searches can proceed

  // Construction
  vtkBucketList2D(vtkStaticPointLocator2D* loc, vtkIdType numPts, int numBuckets)
  {
    this->Locator = loc;
    this->NumPts = numPts;
    this->NumBuckets = numBuckets;
    this->BatchSize = 10000; // building the offset array
    this->DataSet = loc->GetDataSet();

    // Setup internal data members for more efficient processing. Remember this is
    // a 2D locator so just processing (x,y) points.
    double spacing[3], bounds[6];
    loc->GetDivisions(this->Divisions);
    loc->GetSpacing(spacing);
    loc->GetBounds(bounds);
    this->hX = this->H[0] = spacing[0];
    this->hY = this->H[1] = spacing[1];
    this->hX2 = this->hX / 2.0;
    this->hY2 = this->hY / 2.0;
    this->fX = 1.0 / spacing[0];
    this->fY = 1.0 / spacing[1];
    this->bX = this->Bounds[0] = bounds[0];
    this->Bounds[1] = bounds[1];
    this->bY = this->Bounds[2] = bounds[2];
    this->Bounds[3] = bounds[3];
    this->xD = this->Divisions[0];
    this->yD = this->Divisions[1];
    this->zD = 1;

    this->FastPoints = nullptr;
    this->BinRadius = sqrt(hX * hX + hY * hY) / 2.0;
    this->MaxLevel = std::max({ this->xD, this->yD, this->zD });
  }

  // Virtuals for templated subclasses
  virtual ~vtkBucketList2D() = default;
  virtual void BuildLocator() = 0;

  // place points in appropriate buckets
  void GetBucketNeighbors(
    NeighborBuckets2D* buckets, const int ij[2], const int ndivs[2], int level);
  void GenerateFace(int face, int i, int j, int k, vtkPoints* pts, vtkCellArray* polys);
  double Distance2ToBucket(const double x[3], const int nei[3]);
  double Distance2ToBounds(const double x[3], const double bounds[6]);

  //-----------------------------------------------------------------------------
  // Inlined for performance. These function invocations must be called after
  // BuildLocator() is invoked, otherwise the output is indeterminate.
  void GetBucketIndices(const double* x, int ij[2]) const
  {
    // Compute point index. Make sure it lies within range of locator.
    vtkIdType tmp0 = static_cast<vtkIdType>(((x[0] - bX) * fX));
    vtkIdType tmp1 = static_cast<vtkIdType>(((x[1] - bY) * fY));

    ij[0] = std::min(std::max<vtkIdType>(tmp0, 0), xD - 1);
    ij[1] = std::min(std::max<vtkIdType>(tmp1, 0), yD - 1);
  }

  //-----------------------------------------------------------------------------
  vtkIdType GetBucketIndex(const double* x) const
  {
    int ij[2];
    this->GetBucketIndices(x, ij);
    return ij[0] + ij[1] * xD;
  }

  //-----------------------------------------------------------------------------
  // Return the center of the bucket/bin at (i,j). Note, returns a 2D point
  // center[2].
  void GetBucketCenter(int i, int j, double center[3])
  {
    center[0] = this->bX + this->hX2 + i * this->hX;
    center[1] = this->bY + this->hY2 + j * this->hY;
    center[2] = 0.0;
  }

  //-----------------------------------------------------------------------------
  // Return the bounding box (min,max) of a specified bucket (i,j,k).
  void GetBucketBounds(int i, int j, double min[3], double max[3])
  {
    min[0] = this->bX + i * this->hX;
    min[1] = this->bY + j * this->hY;
    min[2] = 0.0;
    max[0] = min[0] + this->hX;
    max[1] = min[1] + this->hY;
    max[2] = 0.0;
  }
}; // vtkBucketList2D

//------------------------------------------------------------------------------
// This templated class manages the creation of the static locator
// structures. It also implements the operator() functors which are supplied
// to vtkSMPTools for threaded processesing.
template <typename TIds>
struct BucketList2D : public vtkBucketList2D
{
  // Okay the various ivars
  vtkLocatorTuple<TIds>* Map; // the map to be sorted
  TIds* Offsets;              // offsets for each bucket into the map

  // Construction
  BucketList2D(vtkStaticPointLocator2D* loc, vtkIdType numPts, int numBuckets)
    : vtkBucketList2D(loc, numPts, numBuckets)
  {
    // one extra to simplify traversal
    this->Map = new vtkLocatorTuple<TIds>[numPts + 1];
    this->Map[numPts].Bucket = numBuckets;
    this->Offsets = new TIds[numBuckets + 1];
    this->Offsets[numBuckets] = numPts;
  }

  // Release allocated memory
  ~BucketList2D() override
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
  double FindNPointsInAnnulus(int N, const double x[3], vtkDist2TupleArray& results,
    double minDist2 = (-0.1), bool sort = true, vtkDoubleArray* petals = nullptr);
  void FindPointsWithinRadius(double R, const double x[3], vtkIdList* result);
  int IntersectWithLine(double a0[3], double a1[3], double tol, double& t, double lineX[3],
    double ptX[3], vtkIdType& ptId);
  double FindCloseNBoundedPoints(int N, const double x[3], vtkIdList* result);

  void MergePoints(double tol, vtkIdType* pointMap);
  void GenerateRepresentation(int vtkNotUsed(level), vtkPolyData* pd);

  // Internal methods
  bool BucketIntersectsCircle(int i, int j, const double center[3], double R2);
  void GetOverlappingBuckets(
    NeighborBuckets2D* buckets, const double x[3], const int ij[2], double dist, int level);
  void GetOverlappingBuckets(NeighborBuckets2D* buckets, const double x[3], double dist,
    int prevMinLevel[2], int prevMaxLevel[2]);

  // Implicit point representation, slower path
  template <typename T>
  struct MapDataSet
  {
    BucketList2D<T>* BList;
    vtkDataSet* DataSet;

    MapDataSet(BucketList2D<T>* blist, vtkDataSet* ds)
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
        t->PtId = ptId;
        t->Bucket = this->BList->GetBucketIndex(p);
      } // for all points in this batch
    }
  };

  template <typename T, typename TPointsArray>
  struct MapPointsArray
  {
    BucketList2D<T>* BList;
    TPointsArray* Points;

    MapPointsArray(BucketList2D<T>* blist, TPointsArray* pts)
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
        t->PtId = ptId;
        t->Bucket = this->BList->GetBucketIndex(p);
      } // for all points in this batch
    }
  };

  struct MapPointsArrayWorker
  {
    template <typename TPointsArray>
    void operator()(TPointsArray* points, BucketList2D* blist)
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
    BucketList2D<T>* BList;
    vtkIdType NumPts;
    int NumBuckets;

    MapOffsets(BucketList2D<T>* blist)
      : BList(blist)
    {
      this->NumPts = this->BList->NumPts;
      this->NumBuckets = this->BList->NumBuckets;
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
    BucketList2D<T>* BList;
    vtkDataSet* DataSet;
    vtkIdType* MergeMap;

    MergePrecise(BucketList2D<T>* blist, vtkIdType* mergeMap)
      : BList(blist)
      , MergeMap(mergeMap)
    {
      this->DataSet = blist->DataSet;
    }

    void operator()(vtkIdType bucket, vtkIdType endBucket)
    {
      BucketList2D<T>* bList = this->BList;
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
                  if (p[0] == p2[0] && p[1] == p2[1])
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

  // Merge points that are coincident within a tolerance. Operates in
  // parallel on points. Needs to check neighbor buckets which slows it down
  // considerably. Note that merging is one direction: larger ids are merged
  // to lower.
  template <typename T>
  struct MergeClose
  {
    BucketList2D<T>* BList;
    vtkDataSet* DataSet;
    vtkIdType* MergeMap;
    double Tol;

    vtkSMPThreadLocalObject<vtkIdList> PIds;

    MergeClose(BucketList2D<T>* blist, double tol, vtkIdType* mergeMap)
      : BList(blist)
      , MergeMap(mergeMap)
      , Tol(tol)
    {
      this->DataSet = blist->DataSet;
    }

    // Just allocate a little bit of memory to get started.
    void Initialize()
    {
      vtkIdList*& pIds = this->PIds.Local();
      pIds->Allocate(128); // allocate some memory
    }

    void operator()(vtkIdType ptId, vtkIdType endPtId)
    {
      BucketList2D<T>* bList = this->BList;
      vtkIdType* mergeMap = this->MergeMap;
      int i;
      double p[3];
      vtkIdType nearId, numIds;
      vtkIdList*& nearby = this->PIds.Local();

      for (; ptId < endPtId; ++ptId)
      {
        if (mergeMap[ptId] < 0)
        {
          mergeMap[ptId] = ptId;
          this->DataSet->GetPoint(ptId, p);
          bList->FindPointsWithinRadius(this->Tol, p, nearby);
          if ((numIds = nearby->GetNumberOfIds()) > 0)
          {
            for (i = 0; i < numIds; i++)
            {
              nearId = nearby->GetId(i);
              if (ptId < nearId && (mergeMap[nearId] < 0 || ptId < mergeMap[nearId]))
              {
                mergeMap[nearId] = ptId;
              }
            }
          }
        } // if point not yet processed
      }   // for all points in this batch
    }

    void Reduce() {}
  };

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

    // Now gather the points into contiguous runs in buckets
    //
    vtkSMPTools::Sort(this->Map, this->Map + this->NumPts);

    // Build the offsets into the Map. The offsets are the positions of
    // each bucket into the sorted list. They mark the beginning of the
    // list of points in each bucket. Amazingly, this can be done in
    // parallel.
    //
    int numBatches = static_cast<int>(ceil(static_cast<double>(this->NumPts) / this->BatchSize));
    MapOffsets<TIds> offMapper(this);
    vtkSMPTools::For(0, numBatches, offMapper);
  }
}; // BucketList2D

VTK_ABI_NAMESPACE_END
#endif // vtkStaticPointLocator2DPrivate_h
// VTK-HeaderTest-Exclude: vtkStaticPointLocator2DPrivate.h
