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

#ifndef viskores_cont_internal_ParallelRadixSortInterface_h
#define viskores_cont_internal_ParallelRadixSortInterface_h

#include <viskores/BinaryPredicates.h>
#include <viskores/cont/ArrayHandle.h>

#include <functional>
#include <type_traits>

namespace viskores
{
namespace cont
{
namespace internal
{
namespace radix
{

const size_t MIN_BYTES_FOR_PARALLEL = 400000;
const size_t BYTES_FOR_MAX_PARALLELISM = 4000000;

struct RadixSortTag
{
};

struct PSortTag
{
};

// Detect supported functors for radix sort:
template <typename T>
struct is_valid_compare_type : std::integral_constant<bool, false>
{
};
template <typename T>
struct is_valid_compare_type<std::less<T>> : std::integral_constant<bool, true>
{
};
template <typename T>
struct is_valid_compare_type<std::greater<T>> : std::integral_constant<bool, true>
{
};
template <>
struct is_valid_compare_type<viskores::SortLess> : std::integral_constant<bool, true>
{
};
template <>
struct is_valid_compare_type<viskores::SortGreater> : std::integral_constant<bool, true>
{
};

// Convert viskores::Sort[Less|Greater] to the std:: equivalents:
template <typename BComp, typename T>
BComp&& get_std_compare(BComp&& b, T&&)
{
  return std::forward<BComp>(b);
}
template <typename T>
std::less<T> get_std_compare(viskores::SortLess, T&&)
{
  return std::less<T>{};
}
template <typename T>
std::greater<T> get_std_compare(viskores::SortGreater, T&&)
{
  return std::greater<T>{};
}

// Determine if radix sort can be used for a given ValueType, StorageType, and
// comparison functor.
template <typename T, typename StorageTag, typename BinaryCompare>
struct sort_tag_type
{
  using type = PSortTag;
};
template <typename T, typename BinaryCompare>
struct sort_tag_type<T, viskores::cont::StorageTagBasic, BinaryCompare>
{
  using PrimT = std::is_arithmetic<T>;
  using LongDT = std::is_same<T, long double>;
  using BComp = is_valid_compare_type<BinaryCompare>;
  using type = typename std::
    conditional<PrimT::value && BComp::value && !LongDT::value, RadixSortTag, PSortTag>::type;
};

template <typename KeyType,
          typename ValueType,
          typename KeyStorageTagType,
          typename ValueStorageTagType,
          class BinaryCompare>
struct sortbykey_tag_type
{
  using type = PSortTag;
};
template <typename KeyType, typename ValueType, class BinaryCompare>
struct sortbykey_tag_type<KeyType,
                          ValueType,
                          viskores::cont::StorageTagBasic,
                          viskores::cont::StorageTagBasic,
                          BinaryCompare>
{
  using PrimKey = std::is_arithmetic<KeyType>;
  using PrimValue = std::is_arithmetic<ValueType>;
  using LongDKey = std::is_same<KeyType, long double>;
  using BComp = is_valid_compare_type<BinaryCompare>;
  using type = typename std::conditional<PrimKey::value && PrimValue::value && BComp::value &&
                                           !LongDKey::value,
                                         RadixSortTag,
                                         PSortTag>::type;
};

#define VISKORES_INTERNAL_RADIX_SORT_DECLARE(key_type)                                         \
  VISKORES_CONT_EXPORT void parallel_radix_sort(                                               \
    key_type* data, size_t num_elems, const std::greater<key_type>& comp);                     \
  VISKORES_CONT_EXPORT void parallel_radix_sort(                                               \
    key_type* data, size_t num_elems, const std::less<key_type>& comp);                        \
  VISKORES_CONT_EXPORT void parallel_radix_sort_key_values(                                    \
    key_type* keys, viskores::Id* vals, size_t num_elems, const std::greater<key_type>& comp); \
  VISKORES_CONT_EXPORT void parallel_radix_sort_key_values(                                    \
    key_type* keys, viskores::Id* vals, size_t num_elems, const std::less<key_type>& comp);

// Generate radix sort interfaces for key and key value sorts.
#define VISKORES_DECLARE_RADIX_SORT()                          \
  VISKORES_INTERNAL_RADIX_SORT_DECLARE(short int)              \
  VISKORES_INTERNAL_RADIX_SORT_DECLARE(unsigned short int)     \
  VISKORES_INTERNAL_RADIX_SORT_DECLARE(int)                    \
  VISKORES_INTERNAL_RADIX_SORT_DECLARE(unsigned int)           \
  VISKORES_INTERNAL_RADIX_SORT_DECLARE(long int)               \
  VISKORES_INTERNAL_RADIX_SORT_DECLARE(unsigned long int)      \
  VISKORES_INTERNAL_RADIX_SORT_DECLARE(long long int)          \
  VISKORES_INTERNAL_RADIX_SORT_DECLARE(unsigned long long int) \
  VISKORES_INTERNAL_RADIX_SORT_DECLARE(unsigned char)          \
  VISKORES_INTERNAL_RADIX_SORT_DECLARE(signed char)            \
  VISKORES_INTERNAL_RADIX_SORT_DECLARE(char)                   \
  VISKORES_INTERNAL_RADIX_SORT_DECLARE(char16_t)               \
  VISKORES_INTERNAL_RADIX_SORT_DECLARE(char32_t)               \
  VISKORES_INTERNAL_RADIX_SORT_DECLARE(wchar_t)                \
  VISKORES_INTERNAL_RADIX_SORT_DECLARE(float)                  \
  VISKORES_INTERNAL_RADIX_SORT_DECLARE(double)
}
}
}
} // end viskores::cont::internal::radix

#endif // viskores_cont_internal_ParallelRadixSortInterface_h
