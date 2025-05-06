//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_worklet_VertexClustering_h
#define viskores_worklet_VertexClustering_h

#include <viskores/BinaryOperators.h>
#include <viskores/BinaryPredicates.h>
#include <viskores/Types.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleDiscard.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Logging.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/DispatcherMapTopology.h>
#include <viskores/worklet/DispatcherReduceByKey.h>
#include <viskores/worklet/Keys.h>
#include <viskores/worklet/StableSortIndices.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>
#include <viskores/worklet/WorkletReduceByKey.h>

//#define __VISKORES_VERTEX_CLUSTERING_BENCHMARK

#ifdef __VISKORES_VERTEX_CLUSTERING_BENCHMARK
#include <viskores/cont/Timer.h>
#endif

namespace viskores
{
namespace worklet
{

namespace internal
{

/// Selects the representative point somewhat randomly from the pool of points
/// in a cluster.
struct SelectRepresentativePoint : public viskores::worklet::WorkletReduceByKey
{
  using ControlSignature = void(KeysIn clusterIds, ValuesIn points, ReducedValuesOut repPoints);
  using ExecutionSignature = _3(_2);
  using InputDomain = _1;

  template <typename PointsInVecType>
  VISKORES_EXEC typename PointsInVecType::ComponentType operator()(
    const PointsInVecType& pointsIn) const
  {
    // Grab the point from the middle of the set. This usually does a decent
    // job of selecting a representative point that won't emphasize the cluster
    // partitions.
    //
    // Note that we must use the stable sorting with the worklet::Keys for this
    // to be reproducible across backends.
    return pointsIn[pointsIn.GetNumberOfComponents() / 2];
  }

  struct RunTrampoline
  {
    template <typename InputPointsArrayType, typename KeyType>
    VISKORES_CONT void operator()(const InputPointsArrayType& points,
                                  const viskores::worklet::Keys<KeyType>& keys,
                                  viskores::cont::UnknownArrayHandle& output) const
    {

      viskores::cont::ArrayHandle<typename InputPointsArrayType::ValueType> out;
      viskores::worklet::DispatcherReduceByKey<SelectRepresentativePoint> dispatcher;
      dispatcher.Invoke(keys, points, out);

      output = out;
    }
  };

  template <typename KeyType, typename InputDynamicPointsArrayType>
  VISKORES_CONT static viskores::cont::UnknownArrayHandle Run(
    const viskores::worklet::Keys<KeyType>& keys,
    const InputDynamicPointsArrayType& inputPoints)
  {
    viskores::cont::UnknownArrayHandle output;
    RunTrampoline trampoline;
    viskores::cont::CastAndCall(inputPoints, trampoline, keys, output);
    return output;
  }
};

template <typename ValueType, typename StorageType, typename IndexArrayType>
VISKORES_CONT viskores::cont::ArrayHandle<ValueType> ConcretePermutationArray(
  const IndexArrayType& indices,
  const viskores::cont::ArrayHandle<ValueType, StorageType>& values)
{
  viskores::cont::ArrayHandle<ValueType> result;
  auto tmp = viskores::cont::make_ArrayHandlePermutation(indices, values);
  viskores::cont::ArrayCopy(tmp, result);
  return result;
}

template <typename T, viskores::IdComponent N>
viskores::cont::ArrayHandle<T> copyFromVec(
  viskores::cont::ArrayHandle<viskores::Vec<T, N>> const& other)
{
  const T* vmem = reinterpret_cast<const T*>(&*other.ReadPortal().GetIteratorBegin());
  viskores::cont::ArrayHandle<T> result =
    viskores::cont::make_ArrayHandle(vmem, other.GetNumberOfValues() * N, viskores::CopyFlag::On);
  return result;
}

} // namespace internal

struct VertexClustering
{
  struct GridInfo
  {
    viskores::Id3 dim;
    viskores::Vec3f_64 origin;
    viskores::Vec3f_64 bin_size;
    viskores::Vec3f_64 inv_bin_size;
  };

