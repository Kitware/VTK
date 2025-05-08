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

#ifndef viskores_cont_ArraySetValues_h
#define viskores_cont_ArraySetValues_h

#include <viskores/cont/viskores_cont_export.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <initializer_list>
#include <vector>

namespace viskores
{
namespace cont
{

namespace internal
{

VISKORES_CONT_EXPORT void ArraySetValuesImpl(const viskores::cont::UnknownArrayHandle& ids,
                                             const viskores::cont::UnknownArrayHandle& values,
                                             const viskores::cont::UnknownArrayHandle& data,
                                             std::false_type extractComponentInefficient);

template <typename IdsArrayHandle, typename ValuesArrayHandle, typename DataArrayHandle>
void ArraySetValuesImpl(const IdsArrayHandle& ids,
                        const ValuesArrayHandle& values,
                        const DataArrayHandle& data,
                        std::true_type viskoresNotUsed(extractComponentInefficient))
{
  // Fallback implementation using control portals when device operations would be inefficient
  viskores::Id numValues = ids.GetNumberOfValues();
  VISKORES_ASSERT(values.GetNumberOfValues() == numValues);

  auto idsPortal = ids.ReadPortal();
  auto valuesPortal = values.ReadPortal();
  auto dataPortal = data.WritePortal();

  for (viskores::Id index = 0; index < numValues; ++index)
  {
    dataPortal.Set(idsPortal.Get(index), valuesPortal.Get(index));
  }
}

} // namespace internal

/// \brief Set a small set of values in an ArrayHandle with minimal device transfers.
///
/// The values in @a values are copied into @a data at the indices specified in @a ids.
/// This is useful for updating a subset of an array on a device without transferring
/// the entire array.
///
/// These functions should not be called repeatedly in a loop to set all values in
/// an array handle. The much more efficient way to do this is to use the proper
/// control-side portals (ArrayHandle::WritePortal()) or to do so in a worklet.
///
/// This method will attempt to copy the data using the device that the input
/// data is already valid on. If the input data is only valid in the control
/// environment or the device copy fails, a control-side copy is performed.
///
/// Since a serial control-side copy may be used, this method is only intended
/// for copying small subsets of the input data. Larger subsets that would
/// benefit from parallelization should prefer using ArrayCopy with an
/// ArrayHandlePermutation.
///
/// This utility provides several convenient overloads:
///
/// A single id and value may be passed into ArraySetValue, or multiple ids and values
/// may be specified to ArraySetValues as ArrayHandles, std::vectors, c-arrays
/// (pointer and size), or as brace-enclosed initializer lists.
///
/// Examples:
///
/// ```cpp
/// viskores::cont::ArrayHandle<T> data = ...;
///
/// // Set a single value in an array handle:
/// viskores::cont::ArraySetValue(0, T{42}, data);
///
/// // Set the first and third values in an array handle:
/// viskores::cont::ArraySetValues({0, 2}, {T{10}, T{30}}, data);
///
/// // Set values using std::vector
/// std::vector<viskores::Id> ids{0, 1, 2, 3};
/// std::vector<T> values{T{10}, T{20}, T{30}, T{40}};
/// viskores::cont::ArraySetValues(ids, values, data);
///
/// // Set values using array handles directly
/// viskores::cont::ArrayHandle<viskores::Id> idsHandle;
/// viskores::cont::ArrayHandle<T> valuesHandle;
/// // ... populate handles ...
/// viskores::cont::ArraySetValues(idsHandle, valuesHandle, data);
///
/// // Set values using raw pointers
/// viskores::Id rawIds[] = {0, 1, 2};
/// T rawValues[] = {T{10}, T{20}, T{30}};
/// viskores::cont::ArraySetValues(rawIds, 3, rawValues, 3, data);
/// ```
///
///@{
///
template <typename SIds, typename T, typename SValues, typename SData>
VISKORES_CONT void ArraySetValues(const viskores::cont::ArrayHandle<viskores::Id, SIds>& ids,
                                  const viskores::cont::ArrayHandle<T, SValues>& values,
                                  const viskores::cont::ArrayHandle<T, SData>& data)
{
  using DataArrayHandle = viskores::cont::ArrayHandle<T, SData>;
  using InefficientExtract =
    viskores::cont::internal::ArrayExtractComponentIsInefficient<DataArrayHandle>;
  internal::ArraySetValuesImpl(ids, values, data, InefficientExtract{});
}

/// Specialization for ArrayHandleCasts
template <typename SIds, typename TIn, typename SValues, typename TOut, typename SData>
VISKORES_CONT void ArraySetValues(
  const viskores::cont::ArrayHandle<viskores::Id, SIds>& ids,
  const viskores::cont::ArrayHandle<TIn, SValues>& values,
  const viskores::cont::ArrayHandle<TOut, viskores::cont::StorageTagCast<TIn, SData>>& data)
{
  viskores::cont::ArrayHandleBasic<TIn> tempValues;
  tempValues.Allocate(values.GetNumberOfValues());
  auto inp = values.ReadPortal();
  auto outp = tempValues.WritePortal();
  for (viskores::Id i = 0; i < values.GetNumberOfValues(); ++i)
  {
    outp.Set(i, static_cast<TIn>(inp.Get(i)));
  }

  viskores::cont::ArrayHandleCast<TOut, viskores::cont::ArrayHandle<TIn, SData>> castArray = data;
  ArraySetValues(ids, tempValues, castArray.GetSourceArray());
}

template <typename SIds, typename T, typename SData, typename Alloc>
VISKORES_CONT void ArraySetValues(const viskores::cont::ArrayHandle<viskores::Id, SIds>& ids,
                                  const std::vector<T, Alloc>& values,
                                  const viskores::cont::ArrayHandle<T, SData>& data)
{
  const auto valuesAH = viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::Off);
  ArraySetValues(ids, valuesAH, data);
}

template <typename T, typename SIds, typename SValues, typename SData>
VISKORES_CONT void ArraySetValues(const std::vector<viskores::Id, SIds>& ids,
                                  const viskores::cont::ArrayHandle<T, SValues>& values,
                                  const viskores::cont::ArrayHandle<T, SData>& data)
{
  const auto idsAH = viskores::cont::make_ArrayHandle(ids, viskores::CopyFlag::Off);
  ArraySetValues(idsAH, values, data);
}

template <typename T, typename AllocId, typename AllocVal, typename SData>
VISKORES_CONT void ArraySetValues(const std::vector<viskores::Id, AllocId>& ids,
                                  const std::vector<T, AllocVal>& values,
                                  const viskores::cont::ArrayHandle<T, SData>& data)
{
  const auto idsAH = viskores::cont::make_ArrayHandle(ids, viskores::CopyFlag::Off);
  const auto valuesAH = viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::Off);
  ArraySetValues(idsAH, valuesAH, data);
}

