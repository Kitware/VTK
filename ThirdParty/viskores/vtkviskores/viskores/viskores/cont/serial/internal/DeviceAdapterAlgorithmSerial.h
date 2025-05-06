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
#ifndef viskores_cont_serial_internal_DeviceAdapterAlgorithmSerial_h
#define viskores_cont_serial_internal_DeviceAdapterAlgorithmSerial_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleZip.h>
#include <viskores/cont/ArrayPortalToIterators.h>
#include <viskores/cont/DeviceAdapterAlgorithm.h>
#include <viskores/cont/ErrorExecution.h>
#include <viskores/cont/internal/DeviceAdapterAlgorithmGeneral.h>
#include <viskores/cont/serial/internal/DeviceAdapterTagSerial.h>

#include <viskores/BinaryOperators.h>

#include <viskores/exec/serial/internal/TaskTiling.h>

#include <algorithm>
#include <iterator>
#include <numeric>
#include <type_traits>

namespace viskores
{
namespace cont
{

template <>
struct DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagSerial>
  : viskores::cont::internal::DeviceAdapterAlgorithmGeneral<
      DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagSerial>,
      viskores::cont::DeviceAdapterTagSerial>
{
private:
  using Device = viskores::cont::DeviceAdapterTagSerial;

  // MSVC likes complain about narrowing type conversions in std::copy and
  // provides no reasonable way to disable the warning. As a work-around, this
  // template calls std::copy if and only if the types match, otherwise falls
  // back to a iterative casting approach. Since std::copy can only really
  // optimize same-type copies, this shouldn't affect performance.
  template <typename InPortal, typename OutPortal>
  static void DoCopy(InPortal src,
                     OutPortal dst,
                     std::false_type,
                     viskores::Id startIndex,
                     viskores::Id numToCopy,
                     viskores::Id outIndex)
  {
    using OutputType = typename OutPortal::ValueType;
    for (viskores::Id index = 0; index < numToCopy; ++index)
    {
      dst.Set(index + startIndex, static_cast<OutputType>(src.Get(index + outIndex)));
    }
  }

  template <typename InPortal, typename OutPortal>
  static void DoCopy(InPortal src,
                     OutPortal dst,
                     std::true_type,
                     viskores::Id startIndex,
                     viskores::Id numToCopy,
                     viskores::Id outIndex)
  {
    std::copy(viskores::cont::ArrayPortalToIteratorBegin(src) + startIndex,
              viskores::cont::ArrayPortalToIteratorBegin(src) + startIndex + numToCopy,
              viskores::cont::ArrayPortalToIteratorBegin(dst) + outIndex);
  }

public:
  template <typename T, typename U, class CIn, class COut>
  VISKORES_CONT static void Copy(const viskores::cont::ArrayHandle<T, CIn>& input,
                                 viskores::cont::ArrayHandle<U, COut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::cont::Token token;

    const viskores::Id inSize = input.GetNumberOfValues();
    auto inputPortal = input.PrepareForInput(DeviceAdapterTagSerial(), token);
    auto outputPortal = output.PrepareForOutput(inSize, DeviceAdapterTagSerial(), token);

    if (inSize <= 0)
    {
      return;
    }

    using InputType = decltype(inputPortal.Get(0));
    using OutputType = decltype(outputPortal.Get(0));

    DoCopy(inputPortal, outputPortal, std::is_same<InputType, OutputType>{}, 0, inSize, 0);
  }

  template <typename T, typename U, class CIn, class CStencil, class COut>
  VISKORES_CONT static void CopyIf(const viskores::cont::ArrayHandle<T, CIn>& input,
                                   const viskores::cont::ArrayHandle<U, CStencil>& stencil,
                                   viskores::cont::ArrayHandle<T, COut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    ::viskores::NotZeroInitialized unary_predicate;
    CopyIf(input, stencil, output, unary_predicate);
  }

  template <typename T, typename U, class CIn, class CStencil, class COut, class UnaryPredicate>
  VISKORES_CONT static void CopyIf(const viskores::cont::ArrayHandle<T, CIn>& input,
                                   const viskores::cont::ArrayHandle<U, CStencil>& stencil,
                                   viskores::cont::ArrayHandle<T, COut>& output,
                                   UnaryPredicate predicate)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::Id writePos = 0;

    {
      viskores::cont::Token token;

      viskores::Id inputSize = input.GetNumberOfValues();
      VISKORES_ASSERT(inputSize == stencil.GetNumberOfValues());

      auto inputPortal = input.PrepareForInput(DeviceAdapterTagSerial(), token);
      auto stencilPortal = stencil.PrepareForInput(DeviceAdapterTagSerial(), token);
      auto outputPortal = output.PrepareForOutput(inputSize, DeviceAdapterTagSerial(), token);

      for (viskores::Id readPos = 0; readPos < inputSize; ++readPos)
      {
        if (predicate(stencilPortal.Get(readPos)))
        {
          outputPortal.Set(writePos, inputPortal.Get(readPos));
          ++writePos;
        }
      }
    }

    output.Allocate(writePos, viskores::CopyFlag::On);
  }

