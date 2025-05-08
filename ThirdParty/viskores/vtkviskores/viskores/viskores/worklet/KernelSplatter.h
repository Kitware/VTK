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
#ifndef viskores_worklet_KernelSplatter_h
#define viskores_worklet_KernelSplatter_h

#include <viskores/Math.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DeviceAdapterAlgorithm.h>
#include <viskores/cont/Timer.h>

#include <viskores/cont/internal/ArrayPortalFromIterators.h>
#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

#include <viskores/worklet/splatkernels/Gaussian.h>
#include <viskores/worklet/splatkernels/KernelBase.h>
#include <viskores/worklet/splatkernels/Spline3rdOrder.h>

//#define __VISKORES_GAUSSIAN_SPLATTER_BENCHMARK

//----------------------------------------------------------------------------
// Macros for timing
//----------------------------------------------------------------------------
#if defined(__VISKORES_GAUSSIAN_SPLATTER_BENCHMARK) && !defined(START_TIMER_BLOCK)
// start timer
#define START_TIMER_BLOCK(name)                          \
  viskores::cont::Timer timer_##name{ DeviceAdapter() }; \
  timer_##name.Start();

// stop timer
#define END_TIMER_BLOCK(name) \
  std::cout << #name " : elapsed : " << timer_##name.GetElapsedTime() << "\n";
#endif
#if !defined(START_TIMER_BLOCK)
#define START_TIMER_BLOCK(name)
#define END_TIMER_BLOCK(name)
#endif

//----------------------------------------------------------------------------
// Kernel splatter worklet/filter
//----------------------------------------------------------------------------
namespace viskores
{
namespace worklet
{

namespace debug
{
#ifdef DEBUG_PRINT
//----------------------------------------------------------------------------
template <typename T, typename S = VISKORES_DEFAULT_STORAGE_TAG>
void OutputArrayDebug(const viskores::cont::ArrayHandle<T, S>& outputArray, const std::string& name)
{
  using ValueType = T;
  using StorageType = viskores::cont::internal::Storage<T, S>;
  using PortalConstType = typename StorageType::PortalConstType;
  PortalConstType readPortal = outputArray.ReadPortal();
  viskores::cont::ArrayPortalToIterators<PortalConstType> iterators(readPortal);
  std::vector<ValueType> result(readPortal.GetNumberOfValues());
  std::copy(iterators.GetBegin(), iterators.GetEnd(), result.begin());
  std::cout << name.c_str() << " " << outputArray.GetNumberOfValues() << "\n";
  std::copy(result.begin(), result.end(), std::ostream_iterator<ValueType>(std::cout, " "));
  std::cout << std::endl;
}

//----------------------------------------------------------------------------
template <typename T, int S>
void OutputArrayDebug(const viskores::cont::ArrayHandle<viskores::Vec<T, S>>& outputArray,
                      const std::string& name)
{
  using ValueType = T;
  using PortalConstType = typename viskores::cont::ArrayHandle<viskores::Vec<T, S>>::ReadPortalType;
  PortalConstType readPortal = outputArray.ReadPortal();
  viskores::cont::ArrayPortalToIterators<PortalConstType> iterators(readPortal);
  std::cout << name.c_str() << " " << outputArray.GetNumberOfValues() << "\n";
  auto portal = outputArray.ReadPortal();
  for (int i = 0; i < outputArray.GetNumberOfValues(); ++i)
  {
    std::cout << portal.Get(i);
  }
  std::cout << std::endl;
}
//----------------------------------------------------------------------------
template <typename I, typename T, int S>
void OutputArrayDebug(
  const viskores::cont::ArrayHandlePermutation<I, viskores::cont::ArrayHandle<viskores::Vec<T, S>>>&
    outputArray,
  const std::string& name)
{
  using PortalConstType = typename viskores::cont::
    ArrayHandlePermutation<I, viskores::cont::ArrayHandle<viskores::Vec<T, S>>>::ReadPortalType;
  PortalConstType readPortal = outputArray.ReadPortal();
  viskores::cont::ArrayPortalToIterators<PortalConstType> iterators(readPortal);
  std::cout << name.c_str() << " " << outputArray.GetNumberOfValues() << "\n";
  auto outputPortal = outputArray.ReadPortal();
  for (int i = 0; i < outputArray.GetNumberOfValues(); ++i)
  {
    std::cout << outputPortal.Get(i);
  }
  std::cout << std::endl;
}

#else
template <typename T, typename S>
void OutputArrayDebug(const viskores::cont::ArrayHandle<T, S>& viskoresNotUsed(outputArray),
                      const std::string& viskoresNotUsed(name))
{
}
//----------------------------------------------------------------------------
template <typename T, int S>
void OutputArrayDebug(
  const viskores::cont::ArrayHandle<viskores::Vec<T, S>>& viskoresNotUsed(outputArray),
  const std::string& viskoresNotUsed(name))
{
}
//----------------------------------------------------------------------------
template <typename I, typename T, int S>
void OutputArrayDebug(
  const viskores::cont::ArrayHandlePermutation<I, viskores::cont::ArrayHandle<viskores::Vec<T, S>>>&
    viskoresNotUsed(outputArray),
  const std::string& viskoresNotUsed(name))
{
}
#endif
} // namespace debug

template <typename Kernel, typename DeviceAdapter>
struct KernelSplatterFilterUniformGrid
{
  using DoubleHandleType = viskores::cont::ArrayHandle<viskores::Float64>;
  using FloatHandleType = viskores::cont::ArrayHandle<viskores::Float32>;
  using VecHandleType = viskores::cont::ArrayHandle<viskores::Id3>;
  using IdHandleType = viskores::cont::ArrayHandle<viskores::Id>;
  //
  using FloatVec = viskores::Vec3f_32;
  using PointType = viskores::Vec3f_64;
  using PointHandleType = viskores::cont::ArrayHandle<PointType>;
  //
  using VecPermType = viskores::cont::ArrayHandlePermutation<IdHandleType, VecHandleType>;
  using PointVecPermType = viskores::cont::ArrayHandlePermutation<IdHandleType, PointHandleType>;
  using IdPermType = viskores::cont::ArrayHandlePermutation<IdHandleType, IdHandleType>;
  using FloatPermType = viskores::cont::ArrayHandlePermutation<IdHandleType, FloatHandleType>;
  //
  using IdCountingType = viskores::cont::ArrayHandleCounting<viskores::Id>;

