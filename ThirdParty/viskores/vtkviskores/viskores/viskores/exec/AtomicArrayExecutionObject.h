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
#ifndef viskores_exec_AtomicArrayExecutionObject_h
#define viskores_exec_AtomicArrayExecutionObject_h

#include <viskores/Atomic.h>
#include <viskores/List.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DeviceAdapter.h>

#include <type_traits>

namespace viskores
{
namespace exec
{

namespace detail
{
// Clang-7 as host compiler under nvcc returns types from std::make_unsigned
// that are not compatible with the viskores::Atomic API, so we define our own
// mapping. This must exist for every entry in viskores::cont::AtomicArrayTypeList.
template <typename>
struct MakeUnsigned;
template <>
struct MakeUnsigned<viskores::UInt32>
{
  using type = viskores::UInt32;
};
template <>
struct MakeUnsigned<viskores::Int32>
{
  using type = viskores::UInt32;
};
template <>
struct MakeUnsigned<viskores::UInt64>
{
  using type = viskores::UInt64;
};
template <>
struct MakeUnsigned<viskores::Int64>
{
  using type = viskores::UInt64;
};
template <>
struct MakeUnsigned<viskores::Float32>
{
  using type = viskores::UInt32;
};
template <>
struct MakeUnsigned<viskores::Float64>
{
  using type = viskores::UInt64;
};

template <typename T>
struct ArithType
{
  using type = typename MakeUnsigned<T>::type;
};
template <>
struct ArithType<viskores::Float32>
{
  using type = viskores::Float32;
};
template <>
struct ArithType<viskores::Float64>
{
  using type = viskores::Float64;
};
}

/// An object passed to a worklet when accessing an atomic array.
/// This object is created for the worklet when a `ControlSignature` argument is
/// `AtomicArrayInOut`.
///
/// `AtomicArrayExecutionObject` behaves similar to a normal `ArrayPortal`.
/// It has similar `Get()` and `Set()` methods, but it has additional features
/// that allow atomic access as well as more methods for atomic operations.
template <typename T>
class AtomicArrayExecutionObject
{
  // Checks if PortalType has a GetIteratorBegin() method that returns a
  // pointer.
  template <typename PortalType,
            typename PointerType = decltype(std::declval<PortalType>().GetIteratorBegin())>
  struct HasPointerAccess : public std::is_pointer<PointerType>
  {
  };

public:
  using ValueType = T;

  AtomicArrayExecutionObject() = default;

  VISKORES_CONT AtomicArrayExecutionObject(viskores::cont::ArrayHandle<T> handle,
                                           viskores::cont::DeviceAdapterId device,
                                           viskores::cont::Token& token)
    : Data{ handle.PrepareForInPlace(device, token).GetIteratorBegin() }
    , NumberOfValues{ handle.GetNumberOfValues() }
  {
    using PortalType = decltype(handle.PrepareForInPlace(device, token));
    VISKORES_STATIC_ASSERT_MSG(HasPointerAccess<PortalType>::value,
                               "Source portal must return a pointer from "
                               "GetIteratorBegin().");
  }

  /// @brief Retrieve the number of values in the atomic array.
  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  viskores::Id GetNumberOfValues() const { return this->NumberOfValues; }

  /// @brief Perform an atomic load of the indexed element with acquire memory
  /// ordering.
  /// @param index The index of the element to load.
  /// @param order The memory ordering to use for the load operation.
  /// @return The value of the atomic array at \a index.
  ///
  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  ValueType Get(viskores::Id index,
                viskores::MemoryOrder order = viskores::MemoryOrder::Acquire) const
  {
    // We only support 32/64 bit signed/unsigned ints, and viskores::Atomic
    // currently only provides API for unsigned types.
    // We'll cast the signed types to unsigned to work around this.
    using APIType = typename detail::MakeUnsigned<ValueType>::type;

    return static_cast<T>(
      viskores::AtomicLoad(reinterpret_cast<APIType*>(this->Data + index), order));
  }

  /// @brief Peform an atomic addition with sequentially consistent memory
  /// ordering.
  /// @param index The index of the array element that will be added to.
  /// @param value The addend of the atomic add operation.
  /// @param order The memory ordering to use for the add operation.
  /// @return The original value of the element at \a index (before addition).
  /// @warning Overflow behavior from this operation is undefined.
  ///
  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  ValueType Add(viskores::Id index,
                const ValueType& value,
                viskores::MemoryOrder order = viskores::MemoryOrder::SequentiallyConsistent) const
  {
    // We only support 32/64 bit signed/unsigned ints, and viskores::Atomic
    // currently only provides API for unsigned types.
    // We'll cast the signed types to unsigned to work around this.
    // This is safe, since the only difference between signed/unsigned types
    // is how overflow works, and signed overflow is already undefined. We also
    // document that overflow is undefined for this operation.
    using APIType = typename detail::ArithType<ValueType>::type;

    return static_cast<T>(viskores::AtomicAdd(
      reinterpret_cast<APIType*>(this->Data + index), static_cast<APIType>(value), order));
  }