  template <typename T, typename U, class CIn, class COut>
  VISKORES_CONT static bool CopySubRange(const viskores::cont::ArrayHandle<T, CIn>& input,
                                         viskores::Id inputStartIndex,
                                         viskores::Id numberOfElementsToCopy,
                                         viskores::cont::ArrayHandle<U, COut>& output,
                                         viskores::Id outputIndex = 0)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    const viskores::Id inSize = input.GetNumberOfValues();

    // Check if the ranges overlap and fail if they do.
    if (input == output &&
        ((outputIndex >= inputStartIndex &&
          outputIndex < inputStartIndex + numberOfElementsToCopy) ||
         (inputStartIndex >= outputIndex &&
          inputStartIndex < outputIndex + numberOfElementsToCopy)))
    {
      return false;
    }

    if (inputStartIndex < 0 || numberOfElementsToCopy < 0 || outputIndex < 0 ||
        inputStartIndex >= inSize)
    { //invalid parameters
      return false;
    }

    //determine if the numberOfElementsToCopy needs to be reduced
    if (inSize < (inputStartIndex + numberOfElementsToCopy))
    { //adjust the size
      numberOfElementsToCopy = (inSize - inputStartIndex);
    }

    const viskores::Id outSize = output.GetNumberOfValues();
    const viskores::Id copyOutEnd = outputIndex + numberOfElementsToCopy;
    if (outSize < copyOutEnd)
    { //output is not large enough
      if (outSize == 0)
      { //since output has nothing, just need to allocate to correct length
        output.Allocate(copyOutEnd);
      }
      else
      { //we currently have data in this array, so preserve it in the new
        //resized array
        viskores::cont::ArrayHandle<U, COut> temp;
        temp.Allocate(copyOutEnd);
        CopySubRange(output, 0, outSize, temp);
        output = temp;
      }
    }

    viskores::cont::Token token;
    auto inputPortal = input.PrepareForInput(DeviceAdapterTagSerial(), token);
    auto outputPortal = output.PrepareForInPlace(DeviceAdapterTagSerial(), token);

    using InputType = decltype(inputPortal.Get(0));
    using OutputType = decltype(outputPortal.Get(0));

    DoCopy(inputPortal,
           outputPortal,
           std::is_same<InputType, OutputType>(),
           inputStartIndex,
           numberOfElementsToCopy,
           outputIndex);

    return true;
  }

