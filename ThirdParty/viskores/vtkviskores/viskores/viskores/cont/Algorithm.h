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

struct Algorithm
{

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

  template <typename IndicesStorage>
  VISKORES_CONT static viskores::Id BitFieldToUnorderedSet(
    const viskores::cont::BitField& bits,
    viskores::cont::ArrayHandle<Id, IndicesStorage>& indices)
  {
    detail::BitFieldToUnorderedSetFunctor functor;
    viskores::cont::TryExecute(functor, bits, indices);
    return functor.Result;
  }

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
  template <typename T, typename U, class CIn, class COut>
  VISKORES_CONT static void Copy(const viskores::cont::ArrayHandle<T, CIn>& input,
                                 viskores::cont::ArrayHandle<U, COut>& output)
  {
    Copy(viskores::cont::DeviceAdapterTagAny(), input, output);
  }


  template <typename T, typename U, class CIn, class CStencil, class COut>
  VISKORES_CONT static void CopyIf(viskores::cont::DeviceAdapterId devId,
                                   const viskores::cont::ArrayHandle<T, CIn>& input,
                                   const viskores::cont::ArrayHandle<U, CStencil>& stencil,
                                   viskores::cont::ArrayHandle<T, COut>& output)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::CopyIfFunctor(), input, stencil, output);
  }
  template <typename T, typename U, class CIn, class CStencil, class COut>
  VISKORES_CONT static void CopyIf(const viskores::cont::ArrayHandle<T, CIn>& input,
                                   const viskores::cont::ArrayHandle<U, CStencil>& stencil,
                                   viskores::cont::ArrayHandle<T, COut>& output)
  {
    CopyIf(viskores::cont::DeviceAdapterTagAny(), input, stencil, output);
  }


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
  template <typename T, typename U, class CIn, class CStencil, class COut, class UnaryPredicate>
  VISKORES_CONT static void CopyIf(const viskores::cont::ArrayHandle<T, CIn>& input,
                                   const viskores::cont::ArrayHandle<U, CStencil>& stencil,
                                   viskores::cont::ArrayHandle<T, COut>& output,
                                   UnaryPredicate unary_predicate)
  {
    CopyIf(viskores::cont::DeviceAdapterTagAny(), input, stencil, output, unary_predicate);
  }


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

  VISKORES_CONT static viskores::Id CountSetBits(viskores::cont::DeviceAdapterId devId,
                                                 const viskores::cont::BitField& bits)
  {
    detail::CountSetBitsFunctor functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, bits);
    return functor.PopCount;
  }

  VISKORES_CONT static viskores::Id CountSetBits(const viskores::cont::BitField& bits)
  {
    return CountSetBits(viskores::cont::DeviceAdapterTagAny{}, bits);
  }

  VISKORES_CONT static void Fill(viskores::cont::DeviceAdapterId devId,
                                 viskores::cont::BitField& bits,
                                 bool value,
                                 viskores::Id numBits)
  {
    detail::FillFunctor functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, bits, value, numBits);
  }

  VISKORES_CONT static void Fill(viskores::cont::BitField& bits, bool value, viskores::Id numBits)
  {
    Fill(viskores::cont::DeviceAdapterTagAny{}, bits, value, numBits);
  }

  VISKORES_CONT static void Fill(viskores::cont::DeviceAdapterId devId,
                                 viskores::cont::BitField& bits,
                                 bool value)
  {
    detail::FillFunctor functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, bits, value);
  }

  VISKORES_CONT static void Fill(viskores::cont::BitField& bits, bool value)
  {
    Fill(viskores::cont::DeviceAdapterTagAny{}, bits, value);
  }

  template <typename WordType>
  VISKORES_CONT static void Fill(viskores::cont::DeviceAdapterId devId,
                                 viskores::cont::BitField& bits,
                                 WordType word,
                                 viskores::Id numBits)
  {
    detail::FillFunctor functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, bits, word, numBits);
  }

  template <typename WordType>
  VISKORES_CONT static void Fill(viskores::cont::BitField& bits,
                                 WordType word,
                                 viskores::Id numBits)
  {
    Fill(viskores::cont::DeviceAdapterTagAny{}, bits, word, numBits);
  }

  template <typename WordType>
  VISKORES_CONT static void Fill(viskores::cont::DeviceAdapterId devId,
                                 viskores::cont::BitField& bits,
                                 WordType word)
  {
    detail::FillFunctor functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, bits, word);
  }

  template <typename WordType>
  VISKORES_CONT static void Fill(viskores::cont::BitField& bits, WordType word)
  {
    Fill(viskores::cont::DeviceAdapterTagAny{}, bits, word);
  }

  template <typename T, typename S>
  VISKORES_CONT static void Fill(viskores::cont::DeviceAdapterId devId,
                                 viskores::cont::ArrayHandle<T, S>& handle,
                                 const T& value)
  {
    detail::FillFunctor functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, handle, value);
  }

  template <typename T, typename S>
  VISKORES_CONT static void Fill(viskores::cont::ArrayHandle<T, S>& handle, const T& value)
  {
    Fill(viskores::cont::DeviceAdapterTagAny{}, handle, value);
  }

  template <typename T, typename S>
  VISKORES_CONT static void Fill(viskores::cont::DeviceAdapterId devId,
                                 viskores::cont::ArrayHandle<T, S>& handle,
                                 const T& value,
                                 const viskores::Id numValues)
  {
    detail::FillFunctor functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, handle, value, numValues);
  }

  template <typename T, typename S>
  VISKORES_CONT static void Fill(viskores::cont::ArrayHandle<T, S>& handle,
                                 const T& value,
                                 const viskores::Id numValues)
  {
    Fill(viskores::cont::DeviceAdapterTagAny{}, handle, value, numValues);
  }

  template <typename T, class CIn, class CVal, class COut>
  VISKORES_CONT static void LowerBounds(viskores::cont::DeviceAdapterId devId,
                                        const viskores::cont::ArrayHandle<T, CIn>& input,
                                        const viskores::cont::ArrayHandle<T, CVal>& values,
                                        viskores::cont::ArrayHandle<viskores::Id, COut>& output)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::LowerBoundsFunctor(), input, values, output);
  }
  template <typename T, class CIn, class CVal, class COut>
  VISKORES_CONT static void LowerBounds(const viskores::cont::ArrayHandle<T, CIn>& input,
                                        const viskores::cont::ArrayHandle<T, CVal>& values,
                                        viskores::cont::ArrayHandle<viskores::Id, COut>& output)
  {
    LowerBounds(viskores::cont::DeviceAdapterTagAny(), input, values, output);
  }


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
  template <typename T, class CIn, class CVal, class COut, class BinaryCompare>
  VISKORES_CONT static void LowerBounds(const viskores::cont::ArrayHandle<T, CIn>& input,
                                        const viskores::cont::ArrayHandle<T, CVal>& values,
                                        viskores::cont::ArrayHandle<viskores::Id, COut>& output,
                                        BinaryCompare binary_compare)
  {
    LowerBounds(viskores::cont::DeviceAdapterTagAny(), input, values, output, binary_compare);
  }


  template <class CIn, class COut>
  VISKORES_CONT static void LowerBounds(
    viskores::cont::DeviceAdapterId devId,
    const viskores::cont::ArrayHandle<viskores::Id, CIn>& input,
    viskores::cont::ArrayHandle<viskores::Id, COut>& values_output)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::LowerBoundsFunctor(), input, values_output);
  }
  template <class CIn, class COut>
  VISKORES_CONT static void LowerBounds(
    const viskores::cont::ArrayHandle<viskores::Id, CIn>& input,
    viskores::cont::ArrayHandle<viskores::Id, COut>& values_output)
  {
    LowerBounds(viskores::cont::DeviceAdapterTagAny(), input, values_output);
  }


  template <typename T, typename U, class CIn>
  VISKORES_CONT static U Reduce(viskores::cont::DeviceAdapterId devId,
                                const viskores::cont::ArrayHandle<T, CIn>& input,
                                U initialValue)
  {
    detail::ReduceFunctor<U> functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, input, initialValue);
    return functor.result;
  }
  template <typename T, typename U, class CIn>
  VISKORES_CONT static U Reduce(const viskores::cont::ArrayHandle<T, CIn>& input, U initialValue)
  {
    return Reduce(viskores::cont::DeviceAdapterTagAny(), input, initialValue);
  }


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
  template <typename T, typename U, class CIn, class BinaryFunctor>
  VISKORES_CONT static U Reduce(const viskores::cont::ArrayHandle<T, CIn>& input,
                                U initialValue,
                                BinaryFunctor binary_functor)
  {
    return Reduce(viskores::cont::DeviceAdapterTagAny(), input, initialValue, binary_functor);
  }


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


  template <typename T, class CIn, class COut>
  VISKORES_CONT static T ScanInclusive(viskores::cont::DeviceAdapterId devId,
                                       const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output)
  {
    detail::ScanInclusiveResultFunctor<T> functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, input, output);
    return functor.result;
  }
  template <typename T, class CIn, class COut>
  VISKORES_CONT static T ScanInclusive(const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output)
  {
    return ScanInclusive(viskores::cont::DeviceAdapterTagAny(), input, output);
  }


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
  template <typename T, class CIn, class COut, class BinaryFunctor>
  VISKORES_CONT static T ScanInclusive(const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output,
                                       BinaryFunctor binary_functor)
  {
    return ScanInclusive(viskores::cont::DeviceAdapterTagAny(), input, output, binary_functor);
  }


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


  template <typename T, typename U, typename KIn, typename VIn, typename VOut>
  VISKORES_CONT static void ScanInclusiveByKey(viskores::cont::DeviceAdapterId devId,
                                               const viskores::cont::ArrayHandle<T, KIn>& keys,
                                               const viskores::cont::ArrayHandle<U, VIn>& values,
                                               viskores::cont::ArrayHandle<U, VOut>& values_output)
  {
    viskores::cont::TryExecuteOnDevice(
      devId, detail::ScanInclusiveByKeyFunctor(), keys, values, values_output);
  }
  template <typename T, typename U, typename KIn, typename VIn, typename VOut>
  VISKORES_CONT static void ScanInclusiveByKey(const viskores::cont::ArrayHandle<T, KIn>& keys,
                                               const viskores::cont::ArrayHandle<U, VIn>& values,
                                               viskores::cont::ArrayHandle<U, VOut>& values_output)
  {
    ScanInclusiveByKey(viskores::cont::DeviceAdapterTagAny(), keys, values, values_output);
  }


  template <typename T, class CIn, class COut>
  VISKORES_CONT static T ScanExclusive(viskores::cont::DeviceAdapterId devId,
                                       const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output)
  {
    detail::ScanExclusiveFunctor<T> functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, input, output);
    return functor.result;
  }
  template <typename T, class CIn, class COut>
  VISKORES_CONT static T ScanExclusive(const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output)
  {
    return ScanExclusive(viskores::cont::DeviceAdapterTagAny(), input, output);
  }


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
  template <typename T, class CIn, class COut, class BinaryFunctor>
  VISKORES_CONT static T ScanExclusive(const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output,
                                       BinaryFunctor binaryFunctor,
                                       const T& initialValue)
  {
    return ScanExclusive(
      viskores::cont::DeviceAdapterTagAny(), input, output, binaryFunctor, initialValue);
  }


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


  template <typename T, typename U, class KIn, typename VIn, typename VOut>
  VISKORES_CONT static void ScanExclusiveByKey(viskores::cont::DeviceAdapterId devId,
                                               const viskores::cont::ArrayHandle<T, KIn>& keys,
                                               const viskores::cont::ArrayHandle<U, VIn>& values,
                                               viskores::cont::ArrayHandle<U, VOut>& output)
  {
    viskores::cont::TryExecuteOnDevice(
      devId, detail::ScanExclusiveByKeyFunctor(), keys, values, output);
  }
  template <typename T, typename U, class KIn, typename VIn, typename VOut>
  VISKORES_CONT static void ScanExclusiveByKey(const viskores::cont::ArrayHandle<T, KIn>& keys,
                                               const viskores::cont::ArrayHandle<U, VIn>& values,
                                               viskores::cont::ArrayHandle<U, VOut>& output)
  {
    ScanExclusiveByKey(viskores::cont::DeviceAdapterTagAny(), keys, values, output);
  }


  template <typename T, class CIn, class COut>
  VISKORES_CONT static void ScanExtended(viskores::cont::DeviceAdapterId devId,
                                         const viskores::cont::ArrayHandle<T, CIn>& input,
                                         viskores::cont::ArrayHandle<T, COut>& output)
  {
    detail::ScanExtendedFunctor<T> functor;
    viskores::cont::TryExecuteOnDevice(devId, functor, input, output);
  }
  template <typename T, class CIn, class COut>
  VISKORES_CONT static void ScanExtended(const viskores::cont::ArrayHandle<T, CIn>& input,
                                         viskores::cont::ArrayHandle<T, COut>& output)
  {
    ScanExtended(viskores::cont::DeviceAdapterTagAny(), input, output);
  }


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
  template <typename T, class CIn, class COut, class BinaryFunctor>
  VISKORES_CONT static void ScanExtended(const viskores::cont::ArrayHandle<T, CIn>& input,
                                         viskores::cont::ArrayHandle<T, COut>& output,
                                         BinaryFunctor binaryFunctor,
                                         const T& initialValue)
  {
    ScanExtended(viskores::cont::DeviceAdapterTagAny(), input, output, binaryFunctor, initialValue);
  }

  // Should this be deprecated in favor of `RuntimeDeviceTracker`?
  template <typename Functor>
  VISKORES_CONT static void Schedule(viskores::cont::DeviceAdapterId devId,
                                     Functor functor,
                                     viskores::Id numInstances)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::ScheduleFunctor{}, functor, numInstances);
  }
  template <typename... Hints, typename Functor>
  VISKORES_CONT static void Schedule(viskores::cont::internal::HintList<Hints...> hints,
                                     Functor functor,
                                     viskores::Id numInstances)
  {
    viskores::cont::TryExecute(detail::ScheduleFunctor{}, hints, functor, numInstances);
  }
  template <typename Functor>
  VISKORES_CONT static void Schedule(Functor functor, viskores::Id numInstances)
  {
    Schedule(viskores::cont::DeviceAdapterTagAny{}, functor, numInstances);
  }


  template <typename Functor>
  VISKORES_CONT static void Schedule(viskores::cont::DeviceAdapterId devId,
                                     Functor functor,
                                     viskores::Id3 rangeMax)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::ScheduleFunctor(), functor, rangeMax);
  }
  template <typename... Hints, typename Functor>
  VISKORES_CONT static void Schedule(viskores::cont::internal::HintList<Hints...> hints,
                                     Functor functor,
                                     viskores::Id3 rangeMax)
  {
    viskores::cont::TryExecute(detail::ScheduleFunctor{}, hints, functor, rangeMax);
  }
  template <typename Functor>
  VISKORES_CONT static void Schedule(Functor functor, viskores::Id3 rangeMax)
  {
    Schedule(viskores::cont::DeviceAdapterTagAny(), functor, rangeMax);
  }


  template <typename T, class Storage>
  VISKORES_CONT static void Sort(viskores::cont::DeviceAdapterId devId,
                                 viskores::cont::ArrayHandle<T, Storage>& values)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::SortFunctor(), values);
  }
  template <typename T, class Storage>
  VISKORES_CONT static void Sort(viskores::cont::ArrayHandle<T, Storage>& values)
  {
    Sort(viskores::cont::DeviceAdapterTagAny(), values);
  }


  template <typename T, class Storage, class BinaryCompare>
  VISKORES_CONT static void Sort(viskores::cont::DeviceAdapterId devId,
                                 viskores::cont::ArrayHandle<T, Storage>& values,
                                 BinaryCompare binary_compare)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::SortFunctor(), values, binary_compare);
  }
  template <typename T, class Storage, class BinaryCompare>
  VISKORES_CONT static void Sort(viskores::cont::ArrayHandle<T, Storage>& values,
                                 BinaryCompare binary_compare)
  {
    Sort(viskores::cont::DeviceAdapterTagAny(), values, binary_compare);
  }


  template <typename T, typename U, class StorageT, class StorageU>
  VISKORES_CONT static void SortByKey(viskores::cont::DeviceAdapterId devId,
                                      viskores::cont::ArrayHandle<T, StorageT>& keys,
                                      viskores::cont::ArrayHandle<U, StorageU>& values)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::SortByKeyFunctor(), keys, values);
  }
  template <typename T, typename U, class StorageT, class StorageU>
  VISKORES_CONT static void SortByKey(viskores::cont::ArrayHandle<T, StorageT>& keys,
                                      viskores::cont::ArrayHandle<U, StorageU>& values)
  {
    SortByKey(viskores::cont::DeviceAdapterTagAny(), keys, values);
  }

  template <typename T, typename U, class StorageT, class StorageU, class BinaryCompare>
  VISKORES_CONT static void SortByKey(viskores::cont::DeviceAdapterId devId,
                                      viskores::cont::ArrayHandle<T, StorageT>& keys,
                                      viskores::cont::ArrayHandle<U, StorageU>& values,
                                      BinaryCompare binary_compare)
  {
    viskores::cont::TryExecuteOnDevice(
      devId, detail::SortByKeyFunctor(), keys, values, binary_compare);
  }
  template <typename T, typename U, class StorageT, class StorageU, class BinaryCompare>
  VISKORES_CONT static void SortByKey(viskores::cont::ArrayHandle<T, StorageT>& keys,
                                      viskores::cont::ArrayHandle<U, StorageU>& values,
                                      BinaryCompare binary_compare)
  {
    SortByKey(viskores::cont::DeviceAdapterTagAny(), keys, values, binary_compare);
  }


  VISKORES_CONT static void Synchronize(viskores::cont::DeviceAdapterId devId)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::SynchronizeFunctor());
  }
  VISKORES_CONT static void Synchronize() { Synchronize(viskores::cont::DeviceAdapterTagAny()); }


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


  template <typename T, class Storage>
  VISKORES_CONT static void Unique(viskores::cont::DeviceAdapterId devId,
                                   viskores::cont::ArrayHandle<T, Storage>& values)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::UniqueFunctor(), values);
  }
  template <typename T, class Storage>
  VISKORES_CONT static void Unique(viskores::cont::ArrayHandle<T, Storage>& values)
  {
    Unique(viskores::cont::DeviceAdapterTagAny(), values);
  }


  template <typename T, class Storage, class BinaryCompare>
  VISKORES_CONT static void Unique(viskores::cont::DeviceAdapterId devId,
                                   viskores::cont::ArrayHandle<T, Storage>& values,
                                   BinaryCompare binary_compare)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::UniqueFunctor(), values, binary_compare);
  }
  template <typename T, class Storage, class BinaryCompare>
  VISKORES_CONT static void Unique(viskores::cont::ArrayHandle<T, Storage>& values,
                                   BinaryCompare binary_compare)
  {
    Unique(viskores::cont::DeviceAdapterTagAny(), values, binary_compare);
  }


  template <typename T, class CIn, class CVal, class COut>
  VISKORES_CONT static void UpperBounds(viskores::cont::DeviceAdapterId devId,
                                        const viskores::cont::ArrayHandle<T, CIn>& input,
                                        const viskores::cont::ArrayHandle<T, CVal>& values,
                                        viskores::cont::ArrayHandle<viskores::Id, COut>& output)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::UpperBoundsFunctor(), input, values, output);
  }
  template <typename T, class CIn, class CVal, class COut>
  VISKORES_CONT static void UpperBounds(const viskores::cont::ArrayHandle<T, CIn>& input,
                                        const viskores::cont::ArrayHandle<T, CVal>& values,
                                        viskores::cont::ArrayHandle<viskores::Id, COut>& output)
  {
    UpperBounds(viskores::cont::DeviceAdapterTagAny(), input, values, output);
  }


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
  template <typename T, class CIn, class CVal, class COut, class BinaryCompare>
  VISKORES_CONT static void UpperBounds(const viskores::cont::ArrayHandle<T, CIn>& input,
                                        const viskores::cont::ArrayHandle<T, CVal>& values,
                                        viskores::cont::ArrayHandle<viskores::Id, COut>& output,
                                        BinaryCompare binary_compare)
  {
    UpperBounds(viskores::cont::DeviceAdapterTagAny(), input, values, output, binary_compare);
  }


  template <class CIn, class COut>
  VISKORES_CONT static void UpperBounds(
    viskores::cont::DeviceAdapterId devId,
    const viskores::cont::ArrayHandle<viskores::Id, CIn>& input,
    viskores::cont::ArrayHandle<viskores::Id, COut>& values_output)
  {
    viskores::cont::TryExecuteOnDevice(devId, detail::UpperBoundsFunctor(), input, values_output);
  }
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