template <typename T, typename SData, typename Alloc>
VISKORES_CONT void ArraySetValues(const std::initializer_list<viskores::Id>& ids,
                                  const std::vector<T, Alloc>& values,
                                  const viskores::cont::ArrayHandle<T, SData>& data)
{
  const auto idsAH = viskores::cont::make_ArrayHandle(
    ids.begin(), static_cast<viskores::Id>(ids.size()), viskores::CopyFlag::Off);
  const auto valuesAH = viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::Off);
  ArraySetValues(idsAH, valuesAH, data);
}

template <typename T, typename SData>
VISKORES_CONT void ArraySetValues(const std::initializer_list<viskores::Id>& ids,
                                  const std::initializer_list<T>& values,
                                  const viskores::cont::ArrayHandle<T, SData>& data)
{
  const auto idsAH = viskores::cont::make_ArrayHandle(
    ids.begin(), static_cast<viskores::Id>(ids.size()), viskores::CopyFlag::Off);
  const auto valuesAH = viskores::cont::make_ArrayHandle(
    values.begin(), static_cast<viskores::Id>(values.size()), viskores::CopyFlag::Off);
  ArraySetValues(idsAH, valuesAH, data);
}

template <typename T, typename SValues, typename SData>
VISKORES_CONT void ArraySetValues(const std::initializer_list<viskores::Id>& ids,
                                  const viskores::cont::ArrayHandle<T, SValues>& values,
                                  const viskores::cont::ArrayHandle<T, SData>& data)
{
  const auto idsAH = viskores::cont::make_ArrayHandle(
    ids.begin(), static_cast<viskores::Id>(ids.size()), viskores::CopyFlag::Off);
  ArraySetValues(idsAH, values, data);
}

template <typename T, typename SData>
VISKORES_CONT void ArraySetValues(const viskores::Id* ids,
                                  const viskores::Id numIds,
                                  const std::vector<T>& values,
                                  const viskores::cont::ArrayHandle<T, SData>& data)
{
  VISKORES_ASSERT(numIds == static_cast<viskores::Id>(values.size()));
  const auto idsAH = viskores::cont::make_ArrayHandle(ids, numIds, viskores::CopyFlag::Off);
  const auto valuesAH =
    viskores::cont::make_ArrayHandle(values.data(), values.size(), viskores::CopyFlag::Off);
  ArraySetValues(idsAH, valuesAH, data);
}

template <typename T, typename SData>
VISKORES_CONT void ArraySetValues(const viskores::Id* ids,
                                  const viskores::Id numIds,
                                  const T* values,
                                  const viskores::Id numValues,
                                  const viskores::cont::ArrayHandle<T, SData>& data)
{
  VISKORES_ASSERT(numIds == numValues);
  const auto idsAH = viskores::cont::make_ArrayHandle(ids, numIds, viskores::CopyFlag::Off);
  const auto valuesAH =
    viskores::cont::make_ArrayHandle(values, numValues, viskores::CopyFlag::Off);
  ArraySetValues(idsAH, valuesAH, data);
}

template <typename T, typename SData>
VISKORES_CONT void ArraySetValues(const viskores::Id* ids,
                                  const viskores::Id numIds,
                                  const viskores::cont::ArrayHandle<T, SData>& values,
                                  const viskores::cont::ArrayHandle<T, SData>& data)
{
  VISKORES_ASSERT(numIds == values.GetNumberOfValues());
  const auto idsAH = viskores::cont::make_ArrayHandle(ids, numIds, viskores::CopyFlag::Off);
  ArraySetValues(idsAH, values, data);
}

/// \brief Set a single value in an ArrayHandle at the specified index.
///
/// This is a convenience function that sets a single value at the given index.
/// It is equivalent to calling ArraySetValues with single-element arrays.
///
template <typename T, typename SData>
VISKORES_CONT void ArraySetValue(viskores::Id id,
                                 const T& value,
                                 const viskores::cont::ArrayHandle<T, SData>& data)
{
  const auto idAH = viskores::cont::make_ArrayHandle(&id, 1, viskores::CopyFlag::Off);
  const auto valueAH = viskores::cont::make_ArrayHandle(&value, 1, viskores::CopyFlag::Off);
  ArraySetValues(idAH, valueAH, data);
}

///@}

} // namespace cont
} // namespace viskores

#endif //viskores_cont_ArraySetValues_h