  // input: points  output: cid of the points
  class MapPointsWorklet : public viskores::worklet::WorkletMapField
  {
  private:
    GridInfo Grid;

  public:
    using ControlSignature = void(FieldIn, FieldOut);
    using ExecutionSignature = void(_1, _2);

    VISKORES_CONT
    MapPointsWorklet(const GridInfo& grid)
      : Grid(grid)
    {
    }

    /// determine grid resolution for clustering
    template <typename PointType>
    VISKORES_EXEC viskores::Id GetClusterId(const PointType& p) const
    {
      using ComponentType = typename PointType::ComponentType;
      PointType gridOrigin(static_cast<ComponentType>(this->Grid.origin[0]),
                           static_cast<ComponentType>(this->Grid.origin[1]),
                           static_cast<ComponentType>(this->Grid.origin[2]));

      PointType p_rel = (p - gridOrigin) * this->Grid.inv_bin_size;

      viskores::Id x = viskores::Min(static_cast<viskores::Id>(p_rel[0]), this->Grid.dim[0] - 1);
      viskores::Id y = viskores::Min(static_cast<viskores::Id>(p_rel[1]), this->Grid.dim[1] - 1);
      viskores::Id z = viskores::Min(static_cast<viskores::Id>(p_rel[2]), this->Grid.dim[2] - 1);

      return x + this->Grid.dim[0] * (y + this->Grid.dim[1] * z); // get a unique hash value
    }

    template <typename PointType>
    VISKORES_EXEC void operator()(const PointType& point, viskores::Id& cid) const
    {
      cid = this->GetClusterId(point);
      VISKORES_ASSERT(cid >= 0); // the id could overflow if too many cells
    }
  };

  class MapCellsWorklet : public viskores::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  FieldInPoint pointClusterIds,
                                  FieldOutCell cellClusterIds);
    using ExecutionSignature = void(_2, _3);

    VISKORES_CONT
    MapCellsWorklet() {}

    // TODO: Currently only works with Triangle cell types
    template <typename ClusterIdsVecType>
    VISKORES_EXEC void operator()(const ClusterIdsVecType& pointClusterIds,
                                  viskores::Id3& cellClusterId) const
    {
      cellClusterId[0] = pointClusterIds[0];
      cellClusterId[1] = pointClusterIds[1];
      cellClusterId[2] = pointClusterIds[2];
    }
  };

