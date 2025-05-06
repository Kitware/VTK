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
#ifndef viskores_worklet_SortAndUniqueIndices_h
#define viskores_worklet_SortAndUniqueIndices_h

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ExecutionObjectBase.h>

namespace viskores
{
namespace worklet
{

/// Produces an ArrayHandle<viskores::Id> index array that stable-sorts and
/// optionally uniquifies an input array.
struct StableSortIndices
{
  using IndexArrayType = viskores::cont::ArrayHandle<viskores::Id>;

  // Allows Sort to be called on an array that indexes into KeyPortal.
  // If the values compare equal, the indices are compared to stabilize the
  // result.
  template <typename KeyPortalType>
  struct IndirectSortPredicate
  {
    using KeyType = typename KeyPortalType::ValueType;

    const KeyPortalType KeyPortal;

    VISKORES_CONT
    IndirectSortPredicate(const KeyPortalType& keyPortal)
      : KeyPortal(keyPortal)
    {
    }

    template <typename IndexType>
    VISKORES_EXEC bool operator()(const IndexType& a, const IndexType& b) const
    {
      // If the values compare equal, compare the indices as well so we get
      // consistent outputs.
      const KeyType valueA = this->KeyPortal.Get(a);
      const KeyType valueB = this->KeyPortal.Get(b);
      if (valueA < valueB)
      {
        return true;
      }
      else if (valueB < valueA)
      {
        return false;
      }
      else
      {
        return a < b;
      }
    }
  };

  // Allows you to pass an IndirectSortPredicate to a device algorithm without knowing the device.
  template <typename KeyArrayType>
  struct IndirectSortPredicateExecObject : public viskores::cont::ExecutionObjectBase
  {
    const KeyArrayType KeyArray;

    VISKORES_CONT IndirectSortPredicateExecObject(const KeyArrayType& keyArray)
      : KeyArray(keyArray)
    {
    }

    template <typename Device>
    IndirectSortPredicate<typename KeyArrayType::ReadPortalType> PrepareForExecution(
      Device,
      viskores::cont::Token& token) const
    {
      auto keyPortal = this->KeyArray.PrepareForInput(Device(), token);
      return IndirectSortPredicate<decltype(keyPortal)>(keyPortal);
    }
  };

  // Allows Unique to be called on an array that indexes into KeyPortal.
  template <typename KeyPortalType>
  struct IndirectUniquePredicate
  {
    const KeyPortalType KeyPortal;

    VISKORES_CONT
    IndirectUniquePredicate(const KeyPortalType& keyPortal)
      : KeyPortal(keyPortal)
    {
    }

    template <typename IndexType>
    VISKORES_EXEC bool operator()(const IndexType& a, const IndexType& b) const
    {
      return this->KeyPortal.Get(a) == this->KeyPortal.Get(b);
    }
  };

  // Allows you to pass an IndirectUniquePredicate to a device algorithm without knowing the device.
  template <typename KeyArrayType>
  struct IndirectUniquePredicateExecObject : public viskores::cont::ExecutionObjectBase
  {
    const KeyArrayType KeyArray;

    VISKORES_CONT IndirectUniquePredicateExecObject(const KeyArrayType& keyArray)
      : KeyArray(keyArray)
    {
    }

    template <typename Device>
    IndirectUniquePredicate<typename KeyArrayType::ReadPortalType> PrepareForExecution(
      Device,
      viskores::cont::Token& token) const
    {
      auto keyPortal = this->KeyArray.PrepareForInput(Device(), token);
      return IndirectUniquePredicate<decltype(keyPortal)>(keyPortal);
    }
  };

