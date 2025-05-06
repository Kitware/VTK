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

#include <viskores/cont/openmp/internal/FunctorsOpenMP.h>
#include <viskores/cont/openmp/internal/ParallelQuickSortOpenMP.h>
#include <viskores/cont/openmp/internal/ParallelRadixSortOpenMP.h>

#include <viskores/BinaryPredicates.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandleZip.h>

#include <omp.h>

namespace viskores
{
namespace cont
{
namespace openmp
{
namespace sort
{

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

  auto portal = values.PrepareForInPlace(DeviceAdapterTagOpenMP(), token);
  auto iter = viskores::cont::ArrayPortalToIteratorBegin(portal);
  viskores::Id2 range(0, values.GetNumberOfValues());

  using IterType = typename std::decay<decltype(iter)>::type;
  using Sorter = quick::QuickSorter<IterType, BinaryCompare>;

  Sorter sorter(iter, binary_compare);
  sorter.Execute(range);
}

// Radix sort values:
template <typename T, typename StorageT, class BinaryCompare>
void parallel_sort(viskores::cont::ArrayHandle<T, StorageT>& values,
                   BinaryCompare binary_compare,
                   viskores::cont::internal::radix::RadixSortTag)
{
  auto c = viskores::cont::internal::radix::get_std_compare(binary_compare, T{});
  viskores::cont::Token token;
  auto valuesPortal = values.PrepareForInPlace(viskores::cont::DeviceAdapterTagOpenMP{}, token);
  radix::parallel_radix_sort(
    valuesPortal.GetIteratorBegin(), static_cast<std::size_t>(values.GetNumberOfValues()), c);
}

// Value sort -- static switch between quicksort & radix sort
template <typename T, typename Container, class BinaryCompare>
void parallel_sort(viskores::cont::ArrayHandle<T, Container>& values, BinaryCompare binary_compare)
{
  using namespace viskores::cont::internal::radix;
  using SortAlgorithmTag = typename sort_tag_type<T, Container, BinaryCompare>::type;

  parallel_sort(values, binary_compare, SortAlgorithmTag{});
}

// Quicksort by key:
template <typename T, typename StorageT, typename U, typename StorageU, class BinaryCompare>
void parallel_sort_bykey(viskores::cont::ArrayHandle<T, StorageT>& keys,
                         viskores::cont::ArrayHandle<U, StorageU>& values,
                         BinaryCompare binary_compare,
                         viskores::cont::internal::radix::PSortTag)
{
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

    // Generate an in-memory index array:
    {
      viskores::cont::Token token;
      auto handle = ArrayHandleIndex(keys.GetNumberOfValues());
      auto inputPortal = handle.PrepareForInput(DeviceAdapterTagOpenMP(), token);
      auto outputPortal =
        indexArray.PrepareForOutput(keys.GetNumberOfValues(), DeviceAdapterTagOpenMP(), token);
      openmp::CopyHelper(inputPortal, outputPortal, 0, 0, keys.GetNumberOfValues());
    }

    // Sort the keys and indices:
    ZipHandleType zipHandle = viskores::cont::make_ArrayHandleZip(keys, indexArray);
    parallel_sort(
      zipHandle,
      viskores::cont::internal::KeyCompare<T, viskores::Id, BinaryCompare>(binary_compare),
      viskores::cont::internal::radix::PSortTag());

    // Permute the values to their sorted locations:
    {
      viskores::cont::Token token;
      auto valuesInPortal = values.PrepareForInput(DeviceAdapterTagOpenMP(), token);
      auto indexPortal = indexArray.PrepareForInput(DeviceAdapterTagOpenMP(), token);
      auto valuesOutPortal =
        valuesScattered.PrepareForOutput(size, DeviceAdapterTagOpenMP(), token);

      VISKORES_OPENMP_DIRECTIVE(parallel for
                            default(none)
                            firstprivate(valuesInPortal, indexPortal, valuesOutPortal)
                            schedule(static)
                            VISKORES_OPENMP_SHARED_CONST(size))
      for (viskores::Id i = 0; i < size; ++i)
      {
        valuesOutPortal.Set(i, valuesInPortal.Get(indexPortal.Get(i)));
      }
    }

    // Copy the values back into the input array:
    {
      viskores::cont::Token token;
      auto inputPortal = valuesScattered.PrepareForInput(DeviceAdapterTagOpenMP(), token);
      auto outputPortal = values.PrepareForOutput(
        valuesScattered.GetNumberOfValues(), DeviceAdapterTagOpenMP(), token);
      openmp::CopyHelper(inputPortal, outputPortal, 0, 0, size);
    }
  }
  else
  {
    using ValueType = viskores::cont::ArrayHandle<U, StorageU>;
    using ZipHandleType = viskores::cont::ArrayHandleZip<KeyType, ValueType>;

    ZipHandleType zipHandle = viskores::cont::make_ArrayHandleZip(keys, values);
    parallel_sort(zipHandle,
                  viskores::cont::internal::KeyCompare<T, U, BinaryCompare>(binary_compare),
                  viskores::cont::internal::radix::PSortTag{});
  }
}

// Radix sort by key:
template <typename T, typename StorageT, typename StorageU, class BinaryCompare>
void parallel_sort_bykey(viskores::cont::ArrayHandle<T, StorageT>& keys,
                         viskores::cont::ArrayHandle<viskores::Id, StorageU>& values,
                         BinaryCompare binary_compare,
                         viskores::cont::internal::radix::RadixSortTag)
{
  using namespace viskores::cont::internal::radix;
  auto c = get_std_compare(binary_compare, T{});
  viskores::cont::Token token;
  auto keysPortal = keys.PrepareForInPlace(viskores::cont::DeviceAdapterTagOpenMP{}, token);
  auto valuesPortal = values.PrepareForInPlace(viskores::cont::DeviceAdapterTagOpenMP{}, token);
  radix::parallel_radix_sort_key_values(keysPortal.GetIteratorBegin(),
                                        valuesPortal.GetIteratorBegin(),
                                        static_cast<std::size_t>(keys.GetNumberOfValues()),
                                        c);
}
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
    auto inputPortal = handle.PrepareForInput(DeviceAdapterTagOpenMP(), token);
    auto outputPortal =
      indexArray.PrepareForOutput(keys.GetNumberOfValues(), DeviceAdapterTagOpenMP(), token);
    openmp::CopyHelper(inputPortal, outputPortal, 0, 0, keys.GetNumberOfValues());
  }