  /// pass 3
  class IndexingWorklet : public viskores::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn, WholeArrayOut);
    using ExecutionSignature = void(WorkIndex, _1, _2); // WorkIndex: use viskores indexing

    template <typename OutPortalType>
    VISKORES_EXEC void operator()(const viskores::Id& counter,
                                  const viskores::Id& cid,
                                  const OutPortalType& outPortal) const
    {
      outPortal.Set(cid, counter);
    }
  };

  class Cid2PointIdWorklet : public viskores::worklet::WorkletMapField
  {
    viskores::Id NPoints;

    VISKORES_EXEC
    void rotate(viskores::Id3& ids) const
    {
      viskores::Id temp = ids[0];
      ids[0] = ids[1];
      ids[1] = ids[2];
      ids[2] = temp;
    }

  public:
    using ControlSignature = void(FieldIn, FieldOut, WholeArrayIn);
    using ExecutionSignature = void(_1, _2, _3);

    VISKORES_CONT
    Cid2PointIdWorklet(viskores::Id nPoints)
      : NPoints(nPoints)
    {
    }

    template <typename InPortalType>
    VISKORES_EXEC void operator()(const viskores::Id3& cid3,
                                  viskores::Id3& pointId3,
                                  const InPortalType& inPortal) const
    {
      if (cid3[0] == cid3[1] || cid3[0] == cid3[2] || cid3[1] == cid3[2])
      {
        pointId3[0] = pointId3[1] = pointId3[2] = this->NPoints; // invalid cell to be removed
      }
      else
      {
        pointId3[0] = inPortal.Get(cid3[0]);
        pointId3[1] = inPortal.Get(cid3[1]);
        pointId3[2] = inPortal.Get(cid3[2]);

        // Sort triangle point ids so that the same triangle will have the same signature
        // Rotate these ids making the first one the smallest
        if (pointId3[0] > pointId3[1] || pointId3[0] > pointId3[2])
        {
          rotate(pointId3);
          if (pointId3[0] > pointId3[1] || pointId3[0] > pointId3[2])
          {
            rotate(pointId3);
          }
        }
      }
    }
  };

  using TypeInt64 = viskores::List<viskores::Int64>;

  class Cid3HashWorklet : public viskores::worklet::WorkletMapField
  {
  private:
    viskores::Int64 NPoints;

  public:
    using ControlSignature = void(FieldIn, FieldOut);
    using ExecutionSignature = void(_1, _2);

    VISKORES_CONT
    Cid3HashWorklet(viskores::Id nPoints)
      : NPoints(nPoints)
    {
    }

    VISKORES_EXEC
    void operator()(const viskores::Id3& cid, viskores::Int64& cidHash) const
    {
      cidHash =
        cid[0] + this->NPoints * (cid[1] + this->NPoints * cid[2]); // get a unique hash value
    }
  };

  class Cid3UnhashWorklet : public viskores::worklet::WorkletMapField
  {
  private:
    viskores::Int64 NPoints;

  public:
    using ControlSignature = void(FieldIn, FieldOut);
    using ExecutionSignature = void(_1, _2);

    VISKORES_CONT
    Cid3UnhashWorklet(viskores::Id nPoints)
      : NPoints(nPoints)
    {
    }

    VISKORES_EXEC
    void operator()(const viskores::Int64& cidHash, viskores::Id3& cid) const
    {
      cid[0] = static_cast<viskores::Id>(cidHash % this->NPoints);
      viskores::Int64 t = cidHash / this->NPoints;
      cid[1] = static_cast<viskores::Id>(t % this->NPoints);
      cid[2] = static_cast<viskores::Id>(t / this->NPoints);
    }
  };

public:
  ///////////////////////////////////////////////////
  /// \brief VertexClustering: Mesh simplification
  ///
  template <typename UnknownCellSetType, typename DynamicCoordinateHandleType>
  void Run(const UnknownCellSetType& cellSet,
           const DynamicCoordinateHandleType& coordinates,
           const viskores::Bounds& bounds,
           const viskores::Id3& nDivisions,
           viskores::cont::UnknownCellSet& outCellSet,
           viskores::cont::UnknownArrayHandle& outCoords)
  {
    VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf, "VertexClustering Worklet");

    /// determine grid resolution for clustering
    GridInfo gridInfo;
    {
      gridInfo.origin[0] = bounds.X.Min;
      gridInfo.origin[1] = bounds.Y.Min;
      gridInfo.origin[2] = bounds.Z.Min;
      gridInfo.dim[0] = nDivisions[0];
      gridInfo.dim[1] = nDivisions[1];
      gridInfo.dim[2] = nDivisions[2];
      gridInfo.bin_size[0] = bounds.X.Length() / static_cast<viskores::Float64>(nDivisions[0]);
      gridInfo.bin_size[1] = bounds.Y.Length() / static_cast<viskores::Float64>(nDivisions[1]);
      gridInfo.bin_size[2] = bounds.Z.Length() / static_cast<viskores::Float64>(nDivisions[2]);
      gridInfo.inv_bin_size[0] = 1. / gridInfo.bin_size[0];
      gridInfo.inv_bin_size[1] = 1. / gridInfo.bin_size[1];
      gridInfo.inv_bin_size[2] = 1. / gridInfo.bin_size[2];
    }

#ifdef __VISKORES_VERTEX_CLUSTERING_BENCHMARK
    viskores::cont::Timer totalTimer;
    totalTimer.Start();
    viskores::cont::Timer timer;
    timer.Start();
#endif

    //////////////////////////////////////////////
    /// start algorithm

    /// pass 1 : assign points with (cluster) ids based on the grid it falls in
    ///
    /// map points
    viskores::cont::ArrayHandle<viskores::Id> pointCidArray;

    viskores::worklet::DispatcherMapField<MapPointsWorklet> mapPointsDispatcher(
      (MapPointsWorklet(gridInfo)));
    mapPointsDispatcher.Invoke(coordinates, pointCidArray);

