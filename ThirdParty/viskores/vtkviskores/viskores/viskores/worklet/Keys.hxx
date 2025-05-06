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
#ifndef viskores_worklet_Keys_hxx
#define viskores_worklet_Keys_hxx

#include <viskores/worklet/Keys.h>

namespace viskores
{
namespace worklet
{
/// Build the internal arrays without modifying the input. This is more
/// efficient for stable sorted arrays, but requires an extra copy of the
/// keys for unstable sorting.
template <typename T>
template <typename KeyArrayType>
VISKORES_CONT void Keys<T>::BuildArrays(const KeyArrayType& keys,
                                        KeysSortType sort,
                                        viskores::cont::DeviceAdapterId device)
{
  VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf, "Keys::BuildArrays");

  switch (sort)
  {
    case KeysSortType::Unstable:
    {
      KeyArrayHandleType mutableKeys;
      viskores::cont::Algorithm::Copy(device, keys, mutableKeys);

      this->BuildArraysInternal(mutableKeys, device);
    }
    break;
    case KeysSortType::Stable:
      this->BuildArraysInternalStable(keys, device);
      break;
  }
}

/// Build the internal arrays and also sort the input keys. This is more
/// efficient for unstable sorting, but requires an extra copy for stable
/// sorting.
template <typename T>
template <typename KeyArrayType>
VISKORES_CONT void Keys<T>::BuildArraysInPlace(KeyArrayType& keys,
                                               KeysSortType sort,
                                               viskores::cont::DeviceAdapterId device)
{
  VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf, "Keys::BuildArraysInPlace");

  switch (sort)
  {
    case KeysSortType::Unstable:
      this->BuildArraysInternal(keys, device);
      break;
    case KeysSortType::Stable:
    {
      this->BuildArraysInternalStable(keys, device);
      KeyArrayHandleType tmp;
      // Copy into a temporary array so that the permutation array copy
      // won't alias input/output memory:
      viskores::cont::Algorithm::Copy(device, keys, tmp);
      viskores::cont::Algorithm::Copy(
        device, viskores::cont::make_ArrayHandlePermutation(this->SortedValuesMap, tmp), keys);
    }
    break;
  }
}

template <typename T>
template <typename KeyArrayType>
VISKORES_CONT void Keys<T>::BuildArraysInternal(KeyArrayType& keys,
                                                viskores::cont::DeviceAdapterId device)
{
  VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf, "Keys::BuildArraysInternal");

  const viskores::Id numKeys = keys.GetNumberOfValues();

  viskores::cont::Algorithm::Copy(
    device, viskores::cont::ArrayHandleIndex(numKeys), this->SortedValuesMap);

  // TODO: Do we need the ability to specify a comparison functor for sort?
  viskores::cont::Algorithm::SortByKey(device, keys, this->SortedValuesMap);

  // Find the unique keys and the number of values per key.
  viskores::cont::ArrayHandle<viskores::IdComponent> counts;
  viskores::cont::Algorithm::ReduceByKey(
    device,
    keys,
    viskores::cont::ArrayHandleConstant<viskores::IdComponent>(1, numKeys),
    this->UniqueKeys,
    counts,
    viskores::Sum());

  // Get the offsets from the counts with a scan.
  viskores::cont::Algorithm::ScanExtended(
    device, viskores::cont::make_ArrayHandleCast(counts, viskores::Id()), this->Offsets);

  VISKORES_ASSERT(
    numKeys == viskores::cont::ArrayGetValue(this->Offsets.GetNumberOfValues() - 1, this->Offsets));
}

template <typename T>
template <typename KeyArrayType>
VISKORES_CONT void Keys<T>::BuildArraysInternalStable(const KeyArrayType& keys,
                                                      viskores::cont::DeviceAdapterId device)
{
  VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf, "Keys::BuildArraysInternalStable");

  const viskores::Id numKeys = keys.GetNumberOfValues();

  // Produce a stable sorted map of the keys:
  this->SortedValuesMap = StableSortIndices::Sort(device, keys);
  auto sortedKeys = viskores::cont::make_ArrayHandlePermutation(this->SortedValuesMap, keys);

  // Find the unique keys and the number of values per key.
  viskores::cont::ArrayHandle<viskores::IdComponent> counts;
  viskores::cont::Algorithm::ReduceByKey(
    device,
    sortedKeys,
    viskores::cont::ArrayHandleConstant<viskores::IdComponent>(1, numKeys),
    this->UniqueKeys,
    counts,
    viskores::Sum());

  // Get the offsets from the counts with a scan.
  viskores::cont::Algorithm::ScanExtended(
    device, viskores::cont::make_ArrayHandleCast(counts, viskores::Id()), this->Offsets);

  VISKORES_ASSERT(
    numKeys == viskores::cont::ArrayGetValue(this->Offsets.GetNumberOfValues() - 1, this->Offsets));
}
}
}
#endif