  /// @brief Peform an atomic store to memory while enforcing, at minimum, "release"
  /// memory ordering.
  /// @param index The index of the array element that will be added to.
  /// @param value The value to write for the atomic store operation.
  /// @param order The memory ordering to use for the store operation.
  /// @warning Using something like:
  /// ```cpp
  /// Set(index, Get(index)+N)
  /// ```
  /// Should not be done as it is not thread safe, instead you should use
  /// the provided Add method instead.
  ///
  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  void Set(viskores::Id index,
           const ValueType& value,
           viskores::MemoryOrder order = viskores::MemoryOrder::Release) const
  {
    // We only support 32/64 bit signed/unsigned ints, and viskores::Atomic
    // currently only provides API for unsigned types.
    // We'll cast the signed types to unsigned to work around this.
    // This is safe, since the only difference between signed/unsigned types
    // is how overflow works, and signed overflow is already undefined. We also
    // document that overflow is undefined for this operation.
    using APIType = typename detail::MakeUnsigned<ValueType>::type;

    viskores::AtomicStore(
      reinterpret_cast<APIType*>(this->Data + index), static_cast<APIType>(value), order);
  }

  /// @brief Perform an atomic compare and exchange operation with sequentially consistent
  /// memory ordering.
  /// @param index The index of the array element that will be atomically
  /// modified.
  /// @param oldValue A pointer to the expected value of the indexed element.
  /// @param newValue The value to replace the indexed element with.
  /// @param order The memory ordering to use for the compare and exchange operation.
  /// @return If the operation is successful, `true` is returned. Otherwise,
  /// `oldValue` is replaced with the current value of the indexed element,
  /// the element is not modified, and `false` is returned. In either case, `oldValue`
  /// becomes the value that was originally in the indexed element.
  ///
  /// This operation is typically used in a loop. For example usage,
  /// an atomic multiplication may be implemented using compare-exchange as follows:
  ///
  /// ```cpp
  /// AtomicArrayExecutionObject<viskores::Int32> atomicArray = ...;
  ///
  /// // Compare-exchange multiplication:
  /// viskores::Int32 current = atomicArray.Get(idx); // Load the current value at idx
  /// viskores::Int32 newVal;
  /// do {
  ///   newVal = current * multFactor; // the actual multiplication
  /// } while (!atomicArray.CompareExchange(idx, &current, newVal));
  /// ```
  ///
  /// The `while` condition here updates `newVal` what the proper multiplication
  /// is given the expected current value. It then compares this to the
  /// value in the array. If the values match, the operation was successful and the
  /// loop exits. If the values do not match, the value at `idx` was changed
  /// by another thread since the initial Get, and the compare-exchange operation failed --
  /// the target element was not modified by the compare-exchange call. If this happens, the
  /// loop body re-executes using the new value of `current` and tries again until
  /// it succeeds.
  ///
  /// Note that for demonstration purposes, the previous code is unnecessarily verbose.
  /// We can express the same atomic operation more succinctly with just two lines where
  /// `newVal` is just computed in place.
  ///
  /// ```cpp
  /// viskores::Int32 current = atomicArray.Get(idx); // Load the current value at idx
  /// while (!atomicArray.CompareExchange(idx, &current, current * multFactor));
  /// ```
  ///
  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  bool CompareExchange(
    viskores::Id index,
    ValueType* oldValue,
    const ValueType& newValue,
    viskores::MemoryOrder order = viskores::MemoryOrder::SequentiallyConsistent) const
  {
    // We only support 32/64 bit signed/unsigned ints, and viskores::Atomic
    // currently only provides API for unsigned types.
    // We'll cast the signed types to unsigned to work around this.
    // This is safe, since the only difference between signed/unsigned types
    // is how overflow works, and signed overflow is already undefined.
    using APIType = typename detail::MakeUnsigned<ValueType>::type;

    return viskores::AtomicCompareExchange(reinterpret_cast<APIType*>(this->Data + index),
                                           reinterpret_cast<APIType*>(oldValue),
                                           static_cast<APIType>(newValue),
                                           order);
  }

private:
  ValueType* Data{ nullptr };
  viskores::Id NumberOfValues{ 0 };
};
}
} // namespace viskores::exec

#endif //viskores_exec_AtomicArrayExecutionObject_h