#ifdef __VISKORES_VERTEX_CLUSTERING_BENCHMARK
    timer.stop();
    std::cout << "Time map points (s): " << timer.GetElapsedTime() << std::endl;
    timer.Start();
#endif

    /// pass 2 : Choose a representative point from each cluster for the output:
    viskores::cont::UnknownArrayHandle repPointArray;
    {
      viskores::worklet::Keys<viskores::Id> keys;
      keys.BuildArrays(pointCidArray, viskores::worklet::KeysSortType::Stable);

      // Create a View with all the keys offsets but the last element since
      // BuildArrays uses ScanExtended
      auto keysView = viskores::cont::make_ArrayHandleView(
        keys.GetOffsets(), 0, keys.GetOffsets().GetNumberOfValues() - 1);

      // For mapping properties, this map will select an arbitrary point from
      // the cluster:
      this->PointIdMap = internal::ConcretePermutationArray(keysView, keys.GetSortedValuesMap());

      // Compute representative points from each cluster (may not match the
      // PointIdMap indexing)
      repPointArray = internal::SelectRepresentativePoint::Run(keys, coordinates);
    }

    auto repPointCidArray =
      viskores::cont::make_ArrayHandlePermutation(this->PointIdMap, pointCidArray);

#ifdef __VISKORES_VERTEX_CLUSTERING_BENCHMARK
    std::cout << "Time after reducing points (s): " << timer.GetElapsedTime() << std::endl;
    timer.Start();
#endif

    /// Pass 3 : Decimated mesh generation
    ///          For each original triangle, only output vertices from
    ///          three different clusters

    /// map each triangle vertex to the cluster id's
    /// of the cell vertices
    viskores::cont::ArrayHandle<viskores::Id3> cid3Array;

    viskores::worklet::DispatcherMapTopology<MapCellsWorklet> mapCellsDispatcher;
    mapCellsDispatcher.Invoke(cellSet, pointCidArray, cid3Array);

#ifdef __VISKORES_VERTEX_CLUSTERING_BENCHMARK
    std::cout << "Time after clustering cells (s): " << timer.GetElapsedTime() << std::endl;
    timer.Start();