  //-----------------------------------------------------------------------
  // zero an array,
  // @TODO, get rid of this
  //-----------------------------------------------------------------------
  struct zero_voxel : public viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn, FieldOut);
    using ExecutionSignature = void(_1, WorkIndex, _2);
    //
    VISKORES_CONT
    zero_voxel() {}

    template <typename T>
    VISKORES_EXEC_CONT void operator()(const viskores::Id&,
                                       const viskores::Id& viskoresNotUsed(index),
                                       T& voxel_value) const
    {
      voxel_value = T(0);
    }
  };

  //-----------------------------------------------------------------------
  // Return the splat footprint/neighborhood of each sample point, as
  // represented by min and max boundaries in each dimension.
  // Also return the size of this footprint and the voxel coordinates
  // of the splat point (floating point).
  //-----------------------------------------------------------------------
  class GetFootprint : public viskores::worklet::WorkletMapField
  {
  private:
    viskores::Vec3f_64 origin_;
    viskores::Vec3f_64 spacing_;
    viskores::Id3 VolumeDimensions;
    Kernel kernel_;

  public:
    using ControlSignature =
      void(FieldIn, FieldIn, FieldIn, FieldIn, FieldOut, FieldOut, FieldOut, FieldOut);
    using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8);

    VISKORES_CONT
    GetFootprint(const viskores::Vec3f_64& o,
                 const viskores::Vec3f_64& s,
                 const viskores::Id3& dim,
                 const Kernel& kernel)
      : origin_(o)
      , spacing_(s)
      , VolumeDimensions(dim)
      , kernel_(kernel)
    {
    }