  template <typename T, typename U, class CIn>
  VISKORES_CONT static U Reduce(const viskores::cont::ArrayHandle<T, CIn>& input, U initialValue)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    return Reduce(input, initialValue, viskores::Add());
  }

  template <typename T, typename U, class CIn, class BinaryFunctor>
  VISKORES_CONT static U Reduce(const viskores::cont::ArrayHandle<T, CIn>& input,
                                U initialValue,
                                BinaryFunctor binary_functor)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::cont::Token token;

    internal::WrappedBinaryOperator<U, BinaryFunctor> wrappedOp(binary_functor);
    auto inputPortal = input.PrepareForInput(Device(), token);
    return std::accumulate(viskores::cont::ArrayPortalToIteratorBegin(inputPortal),
                           viskores::cont::ArrayPortalToIteratorEnd(inputPortal),
                           initialValue,
                           wrappedOp);
  }

  template <typename T,
            typename U,
            class KIn,
            class VIn,
            class KOut,
            class VOut,
            class BinaryFunctor>
  VISKORES_CONT static void ReduceByKey(const viskores::cont::ArrayHandle<T, KIn>& keys,
                                        const viskores::cont::ArrayHandle<U, VIn>& values,
                                        viskores::cont::ArrayHandle<T, KOut>& keys_output,
                                        viskores::cont::ArrayHandle<U, VOut>& values_output,
                                        BinaryFunctor binary_functor)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::Id writePos = 0;
    viskores::Id readPos = 0;

    {
      viskores::cont::Token token;

      auto keysPortalIn = keys.PrepareForInput(Device(), token);
      auto valuesPortalIn = values.PrepareForInput(Device(), token);
      const viskores::Id numberOfKeys = keys.GetNumberOfValues();

      VISKORES_ASSERT(numberOfKeys == values.GetNumberOfValues());
      if (numberOfKeys == 0)
      {
        keys_output.ReleaseResources();
        values_output.ReleaseResources();
        return;
      }

      auto keysPortalOut = keys_output.PrepareForOutput(numberOfKeys, Device(), token);
      auto valuesPortalOut = values_output.PrepareForOutput(numberOfKeys, Device(), token);

      T currentKey = keysPortalIn.Get(readPos);
      U currentValue = valuesPortalIn.Get(readPos);

      for (++readPos; readPos < numberOfKeys; ++readPos)
      {
        while (readPos < numberOfKeys && currentKey == keysPortalIn.Get(readPos))
        {
          currentValue = binary_functor(currentValue, valuesPortalIn.Get(readPos));
          ++readPos;
        }

        if (readPos < numberOfKeys)
        {
          keysPortalOut.Set(writePos, currentKey);
          valuesPortalOut.Set(writePos, currentValue);
          ++writePos;

          currentKey = keysPortalIn.Get(readPos);
          currentValue = valuesPortalIn.Get(readPos);
        }
      }

      //now write out the last set of values
      keysPortalOut.Set(writePos, currentKey);
      valuesPortalOut.Set(writePos, currentValue);
    }

    //now we need to shrink to the correct number of keys/values
    //writePos is zero-based so add 1 to get correct length
    keys_output.Allocate(writePos + 1, viskores::CopyFlag::On);
    values_output.Allocate(writePos + 1, viskores::CopyFlag::On);
  }

  template <typename T, class CIn, class COut, class BinaryFunctor>
  VISKORES_CONT static T ScanInclusive(const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output,
                                       BinaryFunctor binary_functor)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    internal::WrappedBinaryOperator<T, BinaryFunctor> wrappedBinaryOp(binary_functor);

    viskores::Id numberOfValues = input.GetNumberOfValues();

    viskores::cont::Token token;

    auto inputPortal = input.PrepareForInput(Device(), token);
    auto outputPortal = output.PrepareForOutput(numberOfValues, Device(), token);

    if (numberOfValues <= 0)
    {
      return viskores::TypeTraits<T>::ZeroInitialization();
    }

    std::partial_sum(viskores::cont::ArrayPortalToIteratorBegin(inputPortal),
                     viskores::cont::ArrayPortalToIteratorEnd(inputPortal),
                     viskores::cont::ArrayPortalToIteratorBegin(outputPortal),
                     wrappedBinaryOp);

    // Return the value at the last index in the array, which is the full sum.
    return outputPortal.Get(numberOfValues - 1);
  }

  template <typename T, class CIn, class COut>
  VISKORES_CONT static T ScanInclusive(const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    return ScanInclusive(input, output, viskores::Sum());
  }

  template <typename T, class CIn, class COut, class BinaryFunctor>
  VISKORES_CONT static T ScanExclusive(const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output,
                                       BinaryFunctor binaryFunctor,
                                       const T& initialValue)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    internal::WrappedBinaryOperator<T, BinaryFunctor> wrappedBinaryOp(binaryFunctor);

    viskores::Id numberOfValues = input.GetNumberOfValues();

    viskores::cont::Token token;
    auto inputPortal = input.PrepareForInput(Device(), token);
    auto outputPortal = output.PrepareForOutput(numberOfValues, Device(), token);

    if (numberOfValues <= 0)
    {
      return initialValue;
    }

    // Shift right by one, by iterating backwards. We are required to iterate
    //backwards so that the algorithm works correctly when the input and output
    //are the same array, otherwise you just propagate the first element
    //to all elements
    //Note: We explicitly do not use std::copy_backwards for good reason.
    //The ICC compiler has been found to improperly optimize the copy_backwards
    //into a standard copy, causing the above issue.
    T lastValue = inputPortal.Get(numberOfValues - 1);
    for (viskores::Id i = (numberOfValues - 1); i >= 1; --i)
    {
      outputPortal.Set(i, inputPortal.Get(i - 1));
    }
    outputPortal.Set(0, initialValue);

    std::partial_sum(viskores::cont::ArrayPortalToIteratorBegin(outputPortal),
                     viskores::cont::ArrayPortalToIteratorEnd(outputPortal),
                     viskores::cont::ArrayPortalToIteratorBegin(outputPortal),
                     wrappedBinaryOp);

    return wrappedBinaryOp(outputPortal.Get(numberOfValues - 1), lastValue);
  }

  template <typename T, class CIn, class COut>
  VISKORES_CONT static T ScanExclusive(const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    return ScanExclusive(
      input, output, viskores::Sum(), viskores::TypeTraits<T>::ZeroInitialization());
  }

  VISKORES_CONT_EXPORT static void ScheduleTask(
    viskores::exec::serial::internal::TaskTiling1D& functor,
    viskores::Id size);
  VISKORES_CONT_EXPORT static void ScheduleTask(
    viskores::exec::serial::internal::TaskTiling3D& functor,
    viskores::Id3 size);

  template <typename Hints, typename FunctorType>
  VISKORES_CONT static inline void Schedule(Hints, FunctorType functor, viskores::Id size)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::exec::serial::internal::TaskTiling1D kernel(functor);
    ScheduleTask(kernel, size);
  }

  template <typename FunctorType>
  VISKORES_CONT static inline void Schedule(FunctorType&& functor, viskores::Id size)
  {
    Schedule(viskores::cont::internal::HintList<>{}, functor, size);
  }

  template <typename Hints, typename FunctorType>
  VISKORES_CONT static inline void Schedule(Hints, FunctorType functor, viskores::Id3 size)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::exec::serial::internal::TaskTiling3D kernel(functor);
    ScheduleTask(kernel, size);
  }

  template <typename FunctorType>
  VISKORES_CONT static inline void Schedule(FunctorType&& functor, viskores::Id3 size)
  {
    Schedule(viskores::cont::internal::HintList<>{}, functor, size);
  }

