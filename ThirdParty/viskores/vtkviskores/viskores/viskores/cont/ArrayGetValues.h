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
#ifndef viskores_cont_ArrayGetValues_h
#define viskores_cont_ArrayGetValues_h

#include <viskores/cont/viskores_cont_export.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <initializer_list>
#include <vector>

namespace viskores
{
namespace cont
{

// Work around circular dependancy with UnknownArrayHandle.
class UnknownArrayHandle;

namespace internal
{

VISKORES_CONT_EXPORT void ArrayGetValuesImpl(const viskores::cont::UnknownArrayHandle& ids,
                                             const viskores::cont::UnknownArrayHandle& data,
                                             const viskores::cont::UnknownArrayHandle& output,
                                             std::false_type extractComponentInefficient);

template <typename IdsArrayHandle, typename DataArrayHandle, typename OutputArrayHandle>
void ArrayGetValuesImpl(const IdsArrayHandle& ids,
                        const DataArrayHandle& data,
                        const OutputArrayHandle& output,
                        std::true_type viskoresNotUsed(extractComponentInefficient))
{
  // Fallback implementation. Using UnknownArrayHandle to extract the data would be more
  // inefficient than simply getting the ReadPortal (which could potentially copy everything
  // form device to host), so we do that here. The only other alternative would be to write
  // a custom worklet, but that would require a device compiler, and we are avoiding that for
  // this header.
  viskores::Id outputSize = ids.GetNumberOfValues();
  output.Allocate(outputSize);
  auto idsPortal = ids.ReadPortal();
  auto dataPortal = data.ReadPortal();
  auto outputPortal = output.WritePortal();
  for (viskores::Id index = 0; index < outputSize; ++index)
  {
    outputPortal.Set(index, dataPortal.Get(idsPortal.Get(index)));
  }
}

} // namespace internal

/// \brief Obtain a small set of values from an ArrayHandle with minimal device
/// transfers.
///
/// The values in @a data at the indices in @a ids are copied into a new array
/// and returned. This is useful for retrieving a subset of an array from a
/// device without transferring the entire array to the host.
///
/// These functions should not be called repeatedly in a loop to fetch all
/// values from an array handle. The much more efficient way to do this is to
/// use the proper control-side portals (ArrayHandle::WritePortal() and
/// ArrayHandle::ReadPortal()).
///
/// This method will attempt to copy the data using the device that the input
/// data is already valid on. If the input data is only valid in the control
/// environment or the device copy fails, a control-side copy is performed.
///
/// Since a serial control-side copy may be used, this method is only intended
/// for copying small subsets of the input data. Larger subsets that would
/// benefit from parallelization should prefer using the ArrayCopy method with
/// an ArrayHandlePermutation.
///
/// This utility provides several convenient overloads:
///
/// A single id may be passed into ArrayGetValue, or multiple ids may be
/// specified to ArrayGetValues as an ArrayHandle<viskores::Id>, a
/// std::vector<viskores::Id>, a c-array (pointer and size), or as a brace-enclosed
/// initializer list.
///
/// The single result from ArrayGetValue may be returned or written to an output
/// argument. Multiple results from ArrayGetValues may be returned as an
/// std::vector<T>, or written to an output argument as an ArrayHandle<T> or a
/// std::vector<T>.
///
/// Examples:
///
/// ```
/// viskores::cont::ArrayHandle<T> data = ...;
///
/// // Fetch the first value in an array handle:
/// T firstVal = viskores::cont::ArrayGetValue(0, data);
///
/// // Fetch the first and third values in an array handle:
/// std::vector<T> firstAndThird = viskores::cont::ArrayGetValues({0, 2}, data);
///
/// // Fetch the first and last values in an array handle:
/// std::vector<T> firstAndLast =
///     viskores::cont::ArrayGetValues({0, data.GetNumberOfValues() - 1}, data);
///
/// // Fetch the first 4 values into an array handle:
/// const std::vector<viskores::Id> ids{0, 1, 2, 3};
/// viskores::cont::ArrayHandle<T> firstFour;
/// viskores::cont::ArrayGetValues(ids, data, firstFour);
/// ```
///
///
///@{
///
template <typename SIds, typename T, typename SData, typename SOut>
VISKORES_CONT void ArrayGetValues(const viskores::cont::ArrayHandle<viskores::Id, SIds>& ids,
                                  const viskores::cont::ArrayHandle<T, SData>& data,
                                  viskores::cont::ArrayHandle<T, SOut>& output)
{
  using DataArrayHandle = viskores::cont::ArrayHandle<T, SData>;
  using InefficientExtract =
    viskores::cont::internal::ArrayExtractComponentIsInefficient<DataArrayHandle>;
  internal::ArrayGetValuesImpl(ids, data, output, InefficientExtract{});
}

/// We need a specialization for `ArrayHandleCasts` to avoid runtime type missmatch errors inside
/// `ArrayGetValuesImpl`.
template <typename SIds, typename TIn, typename SData, typename TOut, typename SOut>
VISKORES_CONT void ArrayGetValues(
  const viskores::cont::ArrayHandle<viskores::Id, SIds>& ids,
  const viskores::cont::ArrayHandle<TOut, viskores::cont::StorageTagCast<TIn, SData>>& data,
  viskores::cont::ArrayHandle<TOut, SOut>& output)
{
  // In this specialization, we extract the values from the cast array's source array and
  // then cast and copy to output.
  viskores::cont::ArrayHandleBasic<TIn> tempOutput;
  viskores::cont::ArrayHandleCast<TOut, viskores::cont::ArrayHandle<TIn, SData>> castArray = data;
  ArrayGetValues(ids, castArray.GetSourceArray(), tempOutput);

  viskores::Id numExtracted = tempOutput.GetNumberOfValues();
  output.Allocate(numExtracted);
  auto inp = tempOutput.ReadPortal();
  auto outp = output.WritePortal();
  for (viskores::Id i = 0; i < numExtracted; ++i)
  {
    outp.Set(i, static_cast<TOut>(inp.Get(i)));
  }
}

template <typename SIds, typename T, typename SData, typename Alloc>
VISKORES_CONT void ArrayGetValues(const viskores::cont::ArrayHandle<viskores::Id, SIds>& ids,
                                  const viskores::cont::ArrayHandle<T, SData>& data,
                                  std::vector<T, Alloc>& output)
{
  const std::size_t numVals = static_cast<std::size_t>(ids.GetNumberOfValues());

  // Allocate the vector and toss its data pointer into the array handle.
  output.resize(numVals);
  auto result = viskores::cont::make_ArrayHandle(output, viskores::CopyFlag::Off);
  viskores::cont::ArrayGetValues(ids, data, result);
  // Make sure to pull the data back to control before we dealloc the handle
  // that wraps the vec memory:
  result.SyncControlArray();
}

template <typename SIds, typename T, typename SData>
VISKORES_CONT std::vector<T> ArrayGetValues(
  const viskores::cont::ArrayHandle<viskores::Id, SIds>& ids,
  const viskores::cont::ArrayHandle<T, SData>& data)
{
  std::vector<T> result;
  viskores::cont::ArrayGetValues(ids, data, result);
  return result;
}

template <typename T, typename Alloc, typename SData, typename SOut>
VISKORES_CONT void ArrayGetValues(const std::vector<viskores::Id, Alloc>& ids,
                                  const viskores::cont::ArrayHandle<T, SData>& data,
                                  viskores::cont::ArrayHandle<T, SOut>& output)
{
  const auto idsAH = viskores::cont::make_ArrayHandle(ids, viskores::CopyFlag::Off);
  ArrayGetValues(idsAH, data, output);
}

template <typename T, typename AllocId, typename SData, typename AllocOut>
VISKORES_CONT void ArrayGetValues(const std::vector<viskores::Id, AllocId>& ids,
                                  const viskores::cont::ArrayHandle<T, SData>& data,
                                  std::vector<T, AllocOut>& output)
{
  const auto idsAH = viskores::cont::make_ArrayHandle(ids, viskores::CopyFlag::Off);
  ArrayGetValues(idsAH, data, output);
}

template <typename T, typename Alloc, typename SData>
VISKORES_CONT std::vector<T> ArrayGetValues(const std::vector<viskores::Id, Alloc>& ids,
                                            const viskores::cont::ArrayHandle<T, SData>& data)
{
  const auto idsAH = viskores::cont::make_ArrayHandle(ids, viskores::CopyFlag::Off);
  return ArrayGetValues(idsAH, data);
}

template <typename T, typename SData, typename SOut>
VISKORES_CONT void ArrayGetValues(const std::initializer_list<viskores::Id>& ids,
                                  const viskores::cont::ArrayHandle<T, SData>& data,
                                  viskores::cont::ArrayHandle<T, SOut>& output)
{
  const auto idsAH = viskores::cont::make_ArrayHandle(
    ids.begin(), static_cast<viskores::Id>(ids.size()), viskores::CopyFlag::Off);
  ArrayGetValues(idsAH, data, output);
}

template <typename T, typename SData, typename Alloc>
VISKORES_CONT void ArrayGetValues(const std::initializer_list<viskores::Id>& ids,
                                  const viskores::cont::ArrayHandle<T, SData>& data,
                                  std::vector<T, Alloc>& output)
{
  const auto idsAH = viskores::cont::make_ArrayHandle(
    ids.begin(), static_cast<viskores::Id>(ids.size()), viskores::CopyFlag::Off);
  ArrayGetValues(idsAH, data, output);
}
template <typename T, typename SData>
VISKORES_CONT std::vector<T> ArrayGetValues(const std::initializer_list<viskores::Id>& ids,
                                            const viskores::cont::ArrayHandle<T, SData>& data)
{
  const auto idsAH = viskores::cont::make_ArrayHandle(
    ids.begin(), static_cast<viskores::Id>(ids.size()), viskores::CopyFlag::Off);
  return ArrayGetValues(idsAH, data);
}

template <typename T, typename SData, typename SOut>
VISKORES_CONT void ArrayGetValues(const viskores::Id* ids,
                                  const viskores::Id numIds,
                                  const viskores::cont::ArrayHandle<T, SData>& data,
                                  viskores::cont::ArrayHandle<T, SOut>& output)
{
  const auto idsAH = viskores::cont::make_ArrayHandle(ids, numIds, viskores::CopyFlag::Off);
  ArrayGetValues(idsAH, data, output);
}

template <typename T, typename SData, typename Alloc>
VISKORES_CONT void ArrayGetValues(const viskores::Id* ids,
                                  const viskores::Id numIds,
                                  const viskores::cont::ArrayHandle<T, SData>& data,
                                  std::vector<T, Alloc>& output)
{
  const auto idsAH = viskores::cont::make_ArrayHandle(ids, numIds, viskores::CopyFlag::Off);
  ArrayGetValues(idsAH, data, output);
}
template <typename T, typename SData>
VISKORES_CONT std::vector<T> ArrayGetValues(const viskores::Id* ids,
                                            const viskores::Id numIds,
                                            const viskores::cont::ArrayHandle<T, SData>& data)
{
  const auto idsAH = viskores::cont::make_ArrayHandle(ids, numIds, viskores::CopyFlag::Off);
  return ArrayGetValues(idsAH, data);
}

template <typename T, typename S>
VISKORES_CONT T ArrayGetValue(viskores::Id id, const viskores::cont::ArrayHandle<T, S>& data)
{
  const auto idAH = viskores::cont::make_ArrayHandle(&id, 1, viskores::CopyFlag::Off);
  auto result = viskores::cont::ArrayGetValues(idAH, data);
  return result[0];
}

template <typename T, typename S>
VISKORES_CONT void ArrayGetValue(viskores::Id id,
                                 const viskores::cont::ArrayHandle<T, S>& data,
                                 T& val)
{
  val = ArrayGetValue(id, data);
}
/// @}
}
} // namespace viskores::cont

#endif //viskores_cont_ArrayGetValues_h