  /// Permutes the @a indices array so that it will map @a keys into a stable
  /// sorted order. The @a keys array is not modified.
  ///
  /// @param device The Id for the device on which to compute the sort
  /// @param keys The ArrayHandle containing data to be sorted.
  /// @param indices The ArrayHandle<viskores::Id> containing the permutation indices.
  ///
  /// @note @a indices is expected to contain the values (0, numKeys] in
  /// increasing order. If the values in @a indices are not sequential, the sort
  /// will succeed and be consistently reproducible, but the result is not
  /// guaranteed to be stable with respect to the original ordering of @a keys.
  template <typename KeyType, typename Storage>
  VISKORES_CONT static void Sort(viskores::cont::DeviceAdapterId device,
                                 const viskores::cont::ArrayHandle<KeyType, Storage>& keys,
                                 IndexArrayType& indices)
  {
    using KeyArrayType = viskores::cont::ArrayHandle<KeyType, Storage>;
    using SortPredicate = IndirectSortPredicateExecObject<KeyArrayType>;

    VISKORES_ASSERT(keys.GetNumberOfValues() == indices.GetNumberOfValues());

    viskores::cont::Algorithm::Sort(device, indices, SortPredicate(keys));
  }

  /// Permutes the @a indices array so that it will map @a keys into a stable
  /// sorted order. The @a keys array is not modified.
  ///
  /// @param keys The ArrayHandle containing data to be sorted.
  /// @param indices The ArrayHandle<viskores::Id> containing the permutation indices.
  ///
  /// @note @a indices is expected to contain the values (0, numKeys] in
  /// increasing order. If the values in @a indices are not sequential, the sort
  /// will succeed and be consistently reproducible, but the result is not
  /// guaranteed to be stable with respect to the original ordering of @a keys.
  template <typename KeyType, typename Storage>
  VISKORES_CONT static void Sort(const viskores::cont::ArrayHandle<KeyType, Storage>& keys,
                                 IndexArrayType& indices)
  {
    StableSortIndices::Sort(viskores::cont::DeviceAdapterTagAny(), keys, indices);
  }

  /// Returns an index array that maps the @a keys array into a stable sorted
  /// ordering. The @a keys array is not modified.
  ///
  /// This is a convenience overload that generates the index array.
  template <typename KeyType, typename Storage>
  VISKORES_CONT static IndexArrayType Sort(
    viskores::cont::DeviceAdapterId device,
    const viskores::cont::ArrayHandle<KeyType, Storage>& keys)
  {
    // Generate the initial index array
    IndexArrayType indices;
    {
      viskores::cont::ArrayHandleIndex indicesSrc(keys.GetNumberOfValues());
      viskores::cont::Algorithm::Copy(device, indicesSrc, indices);
    }

    StableSortIndices::Sort(device, keys, indices);

    return indices;
  }

  /// Returns an index array that maps the @a keys array into a stable sorted
  /// ordering. The @a keys array is not modified.
  ///
  /// This is a convenience overload that generates the index array.
  template <typename KeyType, typename Storage>
  VISKORES_CONT static IndexArrayType Sort(
    const viskores::cont::ArrayHandle<KeyType, Storage>& keys)
  {
    return StableSortIndices::Sort(viskores::cont::DeviceAdapterTagAny(), keys);
  }

  /// Reduces the array returned by @a Sort so that the mapped @a keys are
  /// unique. The @a indices array will be modified in-place and the @a keys
  /// array is not modified.
  ///
  template <typename KeyType, typename Storage>
  VISKORES_CONT static void Unique(viskores::cont::DeviceAdapterId device,
                                   const viskores::cont::ArrayHandle<KeyType, Storage>& keys,
                                   IndexArrayType& indices)
  {
    using KeyArrayType = viskores::cont::ArrayHandle<KeyType, Storage>;
    using UniquePredicate = IndirectUniquePredicateExecObject<KeyArrayType>;

    viskores::cont::Algorithm::Unique(device, indices, UniquePredicate(keys));
  }

  /// Reduces the array returned by @a Sort so that the mapped @a keys are
  /// unique. The @a indices array will be modified in-place and the @a keys
  /// array is not modified.
  ///
  template <typename KeyType, typename Storage>
  VISKORES_CONT static void Unique(const viskores::cont::ArrayHandle<KeyType, Storage>& keys,
                                   IndexArrayType& indices)
  {
    StableSortIndices::Unique(viskores::cont::DeviceAdapterTagAny(), keys, indices);
  }
};
}
} // end namespace viskores::worklet

#endif // viskores_worklet_SortAndUniqueIndices_h