private:
  template <typename Vin,
            typename I,
            typename Vout,
            class StorageVin,
            class StorageI,
            class StorageVout>
  VISKORES_CONT static void Scatter(viskores::cont::ArrayHandle<Vin, StorageVin>& values,
                                    viskores::cont::ArrayHandle<I, StorageI>& index,
                                    viskores::cont::ArrayHandle<Vout, StorageVout>& values_out)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    const viskores::Id n = values.GetNumberOfValues();
    VISKORES_ASSERT(n == index.GetNumberOfValues());

    viskores::cont::Token token;

    auto valuesPortal = values.PrepareForInput(Device(), token);
    auto indexPortal = index.PrepareForInput(Device(), token);
    auto valuesOutPortal = values_out.PrepareForOutput(n, Device(), token);

    for (viskores::Id i = 0; i < n; i++)
    {
      valuesOutPortal.Set(i, valuesPortal.Get(indexPortal.Get(i)));
    }
  }

  /// Reorder the value array along with the sorting algorithm
  template <typename T, typename U, class StorageT, class StorageU, class BinaryCompare>
  VISKORES_CONT static void SortByKeyDirect(viskores::cont::ArrayHandle<T, StorageT>& keys,
                                            viskores::cont::ArrayHandle<U, StorageU>& values,
                                            BinaryCompare binary_compare)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    //combine the keys and values into a ZipArrayHandle
    //we than need to specify a custom compare function wrapper
    //that only checks for key side of the pair, using the custom compare
    //functor that the user passed in
    auto zipHandle = viskores::cont::make_ArrayHandleZip(keys, values);
    Sort(zipHandle, internal::KeyCompare<T, U, BinaryCompare>(binary_compare));
  }

