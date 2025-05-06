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

#include <viskores/cont/ArrayRangeCompute.h>
#include <viskores/cont/ArrayRangeComputeTemplate.h>

#include <viskores/TypeList.h>

#include <viskores/cont/ArrayHandleBasic.h>
#include <viskores/cont/ArrayHandleCartesianProduct.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandleSOA.h>
#include <viskores/cont/ArrayHandleStride.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/ArrayHandleXGCCoordinates.h>

namespace
{

using AllScalars = viskores::TypeListBaseC;

template <typename viskores::IdComponent N>
struct VecTransform
{
  template <typename T>
  using type = viskores::Vec<T, N>;
};

template <viskores::IdComponent N>
using AllVecOfSize = viskores::ListTransform<AllScalars, VecTransform<N>::template type>;

using AllVec = viskores::ListAppend<AllVecOfSize<2>, AllVecOfSize<3>, AllVecOfSize<4>>;

using AllTypes = viskores::ListAppend<AllScalars, AllVec>;

using CartesianProductStorage =
  viskores::cont::StorageTagCartesianProduct<viskores::cont::StorageTagBasic,
                                             viskores::cont::StorageTagBasic,
                                             viskores::cont::StorageTagBasic>;

using StorageTagsList = viskores::List<viskores::cont::StorageTagBasic,
                                       viskores::cont::StorageTagSOA,
                                       viskores::cont::StorageTagXGCCoordinates,
                                       viskores::cont::StorageTagUniformPoints,
                                       CartesianProductStorage,
                                       viskores::cont::StorageTagConstant,
                                       viskores::cont::StorageTagCounting,
                                       viskores::cont::StorageTagIndex>;

template <typename StorageTag>
struct StorageTagToValueTypesMap;

#define MAP_STORAGE_TAG_VALUE_TYPES(StorageTag, ValueTypesList) \
  template <>                                                   \
  struct StorageTagToValueTypesMap<StorageTag>                  \
  {                                                             \
    using TypeList = ValueTypesList;                            \
  }

MAP_STORAGE_TAG_VALUE_TYPES(viskores::cont::StorageTagBasic, AllTypes);
MAP_STORAGE_TAG_VALUE_TYPES(viskores::cont::StorageTagSOA, AllVec);
MAP_STORAGE_TAG_VALUE_TYPES(viskores::cont::StorageTagXGCCoordinates, viskores::TypeListFieldVec3);
MAP_STORAGE_TAG_VALUE_TYPES(viskores::cont::StorageTagUniformPoints,
                            viskores::List<viskores::Vec3f>);
MAP_STORAGE_TAG_VALUE_TYPES(CartesianProductStorage, viskores::TypeListFieldVec3);
MAP_STORAGE_TAG_VALUE_TYPES(viskores::cont::StorageTagConstant, AllTypes);
MAP_STORAGE_TAG_VALUE_TYPES(viskores::cont::StorageTagCounting, AllTypes);
MAP_STORAGE_TAG_VALUE_TYPES(viskores::cont::StorageTagIndex, viskores::List<viskores::Id>);

#undef MAP_STORAGE_TAG_VALUE_TYPES

} // anonymous namespace