  const viskores::Id valuesBytes = static_cast<viskores::Id>(sizeof(T)) * keys.GetNumberOfValues();
  if (valuesBytes >
      static_cast<viskores::Id>(viskores::cont::internal::radix::MIN_BYTES_FOR_PARALLEL))
  {
    parallel_sort_bykey(keys, indexArray, binary_compare);
  }
  else
  {
    ZipHandleType zipHandle = viskores::cont::make_ArrayHandleZip(keys, indexArray);
    parallel_sort(
      zipHandle,
      viskores::cont::internal::KeyCompare<T, viskores::Id, BinaryCompare>(binary_compare),
      viskores::cont::internal::radix::PSortTag());
  }

  // Permute the values to their sorted locations:
  {
    viskores::cont::Token token;
    auto valuesInPortal = values.PrepareForInput(DeviceAdapterTagOpenMP(), token);
    auto indexPortal = indexArray.PrepareForInput(DeviceAdapterTagOpenMP(), token);
    auto valuesOutPortal = valuesScattered.PrepareForOutput(size, DeviceAdapterTagOpenMP(), token);

    VISKORES_OPENMP_DIRECTIVE(parallel for
                          default(none)
                          firstprivate(valuesInPortal, indexPortal, valuesOutPortal)
                          VISKORES_OPENMP_SHARED_CONST(size)
                          schedule(static))
    for (viskores::Id i = 0; i < size; ++i)
    {
      valuesOutPortal.Set(i, valuesInPortal.Get(indexPortal.Get(i)));
    }
  }

  {
    viskores::cont::Token token;
    auto inputPortal = valuesScattered.PrepareForInput(DeviceAdapterTagOpenMP(), token);
    auto outputPortal =
      values.PrepareForOutput(valuesScattered.GetNumberOfValues(), DeviceAdapterTagOpenMP(), token);
    openmp::CopyHelper(inputPortal, outputPortal, 0, 0, valuesScattered.GetNumberOfValues());
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
} // end namespace viskores::cont::openmp::sort