public:
  template <typename T, typename U, class StorageT, class StorageU>
  VISKORES_CONT static void SortByKey(viskores::cont::ArrayHandle<T, StorageT>& keys,
                                      viskores::cont::ArrayHandle<U, StorageU>& values)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    SortByKey(keys, values, std::less<T>());
  }

  template <typename T, typename U, class StorageT, class StorageU, class BinaryCompare>
  VISKORES_CONT static void SortByKey(viskores::cont::ArrayHandle<T, StorageT>& keys,
                                      viskores::cont::ArrayHandle<U, StorageU>& values,
                                      const BinaryCompare& binary_compare)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    internal::WrappedBinaryOperator<bool, BinaryCompare> wrappedCompare(binary_compare);
    constexpr bool larger_than_64bits = sizeof(U) > sizeof(viskores::Int64);
    if (larger_than_64bits)
    {
      /// More efficient sort:
      /// Move value indexes when sorting and reorder the value array at last
      viskores::cont::ArrayHandle<viskores::Id> indexArray;
      viskores::cont::ArrayHandle<U, StorageU> valuesScattered;

      Copy(ArrayHandleIndex(keys.GetNumberOfValues()), indexArray);
      SortByKeyDirect(keys, indexArray, wrappedCompare);
      Scatter(values, indexArray, valuesScattered);
      Copy(valuesScattered, values);
    }
    else
    {
      SortByKeyDirect(keys, values, wrappedCompare);
    }
  }

  template <typename T, class Storage>
  VISKORES_CONT static void Sort(viskores::cont::ArrayHandle<T, Storage>& values)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    Sort(values, std::less<T>());
  }

  template <typename T, class Storage, class BinaryCompare>
  VISKORES_CONT static void Sort(viskores::cont::ArrayHandle<T, Storage>& values,
                                 BinaryCompare binary_compare)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::cont::Token token;

    auto arrayPortal = values.PrepareForInPlace(Device(), token);
    viskores::cont::ArrayPortalToIterators<decltype(arrayPortal)> iterators(arrayPortal);

    internal::WrappedBinaryOperator<bool, BinaryCompare> wrappedCompare(binary_compare);
    std::sort(iterators.GetBegin(), iterators.GetEnd(), wrappedCompare);
  }

  template <typename T, class Storage>
  VISKORES_CONT static void Unique(viskores::cont::ArrayHandle<T, Storage>& values)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    Unique(values, std::equal_to<T>());
  }

  template <typename T, class Storage, class BinaryCompare>
  VISKORES_CONT static void Unique(viskores::cont::ArrayHandle<T, Storage>& values,
                                   BinaryCompare binary_compare)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::cont::Token token;
    auto arrayPortal = values.PrepareForInPlace(Device(), token);
    viskores::cont::ArrayPortalToIterators<decltype(arrayPortal)> iterators(arrayPortal);
    internal::WrappedBinaryOperator<bool, BinaryCompare> wrappedCompare(binary_compare);

    auto end = std::unique(iterators.GetBegin(), iterators.GetEnd(), wrappedCompare);
    viskores::Id newSize = static_cast<viskores::Id>(end - iterators.GetBegin());
    token.DetachFromAll();
    values.Allocate(newSize, viskores::CopyFlag::On);
  }

  VISKORES_CONT static void Synchronize()
  {
    // Nothing to do. This device is serial and has no asynchronous operations.
  }
};

template <>
class DeviceTaskTypes<viskores::cont::DeviceAdapterTagSerial>
{
public:
  template <typename Hints, typename WorkletType, typename InvocationType>
  static viskores::exec::serial::internal::TaskTiling1D MakeTask(WorkletType& worklet,
                                                                 InvocationType& invocation,
                                                                 viskores::Id,
                                                                 Hints = Hints{})
  {
    // Currently ignoring hints.
    return viskores::exec::serial::internal::TaskTiling1D(worklet, invocation);
  }

  template <typename Hints, typename WorkletType, typename InvocationType>
  static viskores::exec::serial::internal::TaskTiling3D MakeTask(WorkletType& worklet,
                                                                 InvocationType& invocation,
                                                                 viskores::Id3,
                                                                 Hints = Hints{})
  {
    // Currently ignoring hints.
    return viskores::exec::serial::internal::TaskTiling3D(worklet, invocation);
  }

  template <typename WorkletType, typename InvocationType, typename RangeType>
  VISKORES_CONT static auto MakeTask(WorkletType& worklet,
                                     InvocationType& invocation,
                                     const RangeType& range)
  {
    return MakeTask<viskores::cont::internal::HintList<>>(worklet, invocation, range);
  }
};
}
} // namespace viskores::cont

#endif //viskores_cont_serial_internal_DeviceAdapterAlgorithmSerial_h
