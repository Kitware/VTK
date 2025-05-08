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
#ifndef viskores_worklet_Keys_h
#define viskores_worklet_Keys_h

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleGroupVecVariable.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/Logging.h>

#include <viskores/Deprecated.h>
#include <viskores/Hash.h>

#include <viskores/exec/internal/ReduceByKeyLookup.h>

#include <viskores/cont/arg/TransportTagKeyedValuesIn.h>
#include <viskores/cont/arg/TransportTagKeyedValuesInOut.h>
#include <viskores/cont/arg/TransportTagKeyedValuesOut.h>
#include <viskores/cont/arg/TransportTagKeysIn.h>
#include <viskores/cont/arg/TypeCheckTagKeys.h>

#include <viskores/worklet/internal/DispatcherBase.h>

#include <viskores/worklet/StableSortIndices.h>
#include <viskores/worklet/viskores_worklet_export.h>

#include <viskores/BinaryOperators.h>

namespace viskores
{
namespace worklet
{

namespace internal
{

class VISKORES_WORKLET_EXPORT KeysBase
{
public:
  KeysBase(const KeysBase&) = default;
  KeysBase& operator=(const KeysBase&) = default;
  ~KeysBase() = default;

  VISKORES_CONT
  viskores::Id GetInputRange() const { return this->Offsets.GetNumberOfValues() - 1; }

  VISKORES_CONT
  viskores::cont::ArrayHandle<viskores::Id> GetSortedValuesMap() const
  {
    return this->SortedValuesMap;
  }

  VISKORES_CONT
  viskores::cont::ArrayHandle<viskores::Id> GetOffsets() const { return this->Offsets; }

  VISKORES_DEPRECATED(2.2,
                      "Use the `GetOffsets()` array in an `ArrayHandleOffsetsToNumComponents`.")
  VISKORES_CONT
  viskores::cont::ArrayHandle<viskores::IdComponent> GetCounts() const;

  VISKORES_CONT
  viskores::Id GetNumberOfValues() const { return this->SortedValuesMap.GetNumberOfValues(); }

  using ExecLookup = viskores::exec::internal::ReduceByKeyLookupBase<
    typename viskores::cont::ArrayHandle<viskores::Id>::ReadPortalType,
    typename viskores::cont::ArrayHandle<viskores::IdComponent>::ReadPortalType>;

  VISKORES_CONT ExecLookup PrepareForInput(viskores::cont::DeviceAdapterId device,
                                           viskores::cont::Token& token) const
  {
    return ExecLookup(this->SortedValuesMap.PrepareForInput(device, token),
                      this->Offsets.PrepareForInput(device, token));
  }

  VISKORES_CONT
  bool operator==(const viskores::worklet::internal::KeysBase& other) const
  {
    return ((this->SortedValuesMap == other.SortedValuesMap) && (this->Offsets == other.Offsets) &&
            (this->Offsets == other.Offsets));
  }

  VISKORES_CONT
  bool operator!=(const viskores::worklet::internal::KeysBase& other) const
  {
    return !(*this == other);
  }

protected:
  KeysBase() = default;

  viskores::cont::ArrayHandle<viskores::Id> SortedValuesMap;
  viskores::cont::ArrayHandle<viskores::Id> Offsets;
};

} // namespace internal

/// Select the type of sort for BuildArrays calls. Unstable sorting is faster
/// but will not produce consistent ordering for equal keys. Stable sorting
/// is slower, but keeps equal keys in their original order.
enum class KeysSortType
{
  Unstable = 0,
  Stable = 1
};

/// \brief Manage keys for a `viskores::worklet::WorkletReduceByKey`.
///
/// The `viskores::worklet::WorkletReduceByKey` worklet takes an array of keys for
/// its input domain, finds all identical keys, and runs a worklet that produces
/// a single value for every key given all matching values. This class is used
/// as the associated input for the keys input domain.
///
/// `Keys` is templated on the key array handle type and accepts an instance
/// of this array handle as its constructor. It builds the internal structures
/// needed to use the keys.
///
/// The same `Keys` structure can be used for multiple different invokes of
/// different or the same worklets. When used in this way, the processing done in the
/// `Keys` structure is reused for all the invokes. This is more efficient than
/// creating a different `Keys` structure for each invoke.
///
template <typename T>
class VISKORES_ALWAYS_EXPORT Keys : public internal::KeysBase
{
public:
  using KeyType = T;
  using KeyArrayHandleType = viskores::cont::ArrayHandle<KeyType>;

  VISKORES_CONT
  Keys();