    template <typename T, typename T2>
    VISKORES_EXEC_CONT void operator()(const T& x,
                                       const T& y,
                                       const T& z,
                                       const T2& h,
                                       viskores::Vec3f_64& splatPoint,
                                       viskores::Id3& minFootprint,
                                       viskores::Id3& maxFootprint,
                                       viskores::Id& footprintSize) const
    {
      PointType splat, min, max;
      viskores::Vec3f_64 sample = viskores::make_Vec(x, y, z);
      viskores::Id size = 1;
      double cutoff = kernel_.maxDistance(h);
      for (int i = 0; i < 3; i++)
      {
        splat[i] = (sample[i] - this->origin_[i]) / this->spacing_[i];
        min[i] = static_cast<viskores::Id>(ceil(static_cast<double>(splat[i]) - cutoff));
        max[i] = static_cast<viskores::Id>(floor(static_cast<double>(splat[i]) + cutoff));
        if (min[i] < 0)
        {
          min[i] = 0;
        }
        if (max[i] >= this->VolumeDimensions[i])
        {
          max[i] = this->VolumeDimensions[i] - 1;
        }
        size = static_cast<viskores::Id>(size * (1 + max[i] - min[i]));
      }
      splatPoint = splat;
      minFootprint = min;
      maxFootprint = max;
      footprintSize = size;
    }
  };

  //-----------------------------------------------------------------------
  // Return the "local" Id of a voxel within a splat point's footprint.
  // A splat point that affects 5 neighboring voxel gridpoints would
  // have local Ids 0,1,2,3,4
  //-----------------------------------------------------------------------
  class ComputeLocalNeighborId : public viskores::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn, FieldIn, FieldOut);
    using ExecutionSignature = void(_1, _2, WorkIndex, _3);

    VISKORES_CONT
    ComputeLocalNeighborId() {}

    template <typename T>
    VISKORES_EXEC_CONT void operator()(const T& modulus,
                                       const T& offset,
                                       const viskores::Id& index,
                                       T& localId) const
    {
      localId = (index - offset) % modulus;
    }
  };

  //-----------------------------------------------------------------------
  // Compute the splat value of the input neighbour point.
  // The voxel Id of this point within the volume is also determined.
  //-----------------------------------------------------------------------
  class GetSplatValue : public viskores::worklet::WorkletMapField
  {
  private:
    viskores::Vec3f_64 spacing_;
    viskores::Vec3f_64 origin_;
    viskores::Id3 VolumeDim;
    viskores::Float64 Radius2;
    viskores::Float64 ExponentFactor;
    viskores::Float64 ScalingFactor;
    Kernel kernel;

  public:
    using ControlSignature =
      void(FieldIn, FieldIn, FieldIn, FieldIn, FieldIn, FieldIn, FieldOut, FieldOut);
    using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8);

    VISKORES_CONT
    GetSplatValue(const viskores::Vec3f_64& orig,
                  const viskores::Vec3f_64& s,
                  const viskores::Id3& dim,
                  const Kernel& k)
      : spacing_(s)
      , origin_(orig)
      , VolumeDim(dim)
      , kernel(k)
    {
    }

    template <typename T, typename T2, typename P>
    VISKORES_EXEC_CONT void operator()(const viskores::Vec<P, 3>& splatPoint,
                                       const T& minBound,
                                       const T& maxBound,
                                       const T2& kernel_H,
                                       const T2& scale,
                                       const viskores::Id localNeighborId,
                                       viskores::Id& neighborVoxelId,
                                       viskores::Float32& splatValue) const
    {
      viskores::Id yRange = 1 + maxBound[1] - minBound[1];
      viskores::Id xRange = 1 + maxBound[0] - minBound[0];
      viskores::Id divisor = yRange * xRange;
      viskores::Id i = localNeighborId / divisor;
      viskores::Id remainder = localNeighborId % divisor;
      viskores::Id j = remainder / xRange;
      viskores::Id k = remainder % xRange;
      // note the order of k,j,i
      viskores::Id3 voxel = minBound + viskores::make_Vec(k, j, i);
      PointType dist = viskores::make_Vec((splatPoint[0] - voxel[0]) * spacing_[0],
                                          (splatPoint[1] - voxel[1]) * spacing_[0],
                                          (splatPoint[2] - voxel[2]) * spacing_[0]);
      viskores::Float64 dist2 = viskores::Dot(dist, dist);

      // Compute splat value using the kernel distance_squared function
      splatValue = scale * kernel.w2(kernel_H, dist2);
      //
      neighborVoxelId =
        (voxel[2] * VolumeDim[0] * VolumeDim[1]) + (voxel[1] * VolumeDim[0]) + voxel[0];
      if (neighborVoxelId < 0)
        neighborVoxelId = -1;
      else if (neighborVoxelId >= VolumeDim[0] * VolumeDim[1] * VolumeDim[2])
        neighborVoxelId = VolumeDim[0] * VolumeDim[1] * VolumeDim[2] - 1;
    }
  };

  //-----------------------------------------------------------------------
  // Scatter worklet that writes a splat value into the larger,
  // master splat value array, using the splat value's voxel Id as an index.
  //-----------------------------------------------------------------------
  class UpdateVoxelSplats : public viskores::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn, FieldIn, WholeArrayOut);
    using ExecutionSignature = void(_1, _2, _3);

    VISKORES_CONT
    UpdateVoxelSplats() {}

    template <typename ExecArgPortalType>
    VISKORES_EXEC_CONT void operator()(const viskores::Id& voxelIndex,
                                       const viskores::Float64& splatValue,
                                       ExecArgPortalType& execArg) const
    {
      execArg.Set(voxelIndex, static_cast<viskores::Float32>(splatValue));
    }
  };

  //-----------------------------------------------------------------------
  // Construct a splatter filter/object
  //
  // @TODO, get the origin_ and spacing_ from the dataset coordinates
  // instead of requiring them to be passed as parameters.
  //-----------------------------------------------------------------------
  KernelSplatterFilterUniformGrid(const viskores::Id3& dims,
                                  viskores::Vec3f origin,
                                  viskores::Vec3f spacing,
                                  const viskores::cont::DataSet& dataset,
                                  const Kernel& kernel)
    : dims_(dims)
    , origin_(origin)
    , spacing_(spacing)
    , dataset_(dataset)
    , kernel_(kernel)
  {
  }

  //-----------------------------------------------------------------------
  // class variables for the splat filter
  //-----------------------------------------------------------------------
  viskores::Id3 dims_;
  FloatVec origin_;
  FloatVec spacing_;
  viskores::cont::DataSet dataset_;
  // The kernel used for this filter
  Kernel kernel_;

  //-----------------------------------------------------------------------
  // Run the filter, given the input params
  //-----------------------------------------------------------------------
  template <typename StorageT>
  void run(const viskores::cont::ArrayHandle<viskores::Float64, StorageT> xValues,
           const viskores::cont::ArrayHandle<viskores::Float64, StorageT> yValues,
           const viskores::cont::ArrayHandle<viskores::Float64, StorageT> zValues,
           const viskores::cont::ArrayHandle<viskores::Float32, StorageT> rValues,
           const viskores::cont::ArrayHandle<viskores::Float32, StorageT> sValues,
           FloatHandleType scalarSplatOutput)
  {
    // Number of grid points in the volume bounding box
    viskores::Id3 pointDimensions = viskores::make_Vec(dims_[0] + 1, dims_[1] + 1, dims_[2] + 1);
    const viskores::Id numVolumePoints = (dims_[0] + 1) * (dims_[1] + 1) * (dims_[2] + 1);

    //---------------------------------------------------------------
    // Get the splat footprint/neighborhood of each sample point, as
    // represented by min and max boundaries in each dimension.
    //---------------------------------------------------------------
    PointHandleType splatPoints;
    VecHandleType footprintMin;
    VecHandleType footprintMax;
    IdHandleType numNeighbors;
    IdHandleType localNeighborIds;

    GetFootprint footprint_worklet(origin_, spacing_, pointDimensions, kernel_);
    viskores::worklet::DispatcherMapField<GetFootprint> footprintDispatcher(footprint_worklet);
    footprintDispatcher.SetDevice(DeviceAdapter());

    START_TIMER_BLOCK(GetFootprint)
    footprintDispatcher.Invoke(
      xValues, yValues, zValues, rValues, splatPoints, footprintMin, footprintMax, numNeighbors);
    END_TIMER_BLOCK(GetFootprint)

    debug::OutputArrayDebug(numNeighbors, "numNeighbours");
    debug::OutputArrayDebug(footprintMin, "footprintMin");
    debug::OutputArrayDebug(footprintMax, "footprintMax");
    debug::OutputArrayDebug(splatPoints, "splatPoints");

    //---------------------------------------------------------------
    // Prefix sum of the number of affected splat voxels ("neighbors")
    // for each sample point.  The total sum represents the number of
    // voxels for which splat values will be computed.
    // prefix sum is used in neighbour id lookup
    //---------------------------------------------------------------
    IdHandleType numNeighborsPrefixSum;

    START_TIMER_BLOCK(numNeighborsPrefixSum)
    const viskores::Id totalSplatSize =
      viskores::cont::DeviceAdapterAlgorithm<DeviceAdapter>::ScanInclusive(numNeighbors,
                                                                           numNeighborsPrefixSum);
    END_TIMER_BLOCK(numNeighborsPrefixSum)

    std::cout << "totalSplatSize " << totalSplatSize << "\n";
    debug::OutputArrayDebug(numNeighborsPrefixSum, "numNeighborsPrefixSum");

    // also get the neighbour counts exclusive sum for use in lookup of local neighbour id
    IdHandleType numNeighborsExclusiveSum;
    START_TIMER_BLOCK(numNeighborsExclusiveSum)
    viskores::cont::DeviceAdapterAlgorithm<DeviceAdapter>::ScanExclusive(numNeighbors,
                                                                         numNeighborsExclusiveSum);
    //END_TIMER_BLOCK(numNeighborsExclusiveSum)
    debug::OutputArrayDebug(numNeighborsExclusiveSum, "numNeighborsExclusiveSum");

    //---------------------------------------------------------------
    // Generate a lookup array that, for each splat voxel, identifies
    // the Id of its corresponding (sample) splat point.
    // For example, if splat point 0 affects 5 neighbor voxels, then
    // the five entries in the lookup array would be 0,0,0,0,0
    //---------------------------------------------------------------
    IdHandleType neighbor2SplatId;
    IdCountingType countingArray(viskores::Id(0), 1, viskores::Id(totalSplatSize));
    START_TIMER_BLOCK(Upper_bounds)
    viskores::cont::DeviceAdapterAlgorithm<DeviceAdapter>::UpperBounds(
      numNeighborsPrefixSum, countingArray, neighbor2SplatId);
    END_TIMER_BLOCK(Upper_bounds)
    countingArray.ReleaseResources();
    debug::OutputArrayDebug(neighbor2SplatId, "neighbor2SplatId");

    //---------------------------------------------------------------
    // Extract a "local" Id lookup array of the foregoing
    // neighbor2SplatId array.  So, the local version of 0,0,0,0,0
    // would be 0,1,2,3,4
    //---------------------------------------------------------------
    IdPermType modulii(neighbor2SplatId, numNeighbors);
    debug::OutputArrayDebug(modulii, "modulii");

    IdPermType offsets(neighbor2SplatId, numNeighborsExclusiveSum);
    debug::OutputArrayDebug(offsets, "offsets");

    viskores::worklet::DispatcherMapField<ComputeLocalNeighborId> idDispatcher;
    idDispatcher.SetDevice(DeviceAdapter());
    START_TIMER_BLOCK(idDispatcher)
    idDispatcher.Invoke(modulii, offsets, localNeighborIds);
    END_TIMER_BLOCK(idDispatcher)
    debug::OutputArrayDebug(localNeighborIds, "localNeighborIds");

    numNeighbors.ReleaseResources();
    numNeighborsPrefixSum.ReleaseResources();
    numNeighborsExclusiveSum.ReleaseResources();

    //---------------------------------------------------------------
    // We will perform gather operations for the generated splat points
    // using permutation arrays
    //---------------------------------------------------------------
    PointVecPermType ptSplatPoints(neighbor2SplatId, splatPoints);
    VecPermType ptFootprintMins(neighbor2SplatId, footprintMin);
    VecPermType ptFootprintMaxs(neighbor2SplatId, footprintMax);
    FloatPermType radii(neighbor2SplatId, rValues);
    FloatPermType scale(neighbor2SplatId, sValues);

    debug::OutputArrayDebug(radii, "radii");
    debug::OutputArrayDebug(ptSplatPoints, "ptSplatPoints");
    debug::OutputArrayDebug(ptFootprintMins, "ptFootprintMins");

    //---------------------------------------------------------------
    // Calculate the splat value of each affected voxel
    //---------------------------------------------------------------
    FloatHandleType voxelSplatSums;
    IdHandleType neighborVoxelIds;
    IdHandleType uniqueVoxelIds;
    FloatHandleType splatValues;

    GetSplatValue splatterDispatcher_worklet(origin_, spacing_, pointDimensions, kernel_);
    viskores::worklet::DispatcherMapField<GetSplatValue> splatterDispatcher(
      splatterDispatcher_worklet);
    splatterDispatcher.SetDevice(DeviceAdapter());

    START_TIMER_BLOCK(GetSplatValue)
    splatterDispatcher.Invoke(ptSplatPoints,
                              ptFootprintMins,
                              ptFootprintMaxs,
                              radii,
                              scale,
                              localNeighborIds,
                              neighborVoxelIds,
                              splatValues);
    END_TIMER_BLOCK(GetSplatValue)

    debug::OutputArrayDebug(splatValues, "splatValues");
    debug::OutputArrayDebug(neighborVoxelIds, "neighborVoxelIds");

    ptSplatPoints.ReleaseResources();
    ptFootprintMins.ReleaseResources();
    ptFootprintMaxs.ReleaseResources();
    neighbor2SplatId.ReleaseResources();
    localNeighborIds.ReleaseResources();
    splatPoints.ReleaseResources();
    footprintMin.ReleaseResources();
    footprintMax.ReleaseResources();
    radii.ReleaseResources();

    //---------------------------------------------------------------
    // Sort the voxel Ids in ascending order
    //---------------------------------------------------------------
    START_TIMER_BLOCK(SortByKey)
    viskores::cont::DeviceAdapterAlgorithm<DeviceAdapter>::SortByKey(neighborVoxelIds, splatValues);
    END_TIMER_BLOCK(SortByKey)
    debug::OutputArrayDebug(splatValues, "splatValues");

    //---------------------------------------------------------------
    // Do a reduction to sum all contributions for each affected voxel
    //---------------------------------------------------------------
    START_TIMER_BLOCK(ReduceByKey)
    viskores::cont::DeviceAdapterAlgorithm<DeviceAdapter>::ReduceByKey(
      neighborVoxelIds, splatValues, uniqueVoxelIds, voxelSplatSums, viskores::Add());
    END_TIMER_BLOCK(ReduceByKey)

    debug::OutputArrayDebug(neighborVoxelIds, "neighborVoxelIds");
    debug::OutputArrayDebug(uniqueVoxelIds, "uniqueVoxelIds");
    debug::OutputArrayDebug(voxelSplatSums, "voxelSplatSums");
    //
    neighborVoxelIds.ReleaseResources();
    splatValues.ReleaseResources();

    //---------------------------------------------------------------
    // initialize each field value to zero to begin with
    //---------------------------------------------------------------
    IdCountingType indexArray(viskores::Id(0), 1, numVolumePoints);
    viskores::worklet::DispatcherMapField<zero_voxel> zeroDispatcher;
    zeroDispatcher.SetDevice(DeviceAdapter());
    zeroDispatcher.Invoke(indexArray, scalarSplatOutput);
    //
    indexArray.ReleaseResources();

    //---------------------------------------------------------------
    // Scatter operation to write the previously-computed splat
    // value sums into their corresponding entries in the output array
    //---------------------------------------------------------------
    viskores::worklet::DispatcherMapField<UpdateVoxelSplats> scatterDispatcher;
    scatterDispatcher.SetDevice(DeviceAdapter());

    START_TIMER_BLOCK(UpdateVoxelSplats)
    scatterDispatcher.Invoke(uniqueVoxelIds, voxelSplatSums, scalarSplatOutput);
    END_TIMER_BLOCK(UpdateVoxelSplats)
    debug::OutputArrayDebug(scalarSplatOutput, "scalarSplatOutput");
    //
    uniqueVoxelIds.ReleaseResources();
    voxelSplatSums.ReleaseResources();
  }

}; //struct KernelSplatter
}
} //namespace viskores::worklet

#endif //viskores_worklet_KernelSplatter_h