namespace viskores
{
namespace cont
{

namespace internal
{

void ThrowArrayRangeComputeFailed()
{
  throw viskores::cont::ErrorExecution("Failed to run ArrayRangeComputation on any device.");
}

} // namespace internal

viskores::cont::ArrayHandle<viskores::Range> ArrayRangeCompute(
  const viskores::cont::UnknownArrayHandle& array,
  bool computeFiniteRange,
  viskores::cont::DeviceAdapterId device)
{
  return ArrayRangeCompute(
    array, viskores::cont::ArrayHandle<viskores::UInt8>{}, computeFiniteRange, device);
}

viskores::cont::ArrayHandle<viskores::Range> ArrayRangeCompute(
  const viskores::cont::UnknownArrayHandle& array,
  const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
  bool computeFiniteRange,
  viskores::cont::DeviceAdapterId device)
{
  // First, try (potentially fast-paths) for common(ish) array types.
  try
  {
    viskores::cont::ArrayHandle<viskores::Range> ranges;

    auto computeForArrayHandle = [&](const auto& input)
    {
      ranges =
        viskores::cont::ArrayRangeComputeTemplate(input, maskArray, computeFiniteRange, device);
    };

    bool success = false;
    auto computeForStorage = [&](auto storageTag)
    {
      using STag = decltype(storageTag);
      using VTypes = typename StorageTagToValueTypesMap<STag>::TypeList;
      if (array.IsStorageType<STag>())
      {
        array.CastAndCallForTypes<VTypes, viskores::List<STag>>(computeForArrayHandle);
        success = true;
      }
    };

    viskores::ListForEach(computeForStorage, StorageTagsList{});

    if (success)
    {
      return ranges;
    }
  }
  catch (viskores::cont::ErrorBadType&)
  {
    // If a cast/call failed, try falling back to a more general implementation.
  }

  // fallback
  bool success = false;
  viskores::cont::ArrayHandle<viskores::Range> ranges;
  auto computeForExtractComponent = [&](auto valueTypeObj)
  {
    using VT = decltype(valueTypeObj);
    if (!success && array.IsBaseComponentType<VT>())
    {
      viskores::IdComponent numComponents = array.GetNumberOfComponentsFlat();
      ranges.Allocate(numComponents);
      auto rangePortal = ranges.WritePortal();
      for (viskores::IdComponent i = 0; i < numComponents; ++i)
      {
        auto componentArray = array.ExtractComponent<VT>(i);
        auto componentRange = viskores::cont::ArrayRangeComputeTemplate(
          componentArray, maskArray, computeFiniteRange, device);
        rangePortal.Set(i, componentRange.ReadPortal().Get(0));
      }
      success = true;
    }
  };

  viskores::ListForEach(computeForExtractComponent, AllScalars{});
  if (!success)
  {
    internal::ThrowArrayRangeComputeFailed();
  }

  return ranges;
}

viskores::Range ArrayRangeComputeMagnitude(const viskores::cont::UnknownArrayHandle& array,
                                           bool computeFiniteRange,
                                           viskores::cont::DeviceAdapterId device)
{
  return ArrayRangeComputeMagnitude(
    array, viskores::cont::ArrayHandle<viskores::UInt8>{}, computeFiniteRange, device);
}

viskores::Range ArrayRangeComputeMagnitude(
  const viskores::cont::UnknownArrayHandle& array,
  const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
  bool computeFiniteRange,
  viskores::cont::DeviceAdapterId device)
{
  // First, try (potentially fast-paths) for common(ish) array types.
  try
  {
    viskores::Range range;

    auto computeForArrayHandle = [&](const auto& input)
    {
      range = viskores::cont::ArrayRangeComputeMagnitudeTemplate(
        input, maskArray, computeFiniteRange, device);
    };

    bool success = false;
    auto computeForStorage = [&](auto storageTag)
    {
      using STag = decltype(storageTag);
      using VTypes = typename StorageTagToValueTypesMap<STag>::TypeList;
      if (array.IsStorageType<STag>())
      {
        array.CastAndCallForTypes<VTypes, viskores::List<STag>>(computeForArrayHandle);
        success = true;
      }
    };

    viskores::ListForEach(computeForStorage, StorageTagsList{});

    if (success)
    {
      return range;
    }
  }
  catch (viskores::cont::ErrorBadType&)
  {
    // If a cast/call failed, try falling back to a more general implementation.
  }

  // fallback
  bool success = false;
  viskores::Range range;
  auto computeForExtractArrayFromComponents = [&](auto valueTypeObj)
  {
    using VT = decltype(valueTypeObj);
    if (!success && array.IsBaseComponentType<VT>())
    {
      auto extractedArray = array.ExtractArrayFromComponents<VT>();
      range = viskores::cont::ArrayRangeComputeMagnitudeTemplate(
        extractedArray, maskArray, computeFiniteRange, device);
      success = true;
    }
  };

  viskores::ListForEach(computeForExtractArrayFromComponents, AllScalars{});
  if (!success)
  {
    internal::ThrowArrayRangeComputeFailed();
  }

  return range;
}

}
} // namespace viskores::cont
