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
#ifndef viskores_cont_Algorithm_h
#define viskores_cont_Algorithm_h

#include <viskores/Types.h>

#include <viskores/cont/BitField.h>
#include <viskores/cont/DeviceAdapter.h>
#include <viskores/cont/ExecutionObjectBase.h>
#include <viskores/cont/Token.h>
#include <viskores/cont/TryExecute.h>
#include <viskores/cont/internal/Hints.h>


namespace viskores
{
namespace cont
{
/// @cond NONE
namespace detail
{
template <typename Device, typename T>
inline auto DoPrepareArgForExec(T&& object, viskores::cont::Token& token, std::true_type)
  -> decltype(viskores::cont::internal::CallPrepareForExecution(std::forward<T>(object),
                                                                Device{},
                                                                token))
{
  VISKORES_IS_EXECUTION_OBJECT(T);
  return viskores::cont::internal::CallPrepareForExecution(
    std::forward<T>(object), Device{}, token);
}

template <typename Device, typename T>
inline T&& DoPrepareArgForExec(T&& object, viskores::cont::Token&, std::false_type)
{
  static_assert(!viskores::cont::internal::IsExecutionObjectBase<T>::value,
                "Internal error: failed to detect execution object.");
  return std::forward<T>(object);
}

template <typename Device, typename T>
auto PrepareArgForExec(T&& object, viskores::cont::Token& token)
  -> decltype(DoPrepareArgForExec<Device>(std::forward<T>(object),
                                          token,
                                          viskores::cont::internal::IsExecutionObjectBase<T>{}))
{
  return DoPrepareArgForExec<Device>(
    std::forward<T>(object), token, viskores::cont::internal::IsExecutionObjectBase<T>{});
}

struct BitFieldToUnorderedSetFunctor
{
  viskores::Id Result{ 0 };

  template <typename Device, typename... Args>
  VISKORES_CONT bool operator()(Device, Args&&... args)
  {
    VISKORES_IS_DEVICE_ADAPTER_TAG(Device);
    viskores::cont::Token token;
    this->Result = viskores::cont::DeviceAdapterAlgorithm<Device>::BitFieldToUnorderedSet(
      PrepareArgForExec<Device>(std::forward<Args>(args), token)...);
    return true;
  }
};

struct CopyFunctor
{
  template <typename T, typename S, typename... Args>
  VISKORES_CONT bool InputArrayOnDevice(viskores::cont::DeviceAdapterId device,
                                        const viskores::cont::ArrayHandle<T, S>& input,
                                        Args&&...) const
  {
    return input.IsOnDevice(device);
  }

  template <typename Device, typename... Args>
  VISKORES_CONT bool operator()(Device device, bool useExistingDevice, Args&&... args) const
  {
    VISKORES_IS_DEVICE_ADAPTER_TAG(Device);
    if (!useExistingDevice || this->InputArrayOnDevice(device, std::forward<Args>(args)...))
    {
      viskores::cont::Token token;
      viskores::cont::DeviceAdapterAlgorithm<Device>::Copy(
        PrepareArgForExec<Device>(std::forward<Args>(args), token)...);
      return true;
    }
    else
    {
      return false;
    }
  }
};

struct CopyIfFunctor
{

  template <typename Device, typename... Args>
  VISKORES_CONT bool operator()(Device, Args&&... args) const
  {
    VISKORES_IS_DEVICE_ADAPTER_TAG(Device);
    viskores::cont::Token token;
    viskores::cont::DeviceAdapterAlgorithm<Device>::CopyIf(
      PrepareArgForExec<Device>(std::forward<Args>(args), token)...);
    return true;
  }
};

struct CopySubRangeFunctor
{
  bool valid;

  CopySubRangeFunctor()
    : valid(false)
  {
  }

  template <typename Device, typename... Args>
  VISKORES_CONT bool operator()(Device, Args&&... args)
  {
    VISKORES_IS_DEVICE_ADAPTER_TAG(Device);
    viskores::cont::Token token;
    valid = viskores::cont::DeviceAdapterAlgorithm<Device>::CopySubRange(
      PrepareArgForExec<Device>(std::forward<Args>(args), token)...);
    return true;
  }
};

struct CountSetBitsFunctor
{
  viskores::Id PopCount{ 0 };

  template <typename Device, typename... Args>
  VISKORES_CONT bool operator()(Device, Args&&... args)
  {
    VISKORES_IS_DEVICE_ADAPTER_TAG(Device);
    viskores::cont::Token token;
    this->PopCount = viskores::cont::DeviceAdapterAlgorithm<Device>::CountSetBits(
      PrepareArgForExec<Device>(std::forward<Args>(args), token)...);
    return true;
  }
};

struct FillFunctor
{
  template <typename Device, typename... Args>
  VISKORES_CONT bool operator()(Device, Args&&... args)
  {
    VISKORES_IS_DEVICE_ADAPTER_TAG(Device);
    viskores::cont::Token token;
    viskores::cont::DeviceAdapterAlgorithm<Device>::Fill(
      PrepareArgForExec<Device>(std::forward<Args>(args), token)...);
    return true;
  }
};

struct LowerBoundsFunctor
{

  template <typename Device, typename... Args>
  VISKORES_CONT bool operator()(Device, Args&&... args) const
  {
    VISKORES_IS_DEVICE_ADAPTER_TAG(Device);
    viskores::cont::Token token;
    viskores::cont::DeviceAdapterAlgorithm<Device>::LowerBounds(
      PrepareArgForExec<Device>(std::forward<Args>(args), token)...);
    return true;
  }
};

template <typename U>
struct ReduceFunctor
{
  U result;

  ReduceFunctor()
    : result(viskores::TypeTraits<U>::ZeroInitialization())
  {
  }

  template <typename Device, typename... Args>
  VISKORES_CONT bool operator()(Device, Args&&... args)
  {
    VISKORES_IS_DEVICE_ADAPTER_TAG(Device);
    viskores::cont::Token token;
    result = viskores::cont::DeviceAdapterAlgorithm<Device>::Reduce(
      PrepareArgForExec<Device>(std::forward<Args>(args), token)...);
    return true;
  }
};

struct ReduceByKeyFunctor
{
  template <typename Device, typename... Args>
  VISKORES_CONT bool operator()(Device, Args&&... args) const
  {
    VISKORES_IS_DEVICE_ADAPTER_TAG(Device);
    viskores::cont::Token token;
    viskores::cont::DeviceAdapterAlgorithm<Device>::ReduceByKey(
      PrepareArgForExec<Device>(std::forward<Args>(args), token)...);
    return true;
  }
};

template <typename U>
struct ScanInclusiveResultFunctor
{
  U result;

  ScanInclusiveResultFunctor()
    : result(viskores::TypeTraits<U>::ZeroInitialization())
  {
  }

  template <typename Device, typename... Args>
  VISKORES_CONT bool operator()(Device, Args&&... args)
  {
    VISKORES_IS_DEVICE_ADAPTER_TAG(Device);
    viskores::cont::Token token;
    result = viskores::cont::DeviceAdapterAlgorithm<Device>::ScanInclusive(
      PrepareArgForExec<Device>(std::forward<Args>(args), token)...);
    return true;
  }
};

struct ScanInclusiveByKeyFunctor
{
  ScanInclusiveByKeyFunctor() {}

  template <typename Device, typename... Args>
  VISKORES_CONT bool operator()(Device, Args&&... args) const
  {
    VISKORES_IS_DEVICE_ADAPTER_TAG(Device);
    viskores::cont::Token token;
    viskores::cont::DeviceAdapterAlgorithm<Device>::ScanInclusiveByKey(
      PrepareArgForExec<Device>(std::forward<Args>(args), token)...);
    return true;
  }
};

template <typename T>
struct ScanExclusiveFunctor
{
  T result;