#endif

    /// preparation: Get the indexes of the clustered points to prepare for new cell array
    viskores::cont::ArrayHandle<viskores::Id> cidIndexArray;
    cidIndexArray.Allocate(gridInfo.dim[0] * gridInfo.dim[1] * gridInfo.dim[2]);

    viskores::worklet::DispatcherMapField<IndexingWorklet> indexingDispatcher;
    indexingDispatcher.Invoke(repPointCidArray, cidIndexArray);

    pointCidArray.ReleaseResources();
    repPointCidArray.ReleaseResources();

    ///
    /// map: convert each triangle vertices from original point id to the new cluster indexes
    ///      If the triangle is degenerated, set the ids to <nPoints, nPoints, nPoints>
    ///      This ensures it will be placed at the end of the array when sorted.
    ///
    viskores::Id nPoints = repPointArray.GetNumberOfValues();

    viskores::cont::ArrayHandle<viskores::Id3> pointId3Array;

    viskores::worklet::DispatcherMapField<Cid2PointIdWorklet> cid2PointIdDispatcher(
      (Cid2PointIdWorklet(nPoints)));
    cid2PointIdDispatcher.Invoke(cid3Array, pointId3Array, cidIndexArray);

    cid3Array.ReleaseResources();
    cidIndexArray.ReleaseResources();

    bool doHashing = (nPoints < (1 << 21)); // Check whether we can hash Id3 into 64-bit integers

    if (doHashing)
    {
      /// Create hashed array
      viskores::cont::ArrayHandle<viskores::Int64> pointId3HashArray;

      viskores::worklet::DispatcherMapField<Cid3HashWorklet> cid3HashDispatcher(
        (Cid3HashWorklet(nPoints)));
      cid3HashDispatcher.Invoke(pointId3Array, pointId3HashArray);

      pointId3Array.ReleaseResources();

#ifdef __VISKORES_VERTEX_CLUSTERING_BENCHMARK
      std::cout << "Time before sort and unique with hashing (s): " << timer.GetElapsedTime()
                << std::endl;
      timer.Start();
#endif

      this->CellIdMap = viskores::worklet::StableSortIndices::Sort(pointId3HashArray);
      viskores::worklet::StableSortIndices::Unique(pointId3HashArray, this->CellIdMap);

#ifdef __VISKORES_VERTEX_CLUSTERING_BENCHMARK
      std::cout << "Time after sort and unique with hashing (s): " << timer.GetElapsedTime()
                << std::endl;
      timer.Start();
#endif

      // Create a temporary permutation array and use that for unhashing.
      auto tmpPerm =
        viskores::cont::make_ArrayHandlePermutation(this->CellIdMap, pointId3HashArray);

      // decode
      viskores::worklet::DispatcherMapField<Cid3UnhashWorklet> cid3UnhashDispatcher(
        (Cid3UnhashWorklet(nPoints)));
      cid3UnhashDispatcher.Invoke(tmpPerm, pointId3Array);
    }
    else
    {
#ifdef __VISKORES_VERTEX_CLUSTERING_BENCHMARK
      std::cout << "Time before sort and unique [no hashing] (s): " << timer.GetElapsedTime()
                << std::endl;
      timer.Start();
#endif

      this->CellIdMap = viskores::worklet::StableSortIndices::Sort(pointId3Array);
      viskores::worklet::StableSortIndices::Unique(pointId3Array, this->CellIdMap);

#ifdef __VISKORES_VERTEX_CLUSTERING_BENCHMARK
      std::cout << "Time after sort and unique [no hashing] (s): " << timer.GetElapsedTime()
                << std::endl;
      timer.Start();
#endif

      // Permute the connectivity array into a basic array handle. Use a
      // temporary array handle to avoid memory aliasing.
      {
        viskores::cont::ArrayHandle<viskores::Id3> tmp;
        tmp = internal::ConcretePermutationArray(this->CellIdMap, pointId3Array);
        pointId3Array = tmp;
      }
    }

    // remove the last element if invalid
    viskores::Id cells = pointId3Array.GetNumberOfValues();
    if (cells > 0 && pointId3Array.ReadPortal().Get(cells - 1)[2] >= nPoints)
    {
      cells--;
      pointId3Array.Allocate(cells, viskores::CopyFlag::On);
      this->CellIdMap.Allocate(cells, viskores::CopyFlag::On);
    }

    /// output
    outCoords = repPointArray;

    viskores::cont::CellSetSingleType<> triangles;
    triangles.Fill(repPointArray.GetNumberOfValues(),
                   viskores::CellShapeTagTriangle::Id,
                   3,
                   internal::copyFromVec(pointId3Array));
    outCellSet = triangles;

#ifdef __VISKORES_VERTEX_CLUSTERING_BENCHMARK
    std::cout << "Wrap-up (s): " << timer.GetElapsedTime() << std::endl;
    viskores::Float64 t = totalTimer.GetElapsedTime();
    std::cout << "Time (s): " << t << std::endl;
    std::cout << "number of output points: " << repPointArray.GetNumberOfValues() << std::endl;
    std::cout << "number of output cells: " << pointId3Array.GetNumberOfValues() << std::endl;
#endif
  }

  viskores::cont::ArrayHandle<viskores::Id> GetPointIdMap() const { return this->PointIdMap; }
  viskores::cont::ArrayHandle<viskores::Id> GetCellIdMap() const { return this->CellIdMap; }

private:
  viskores::cont::ArrayHandle<viskores::Id> PointIdMap;
  viskores::cont::ArrayHandle<viskores::Id> CellIdMap;
}; // struct VertexClustering
}
} // namespace viskores::worklet

#endif // viskores_worklet_VertexClustering_h