  /// Construct a `Keys` class from an array of keys.
  ///
  /// Given an array of keys, construct a `Keys` class that will manage
  /// using these keys to perform reduce-by-key operations.
  ///
  /// The input keys object is not modified and the result is not stable
  /// sorted. This is the equivalent of calling
  /// `BuildArrays(keys, KeysSortType::Unstable, device)`.
  ///
  template <typename KeyStorage>
  VISKORES_CONT Keys(const viskores::cont::ArrayHandle<KeyType, KeyStorage>& keys,
                     viskores::cont::DeviceAdapterId device = viskores::cont::DeviceAdapterTagAny())
  {
    this->BuildArrays(keys, KeysSortType::Unstable, device);
  }

  /// Build the internal arrays without modifying the input. This is more
  /// efficient for stable sorted arrays, but requires an extra copy of the
  /// keys for unstable sorting.
  template <typename KeyArrayType>
  VISKORES_CONT void BuildArrays(
    const KeyArrayType& keys,
    KeysSortType sort,
    viskores::cont::DeviceAdapterId device = viskores::cont::DeviceAdapterTagAny());

  /// Build the internal arrays and also sort the input keys. This is more
  /// efficient for unstable sorting, but requires an extra copy for stable
  /// sorting.
  template <typename KeyArrayType>
  VISKORES_CONT void BuildArraysInPlace(
    KeyArrayType& keys,
    KeysSortType sort,
    viskores::cont::DeviceAdapterId device = viskores::cont::DeviceAdapterTagAny());

  /// Returns an array of unique keys. The order of keys in this array describes
  /// the order that result values will be placed in a `viskores::worklet::WorkletReduceByKey`.
  VISKORES_CONT
  KeyArrayHandleType GetUniqueKeys() const { return this->UniqueKeys; }

#ifdef VISKORES_DOXYGEN_ONLY
  // Document the superclass' methods as methods in this class.

  /// @brief Returns the input range of a keys object when used as an input domain.
  ///
  /// This will be equal to the number of unique keys.
  viskores::Id GetInputRange() const;

  /// @brief Returns the array that maps each input value to an array of sorted keys.
  ///
  /// This array is used internally as the indices to a `viskores::cont::ArrayHandlePermutation`
  /// to order input values with the grouped keys so that they can then be grouped. This is
  /// an internal array that is seldom of use to code outside the
  /// `viskores::worklet::WorkletReduceByKey` implementation.
  viskores::cont::ArrayHandle<viskores::Id> GetSortedValuesMap() const;

  /// @brief Returns an offsets array to group keys.
  ///
  /// Given an array of sorted keys (or more frequently values permuted to the sorting of
  /// the keys), this array of indices can be used as offsets for a
  /// `viskores::cont::ArrayHandleGroupVecVariable`. This is an internal array that is seldom of
  /// use to code outside the `viskores::worklet::WorkletReduceByKey` implementation.
  viskores::cont::ArrayHandle<viskores::Id> GetOffsets() const;

  /// @brief Returns the number of input keys and values used to build this structure.
  ///
  /// This is also the size of input arrays to a `viskores::worklet::WorkletReduceByKey`.
  viskores::Id GetNumberOfValues() const;
#endif

  using ExecLookup = viskores::exec::internal::ReduceByKeyLookup<
    typename KeyArrayHandleType::ReadPortalType,
    typename viskores::cont::ArrayHandle<viskores::Id>::ReadPortalType,
    typename viskores::cont::ArrayHandle<viskores::IdComponent>::ReadPortalType>;

  VISKORES_CONT ExecLookup PrepareForInput(viskores::cont::DeviceAdapterId device,
                                           viskores::cont::Token& token) const
  {
    return ExecLookup(this->UniqueKeys.PrepareForInput(device, token),
                      this->SortedValuesMap.PrepareForInput(device, token),
                      this->Offsets.PrepareForInput(device, token));
  }

  VISKORES_CONT
  bool operator==(const viskores::worklet::Keys<KeyType>& other) const
  {
    return ((this->UniqueKeys == other.UniqueKeys) &&
            (this->SortedValuesMap == other.SortedValuesMap) && (this->Offsets == other.Offsets));
  }

  VISKORES_CONT
  bool operator!=(const viskores::worklet::Keys<KeyType>& other) const { return !(*this == other); }

private:
  /// @cond NONE
  KeyArrayHandleType UniqueKeys;

  template <typename KeyArrayType>
  VISKORES_CONT void BuildArraysInternal(KeyArrayType& keys,
                                         viskores::cont::DeviceAdapterId device);