  ScanExclusiveFunctor()
    : result(T())
  {
  }

  template <typename Device, typename... Args>
  VISKORES_CONT bool operator()(Device, Args&&... args)
  {
    VISKORES_IS_DEVICE_ADAPTER_TAG(Device);
    viskores::cont::Token token;
    result = viskores::cont::DeviceAdapterAlgorithm<Device>::ScanExclusive(
      PrepareArgForExec<Device>(std::forward<Args>(args), token)...);
    return true;
  }
};

struct ScanExclusiveByKeyFunctor
{
  ScanExclusiveByKeyFunctor() {}

  template <typename Device, typename... Args>
  VISKORES_CONT bool operator()(Device, Args&&... args) const
  {
    VISKORES_IS_DEVICE_ADAPTER_TAG(Device);
    viskores::cont::Token token;
    viskores::cont::DeviceAdapterAlgorithm<Device>::ScanExclusiveByKey(
      PrepareArgForExec<Device>(std::forward<Args>(args), token)...);
    return true;
  }
};

template <typename T>
struct ScanExtendedFunctor
{
  template <typename Device, typename... Args>
  VISKORES_CONT bool operator()(Device, Args&&... args)
  {
    VISKORES_IS_DEVICE_ADAPTER_TAG(Device);
    viskores::cont::Token token;
    viskores::cont::DeviceAdapterAlgorithm<Device>::ScanExtended(
      PrepareArgForExec<Device>(std::forward<Args>(args), token)...);
    return true;
  }
};

struct ScheduleFunctor
{
  template <typename Device, typename... Args>
  VISKORES_CONT bool operator()(Device, Args&&... args)
  {
    VISKORES_IS_DEVICE_ADAPTER_TAG(Device);
    viskores::cont::Token token;
    viskores::cont::DeviceAdapterAlgorithm<Device>::Schedule(
      PrepareArgForExec<Device>(std::forward<Args>(args), token)...);
    return true;
  }
};

struct SortFunctor
{
  template <typename Device, typename... Args>
  VISKORES_CONT bool operator()(Device, Args&&... args) const
  {
    VISKORES_IS_DEVICE_ADAPTER_TAG(Device);
    viskores::cont::Token token;
    viskores::cont::DeviceAdapterAlgorithm<Device>::Sort(
      PrepareArgForExec<Device>(std::forward<Args>(args), token)...);
    return true;
  }
};

struct SortByKeyFunctor
{
  template <typename Device, typename... Args>
  VISKORES_CONT bool operator()(Device, Args&&... args) const
  {
    VISKORES_IS_DEVICE_ADAPTER_TAG(Device);
    viskores::cont::Token token;
    viskores::cont::DeviceAdapterAlgorithm<Device>::SortByKey(
      PrepareArgForExec<Device>(std::forward<Args>(args), token)...);
    return true;
  }
};

struct SynchronizeFunctor
{
  template <typename Device>
  VISKORES_CONT bool operator()(Device)
  {
    VISKORES_IS_DEVICE_ADAPTER_TAG(Device);
    viskores::cont::DeviceAdapterAlgorithm<Device>::Synchronize();
    return true;
  }
};

struct TransformFunctor
{
  template <typename Device, typename... Args>
  VISKORES_CONT bool operator()(Device, Args&&... args) const
  {
    VISKORES_IS_DEVICE_ADAPTER_TAG(Device);
    viskores::cont::Token token;
    viskores::cont::DeviceAdapterAlgorithm<Device>::Transform(
      PrepareArgForExec<Device>(std::forward<Args>(args), token)...);
    return true;
  }
};

struct UniqueFunctor
{
  template <typename Device, typename... Args>
  VISKORES_CONT bool operator()(Device, Args&&... args) const
  {
    VISKORES_IS_DEVICE_ADAPTER_TAG(Device);
    viskores::cont::Token token;
    viskores::cont::DeviceAdapterAlgorithm<Device>::Unique(
      PrepareArgForExec<Device>(std::forward<Args>(args), token)...);
    return true;
  }
};

struct UpperBoundsFunctor
{
  template <typename Device, typename... Args>
  VISKORES_CONT bool operator()(Device, Args&&... args) const
  {
    VISKORES_IS_DEVICE_ADAPTER_TAG(Device);
    viskores::cont::Token token;
    viskores::cont::DeviceAdapterAlgorithm<Device>::UpperBounds(
      PrepareArgForExec<Device>(std::forward<Args>(args), token)...);
    return true;
  }
};
} // namespace detail
/// @endcond

/// @brief Static control-side entry points for common device algorithms.
///
/// `Algorithm` contains no state. It provides static methods that are invoked
/// in the control environment and execute on a device adapter. Overloads that
/// take a `DeviceAdapterId` use that device; overloads without an explicit
/// device use the runtime device selection mechanism.
///
/// Unless otherwise stated, output `ArrayHandle` objects are resized or
/// allocated by these methods as needed.
struct Algorithm
{

  /// @brief Create a unique, unordered list of set bit indices.
  ///
  /// Writes into @a indices the indices of all bits set in @a bits. The
  /// resulting indices are unique, but no ordering is guaranteed.
  ///
  /// @param devId Device adapter to run on.
  /// @param bits Bit field to search.
  /// @param indices Output array that receives the set bit indices.
  /// @returns The number of set bits, which is also the number of values
  /// written to @a indices.
  template <typename IndicesStorage>
  VISKORES_CONT static viskores::Id BitFieldToUnorderedSet(
    viskores::cont::DeviceAdapterId devId,
    const viskores::cont::BitField& bits,
    viskores::cont::ArrayHandle<Id, IndicesStorage>& indices)
  {
    detail::BitFieldToUnorderedSetFunctor functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, bits, indices);
    return functor.Result;
  }

  /// @overload
  /// Uses the runtime device selection mechanism.
  template <typename IndicesStorage>
  VISKORES_CONT static viskores::Id BitFieldToUnorderedSet(
    const viskores::cont::BitField& bits,
    viskores::cont::ArrayHandle<Id, IndicesStorage>& indices)
  {
    detail::BitFieldToUnorderedSetFunctor functor;
    viskores::cont::TryExecute(functor, bits, indices);
    return functor.Result;
  }

  /// @brief Copy values from one array to another in the execution environment.
  ///
  /// Copies all values from @a input to @a output. The output array is resized
  /// to the same number of values as @a input. If @a output is already
  /// allocated, its previous contents are discarded.
  ///
  /// @param devId Device adapter to run on.
  /// @param input Source array.
  /// @param output Destination array.
  /// @returns `true` if the copy was successfully executed on the selected
  /// device.
  template <typename T, typename U, class CIn, class COut>
  VISKORES_CONT static bool Copy(viskores::cont::DeviceAdapterId devId,
                                 const viskores::cont::ArrayHandle<T, CIn>& input,
                                 viskores::cont::ArrayHandle<U, COut>& output)
  {
    // If we can use any device, prefer to use source's already loaded device.
    if (devId == viskores::cont::DeviceAdapterTagAny())
    {
      bool isCopied =
        viskores::cont::TryExecuteOnDevice(devId, detail::CopyFunctor(), true, input, output);
      if (isCopied)
      {
        return true;
      }
    }
    return viskores::cont::TryExecuteOnDevice(devId, detail::CopyFunctor(), false, input, output);
  }

  /// @overload
  /// Uses any available device. When possible, this overload prefers a device
  /// where @a input is already valid.
  template <typename T, typename U, class CIn, class COut>
  VISKORES_CONT static void Copy(const viskores::cont::ArrayHandle<T, CIn>& input,
                                 viskores::cont::ArrayHandle<U, COut>& output)
  {
    Copy(viskores::cont::DeviceAdapterTagAny(), input, output);
  }


