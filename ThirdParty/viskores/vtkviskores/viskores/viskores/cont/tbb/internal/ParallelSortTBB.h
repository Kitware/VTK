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

#ifndef viskores_cont_tbb_internal_ParallelSort_h
#define viskores_cont_tbb_internal_ParallelSort_h

#include <viskores/BinaryPredicates.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleZip.h>
#include <viskores/cont/internal/ParallelRadixSortInterface.h>

#include <viskores/cont/tbb/internal/DeviceAdapterTagTBB.h>
#include <viskores/cont/tbb/internal/FunctorsTBB.h>
#include <viskores/cont/tbb/internal/ParallelSortTBB.hxx>

#include <type_traits>

namespace viskores
{
namespace cont
{
namespace tbb
{
namespace sort
{

// Declare the compiled radix sort specializations:
VISKORES_DECLARE_RADIX_SORT()

// Forward declare entry points (See stack overflow discussion 7255281 --
// templated overloads of template functions are not specialization, and will
// be resolved during the first phase of two part lookup).
template <typename T, typename Container, class BinaryCompare>
void parallel_sort(viskores::cont::ArrayHandle<T, Container>&, BinaryCompare);
template <typename T, typename StorageT, typename U, typename StorageU, class BinaryCompare>
void parallel_sort_bykey(viskores::cont::ArrayHandle<T, StorageT>&,
                         viskores::cont::ArrayHandle<U, StorageU>&,
                         BinaryCompare);

// Quicksort values:
template <typename HandleType, class BinaryCompare>
void parallel_sort(HandleType& values,
                   BinaryCompare binary_compare,
                   viskores::cont::internal::radix::PSortTag)
{
  viskores::cont::Token token;
  auto arrayPortal = values.PrepareForInPlace(viskores::cont::DeviceAdapterTagTBB(), token);

  using IteratorsType = viskores::cont::ArrayPortalToIterators<decltype(arrayPortal)>;
  IteratorsType iterators(arrayPortal);

  internal::WrappedBinaryOperator<bool, BinaryCompare> wrappedCompare(binary_compare);
  ::tbb::parallel_sort(iterators.GetBegin(), iterators.GetEnd(), wrappedCompare);
}

// Radix sort values:
template <typename T, typename StorageT, class BinaryCompare>
void parallel_sort(viskores::cont::ArrayHandle<T, StorageT>& values,
                   BinaryCompare binary_compare,
                   viskores::cont::internal::radix::RadixSortTag)
{
  using namespace viskores::cont::internal::radix;
  auto c = get_std_compare(binary_compare, T{});
  viskores::cont::Token token;
  auto valuesPortal = values.PrepareForInPlace(viskores::cont::DeviceAdapterTagTBB{}, token);
  parallel_radix_sort(
    valuesPortal.GetIteratorBegin(), static_cast<std::size_t>(values.GetNumberOfValues()), c);
}

// Value sort -- static switch between quicksort and radix sort
template <typename T, typename Container, class BinaryCompare>
void parallel_sort(viskores::cont::ArrayHandle<T, Container>& values, BinaryCompare binary_compare)
{
  using namespace viskores::cont::internal::radix;
  using SortAlgorithmTag = typename sort_tag_type<T, Container, BinaryCompare>::type;
  parallel_sort(values, binary_compare, SortAlgorithmTag{});
}


// Quicksort by key
template <typename T, typename StorageT, typename U, typename StorageU, class BinaryCompare>
void parallel_sort_bykey(viskores::cont::ArrayHandle<T, StorageT>& keys,
                         viskores::cont::ArrayHandle<U, StorageU>& values,
                         BinaryCompare binary_compare,
                         viskores::cont::internal::radix::PSortTag)
{
  using namespace viskores::cont::internal::radix;
  using KeyType = viskores::cont::ArrayHandle<T, StorageT>;
  constexpr bool larger_than_64bits = sizeof(U) > sizeof(viskores::Int64);
  if (larger_than_64bits)
  {
    /// More efficient sort:
    /// Move value indexes when sorting and reorder the value array at last

    using ValueType = viskores::cont::ArrayHandle<U, StorageU>;
    using IndexType = viskores::cont::ArrayHandle<viskores::Id>;
    using ZipHandleType = viskores::cont::ArrayHandleZip<KeyType, IndexType>;

    IndexType indexArray;
    ValueType valuesScattered;
    const viskores::Id size = values.GetNumberOfValues();

    {
      viskores::cont::Token token;
      auto handle = ArrayHandleIndex(keys.GetNumberOfValues());
      auto inputPortal = handle.PrepareForInput(DeviceAdapterTagTBB(), token);
      auto outputPortal =
        indexArray.PrepareForOutput(keys.GetNumberOfValues(), DeviceAdapterTagTBB(), token);
      tbb::CopyPortals(inputPortal, outputPortal, 0, 0, keys.GetNumberOfValues());
    }

    ZipHandleType zipHandle = viskores::cont::make_ArrayHandleZip(keys, indexArray);
    parallel_sort(
      zipHandle,
      viskores::cont::internal::KeyCompare<T, viskores::Id, BinaryCompare>(binary_compare),
      PSortTag());

    {
      viskores::cont::Token token;
      tbb::ScatterPortal(
        values.PrepareForInput(viskores::cont::DeviceAdapterTagTBB(), token),
        indexArray.PrepareForInput(viskores::cont::DeviceAdapterTagTBB(), token),
        valuesScattered.PrepareForOutput(size, viskores::cont::DeviceAdapterTagTBB(), token));
    }

    {
      viskores::cont::Token token;
      auto inputPortal = valuesScattered.PrepareForInput(DeviceAdapterTagTBB(), token);
      auto outputPortal =
        values.PrepareForOutput(valuesScattered.GetNumberOfValues(), DeviceAdapterTagTBB(), token);
      tbb::CopyPortals(inputPortal, outputPortal, 0, 0, valuesScattered.GetNumberOfValues());
    }
  }
  else
  {
    using ValueType = viskores::cont::ArrayHandle<U, StorageU>;
    using ZipHandleType = viskores::cont::ArrayHandleZip<KeyType, ValueType>;

    ZipHandleType zipHandle = viskores::cont::make_ArrayHandleZip(keys, values);
    parallel_sort(zipHandle,
                  viskores::cont::internal::KeyCompare<T, U, BinaryCompare>(binary_compare),
                  PSortTag{});
  }
}

// Radix sort by key -- Specialize for viskores::Id values:
template <typename T, typename StorageT, typename StorageU, class BinaryCompare>
void parallel_sort_bykey(viskores::cont::ArrayHandle<T, StorageT>& keys,
                         viskores::cont::ArrayHandle<viskores::Id, StorageU>& values,
                         BinaryCompare binary_compare,
                         viskores::cont::internal::radix::RadixSortTag)
{
  using namespace viskores::cont::internal::radix;
  auto c = get_std_compare(binary_compare, T{});
  viskores::cont::Token token;
  auto keysPortal = keys.PrepareForInPlace(viskores::cont::DeviceAdapterTagTBB{}, token);
  auto valuesPortal = values.PrepareForInPlace(viskores::cont::DeviceAdapterTagTBB{}, token);
  parallel_radix_sort_key_values(keysPortal.GetIteratorBegin(),
                                 valuesPortal.GetIteratorBegin(),
                                 static_cast<std::size_t>(keys.GetNumberOfValues()),
                                 c);
}

// Radix sort by key -- Generic impl:
template <typename T, typename StorageT, typename U, typename StorageU, class BinaryCompare>
void parallel_sort_bykey(viskores::cont::ArrayHandle<T, StorageT>& keys,
                         viskores::cont::ArrayHandle<U, StorageU>& values,
                         BinaryCompare binary_compare,
                         viskores::cont::internal::radix::RadixSortTag)
{
  using KeyType = viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic>;
  using ValueType = viskores::cont::ArrayHandle<U, viskores::cont::StorageTagBasic>;
  using IndexType = viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagBasic>;
  using ZipHandleType = viskores::cont::ArrayHandleZip<KeyType, IndexType>;

  IndexType indexArray;
  ValueType valuesScattered;
  const viskores::Id size = values.GetNumberOfValues();

  {
    viskores::cont::Token token;
    auto handle = ArrayHandleIndex(keys.GetNumberOfValues());
    auto inputPortal = handle.PrepareForInput(DeviceAdapterTagTBB(), token);
    auto outputPortal =
      indexArray.PrepareForOutput(keys.GetNumberOfValues(), DeviceAdapterTagTBB(), token);
    tbb::CopyPortals(inputPortal, outputPortal, 0, 0, keys.GetNumberOfValues());
  }

  if (static_cast<viskores::Id>(sizeof(T)) * keys.GetNumberOfValues() > 400000)
  {
    parallel_sort_bykey(keys, indexArray, binary_compare);
  }
  else
  {
    ZipHandleType zipHandle = viskores::cont::make_ArrayHandleZip(keys, indexArray);
    parallel_sort(
      zipHandle,
      viskores::cont::internal::KeyCompare<T, viskores::Id, BinaryCompare>(binary_compare),
      viskores::cont::internal::radix::PSortTag{});
  }

  {
    viskores::cont::Token token;
    tbb::ScatterPortal(
      values.PrepareForInput(viskores::cont::DeviceAdapterTagTBB(), token),
      indexArray.PrepareForInput(viskores::cont::DeviceAdapterTagTBB(), token),
      valuesScattered.PrepareForOutput(size, viskores::cont::DeviceAdapterTagTBB(), token));
  }

  {
    viskores::cont::Token token;
    auto inputPortal = valuesScattered.PrepareForInput(DeviceAdapterTagTBB(), token);
    auto outputPortal =
      values.PrepareForOutput(valuesScattered.GetNumberOfValues(), DeviceAdapterTagTBB(), token);
    tbb::CopyPortals(inputPortal, outputPortal, 0, 0, valuesScattered.GetNumberOfValues());
  }
}

// Sort by key -- static switch between radix and quick sort:
template <typename T, typename StorageT, typename U, typename StorageU, class BinaryCompare>
void parallel_sort_bykey(viskores::cont::ArrayHandle<T, StorageT>& keys,
                         viskores::cont::ArrayHandle<U, StorageU>& values,
                         BinaryCompare binary_compare)
{
  using namespace viskores::cont::internal::radix;
  using SortAlgorithmTag =
    typename sortbykey_tag_type<T, U, StorageT, StorageU, BinaryCompare>::type;
  parallel_sort_bykey(keys, values, binary_compare, SortAlgorithmTag{});
}
}
}
}
} // end namespace viskores::cont::tbb::sort

#endif // viskores_cont_tbb_internal_ParallelSort_h