  template <typename KeyArrayType>
  VISKORES_CONT void BuildArraysInternalStable(const KeyArrayType& keys,
                                               viskores::cont::DeviceAdapterId device);
  /// @endcond
};

template <typename T>
VISKORES_CONT Keys<T>::Keys() = default;

namespace internal
{

template <typename KeyType>
inline auto SchedulingRange(const viskores::worklet::Keys<KeyType>& inputDomain)
  -> decltype(inputDomain.GetInputRange())
{
  return inputDomain.GetInputRange();
}

template <typename KeyType>
inline auto SchedulingRange(const viskores::worklet::Keys<KeyType>* const inputDomain)
  -> decltype(inputDomain->GetInputRange())
{
  return inputDomain->GetInputRange();
}

inline auto SchedulingRange(const viskores::worklet::internal::KeysBase& inputDomain)
  -> decltype(inputDomain.GetInputRange())
{
  return inputDomain.GetInputRange();
}

inline auto SchedulingRange(const viskores::worklet::internal::KeysBase* const inputDomain)
  -> decltype(inputDomain->GetInputRange())
{
  return inputDomain->GetInputRange();
}
} // namespace internal
}
} // namespace viskores::worklet

// Here we implement the type checks and transports that rely on the Keys
// class. We implement them here because the Keys class is not accessible to
// the arg classes. (The worklet package depends on the cont and exec packages,
// not the other way around.)

namespace viskores
{
namespace cont
{
namespace arg
{

template <typename KeyType>
struct TypeCheck<viskores::cont::arg::TypeCheckTagKeys, KeyType>
{
  static constexpr bool value = std::is_base_of<viskores::worklet::internal::KeysBase,
                                                typename std::decay<KeyType>::type>::value;
};

template <typename KeyType, typename Device>
struct Transport<viskores::cont::arg::TransportTagKeysIn, KeyType, Device>
{
  using ContObjectType = KeyType;
  using ExecObjectType = typename ContObjectType::ExecLookup;

  VISKORES_CONT
  ExecObjectType operator()(const ContObjectType& object,
                            const ContObjectType& inputDomain,
                            viskores::Id,
                            viskores::Id,
                            viskores::cont::Token& token) const
  {
    if (object != inputDomain)
    {
      throw viskores::cont::ErrorBadValue("A Keys object must be the input domain.");
    }

    return object.PrepareForInput(Device(), token);
  }

  // If you get a compile error here, it means that you have used a KeysIn
  // tag in your ControlSignature that was not marked as the InputDomain.
  template <typename InputDomainType>
  VISKORES_CONT ExecObjectType operator()(const ContObjectType&,
                                          const InputDomainType&,
                                          viskores::Id,
                                          viskores::Id) const = delete;
};

template <typename ArrayHandleType, typename Device>
struct Transport<viskores::cont::arg::TransportTagKeyedValuesIn, ArrayHandleType, Device>
{
  VISKORES_IS_ARRAY_HANDLE(ArrayHandleType);

  using ContObjectType = ArrayHandleType;

  using IdArrayType = viskores::cont::ArrayHandle<viskores::Id>;
  using PermutedArrayType = viskores::cont::ArrayHandlePermutation<IdArrayType, ContObjectType>;
  using GroupedArrayType =
    viskores::cont::ArrayHandleGroupVecVariable<PermutedArrayType, IdArrayType>;

  using ExecObjectType = typename GroupedArrayType::ReadPortalType;

  VISKORES_CONT ExecObjectType operator()(const ContObjectType& object,
                                          const viskores::worklet::internal::KeysBase& keys,
                                          viskores::Id,
                                          viskores::Id,
                                          viskores::cont::Token& token) const
  {
    if (object.GetNumberOfValues() != keys.GetNumberOfValues())
    {
      throw viskores::cont::ErrorBadValue("Input values array is wrong size.");
    }

    PermutedArrayType permutedArray(keys.GetSortedValuesMap(), object);
    GroupedArrayType groupedArray(permutedArray, keys.GetOffsets());
    // There is a bit of an issue here where groupedArray goes out of scope,
    // and array portals usually rely on the associated array handle
    // maintaining the resources it points to. However, the entire state of the
    // portal should be self contained except for the data managed by the
    // object argument, which should stay in scope.
    return groupedArray.PrepareForInput(Device(), token);
  }
};

template <typename ArrayHandleType, typename Device>
struct Transport<viskores::cont::arg::TransportTagKeyedValuesInOut, ArrayHandleType, Device>
{
  VISKORES_IS_ARRAY_HANDLE(ArrayHandleType);