  /// @brief Conditionally copy input values selected by a stencil.
  ///
  /// Performs stream compaction on @a input. For each position, the
  /// corresponding value in @a stencil determines whether that input value is
  /// copied to @a output. Without an explicit predicate, stencil values not
  /// equal to `viskores::TypeTraits<U>::ZeroInitialization()` are considered
  /// true. The output array is resized to the number of selected values, and
  /// selected input values are written in input order.
  ///
  /// @param devId Device adapter to run on.
  /// @param input Values to compact.
  /// @param stencil Stencil values. Must have the same number of values as
  /// @a input.
  /// @param output Destination array for selected values.
  template <typename T, typename U, class CIn, class CStencil, class COut>
  VISKORES_CONT static void CopyIf(viskores::cont::DeviceAdapterId devId,
                                   const viskores::cont::ArrayHandle<T, CIn>& input,
                                   const viskores::cont::ArrayHandle<U, CStencil>& stencil,
                                   viskores::cont::ArrayHandle<T, COut>& output)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::CopyIfFunctor(), input, stencil, output);
  }

  /// @overload
  /// Uses the runtime device selection mechanism.
  template <typename T, typename U, class CIn, class CStencil, class COut>
  VISKORES_CONT static void CopyIf(const viskores::cont::ArrayHandle<T, CIn>& input,
                                   const viskores::cont::ArrayHandle<U, CStencil>& stencil,
                                   viskores::cont::ArrayHandle<T, COut>& output)
  {
    CopyIf(viskores::cont::DeviceAdapterTagAny(), input, stencil, output);
  }


  /// @overload
  /// Uses @a unary_predicate to decide which stencil values are true.
  template <typename T, typename U, class CIn, class CStencil, class COut, class UnaryPredicate>
  VISKORES_CONT static void CopyIf(viskores::cont::DeviceAdapterId devId,
                                   const viskores::cont::ArrayHandle<T, CIn>& input,
                                   const viskores::cont::ArrayHandle<U, CStencil>& stencil,
                                   viskores::cont::ArrayHandle<T, COut>& output,
                                   UnaryPredicate unary_predicate)
  {
    viskores::cont::TryExecuteOnDevice(
      devId, detail::CopyIfFunctor(), input, stencil, output, unary_predicate);
  }

  /// @overload
  /// Uses @a unary_predicate and the runtime device selection mechanism.
  template <typename T, typename U, class CIn, class CStencil, class COut, class UnaryPredicate>
  VISKORES_CONT static void CopyIf(const viskores::cont::ArrayHandle<T, CIn>& input,
                                   const viskores::cont::ArrayHandle<U, CStencil>& stencil,
                                   viskores::cont::ArrayHandle<T, COut>& output,
                                   UnaryPredicate unary_predicate)
  {
    CopyIf(viskores::cont::DeviceAdapterTagAny(), input, stencil, output, unary_predicate);
  }


  /// @brief Copy a subrange from one array into another.
  ///
  /// Copies up to @a numberOfElementsToCopy values from @a input, beginning at
  /// @a inputStartIndex, into @a output beginning at @a outputIndex. If the
  /// requested input range extends past the end of @a input, only the values up
  /// to the end of @a input are copied. If @a output is not large enough, it is
  /// resized.
  ///
  /// @pre If @a input and @a output share memory, the source and destination
  /// ranges must not overlap.
  ///
  /// @param devId Device adapter to run on.
  /// @param input Source array.
  /// @param inputStartIndex First source index to copy.
  /// @param numberOfElementsToCopy Maximum number of values to copy.
  /// @param output Destination array.
  /// @param outputIndex First destination index to write.
  /// @returns `true` if the requested copy range is valid for the selected
  /// device implementation.
  template <typename T, typename U, class CIn, class COut>
  VISKORES_CONT static bool CopySubRange(viskores::cont::DeviceAdapterId devId,
                                         const viskores::cont::ArrayHandle<T, CIn>& input,
                                         viskores::Id inputStartIndex,
                                         viskores::Id numberOfElementsToCopy,
                                         viskores::cont::ArrayHandle<U, COut>& output,
                                         viskores::Id outputIndex = 0)
  {
    detail::CopySubRangeFunctor functor;
    viskores::cont::TryExecuteOnDevice(
      devId, functor, input, inputStartIndex, numberOfElementsToCopy, output, outputIndex);
    return functor.valid;
  }

  /// @overload
  /// Uses the runtime device selection mechanism.
  template <typename T, typename U, class CIn, class COut>
  VISKORES_CONT static bool CopySubRange(const viskores::cont::ArrayHandle<T, CIn>& input,
                                         viskores::Id inputStartIndex,
                                         viskores::Id numberOfElementsToCopy,
                                         viskores::cont::ArrayHandle<U, COut>& output,
                                         viskores::Id outputIndex = 0)
  {
    return CopySubRange(viskores::cont::DeviceAdapterTagAny(),
                        input,
                        inputStartIndex,
                        numberOfElementsToCopy,
                        output,
                        outputIndex);
  }

  /// @brief Count the total number of set bits in a bit field.
  ///
  /// @param devId Device adapter to run on.
  /// @param bits Bit field to inspect.
  /// @returns The number of bits set to 1 in @a bits.
  VISKORES_CONT static viskores::Id CountSetBits(viskores::cont::DeviceAdapterId devId,
                                                 const viskores::cont::BitField& bits)
  {
    detail::CountSetBitsFunctor functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, bits);
    return functor.PopCount;
  }

  /// @overload
  /// Uses the runtime device selection mechanism.
  VISKORES_CONT static viskores::Id CountSetBits(const viskores::cont::BitField& bits)
  {
    return CountSetBits(viskores::cont::DeviceAdapterTagAny{}, bits);
  }

  /// @brief Fill a bit field with a boolean value.
  ///
  /// All bits are set to 1 when @a value is true and 0 when @a value is false.
  /// The bit field is resized to @a numBits before it is filled.
  ///
  /// @param devId Device adapter to run on.
  /// @param bits Bit field to fill.
  /// @param value Boolean value to stamp across the bit field.
  /// @param numBits Number of bits in the resized bit field.
  VISKORES_CONT static void Fill(viskores::cont::DeviceAdapterId devId,
                                 viskores::cont::BitField& bits,
                                 bool value,
                                 viskores::Id numBits)
  {
    detail::FillFunctor functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, bits, value, numBits);
  }

  /// @overload
  /// Uses the runtime device selection mechanism.
  VISKORES_CONT static void Fill(viskores::cont::BitField& bits, bool value, viskores::Id numBits)
  {
    Fill(viskores::cont::DeviceAdapterTagAny{}, bits, value, numBits);
  }

  /// @overload
  /// Fills the existing bit field size.
  VISKORES_CONT static void Fill(viskores::cont::DeviceAdapterId devId,
                                 viskores::cont::BitField& bits,
                                 bool value)
  {
    detail::FillFunctor functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, bits, value);
  }

  /// @overload
  /// Fills the existing bit field size using the runtime device selection
  /// mechanism.
  VISKORES_CONT static void Fill(viskores::cont::BitField& bits, bool value)
  {
    Fill(viskores::cont::DeviceAdapterTagAny{}, bits, value);
  }

  /// @brief Fill a bit field by stamping a word mask across it.
  ///
  /// The bit field is resized to @a numBits before it is filled.
  ///
  /// @pre `WordType` must be an unsigned integral type.
  ///
  /// @param devId Device adapter to run on.
  /// @param bits Bit field to fill.
  /// @param word Word mask to stamp across the bit field.
  /// @param numBits Number of bits in the resized bit field.
  template <typename WordType>
  VISKORES_CONT static void Fill(viskores::cont::DeviceAdapterId devId,
                                 viskores::cont::BitField& bits,
                                 WordType word,
                                 viskores::Id numBits)
  {
    detail::FillFunctor functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, bits, word, numBits);
  }

  /// @overload
  /// Uses the runtime device selection mechanism.
  template <typename WordType>
  VISKORES_CONT static void Fill(viskores::cont::BitField& bits,
                                 WordType word,
                                 viskores::Id numBits)
  {
    Fill(viskores::cont::DeviceAdapterTagAny{}, bits, word, numBits);
  }

  /// @overload
  /// Fills the existing bit field size.
  template <typename WordType>
  VISKORES_CONT static void Fill(viskores::cont::DeviceAdapterId devId,
                                 viskores::cont::BitField& bits,
                                 WordType word)
  {
    detail::FillFunctor functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, bits, word);
  }

  /// @overload
  /// Fills the existing bit field size using the runtime device selection
  /// mechanism.
  template <typename WordType>
  VISKORES_CONT static void Fill(viskores::cont::BitField& bits, WordType word)
  {
    Fill(viskores::cont::DeviceAdapterTagAny{}, bits, word);
  }

  /// @brief Fill an array with a value.
  ///
  /// Replaces every value in @a handle with @a value.
  ///
  /// @param devId Device adapter to run on.
  /// @param handle Array to fill.
  /// @param value Value to copy into every entry.
  template <typename T, typename S>
  VISKORES_CONT static void Fill(viskores::cont::DeviceAdapterId devId,
                                 viskores::cont::ArrayHandle<T, S>& handle,
                                 const T& value)
  {
    detail::FillFunctor functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, handle, value);
  }

  /// @overload
  /// Uses the runtime device selection mechanism.
  template <typename T, typename S>
  VISKORES_CONT static void Fill(viskores::cont::ArrayHandle<T, S>& handle, const T& value)
  {
    Fill(viskores::cont::DeviceAdapterTagAny{}, handle, value);
  }

  /// @overload
  /// Resizes @a handle to @a numValues before filling it.
  template <typename T, typename S>
  VISKORES_CONT static void Fill(viskores::cont::DeviceAdapterId devId,
                                 viskores::cont::ArrayHandle<T, S>& handle,
                                 const T& value,
                                 const viskores::Id numValues)
  {
    detail::FillFunctor functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, handle, value, numValues);
  }

  /// @overload
  /// Resizes @a handle to @a numValues and uses the runtime device selection
  /// mechanism.
  template <typename T, typename S>
  VISKORES_CONT static void Fill(viskores::cont::ArrayHandle<T, S>& handle,
                                 const T& value,
                                 const viskores::Id numValues)
  {
    Fill(viskores::cont::DeviceAdapterTagAny{}, handle, value, numValues);
  }

  /// @brief Find lower-bound insertion indices for a vector of query values.
  ///
  /// For each value in @a values, finds the first index in sorted @a input
  /// whose value is greater than or equal to the query value. The results are
  /// written to @a output. This is the vectorized equivalent of
  /// `std::lower_bound`.
  ///
  /// @pre @a input must be sorted according to the default less-than ordering.
  ///
  /// @param devId Device adapter to run on.
  /// @param input Sorted values to search.
  /// @param values Query values.
  /// @param output Output lower-bound indices.
  template <typename T, class CIn, class CVal, class COut>
  VISKORES_CONT static void LowerBounds(viskores::cont::DeviceAdapterId devId,
                                        const viskores::cont::ArrayHandle<T, CIn>& input,
                                        const viskores::cont::ArrayHandle<T, CVal>& values,
                                        viskores::cont::ArrayHandle<viskores::Id, COut>& output)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::LowerBoundsFunctor(), input, values, output);
  }

  /// @overload
  /// Uses the runtime device selection mechanism.
  template <typename T, class CIn, class CVal, class COut>
  VISKORES_CONT static void LowerBounds(const viskores::cont::ArrayHandle<T, CIn>& input,
                                        const viskores::cont::ArrayHandle<T, CVal>& values,
                                        viskores::cont::ArrayHandle<viskores::Id, COut>& output)
  {
    LowerBounds(viskores::cont::DeviceAdapterTagAny(), input, values, output);
  }


  /// @overload
  /// Uses @a binary_compare as the less-than operation. @a input must be sorted
  /// according to the same comparison.
  template <typename T, class CIn, class CVal, class COut, class BinaryCompare>
  VISKORES_CONT static void LowerBounds(viskores::cont::DeviceAdapterId devId,
                                        const viskores::cont::ArrayHandle<T, CIn>& input,
                                        const viskores::cont::ArrayHandle<T, CVal>& values,
                                        viskores::cont::ArrayHandle<viskores::Id, COut>& output,
                                        BinaryCompare binary_compare)
  {
    viskores::cont::TryExecuteOnDevice(
      devId, detail::LowerBoundsFunctor(), input, values, output, binary_compare);
  }

  /// @overload
  /// Uses @a binary_compare and the runtime device selection mechanism.
  template <typename T, class CIn, class CVal, class COut, class BinaryCompare>
  VISKORES_CONT static void LowerBounds(const viskores::cont::ArrayHandle<T, CIn>& input,
                                        const viskores::cont::ArrayHandle<T, CVal>& values,
                                        viskores::cont::ArrayHandle<viskores::Id, COut>& output,
                                        BinaryCompare binary_compare)
  {
    LowerBounds(viskores::cont::DeviceAdapterTagAny(), input, values, output, binary_compare);
  }


  /// @overload
  /// In-place specialization for `viskores::Id` arrays. The values in
  /// @a values_output are replaced with their lower-bound indices in @a input.
  /// This form is useful for inverting index maps.
  template <class CIn, class COut>
  VISKORES_CONT static void LowerBounds(
    viskores::cont::DeviceAdapterId devId,
    const viskores::cont::ArrayHandle<viskores::Id, CIn>& input,
    viskores::cont::ArrayHandle<viskores::Id, COut>& values_output)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::LowerBoundsFunctor(), input, values_output);
  }

  /// @overload
  /// In-place `viskores::Id` specialization using the runtime device selection
  /// mechanism.
  template <class CIn, class COut>
  VISKORES_CONT static void LowerBounds(
    const viskores::cont::ArrayHandle<viskores::Id, CIn>& input,
    viskores::cont::ArrayHandle<viskores::Id, COut>& values_output)
  {
    LowerBounds(viskores::cont::DeviceAdapterTagAny(), input, values_output);
  }


  /// @brief Reduce an array to a single accumulated value.
  ///
  /// Computes the accumulated sum of @a initialValue and all values in
  /// @a input, using the default addition operation. The accumulation is run in
  /// parallel and is not a serial summation, so the operation must be
  /// associative to produce consistent results.
  ///
  /// @param devId Device adapter to run on.
  /// @param input Values to reduce.
  /// @param initialValue Initial value for the reduction.
  /// @returns The reduced total.
  template <typename T, typename U, class CIn>
  VISKORES_CONT static U Reduce(viskores::cont::DeviceAdapterId devId,
                                const viskores::cont::ArrayHandle<T, CIn>& input,
                                U initialValue)
  {
    detail::ReduceFunctor<U> functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, input, initialValue);
    return functor.result;
  }

  /// @overload
  /// Uses the runtime device selection mechanism.
  template <typename T, typename U, class CIn>
  VISKORES_CONT static U Reduce(const viskores::cont::ArrayHandle<T, CIn>& input, U initialValue)
  {
    return Reduce(viskores::cont::DeviceAdapterTagAny(), input, initialValue);
  }


  /// @overload
  /// Uses @a binary_functor as the reduction operation. The functor must be
  /// associative.
  template <typename T, typename U, class CIn, class BinaryFunctor>
  VISKORES_CONT static U Reduce(viskores::cont::DeviceAdapterId devId,
                                const viskores::cont::ArrayHandle<T, CIn>& input,
                                U initialValue,
                                BinaryFunctor binary_functor)
  {
    detail::ReduceFunctor<U> functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, input, initialValue, binary_functor);
    return functor.result;
  }

  /// @overload
  /// Uses @a binary_functor and the runtime device selection mechanism.
  template <typename T, typename U, class CIn, class BinaryFunctor>
  VISKORES_CONT static U Reduce(const viskores::cont::ArrayHandle<T, CIn>& input,
                                U initialValue,
                                BinaryFunctor binary_functor)
  {
    return Reduce(viskores::cont::DeviceAdapterTagAny(), input, initialValue, binary_functor);
  }


  /// @brief Reduce consecutive segments of values that share equal keys.
  ///
  /// Partitions @a keys and @a values into consecutive segments with identical
  /// adjacent keys. Each segment is reduced with @a binary_functor. The unique
  /// segment keys are written to @a keys_output, and the reduced values are
  /// written to @a values_output.
  ///
  /// @pre @a keys and @a values must have the same number of values.
  /// @pre @a binary_functor must be associative.
  ///
  /// @param devId Device adapter to run on.
  /// @param keys Input keys.
  /// @param values Input values to reduce.
  /// @param keys_output Output unique segment keys.
  /// @param values_output Output reduced segment values.
  /// @param binary_functor Binary reduction functor.
  template <typename T,
            typename U,
            class CKeyIn,
            class CValIn,
            class CKeyOut,
            class CValOut,
            class BinaryFunctor>
  VISKORES_CONT static void ReduceByKey(viskores::cont::DeviceAdapterId devId,
                                        const viskores::cont::ArrayHandle<T, CKeyIn>& keys,
                                        const viskores::cont::ArrayHandle<U, CValIn>& values,
                                        viskores::cont::ArrayHandle<T, CKeyOut>& keys_output,
                                        viskores::cont::ArrayHandle<U, CValOut>& values_output,
                                        BinaryFunctor binary_functor)
  {
    viskores::cont::TryExecuteOnDevice(devId,
                                       detail::ReduceByKeyFunctor(),
                                       keys,
                                       values,
                                       keys_output,
                                       values_output,
                                       binary_functor);
  }

  /// @overload
  /// Uses the runtime device selection mechanism.
  template <typename T,
            typename U,
            class CKeyIn,
            class CValIn,
            class CKeyOut,
            class CValOut,
            class BinaryFunctor>
  VISKORES_CONT static void ReduceByKey(const viskores::cont::ArrayHandle<T, CKeyIn>& keys,
                                        const viskores::cont::ArrayHandle<U, CValIn>& values,
                                        viskores::cont::ArrayHandle<T, CKeyOut>& keys_output,
                                        viskores::cont::ArrayHandle<U, CValOut>& values_output,
                                        BinaryFunctor binary_functor)
  {
    ReduceByKey(viskores::cont::DeviceAdapterTagAny(),
                keys,
                values,
                keys_output,
                values_output,
                binary_functor);
  }


  /// @brief Compute an inclusive prefix sum.
  ///
  /// Performs a running sum on @a input and stores the result in @a output.
  /// For inclusive scans, the value at position `i` includes the input element
  /// at position `i`. For example, the third output value is the sum of the
  /// first three input values. If @a input and @a output are the same array,
  /// the operation is performed in place.
  ///
  /// This overload uses the default addition operation. The operation must be
  /// associative to produce consistent results.
  ///
  /// @param devId Device adapter to run on.
  /// @param input Values to scan.
  /// @param output Inclusive scan output.
  /// @returns The sum of all input values.
  template <typename T, class CIn, class COut>
  VISKORES_CONT static T ScanInclusive(viskores::cont::DeviceAdapterId devId,
                                       const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output)
  {
    detail::ScanInclusiveResultFunctor<T> functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, input, output);
    return functor.result;
  }

  /// @overload
  /// Uses the runtime device selection mechanism.
  template <typename T, class CIn, class COut>
  VISKORES_CONT static T ScanInclusive(const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output)
  {
    return ScanInclusive(viskores::cont::DeviceAdapterTagAny(), input, output);
  }


  /// @overload
  /// Uses @a binary_functor as the scan operation. The functor must be
  /// associative.
  template <typename T, class CIn, class COut, class BinaryFunctor>
  VISKORES_CONT static T ScanInclusive(viskores::cont::DeviceAdapterId devId,
                                       const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output,
                                       BinaryFunctor binary_functor)
  {
    detail::ScanInclusiveResultFunctor<T> functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, input, output, binary_functor);
    return functor.result;
  }

  /// @overload
  /// Uses @a binary_functor and the runtime device selection mechanism.
  template <typename T, class CIn, class COut, class BinaryFunctor>
  VISKORES_CONT static T ScanInclusive(const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output,
                                       BinaryFunctor binary_functor)
  {
    return ScanInclusive(viskores::cont::DeviceAdapterTagAny(), input, output, binary_functor);
  }


  /// @brief Compute inclusive prefix sums over consecutive equal-key segments.
  ///
  /// Works like `ScanInclusive`, but @a keys partitions @a values into
  /// consecutive segments with identical adjacent keys. A separate inclusive
  /// scan is performed on each segment, and only the scanned values are
  /// returned.
  ///
  /// @pre @a keys and @a values must have the same number of values.
  /// @pre @a binary_functor must be associative.
  ///
  /// @param devId Device adapter to run on.
  /// @param keys Input keys defining scan segments.
  /// @param values Values to scan.
  /// @param values_output Output scanned values.
  /// @param binary_functor Binary scan functor.
  template <typename T,
            typename U,
            typename KIn,
            typename VIn,
            typename VOut,
            typename BinaryFunctor>
  VISKORES_CONT static void ScanInclusiveByKey(viskores::cont::DeviceAdapterId devId,
                                               const viskores::cont::ArrayHandle<T, KIn>& keys,
                                               const viskores::cont::ArrayHandle<U, VIn>& values,
                                               viskores::cont::ArrayHandle<U, VOut>& values_output,
                                               BinaryFunctor binary_functor)
  {
    viskores::cont::TryExecuteOnDevice(
      devId, detail::ScanInclusiveByKeyFunctor(), keys, values, values_output, binary_functor);
  }

  /// @overload
  /// Uses @a binary_functor and the runtime device selection mechanism.
  template <typename T,
            typename U,
            typename KIn,
            typename VIn,
            typename VOut,
            typename BinaryFunctor>
  VISKORES_CONT static void ScanInclusiveByKey(const viskores::cont::ArrayHandle<T, KIn>& keys,
                                               const viskores::cont::ArrayHandle<U, VIn>& values,
                                               viskores::cont::ArrayHandle<U, VOut>& values_output,
                                               BinaryFunctor binary_functor)
  {
    ScanInclusiveByKey(
      viskores::cont::DeviceAdapterTagAny(), keys, values, values_output, binary_functor);
  }


  /// @overload
  /// Uses the default addition operation.
  template <typename T, typename U, typename KIn, typename VIn, typename VOut>
  VISKORES_CONT static void ScanInclusiveByKey(viskores::cont::DeviceAdapterId devId,
                                               const viskores::cont::ArrayHandle<T, KIn>& keys,
                                               const viskores::cont::ArrayHandle<U, VIn>& values,
                                               viskores::cont::ArrayHandle<U, VOut>& values_output)
  {
    viskores::cont::TryExecuteOnDevice(
      devId, detail::ScanInclusiveByKeyFunctor(), keys, values, values_output);
  }

  /// @overload
  /// Uses the default addition operation and the runtime device selection
  /// mechanism.
  template <typename T, typename U, typename KIn, typename VIn, typename VOut>
  VISKORES_CONT static void ScanInclusiveByKey(const viskores::cont::ArrayHandle<T, KIn>& keys,
                                               const viskores::cont::ArrayHandle<U, VIn>& values,
                                               viskores::cont::ArrayHandle<U, VOut>& values_output)
  {
    ScanInclusiveByKey(viskores::cont::DeviceAdapterTagAny(), keys, values, values_output);
  }


  /// @brief Compute an exclusive prefix sum.
  ///
  /// Performs a running sum on @a input and stores the result in @a output.
  /// For exclusive scans, the value at position `i` excludes the input element
  /// at position `i`. With the default addition operation, the first output
  /// value is 0, the second output value is the first input value, and so on.
  /// If @a input and @a output are the same array, the operation is performed
  /// in place.
  ///
  /// This overload uses the default addition operation. The operation must be
  /// associative to produce consistent results.
  ///
  /// @param devId Device adapter to run on.
  /// @param input Values to scan.
  /// @param output Exclusive scan output.
  /// @returns The sum of all input values.
  template <typename T, class CIn, class COut>
  VISKORES_CONT static T ScanExclusive(viskores::cont::DeviceAdapterId devId,
                                       const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output)
  {
    detail::ScanExclusiveFunctor<T> functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, input, output);
    return functor.result;
  }

  /// @overload
  /// Uses the runtime device selection mechanism.
  template <typename T, class CIn, class COut>
  VISKORES_CONT static T ScanExclusive(const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output)
  {
    return ScanExclusive(viskores::cont::DeviceAdapterTagAny(), input, output);
  }


  /// @overload
  /// Uses @a binaryFunctor as the scan operation and @a initialValue as the
  /// first exclusive value. The functor must be associative.
  template <typename T, class CIn, class COut, class BinaryFunctor>
  VISKORES_CONT static T ScanExclusive(viskores::cont::DeviceAdapterId devId,
                                       const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output,
                                       BinaryFunctor binaryFunctor,
                                       const T& initialValue)
  {
    detail::ScanExclusiveFunctor<T> functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, input, output, binaryFunctor, initialValue);
    return functor.result;
  }

  /// @overload
  /// Uses @a binaryFunctor, @a initialValue, and the runtime device selection
  /// mechanism.
  template <typename T, class CIn, class COut, class BinaryFunctor>
  VISKORES_CONT static T ScanExclusive(const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output,
                                       BinaryFunctor binaryFunctor,
                                       const T& initialValue)
  {
    return ScanExclusive(
      viskores::cont::DeviceAdapterTagAny(), input, output, binaryFunctor, initialValue);
  }


  /// @brief Compute exclusive prefix sums over consecutive equal-key segments.
  ///
  /// Works like `ScanExclusive`, but @a keys partitions @a values into
  /// consecutive segments with identical adjacent keys. A separate exclusive
  /// scan is performed on each segment, and only the scanned values are
  /// returned.
  ///
  /// @pre @a keys and @a values must have the same number of values.
  /// @pre @a binaryFunctor must be associative.
  ///
  /// @param devId Device adapter to run on.
  /// @param keys Input keys defining scan segments.
  /// @param values Values to scan.
  /// @param output Output scanned values.
  /// @param initialValue Initial value for each segment.
  /// @param binaryFunctor Binary scan functor.
  template <typename T, typename U, typename KIn, typename VIn, typename VOut, class BinaryFunctor>
  VISKORES_CONT static void ScanExclusiveByKey(viskores::cont::DeviceAdapterId devId,
                                               const viskores::cont::ArrayHandle<T, KIn>& keys,
                                               const viskores::cont::ArrayHandle<U, VIn>& values,
                                               viskores::cont::ArrayHandle<U, VOut>& output,
                                               const U& initialValue,
                                               BinaryFunctor binaryFunctor)
  {
    viskores::cont::TryExecuteOnDevice(devId,
                                       detail::ScanExclusiveByKeyFunctor(),
                                       keys,
                                       values,
                                       output,
                                       initialValue,
                                       binaryFunctor);
  }

  /// @overload
  /// Uses @a binaryFunctor, @a initialValue, and the runtime device selection
  /// mechanism.
  template <typename T, typename U, typename KIn, typename VIn, typename VOut, class BinaryFunctor>
  VISKORES_CONT static void ScanExclusiveByKey(const viskores::cont::ArrayHandle<T, KIn>& keys,
                                               const viskores::cont::ArrayHandle<U, VIn>& values,
                                               viskores::cont::ArrayHandle<U, VOut>& output,
                                               const U& initialValue,
                                               BinaryFunctor binaryFunctor)
  {
    ScanExclusiveByKey(
      viskores::cont::DeviceAdapterTagAny(), keys, values, output, initialValue, binaryFunctor);
  }


  /// @overload
  /// Uses the default addition operation and its zero-initialized value as the
  /// initial value for each segment.
  template <typename T, typename U, class KIn, typename VIn, typename VOut>
  VISKORES_CONT static void ScanExclusiveByKey(viskores::cont::DeviceAdapterId devId,
                                               const viskores::cont::ArrayHandle<T, KIn>& keys,
                                               const viskores::cont::ArrayHandle<U, VIn>& values,
                                               viskores::cont::ArrayHandle<U, VOut>& output)
  {
    viskores::cont::TryExecuteOnDevice(
      devId, detail::ScanExclusiveByKeyFunctor(), keys, values, output);
  }

  /// @overload
  /// Uses the default addition operation and the runtime device selection
  /// mechanism.
  template <typename T, typename U, class KIn, typename VIn, typename VOut>
  VISKORES_CONT static void ScanExclusiveByKey(const viskores::cont::ArrayHandle<T, KIn>& keys,
                                               const viskores::cont::ArrayHandle<U, VIn>& values,
                                               viskores::cont::ArrayHandle<U, VOut>& output)
  {
    ScanExclusiveByKey(viskores::cont::DeviceAdapterTagAny(), keys, values, output);
  }


  /// @brief Compute an extended prefix sum.
  ///
  /// Computes an extended prefix sum of @a input and stores it in @a output.
  /// The output array has one more value than the input array. Exclusive scan
  /// values are stored in indices `[0, size - 1]`, and inclusive scan values
  /// are stored in indices `[1, size]`. Thus, the first output entry is 0 and
  /// the last output entry is the total sum.
  ///
  /// Unlike `ScanInclusive` and `ScanExclusive`, this method does not return
  /// the total. This can be more efficient on some devices because the total
  /// does not need to be copied back to the control environment.
  ///
  /// This overload uses the default addition operation. The operation must be
  /// associative to produce consistent results.
  ///
  /// @param devId Device adapter to run on.
  /// @param input Values to scan.
  /// @param output Extended scan output. Resized to
  /// `input.GetNumberOfValues() + 1`.
  template <typename T, class CIn, class COut>
  VISKORES_CONT static void ScanExtended(viskores::cont::DeviceAdapterId devId,
                                         const viskores::cont::ArrayHandle<T, CIn>& input,
                                         viskores::cont::ArrayHandle<T, COut>& output)
  {
    detail::ScanExtendedFunctor<T> functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, input, output);
  }

  /// @overload
  /// Uses the runtime device selection mechanism.
  template <typename T, class CIn, class COut>
  VISKORES_CONT static void ScanExtended(const viskores::cont::ArrayHandle<T, CIn>& input,
                                         viskores::cont::ArrayHandle<T, COut>& output)
  {
    ScanExtended(viskores::cont::DeviceAdapterTagAny(), input, output);
  }


  /// @overload
  /// Uses @a binaryFunctor as the scan operation and @a initialValue as the
  /// first output entry. The functor must be associative.
  template <typename T, class CIn, class COut, class BinaryFunctor>
  VISKORES_CONT static void ScanExtended(viskores::cont::DeviceAdapterId devId,
                                         const viskores::cont::ArrayHandle<T, CIn>& input,
                                         viskores::cont::ArrayHandle<T, COut>& output,
                                         BinaryFunctor binaryFunctor,
                                         const T& initialValue)
  {
    detail::ScanExtendedFunctor<T> functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, input, output, binaryFunctor, initialValue);
  }

  /// @overload
  /// Uses @a binaryFunctor, @a initialValue, and the runtime device selection
  /// mechanism.
  template <typename T, class CIn, class COut, class BinaryFunctor>
  VISKORES_CONT static void ScanExtended(const viskores::cont::ArrayHandle<T, CIn>& input,
                                         viskores::cont::ArrayHandle<T, COut>& output,
                                         BinaryFunctor binaryFunctor,
                                         const T& initialValue)
  {
    ScanExtended(viskores::cont::DeviceAdapterTagAny(), input, output, binaryFunctor, initialValue);
  }

  // Should this be deprecated in favor of `RuntimeDeviceTracker`?
  /// @brief Schedule a 1D set of functor invocations on a device.
  ///
  /// Invokes @a functor once for each index in `[0, numInstances)`. Each
  /// invocation should be assumed to occur on a separate thread, although a
  /// device implementation may share threads in practice.
  ///
  /// The functor must provide a const `operator()(viskores::Id)` and must
  /// subclass `viskores::exec::FunctorBase` for execution-environment error
  /// handling.
  ///
  /// @param devId Device adapter to run on.
  /// @param functor Function object to invoke.
  /// @param numInstances Number of invocations.
  template <typename Functor>
  VISKORES_CONT static void Schedule(viskores::cont::DeviceAdapterId devId,
                                     Functor functor,
                                     viskores::Id numInstances)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::ScheduleFunctor{}, functor, numInstances);
  }

  /// @overload
  /// Uses @a hints to influence runtime device selection.
  template <typename... Hints, typename Functor>
  VISKORES_CONT static void Schedule(viskores::cont::internal::HintList<Hints...> hints,
                                     Functor functor,
                                     viskores::Id numInstances)
  {
    viskores::cont::TryExecute(detail::ScheduleFunctor{}, hints, functor, numInstances);
  }

  /// @overload
  /// Uses the runtime device selection mechanism.
  template <typename Functor>
  VISKORES_CONT static void Schedule(Functor functor, viskores::Id numInstances)
  {
    Schedule(viskores::cont::DeviceAdapterTagAny{}, functor, numInstances);
  }


  /// @brief Schedule a 3D set of functor invocations on a device.
  ///
  /// Invokes @a functor once for every entry in a 3D index space with
  /// dimensions @a rangeMax. Each invocation should be assumed to occur on a
  /// separate thread, although a device implementation may share threads in
  /// practice.
  ///
  /// The functor must provide a const `operator()(viskores::Id3)` and must
  /// subclass `viskores::exec::FunctorBase` for execution-environment error
  /// handling.
  ///
  /// @param devId Device adapter to run on.
  /// @param functor Function object to invoke.
  /// @param rangeMax Dimensions of the 3D index space.
  template <typename Functor>
  VISKORES_CONT static void Schedule(viskores::cont::DeviceAdapterId devId,
                                     Functor functor,
                                     viskores::Id3 rangeMax)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::ScheduleFunctor(), functor, rangeMax);
  }

  /// @overload
  /// Uses @a hints to influence runtime device selection.
  template <typename... Hints, typename Functor>
  VISKORES_CONT static void Schedule(viskores::cont::internal::HintList<Hints...> hints,
                                     Functor functor,
                                     viskores::Id3 rangeMax)
  {
    viskores::cont::TryExecute(detail::ScheduleFunctor{}, hints, functor, rangeMax);
  }

  /// @overload
  /// Uses the runtime device selection mechanism.
  template <typename Functor>
  VISKORES_CONT static void Schedule(Functor functor, viskores::Id3 rangeMax)
  {
    Schedule(viskores::cont::DeviceAdapterTagAny(), functor, rangeMax);
  }


  /// @brief Sort an array in ascending order.
  ///
  /// Performs an unstable in-place sort of @a values using the default
  /// less-than ordering.
  ///
  /// @param devId Device adapter to run on.
  /// @param values Array to sort in place.
  template <typename T, class Storage>
  VISKORES_CONT static void Sort(viskores::cont::DeviceAdapterId devId,
                                 viskores::cont::ArrayHandle<T, Storage>& values)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::SortFunctor(), values);
  }

  /// @overload
  /// Uses the runtime device selection mechanism.
  template <typename T, class Storage>
  VISKORES_CONT static void Sort(viskores::cont::ArrayHandle<T, Storage>& values)
  {
    Sort(viskores::cont::DeviceAdapterTagAny(), values);
  }


  /// @overload
  /// Sorts according to @a binary_compare, which must define a strict weak
  /// ordering.
  template <typename T, class Storage, class BinaryCompare>
  VISKORES_CONT static void Sort(viskores::cont::DeviceAdapterId devId,
                                 viskores::cont::ArrayHandle<T, Storage>& values,
                                 BinaryCompare binary_compare)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::SortFunctor(), values, binary_compare);
  }

  /// @overload
  /// Uses @a binary_compare and the runtime device selection mechanism.
  template <typename T, class Storage, class BinaryCompare>
  VISKORES_CONT static void Sort(viskores::cont::ArrayHandle<T, Storage>& values,
                                 BinaryCompare binary_compare)
  {
    Sort(viskores::cont::DeviceAdapterTagAny(), values, binary_compare);
  }


  /// @brief Sort keys and reorder associated values.
  ///
  /// Performs an unstable in-place sort of @a keys using the default less-than
  /// ordering. @a values is reordered so each value remains paired with its
  /// original key.
  ///
  /// @pre @a keys and @a values must have the same number of values.
  ///
  /// @param devId Device adapter to run on.
  /// @param keys Keys to sort in place.
  /// @param values Values to reorder with their corresponding keys.
  template <typename T, typename U, class StorageT, class StorageU>
  VISKORES_CONT static void SortByKey(viskores::cont::DeviceAdapterId devId,
                                      viskores::cont::ArrayHandle<T, StorageT>& keys,
                                      viskores::cont::ArrayHandle<U, StorageU>& values)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::SortByKeyFunctor(), keys, values);
  }

  /// @overload
  /// Uses the runtime device selection mechanism.
  template <typename T, typename U, class StorageT, class StorageU>
  VISKORES_CONT static void SortByKey(viskores::cont::ArrayHandle<T, StorageT>& keys,
                                      viskores::cont::ArrayHandle<U, StorageU>& values)
  {
    SortByKey(viskores::cont::DeviceAdapterTagAny(), keys, values);
  }

  /// @overload
  /// Sorts keys according to @a binary_compare, which must define a strict weak
  /// ordering.
  template <typename T, typename U, class StorageT, class StorageU, class BinaryCompare>
  VISKORES_CONT static void SortByKey(viskores::cont::DeviceAdapterId devId,
                                      viskores::cont::ArrayHandle<T, StorageT>& keys,
                                      viskores::cont::ArrayHandle<U, StorageU>& values,
                                      BinaryCompare binary_compare)
  {
    viskores::cont::TryExecuteOnDevice(
      devId, detail::SortByKeyFunctor(), keys, values, binary_compare);
  }

  /// @overload
  /// Uses @a binary_compare and the runtime device selection mechanism.
  template <typename T, typename U, class StorageT, class StorageU, class BinaryCompare>
  VISKORES_CONT static void SortByKey(viskores::cont::ArrayHandle<T, StorageT>& keys,
                                      viskores::cont::ArrayHandle<U, StorageU>& values,
                                      BinaryCompare binary_compare)
  {
    SortByKey(viskores::cont::DeviceAdapterTagAny(), keys, values, binary_compare);
  }


  /// @brief Wait for asynchronous device operations to complete.
  ///
  /// @param devId Device adapter to synchronize.
  VISKORES_CONT static void Synchronize(viskores::cont::DeviceAdapterId devId)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::SynchronizeFunctor());
  }

  /// @overload
  /// Synchronizes any selected runtime device.
  VISKORES_CONT static void Synchronize() { Synchronize(viskores::cont::DeviceAdapterTagAny()); }


  /// @brief Apply a binary operation element-wise to two arrays.
  ///
  /// Applies @a binaryFunctor to corresponding values from @a input1 and
  /// @a input2 and stores the result in @a output. The input arrays do not need
  /// to have the same number of values. The output array is resized to the
  /// smaller input length.
  ///
  /// @param devId Device adapter to run on.
  /// @param input1 First input array.
  /// @param input2 Second input array.
  /// @param output Destination array.
  /// @param binaryFunctor Binary operation applied to corresponding input
  /// values.
  template <typename T,
            typename U,
            typename V,
            typename StorageT,
            typename StorageU,
            typename StorageV,
            typename BinaryFunctor>
  VISKORES_CONT static void Transform(viskores::cont::DeviceAdapterId devId,
                                      const viskores::cont::ArrayHandle<T, StorageT>& input1,
                                      const viskores::cont::ArrayHandle<U, StorageU>& input2,
                                      viskores::cont::ArrayHandle<V, StorageV>& output,
                                      BinaryFunctor binaryFunctor)
  {
    viskores::cont::TryExecuteOnDevice(
      devId, detail::TransformFunctor(), input1, input2, output, binaryFunctor);
  }

  /// @overload
  /// Uses the runtime device selection mechanism.
  template <typename T,
            typename U,
            typename V,
            typename StorageT,
            typename StorageU,
            typename StorageV,
            typename BinaryFunctor>
  VISKORES_CONT static void Transform(const viskores::cont::ArrayHandle<T, StorageT>& input1,
                                      const viskores::cont::ArrayHandle<U, StorageU>& input2,
                                      viskores::cont::ArrayHandle<V, StorageV>& output,
                                      BinaryFunctor binaryFunctor)
  {
    Transform(viskores::cont::DeviceAdapterTagAny(), input1, input2, output, binaryFunctor);
  }


  /// @brief Remove adjacent duplicate values from an array.
  ///
  /// Removes duplicate values from @a values in place. Only adjacent duplicate
  /// values are removed. Sort the array first when all duplicate values should
  /// be adjacent and removed.
  ///
  /// This overload uses `viskores::Equal` to compare values.
  ///
  /// @param devId Device adapter to run on.
  /// @param values Array to compact to unique adjacent values.
  template <typename T, class Storage>
  VISKORES_CONT static void Unique(viskores::cont::DeviceAdapterId devId,
                                   viskores::cont::ArrayHandle<T, Storage>& values)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::UniqueFunctor(), values);
  }

  /// @overload
  /// Uses the runtime device selection mechanism.
  template <typename T, class Storage>
  VISKORES_CONT static void Unique(viskores::cont::ArrayHandle<T, Storage>& values)
  {
    Unique(viskores::cont::DeviceAdapterTagAny(), values);
  }


  /// @overload
  /// Uses @a binary_compare to decide when two adjacent values are equal. The
  /// predicate must return true when the two values should be considered the
  /// same.
  template <typename T, class Storage, class BinaryCompare>
  VISKORES_CONT static void Unique(viskores::cont::DeviceAdapterId devId,
                                   viskores::cont::ArrayHandle<T, Storage>& values,
                                   BinaryCompare binary_compare)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::UniqueFunctor(), values, binary_compare);
  }

  /// @overload
  /// Uses @a binary_compare and the runtime device selection mechanism.
  template <typename T, class Storage, class BinaryCompare>
  VISKORES_CONT static void Unique(viskores::cont::ArrayHandle<T, Storage>& values,
                                   BinaryCompare binary_compare)
  {
    Unique(viskores::cont::DeviceAdapterTagAny(), values, binary_compare);
  }


  /// @brief Find upper-bound insertion indices for a vector of query values.
  ///
  /// For each value in @a values, finds the first index in sorted @a input
  /// whose value is greater than the query value. The results are written to
  /// @a output. This is the vectorized equivalent of `std::upper_bound`.
  ///
  /// @pre @a input must be sorted according to the default less-than ordering.
  ///
  /// @param devId Device adapter to run on.
  /// @param input Sorted values to search.
  /// @param values Query values.
  /// @param output Output upper-bound indices.
  template <typename T, class CIn, class CVal, class COut>
  VISKORES_CONT static void UpperBounds(viskores::cont::DeviceAdapterId devId,
                                        const viskores::cont::ArrayHandle<T, CIn>& input,
                                        const viskores::cont::ArrayHandle<T, CVal>& values,
                                        viskores::cont::ArrayHandle<viskores::Id, COut>& output)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::UpperBoundsFunctor(), input, values, output);
  }

  /// @overload
  /// Uses the runtime device selection mechanism.
  template <typename T, class CIn, class CVal, class COut>
  VISKORES_CONT static void UpperBounds(const viskores::cont::ArrayHandle<T, CIn>& input,
                                        const viskores::cont::ArrayHandle<T, CVal>& values,
                                        viskores::cont::ArrayHandle<viskores::Id, COut>& output)
  {
    UpperBounds(viskores::cont::DeviceAdapterTagAny(), input, values, output);
  }


  /// @overload
  /// Uses @a binary_compare as the less-than operation. @a input must be sorted
  /// according to the same comparison.
  template <typename T, class CIn, class CVal, class COut, class BinaryCompare>
  VISKORES_CONT static void UpperBounds(viskores::cont::DeviceAdapterId devId,
                                        const viskores::cont::ArrayHandle<T, CIn>& input,
                                        const viskores::cont::ArrayHandle<T, CVal>& values,
                                        viskores::cont::ArrayHandle<viskores::Id, COut>& output,
                                        BinaryCompare binary_compare)
  {
    viskores::cont::TryExecuteOnDevice(
      devId, detail::UpperBoundsFunctor(), input, values, output, binary_compare);
  }

  /// @overload
  /// Uses @a binary_compare and the runtime device selection mechanism.
  template <typename T, class CIn, class CVal, class COut, class BinaryCompare>
  VISKORES_CONT static void UpperBounds(const viskores::cont::ArrayHandle<T, CIn>& input,
                                        const viskores::cont::ArrayHandle<T, CVal>& values,
                                        viskores::cont::ArrayHandle<viskores::Id, COut>& output,
                                        BinaryCompare binary_compare)
  {
    UpperBounds(viskores::cont::DeviceAdapterTagAny(), input, values, output, binary_compare);
  }


  /// @overload
  /// In-place specialization for `viskores::Id` arrays. The values in
  /// @a values_output are replaced with their upper-bound indices in @a input.
  /// This form is useful for inverting index maps.
  template <class CIn, class COut>
  VISKORES_CONT static void UpperBounds(
    viskores::cont::DeviceAdapterId devId,
    const viskores::cont::ArrayHandle<viskores::Id, CIn>& input,
    viskores::cont::ArrayHandle<viskores::Id, COut>& values_output)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::UpperBoundsFunctor(), input, values_output);
  }

  /// @overload
  /// In-place `viskores::Id` specialization using the runtime device selection
  /// mechanism.
  template <class CIn, class COut>
  VISKORES_CONT static void UpperBounds(
    const viskores::cont::ArrayHandle<viskores::Id, CIn>& input,
    viskores::cont::ArrayHandle<viskores::Id, COut>& values_output)
  {
    UpperBounds(viskores::cont::DeviceAdapterTagAny(), input, values_output);
  }
};
}
} // namespace viskores::cont

#endif //viskores_cont_Algorithm_h