  using ContObjectType = ArrayHandleType;

  using IdArrayType = viskores::cont::ArrayHandle<viskores::Id>;
  using PermutedArrayType = viskores::cont::ArrayHandlePermutation<IdArrayType, ContObjectType>;
  using GroupedArrayType =
    viskores::cont::ArrayHandleGroupVecVariable<PermutedArrayType, IdArrayType>;

  using ExecObjectType = typename GroupedArrayType::WritePortalType;

  VISKORES_CONT ExecObjectType operator()(ContObjectType object,
                                          const viskores::worklet::internal::KeysBase& keys,
                                          viskores::Id,
                                          viskores::Id,
                                          viskores::cont::Token& token) const
  {
    if (object.GetNumberOfValues() != keys.GetNumberOfValues())
    {
      throw viskores::cont::ErrorBadValue("Input/output values array is wrong size.");
    }

    PermutedArrayType permutedArray(keys.GetSortedValuesMap(), object);
    GroupedArrayType groupedArray(permutedArray, keys.GetOffsets());
    // There is a bit of an issue here where groupedArray goes out of scope,
    // and array portals usually rely on the associated array handle
    // maintaining the resources it points to. However, the entire state of the
    // portal should be self contained except for the data managed by the
    // object argument, which should stay in scope.
    return groupedArray.PrepareForInPlace(Device(), token);
  }
};

template <typename ArrayHandleType, typename Device>
struct Transport<viskores::cont::arg::TransportTagKeyedValuesOut, ArrayHandleType, Device>
{
  VISKORES_IS_ARRAY_HANDLE(ArrayHandleType);

  using ContObjectType = ArrayHandleType;

  using IdArrayType = viskores::cont::ArrayHandle<viskores::Id>;
  using PermutedArrayType = viskores::cont::ArrayHandlePermutation<IdArrayType, ContObjectType>;
  using GroupedArrayType =
    viskores::cont::ArrayHandleGroupVecVariable<PermutedArrayType, IdArrayType>;

  using ExecObjectType = typename GroupedArrayType::WritePortalType;

  VISKORES_CONT ExecObjectType operator()(ContObjectType object,
                                          const viskores::worklet::internal::KeysBase& keys,
                                          viskores::Id,
                                          viskores::Id,
                                          viskores::cont::Token& token) const
  {
    // The PrepareForOutput for ArrayHandleGroupVecVariable and
    // ArrayHandlePermutation cannot determine the actual size expected for the
    // target array (object), so we have to make sure it gets allocated here.
    object.PrepareForOutput(keys.GetNumberOfValues(), Device(), token);

    PermutedArrayType permutedArray(keys.GetSortedValuesMap(), object);
    GroupedArrayType groupedArray(permutedArray, keys.GetOffsets());
    // There is a bit of an issue here where groupedArray goes out of scope,
    // and array portals usually rely on the associated array handle
    // maintaining the resources it points to. However, the entire state of the
    // portal should be self contained except for the data managed by the
    // object argument, which should stay in scope.
    return groupedArray.PrepareForOutput(keys.GetInputRange(), Device(), token);
  }
};
}
}
} // namespace viskores::cont::arg

#ifndef viskores_worklet_Keys_cxx

#define VISKORES_KEYS_EXPORT(T)                                                       \
  extern template class VISKORES_WORKLET_TEMPLATE_EXPORT viskores::worklet::Keys<T>;  \
  extern template VISKORES_WORKLET_TEMPLATE_EXPORT VISKORES_CONT void                 \
  viskores::worklet::Keys<T>::BuildArrays(const viskores::cont::ArrayHandle<T>& keys, \
                                          viskores::worklet::KeysSortType sort,       \
                                          viskores::cont::DeviceAdapterId device)

VISKORES_KEYS_EXPORT(viskores::UInt8);
VISKORES_KEYS_EXPORT(viskores::HashType);
VISKORES_KEYS_EXPORT(viskores::Id);
VISKORES_KEYS_EXPORT(viskores::Id2);
VISKORES_KEYS_EXPORT(viskores::Id3);
using Pair_UInt8_Id2 = viskores::Pair<viskores::UInt8, viskores::Id2>;
VISKORES_KEYS_EXPORT(Pair_UInt8_Id2);
#ifdef VISKORES_USE_64BIT_IDS
VISKORES_KEYS_EXPORT(viskores::IdComponent);
#endif

#undef VISKORES_KEYS_EXPORT

#endif // !viskores_worklet_Keys_cxx

#endif //viskores_worklet_Keys_h
