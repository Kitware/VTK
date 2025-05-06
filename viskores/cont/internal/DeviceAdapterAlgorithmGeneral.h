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

#ifndef viskores_cont_internal_DeviceAdapterAlgorithmGeneral_h
#define viskores_cont_internal_DeviceAdapterAlgorithmGeneral_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleDecorator.h>
#include <viskores/cont/ArrayHandleDiscard.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandleView.h>
#include <viskores/cont/ArrayHandleZip.h>
#include <viskores/cont/BitField.h>
#include <viskores/cont/Logging.h>
#include <viskores/cont/internal/FunctorsGeneral.h>
#include <viskores/cont/internal/Hints.h>

#include <viskores/exec/internal/ErrorMessageBuffer.h>
#include <viskores/exec/internal/TaskSingular.h>

#include <viskores/BinaryPredicates.h>
#include <viskores/TypeTraits.h>

#include <viskores/internal/Windows.h>

#include <type_traits>

namespace viskores
{
namespace cont
{
namespace internal
{

/// \brief General implementations of device adapter algorithms.
///
/// This struct provides algorithms that implement "general" device adapter
/// algorithms. If a device adapter provides implementations for Schedule,
/// and Synchronize, the rest of the algorithms can be implemented by calling
/// these functions.
///
/// It should be noted that we recommend that you also implement Sort,
/// ScanInclusive, and ScanExclusive for improved performance.
///
/// An easy way to implement the DeviceAdapterAlgorithm specialization is to
/// subclass this and override the implementation of methods as necessary.
/// As an example, the code would look something like this.
///
/// \code{.cpp}
/// template<>
/// struct DeviceAdapterAlgorithm<DeviceAdapterTagFoo>
///    : DeviceAdapterAlgorithmGeneral<DeviceAdapterAlgorithm<DeviceAdapterTagFoo>,
///                                    DeviceAdapterTagFoo>
/// {
///   template<typename Hints, typename Functor>
///   VISKORES_CONT static void Schedule(Hints, Functor functor, viskores::Id numInstances)
///   {
///     ...
///   }
///
///   template<typename Functor>
///   VISKORES_CONT static void Schedule(Functor&& functor, viskores::Id numInstances)
///   {
///     Schedule(viskores::cont::internal::HintList<>{}, functor, numInstances);
///   }
///
///   template<typename Hints, typename Functor>
///   VISKORES_CONT static void Schedule(Hints, Functor functor, viskores::Id3 maxRange)
///   {
///     ...
///   }
///
///   template<typename Functor>
///   VISKORES_CONT static void Schedule(Functor&& functor, viskores::Id3 maxRange)
///   {
///     Schedule(viskores::cont::internal::HintList<>{}, functor, numInstances);
///   }
///
///   VISKORES_CONT static void Synchronize()
///   {
///     ...
///   }
/// };
/// \endcode
///
/// You might note that DeviceAdapterAlgorithmGeneral has two template
/// parameters that are redundant. Although the first parameter, the class for
/// the actual DeviceAdapterAlgorithm class containing Schedule, and
/// Synchronize is the same as DeviceAdapterAlgorithm<DeviceAdapterTag>, it is
/// made a separate template parameter to avoid a recursive dependence between
/// DeviceAdapterAlgorithmGeneral.h and DeviceAdapterAlgorithm.h
///
template <class DerivedAlgorithm, class DeviceAdapterTag>
struct DeviceAdapterAlgorithmGeneral
{
  //--------------------------------------------------------------------------
  // Get Execution Value
  // This method is used internally to get a single element from the execution
  // array. Normally you would just use ArrayGetValue, but that functionality
  // relies on the device adapter algorithm and would create a circular
  // dependency.
private:
  template <typename T, class CIn>
  VISKORES_CONT static T GetExecutionValue(const viskores::cont::ArrayHandle<T, CIn>& input,
                                           viskores::Id index)
  {
    viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic> output;

    {
      viskores::cont::Token token;

      auto inputPortal = input.PrepareForInput(DeviceAdapterTag(), token);
      auto outputPortal = output.PrepareForOutput(1, DeviceAdapterTag(), token);

      CopyKernel<decltype(inputPortal), decltype(outputPortal)> kernel(
        inputPortal, outputPortal, index);

      DerivedAlgorithm::Schedule(kernel, 1);
    }

    return output.ReadPortal().Get(0);
  }

public:
  //--------------------------------------------------------------------------
  // BitFieldToUnorderedSet
  template <typename IndicesStorage>
  VISKORES_CONT static viskores::Id BitFieldToUnorderedSet(
    const viskores::cont::BitField& bits,
    viskores::cont::ArrayHandle<Id, IndicesStorage>& indices)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::Id numBits = bits.GetNumberOfBits();

    viskores::cont::Token token;

    auto bitsPortal = bits.PrepareForInput(DeviceAdapterTag{}, token);
    auto indicesPortal = indices.PrepareForOutput(numBits, DeviceAdapterTag{}, token);

    std::atomic<viskores::UInt64> popCount;
    popCount.store(0, std::memory_order_seq_cst);

    using Functor = BitFieldToUnorderedSetFunctor<decltype(bitsPortal), decltype(indicesPortal)>;
    Functor functor{ bitsPortal, indicesPortal, popCount };

    DerivedAlgorithm::Schedule(functor, functor.GetNumberOfInstances());
    DerivedAlgorithm::Synchronize();

    token.DetachFromAll();

    numBits = static_cast<viskores::Id>(popCount.load(std::memory_order_seq_cst));

    indices.Allocate(numBits, viskores::CopyFlag::On);
    return numBits;
  }

  //--------------------------------------------------------------------------
  // Copy
  template <typename T, typename U, class CIn, class COut>
  VISKORES_CONT static void Copy(const viskores::cont::ArrayHandle<T, CIn>& input,
                                 viskores::cont::ArrayHandle<U, COut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::cont::Token token;

    const viskores::Id inSize = input.GetNumberOfValues();
    auto inputPortal = input.PrepareForInput(DeviceAdapterTag(), token);
    auto outputPortal = output.PrepareForOutput(inSize, DeviceAdapterTag(), token);

    CopyKernel<decltype(inputPortal), decltype(outputPortal)> kernel(inputPortal, outputPortal);
    DerivedAlgorithm::Schedule(kernel, inSize);
  }

  //--------------------------------------------------------------------------
  // CopyIf
  template <typename T, typename U, class CIn, class CStencil, class COut, class UnaryPredicate>
  VISKORES_CONT static void CopyIf(const viskores::cont::ArrayHandle<T, CIn>& input,
                                   const viskores::cont::ArrayHandle<U, CStencil>& stencil,
                                   viskores::cont::ArrayHandle<T, COut>& output,
                                   UnaryPredicate unary_predicate)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    VISKORES_ASSERT(input.GetNumberOfValues() == stencil.GetNumberOfValues());
    viskores::Id arrayLength = stencil.GetNumberOfValues();

    using IndexArrayType =
      viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagBasic>;
    IndexArrayType indices;

    {
      viskores::cont::Token token;

      auto stencilPortal = stencil.PrepareForInput(DeviceAdapterTag(), token);
      auto indexPortal = indices.PrepareForOutput(arrayLength, DeviceAdapterTag(), token);

      StencilToIndexFlagKernel<decltype(stencilPortal), decltype(indexPortal), UnaryPredicate>
        indexKernel(stencilPortal, indexPortal, unary_predicate);

      DerivedAlgorithm::Schedule(indexKernel, arrayLength);
    }

    viskores::Id outArrayLength = DerivedAlgorithm::ScanExclusive(indices, indices);

    {
      viskores::cont::Token token;

      auto inputPortal = input.PrepareForInput(DeviceAdapterTag(), token);
      auto stencilPortal = stencil.PrepareForInput(DeviceAdapterTag(), token);
      auto indexPortal = indices.PrepareForOutput(arrayLength, DeviceAdapterTag(), token);
      auto outputPortal = output.PrepareForOutput(outArrayLength, DeviceAdapterTag(), token);

      CopyIfKernel<decltype(inputPortal),
                   decltype(stencilPortal),
                   decltype(indexPortal),
                   decltype(outputPortal),
                   UnaryPredicate>
        copyKernel(inputPortal, stencilPortal, indexPortal, outputPortal, unary_predicate);
      DerivedAlgorithm::Schedule(copyKernel, arrayLength);
    }
  }

  template <typename T, typename U, class CIn, class CStencil, class COut>
  VISKORES_CONT static void CopyIf(const viskores::cont::ArrayHandle<T, CIn>& input,
                                   const viskores::cont::ArrayHandle<U, CStencil>& stencil,
                                   viskores::cont::ArrayHandle<T, COut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    ::viskores::NotZeroInitialized unary_predicate;
    DerivedAlgorithm::CopyIf(input, stencil, output, unary_predicate);
  }

  //--------------------------------------------------------------------------
  // CopySubRange
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
        DerivedAlgorithm::CopySubRange(output, 0, outSize, temp);
        output = temp;
      }
    }

    viskores::cont::Token token;

    auto inputPortal = input.PrepareForInput(DeviceAdapterTag(), token);
    auto outputPortal = output.PrepareForInPlace(DeviceAdapterTag(), token);

    CopyKernel<decltype(inputPortal), decltype(outputPortal)> kernel(
      inputPortal, outputPortal, inputStartIndex, outputIndex);
    DerivedAlgorithm::Schedule(kernel, numberOfElementsToCopy);
    return true;
  }

  //--------------------------------------------------------------------------
  // Count Set Bits
  VISKORES_CONT static viskores::Id CountSetBits(const viskores::cont::BitField& bits)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::cont::Token token;

    auto bitsPortal = bits.PrepareForInput(DeviceAdapterTag{}, token);

    std::atomic<viskores::UInt64> popCount;
    popCount.store(0, std::memory_order_relaxed);

    using Functor = CountSetBitsFunctor<decltype(bitsPortal)>;
    Functor functor{ bitsPortal, popCount };

    DerivedAlgorithm::Schedule(functor, functor.GetNumberOfInstances());
    DerivedAlgorithm::Synchronize();

    return static_cast<viskores::Id>(popCount.load(std::memory_order_seq_cst));
  }

  //--------------------------------------------------------------------------
  // Fill Bit Field (bool, resize)
  VISKORES_CONT static void Fill(viskores::cont::BitField& bits, bool value, viskores::Id numBits)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    if (numBits == 0)
    {
      bits.Allocate(0);
      return;
    }

    viskores::cont::Token token;

    auto portal = bits.PrepareForOutput(numBits, DeviceAdapterTag{}, token);

    using WordType = typename viskores::cont::BitField::template ExecutionTypes<
      DeviceAdapterTag>::WordTypePreferred;

    using Functor = FillBitFieldFunctor<decltype(portal), WordType>;
    Functor functor{ portal, value ? ~WordType{ 0 } : WordType{ 0 } };

    const viskores::Id numWords = portal.template GetNumberOfWords<WordType>();
    DerivedAlgorithm::Schedule(functor, numWords);
  }

  //--------------------------------------------------------------------------
  // Fill Bit Field (bool)
  VISKORES_CONT static void Fill(viskores::cont::BitField& bits, bool value)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    const viskores::Id numBits = bits.GetNumberOfBits();
    if (numBits == 0)
    {
      return;
    }

    viskores::cont::Token token;

    auto portal = bits.PrepareForOutput(numBits, DeviceAdapterTag{}, token);

    using WordType = typename viskores::cont::BitField::template ExecutionTypes<
      DeviceAdapterTag>::WordTypePreferred;

    using Functor = FillBitFieldFunctor<decltype(portal), WordType>;
    Functor functor{ portal, value ? ~WordType{ 0 } : WordType{ 0 } };

    const viskores::Id numWords = portal.template GetNumberOfWords<WordType>();
    DerivedAlgorithm::Schedule(functor, numWords);
  }

  //--------------------------------------------------------------------------
  // Fill Bit Field (mask, resize)
  template <typename WordType>
  VISKORES_CONT static void Fill(viskores::cont::BitField& bits,
                                 WordType word,
                                 viskores::Id numBits)
  {
    VISKORES_STATIC_ASSERT_MSG(viskores::cont::BitField::IsValidWordType<WordType>{},
                               "Invalid word type.");

    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    if (numBits == 0)
    {
      bits.Allocate(0);
      return;
    }

    viskores::cont::Token token;

    auto portal = bits.PrepareForOutput(numBits, DeviceAdapterTag{}, token);

    // If less than 32 bits, repeat the word until we get a 32 bit pattern.
    // Using this for the pattern prevents races while writing small numbers
    // to adjacent memory locations.
    auto repWord = RepeatTo32BitsIfNeeded(word);
    using RepWordType = decltype(repWord);

    using Functor = FillBitFieldFunctor<decltype(portal), RepWordType>;
    Functor functor{ portal, repWord };

    const viskores::Id numWords = portal.template GetNumberOfWords<RepWordType>();
    DerivedAlgorithm::Schedule(functor, numWords);
  }

  //--------------------------------------------------------------------------
  // Fill Bit Field (mask)
  template <typename WordType>
  VISKORES_CONT static void Fill(viskores::cont::BitField& bits, WordType word)
  {
    VISKORES_STATIC_ASSERT_MSG(viskores::cont::BitField::IsValidWordType<WordType>{},
                               "Invalid word type.");
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    const viskores::Id numBits = bits.GetNumberOfBits();
    if (numBits == 0)
    {
      return;
    }

    viskores::cont::Token token;

    auto portal = bits.PrepareForOutput(numBits, DeviceAdapterTag{}, token);

    // If less than 32 bits, repeat the word until we get a 32 bit pattern.
    // Using this for the pattern prevents races while writing small numbers
    // to adjacent memory locations.
    auto repWord = RepeatTo32BitsIfNeeded(word);
    using RepWordType = decltype(repWord);

    using Functor = FillBitFieldFunctor<decltype(portal), RepWordType>;
    Functor functor{ portal, repWord };

    const viskores::Id numWords = portal.template GetNumberOfWords<RepWordType>();
    DerivedAlgorithm::Schedule(functor, numWords);
  }

  //--------------------------------------------------------------------------
  // Fill ArrayHandle
  template <typename T, typename S>
  VISKORES_CONT static void Fill(viskores::cont::ArrayHandle<T, S>& handle, const T& value)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    const viskores::Id numValues = handle.GetNumberOfValues();
    if (numValues == 0)
    {
      return;
    }

    viskores::cont::Token token;

    auto portal = handle.PrepareForOutput(numValues, DeviceAdapterTag{}, token);
    FillArrayHandleFunctor<decltype(portal)> functor{ portal, value };
    DerivedAlgorithm::Schedule(functor, numValues);
  }

  //--------------------------------------------------------------------------
  // Fill ArrayHandle (resize)
  template <typename T, typename S>
  VISKORES_CONT static void Fill(viskores::cont::ArrayHandle<T, S>& handle,
                                 const T& value,
                                 const viskores::Id numValues)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);
    if (numValues == 0)
    {
      handle.ReleaseResources();
      return;
    }

    viskores::cont::Token token;

    auto portal = handle.PrepareForOutput(numValues, DeviceAdapterTag{}, token);
    FillArrayHandleFunctor<decltype(portal)> functor{ portal, value };
    DerivedAlgorithm::Schedule(functor, numValues);
  }

  //--------------------------------------------------------------------------
  // Lower Bounds
  template <typename T, class CIn, class CVal, class COut>
  VISKORES_CONT static void LowerBounds(const viskores::cont::ArrayHandle<T, CIn>& input,
                                        const viskores::cont::ArrayHandle<T, CVal>& values,
                                        viskores::cont::ArrayHandle<viskores::Id, COut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::Id arraySize = values.GetNumberOfValues();

    viskores::cont::Token token;

    auto inputPortal = input.PrepareForInput(DeviceAdapterTag(), token);
    auto valuesPortal = values.PrepareForInput(DeviceAdapterTag(), token);
    auto outputPortal = output.PrepareForOutput(arraySize, DeviceAdapterTag(), token);

    LowerBoundsKernel<decltype(inputPortal), decltype(valuesPortal), decltype(outputPortal)> kernel(
      inputPortal, valuesPortal, outputPortal);

    DerivedAlgorithm::Schedule(kernel, arraySize);
  }

  template <typename T, class CIn, class CVal, class COut, class BinaryCompare>
  VISKORES_CONT static void LowerBounds(const viskores::cont::ArrayHandle<T, CIn>& input,
                                        const viskores::cont::ArrayHandle<T, CVal>& values,
                                        viskores::cont::ArrayHandle<viskores::Id, COut>& output,
                                        BinaryCompare binary_compare)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::Id arraySize = values.GetNumberOfValues();

    viskores::cont::Token token;

    auto inputPortal = input.PrepareForInput(DeviceAdapterTag(), token);
    auto valuesPortal = values.PrepareForInput(DeviceAdapterTag(), token);
    auto outputPortal = output.PrepareForOutput(arraySize, DeviceAdapterTag(), token);

    LowerBoundsComparisonKernel<decltype(inputPortal),
                                decltype(valuesPortal),
                                decltype(outputPortal),
                                BinaryCompare>
      kernel(inputPortal, valuesPortal, outputPortal, binary_compare);

    DerivedAlgorithm::Schedule(kernel, arraySize);
  }

  template <class CIn, class COut>
  VISKORES_CONT static void LowerBounds(
    const viskores::cont::ArrayHandle<viskores::Id, CIn>& input,
    viskores::cont::ArrayHandle<viskores::Id, COut>& values_output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    DeviceAdapterAlgorithmGeneral<DerivedAlgorithm, DeviceAdapterTag>::LowerBounds(
      input, values_output, values_output);
  }

  //--------------------------------------------------------------------------
  // Reduce
#ifndef VISKORES_CUDA
  // nvcc doesn't like the private class declaration so disable under CUDA
private:
#endif
  template <typename T, typename BinaryFunctor>
  class ReduceDecoratorImpl
  {
  public:
    VISKORES_CONT ReduceDecoratorImpl() = default;

    VISKORES_CONT
    ReduceDecoratorImpl(const T& initialValue, const BinaryFunctor& binaryFunctor)
      : InitialValue(initialValue)
      , ReduceOperator(binaryFunctor)
    {
    }

    template <typename Portal>
    VISKORES_CONT ReduceKernel<Portal, T, BinaryFunctor> CreateFunctor(const Portal& portal) const
    {
      return ReduceKernel<Portal, T, BinaryFunctor>(
        portal, this->InitialValue, this->ReduceOperator);
    }

  private:
    T InitialValue;
    BinaryFunctor ReduceOperator;
  };

public:
  template <typename T, typename U, class CIn>
  VISKORES_CONT static U Reduce(const viskores::cont::ArrayHandle<T, CIn>& input, U initialValue)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    return DerivedAlgorithm::Reduce(input, initialValue, viskores::Add());
  }

  template <typename T, typename U, class CIn, class BinaryFunctor>
  VISKORES_CONT static U Reduce(const viskores::cont::ArrayHandle<T, CIn>& input,
                                U initialValue,
                                BinaryFunctor binary_functor)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    //Crazy Idea:
    //We perform the reduction in two levels. The first level is performed by
    //an `ArrayHandleDecorator` which reduces 16 input values and maps them to
    //one value. The decorator array is then 1/16 the length of the input array,
    //and we can use inclusive scan as the second level to compute the final
    //result.
    viskores::Id length = (input.GetNumberOfValues() / 16);
    length += (input.GetNumberOfValues() % 16 == 0) ? 0 : 1;
    auto reduced = viskores::cont::make_ArrayHandleDecorator(
      length, ReduceDecoratorImpl<U, BinaryFunctor>(initialValue, binary_functor), input);

    viskores::cont::ArrayHandle<U, viskores::cont::StorageTagBasic> inclusiveScanStorage;
    const U scanResult =
      DerivedAlgorithm::ScanInclusive(reduced, inclusiveScanStorage, binary_functor);
    return scanResult;
  }

  //--------------------------------------------------------------------------
  // Reduce By Key
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

    using KeysOutputType = viskores::cont::ArrayHandle<U, KOut>;

    VISKORES_ASSERT(keys.GetNumberOfValues() == values.GetNumberOfValues());
    const viskores::Id numberOfKeys = keys.GetNumberOfValues();

    if (numberOfKeys <= 1)
    { //we only have a single key/value so that is our output
      DerivedAlgorithm::Copy(keys, keys_output);
      DerivedAlgorithm::Copy(values, values_output);
      return;
    }

    //we need to determine based on the keys what is the keystate for
    //each key. The states are start, middle, end of a series and the special
    //state start and end of a series
    viskores::cont::ArrayHandle<ReduceKeySeriesStates> keystate;

    {
      viskores::cont::Token token;
      auto inputPortal = keys.PrepareForInput(DeviceAdapterTag(), token);
      auto keyStatePortal = keystate.PrepareForOutput(numberOfKeys, DeviceAdapterTag(), token);
      ReduceStencilGeneration<decltype(inputPortal), decltype(keyStatePortal)> kernel(
        inputPortal, keyStatePortal);
      DerivedAlgorithm::Schedule(kernel, numberOfKeys);
    }

    //next step is we need to reduce the values for each key. This is done
    //by running an inclusive scan over the values array using the stencil.
    //
    // this inclusive scan will write out two values, the first being
    // the value summed currently, the second being 0 or 1, with 1 being used
    // when this is a value of a key we need to write ( END or START_AND_END)
    {
      viskores::cont::ArrayHandle<ReduceKeySeriesStates> stencil;
      viskores::cont::ArrayHandle<U> reducedValues;

      auto scanInput = viskores::cont::make_ArrayHandleZip(values, keystate);
      auto scanOutput = viskores::cont::make_ArrayHandleZip(reducedValues, stencil);

      DerivedAlgorithm::ScanInclusive(
        scanInput, scanOutput, ReduceByKeyAdd<BinaryFunctor>(binary_functor));

      //at this point we are done with keystate, so free the memory
      keystate.ReleaseResources();

      // all we need know is an efficient way of doing the write back to the
      // reduced global memory. this is done by using CopyIf with the
      // stencil and values we just created with the inclusive scan
      DerivedAlgorithm::CopyIf(reducedValues, stencil, values_output, ReduceByKeyUnaryStencilOp());

    } //release all temporary memory

    // Don't bother with the keys_output if it's an ArrayHandleDiscard -- there
    // will be a runtime exception in Unique() otherwise:
    if (!viskores::cont::IsArrayHandleDiscard<KeysOutputType>::value)
    {
      //find all the unique keys
      DerivedAlgorithm::Copy(keys, keys_output);
      DerivedAlgorithm::Unique(keys_output);
    }
  }

  //--------------------------------------------------------------------------
  // Scan Exclusive
  template <typename T, class CIn, class COut, class BinaryFunctor>
  VISKORES_CONT static T ScanExclusive(const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output,
                                       BinaryFunctor binaryFunctor,
                                       const T& initialValue)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::Id numValues = input.GetNumberOfValues();
    if (numValues <= 0)
    {
      output.ReleaseResources();
      return initialValue;
    }

    viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic> inclusiveScan;
    T result = DerivedAlgorithm::ScanInclusive(input, inclusiveScan, binaryFunctor);

    viskores::cont::Token token;

    auto inputPortal = inclusiveScan.PrepareForInput(DeviceAdapterTag(), token);
    auto outputPortal = output.PrepareForOutput(numValues, DeviceAdapterTag(), token);

    InclusiveToExclusiveKernel<decltype(inputPortal), decltype(outputPortal), BinaryFunctor>
      inclusiveToExclusive(inputPortal, outputPortal, binaryFunctor, initialValue);

    DerivedAlgorithm::Schedule(inclusiveToExclusive, numValues);

    return binaryFunctor(initialValue, result);
  }

  template <typename T, class CIn, class COut>
  VISKORES_CONT static T ScanExclusive(const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    return DerivedAlgorithm::ScanExclusive(
      input, output, viskores::Sum(), viskores::TypeTraits<T>::ZeroInitialization());
  }

  //--------------------------------------------------------------------------
  // Scan Exclusive Extend
  template <typename T, class CIn, class COut, class BinaryFunctor>
  VISKORES_CONT static void ScanExtended(const viskores::cont::ArrayHandle<T, CIn>& input,
                                         viskores::cont::ArrayHandle<T, COut>& output,
                                         BinaryFunctor binaryFunctor,
                                         const T& initialValue)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::Id numValues = input.GetNumberOfValues();
    if (numValues <= 0)
    {
      output.Allocate(1);
      output.WritePortal().Set(0, initialValue);
      return;
    }

    viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic> inclusiveScan;
    T result = DerivedAlgorithm::ScanInclusive(input, inclusiveScan, binaryFunctor);

    viskores::cont::Token token;

    auto inputPortal = inclusiveScan.PrepareForInput(DeviceAdapterTag(), token);
    auto outputPortal = output.PrepareForOutput(numValues + 1, DeviceAdapterTag(), token);

    InclusiveToExtendedKernel<decltype(inputPortal), decltype(outputPortal), BinaryFunctor>
      inclusiveToExtended(inputPortal,
                          outputPortal,
                          binaryFunctor,
                          initialValue,
                          binaryFunctor(initialValue, result));

    DerivedAlgorithm::Schedule(inclusiveToExtended, numValues + 1);
  }

  template <typename T, class CIn, class COut>
  VISKORES_CONT static void ScanExtended(const viskores::cont::ArrayHandle<T, CIn>& input,
                                         viskores::cont::ArrayHandle<T, COut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    DerivedAlgorithm::ScanExtended(
      input, output, viskores::Sum(), viskores::TypeTraits<T>::ZeroInitialization());
  }

  //--------------------------------------------------------------------------
  // Scan Exclusive By Key
  template <typename KeyT,
            typename ValueT,
            typename KIn,
            typename VIn,
            typename VOut,
            class BinaryFunctor>
  VISKORES_CONT static void ScanExclusiveByKey(
    const viskores::cont::ArrayHandle<KeyT, KIn>& keys,
    const viskores::cont::ArrayHandle<ValueT, VIn>& values,
    viskores::cont::ArrayHandle<ValueT, VOut>& output,
    const ValueT& initialValue,
    BinaryFunctor binaryFunctor)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    VISKORES_ASSERT(keys.GetNumberOfValues() == values.GetNumberOfValues());

    // 0. Special case for 0 and 1 element input
    viskores::Id numberOfKeys = keys.GetNumberOfValues();

    if (numberOfKeys == 0)
    {
      return;
    }
    else if (numberOfKeys == 1)
    {
      output.Allocate(1);
      output.WritePortal().Set(0, initialValue);
      return;
    }

    // 1. Create head flags
    //we need to determine based on the keys what is the keystate for
    //each key. The states are start, middle, end of a series and the special
    //state start and end of a series
    viskores::cont::ArrayHandle<ReduceKeySeriesStates> keystate;

    {
      viskores::cont::Token token;
      auto inputPortal = keys.PrepareForInput(DeviceAdapterTag(), token);
      auto keyStatePortal = keystate.PrepareForOutput(numberOfKeys, DeviceAdapterTag(), token);
      ReduceStencilGeneration<decltype(inputPortal), decltype(keyStatePortal)> kernel(
        inputPortal, keyStatePortal);
      DerivedAlgorithm::Schedule(kernel, numberOfKeys);
    }

    // 2. Shift input and initialize elements at head flags position to initValue
    viskores::cont::ArrayHandle<ValueT, viskores::cont::StorageTagBasic> temp;
    {
      viskores::cont::Token token;
      auto inputPortal = values.PrepareForInput(DeviceAdapterTag(), token);
      auto keyStatePortal = keystate.PrepareForInput(DeviceAdapterTag(), token);
      auto tempPortal = temp.PrepareForOutput(numberOfKeys, DeviceAdapterTag(), token);

      ShiftCopyAndInit<ValueT,
                       decltype(inputPortal),
                       decltype(keyStatePortal),
                       decltype(tempPortal)>
        kernel(inputPortal, keyStatePortal, tempPortal, initialValue);
      DerivedAlgorithm::Schedule(kernel, numberOfKeys);
    }
    // 3. Perform a ScanInclusiveByKey
    DerivedAlgorithm::ScanInclusiveByKey(keys, temp, output, binaryFunctor);
  }

  template <typename KeyT, typename ValueT, class KIn, typename VIn, typename VOut>
  VISKORES_CONT static void ScanExclusiveByKey(
    const viskores::cont::ArrayHandle<KeyT, KIn>& keys,
    const viskores::cont::ArrayHandle<ValueT, VIn>& values,
    viskores::cont::ArrayHandle<ValueT, VOut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    DerivedAlgorithm::ScanExclusiveByKey(
      keys, values, output, viskores::TypeTraits<ValueT>::ZeroInitialization(), viskores::Sum());
  }

  //--------------------------------------------------------------------------
  // Scan Inclusive
  template <typename T, class CIn, class COut>
  VISKORES_CONT static T ScanInclusive(const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    return DerivedAlgorithm::ScanInclusive(input, output, viskores::Add());
  }

private:
  template <typename T1, typename S1, typename T2, typename S2>
  VISKORES_CONT static bool ArrayHandlesAreSame(const viskores::cont::ArrayHandle<T1, S1>&,
                                                const viskores::cont::ArrayHandle<T2, S2>&)
  {
    return false;
  }

  template <typename T, typename S>
  VISKORES_CONT static bool ArrayHandlesAreSame(const viskores::cont::ArrayHandle<T, S>& a1,
                                                const viskores::cont::ArrayHandle<T, S>& a2)
  {
    return a1 == a2;
  }

public:
  template <typename T, class CIn, class COut, class BinaryFunctor>
  VISKORES_CONT static T ScanInclusive(const viskores::cont::ArrayHandle<T, CIn>& input,
                                       viskores::cont::ArrayHandle<T, COut>& output,
                                       BinaryFunctor binary_functor)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    if (!ArrayHandlesAreSame(input, output))
    {
      DerivedAlgorithm::Copy(input, output);
    }

    viskores::Id numValues = output.GetNumberOfValues();
    if (numValues < 1)
    {
      return viskores::TypeTraits<T>::ZeroInitialization();
    }

    {
      viskores::cont::Token token;

      auto portal = output.PrepareForInPlace(DeviceAdapterTag(), token);
      using ScanKernelType = ScanKernel<decltype(portal), BinaryFunctor>;


      viskores::Id stride;
      for (stride = 2; stride - 1 < numValues; stride *= 2)
      {
        ScanKernelType kernel(portal, binary_functor, stride, stride / 2 - 1);
        DerivedAlgorithm::Schedule(kernel, numValues / stride);
      }

      // Do reverse operation on odd indices. Start at stride we were just at.
      for (stride /= 2; stride > 1; stride /= 2)
      {
        ScanKernelType kernel(portal, binary_functor, stride, stride - 1);
        DerivedAlgorithm::Schedule(kernel, numValues / stride);
      }
    }

    return GetExecutionValue(output, numValues - 1);
  }

  template <typename KeyT, typename ValueT, class KIn, class VIn, class VOut>
  VISKORES_CONT static void ScanInclusiveByKey(
    const viskores::cont::ArrayHandle<KeyT, KIn>& keys,
    const viskores::cont::ArrayHandle<ValueT, VIn>& values,
    viskores::cont::ArrayHandle<ValueT, VOut>& values_output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    return DerivedAlgorithm::ScanInclusiveByKey(keys, values, values_output, viskores::Add());
  }

  template <typename KeyT, typename ValueT, class KIn, class VIn, class VOut, class BinaryFunctor>
  VISKORES_CONT static void ScanInclusiveByKey(
    const viskores::cont::ArrayHandle<KeyT, KIn>& keys,
    const viskores::cont::ArrayHandle<ValueT, VIn>& values,
    viskores::cont::ArrayHandle<ValueT, VOut>& values_output,
    BinaryFunctor binary_functor)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    VISKORES_ASSERT(keys.GetNumberOfValues() == values.GetNumberOfValues());
    const viskores::Id numberOfKeys = keys.GetNumberOfValues();

    if (numberOfKeys <= 1)
    { //we only have a single key/value so that is our output
      DerivedAlgorithm::Copy(values, values_output);
      return;
    }

    //we need to determine based on the keys what is the keystate for
    //each key. The states are start, middle, end of a series and the special
    //state start and end of a series
    viskores::cont::ArrayHandle<ReduceKeySeriesStates> keystate;

    {
      viskores::cont::Token token;
      auto inputPortal = keys.PrepareForInput(DeviceAdapterTag(), token);
      auto keyStatePortal = keystate.PrepareForOutput(numberOfKeys, DeviceAdapterTag(), token);
      ReduceStencilGeneration<decltype(inputPortal), decltype(keyStatePortal)> kernel(
        inputPortal, keyStatePortal);
      DerivedAlgorithm::Schedule(kernel, numberOfKeys);
    }

    //next step is we need to reduce the values for each key. This is done
    //by running an inclusive scan over the values array using the stencil.
    //
    // this inclusive scan will write out two values, the first being
    // the value summed currently, the second being 0 or 1, with 1 being used
    // when this is a value of a key we need to write ( END or START_AND_END)
    {
      viskores::cont::ArrayHandle<ValueT> reducedValues;
      viskores::cont::ArrayHandle<ReduceKeySeriesStates> stencil;
      auto scanInput = viskores::cont::make_ArrayHandleZip(values, keystate);
      auto scanOutput = viskores::cont::make_ArrayHandleZip(reducedValues, stencil);

      DerivedAlgorithm::ScanInclusive(
        scanInput, scanOutput, ReduceByKeyAdd<BinaryFunctor>(binary_functor));
      //at this point we are done with keystate, so free the memory
      keystate.ReleaseResources();
      DerivedAlgorithm::Copy(reducedValues, values_output);
    }
  }

  //--------------------------------------------------------------------------
  // Sort
  template <typename T, class Storage, class BinaryCompare>
  VISKORES_CONT static void Sort(viskores::cont::ArrayHandle<T, Storage>& values,
                                 BinaryCompare binary_compare)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::Id numValues = values.GetNumberOfValues();
    if (numValues < 2)
    {
      return;
    }
    viskores::Id numThreads = 1;
    while (numThreads < numValues)
    {
      numThreads *= 2;
    }
    numThreads /= 2;

    viskores::cont::Token token;

    auto portal = values.PrepareForInPlace(DeviceAdapterTag(), token);
    using MergeKernel = BitonicSortMergeKernel<decltype(portal), BinaryCompare>;
    using CrossoverKernel = BitonicSortCrossoverKernel<decltype(portal), BinaryCompare>;

    for (viskores::Id crossoverSize = 1; crossoverSize < numValues; crossoverSize *= 2)
    {
      DerivedAlgorithm::Schedule(CrossoverKernel(portal, binary_compare, crossoverSize),
                                 numThreads);
      for (viskores::Id mergeSize = crossoverSize / 2; mergeSize > 0; mergeSize /= 2)
      {
        DerivedAlgorithm::Schedule(MergeKernel(portal, binary_compare, mergeSize), numThreads);
      }
    }
  }

  template <typename T, class Storage>
  VISKORES_CONT static void Sort(viskores::cont::ArrayHandle<T, Storage>& values)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    DerivedAlgorithm::Sort(values, DefaultCompareFunctor());
  }

  //--------------------------------------------------------------------------
  // Sort by Key
  template <typename T, typename U, class StorageT, class StorageU>
  VISKORES_CONT static void SortByKey(viskores::cont::ArrayHandle<T, StorageT>& keys,
                                      viskores::cont::ArrayHandle<U, StorageU>& values)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    //combine the keys and values into a ZipArrayHandle
    //we than need to specify a custom compare function wrapper
    //that only checks for key side of the pair, using a custom compare functor.
    auto zipHandle = viskores::cont::make_ArrayHandleZip(keys, values);
    DerivedAlgorithm::Sort(zipHandle, internal::KeyCompare<T, U>());
  }

  template <typename T, typename U, class StorageT, class StorageU, class BinaryCompare>
  VISKORES_CONT static void SortByKey(viskores::cont::ArrayHandle<T, StorageT>& keys,
                                      viskores::cont::ArrayHandle<U, StorageU>& values,
                                      BinaryCompare binary_compare)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    //combine the keys and values into a ZipArrayHandle
    //we than need to specify a custom compare function wrapper
    //that only checks for key side of the pair, using the custom compare
    //functor that the user passed in
    auto zipHandle = viskores::cont::make_ArrayHandleZip(keys, values);
    DerivedAlgorithm::Sort(zipHandle, internal::KeyCompare<T, U, BinaryCompare>(binary_compare));
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
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::Id numValues = viskores::Min(input1.GetNumberOfValues(), input2.GetNumberOfValues());
    if (numValues <= 0)
    {
      return;
    }

    viskores::cont::Token token;

    auto input1Portal = input1.PrepareForInput(DeviceAdapterTag(), token);
    auto input2Portal = input2.PrepareForInput(DeviceAdapterTag(), token);
    auto outputPortal = output.PrepareForOutput(numValues, DeviceAdapterTag(), token);

    BinaryTransformKernel<decltype(input1Portal),
                          decltype(input2Portal),
                          decltype(outputPortal),
                          BinaryFunctor>
      binaryKernel(input1Portal, input2Portal, outputPortal, binaryFunctor);
    DerivedAlgorithm::Schedule(binaryKernel, numValues);
  }

  //};
  //--------------------------------------------------------------------------
  // Unique
  template <typename T, class Storage>
  VISKORES_CONT static void Unique(viskores::cont::ArrayHandle<T, Storage>& values)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    DerivedAlgorithm::Unique(values, viskores::Equal());
  }

  template <typename T, class Storage, class BinaryCompare>
  VISKORES_CONT static void Unique(viskores::cont::ArrayHandle<T, Storage>& values,
                                   BinaryCompare binary_compare)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagBasic> stencilArray;
    viskores::Id inputSize = values.GetNumberOfValues();

    using WrappedBOpType = internal::WrappedBinaryOperator<bool, BinaryCompare>;
    WrappedBOpType wrappedCompare(binary_compare);

    {
      viskores::cont::Token token;
      auto valuesPortal = values.PrepareForInput(DeviceAdapterTag(), token);
      auto stencilPortal = stencilArray.PrepareForOutput(inputSize, DeviceAdapterTag(), token);
      ClassifyUniqueComparisonKernel<decltype(valuesPortal),
                                     decltype(stencilPortal),
                                     WrappedBOpType>
        classifyKernel(valuesPortal, stencilPortal, wrappedCompare);

      DerivedAlgorithm::Schedule(classifyKernel, inputSize);
    }

    viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic> outputArray;

    DerivedAlgorithm::CopyIf(values, stencilArray, outputArray);

    values.Allocate(outputArray.GetNumberOfValues());
    DerivedAlgorithm::Copy(outputArray, values);
  }

  //--------------------------------------------------------------------------
  // Upper bounds
  template <typename T, class CIn, class CVal, class COut>
  VISKORES_CONT static void UpperBounds(const viskores::cont::ArrayHandle<T, CIn>& input,
                                        const viskores::cont::ArrayHandle<T, CVal>& values,
                                        viskores::cont::ArrayHandle<viskores::Id, COut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::Id arraySize = values.GetNumberOfValues();

    viskores::cont::Token token;

    auto inputPortal = input.PrepareForInput(DeviceAdapterTag(), token);
    auto valuesPortal = values.PrepareForInput(DeviceAdapterTag(), token);
    auto outputPortal = output.PrepareForOutput(arraySize, DeviceAdapterTag(), token);

    UpperBoundsKernel<decltype(inputPortal), decltype(valuesPortal), decltype(outputPortal)> kernel(
      inputPortal, valuesPortal, outputPortal);
    DerivedAlgorithm::Schedule(kernel, arraySize);
  }

  template <typename T, class CIn, class CVal, class COut, class BinaryCompare>
  VISKORES_CONT static void UpperBounds(const viskores::cont::ArrayHandle<T, CIn>& input,
                                        const viskores::cont::ArrayHandle<T, CVal>& values,
                                        viskores::cont::ArrayHandle<viskores::Id, COut>& output,
                                        BinaryCompare binary_compare)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::Id arraySize = values.GetNumberOfValues();

    viskores::cont::Token token;

    auto inputPortal = input.PrepareForInput(DeviceAdapterTag(), token);
    auto valuesPortal = values.PrepareForInput(DeviceAdapterTag(), token);
    auto outputPortal = output.PrepareForOutput(arraySize, DeviceAdapterTag(), token);

    UpperBoundsKernelComparisonKernel<decltype(inputPortal),
                                      decltype(valuesPortal),
                                      decltype(outputPortal),
                                      BinaryCompare>
      kernel(inputPortal, valuesPortal, outputPortal, binary_compare);

    DerivedAlgorithm::Schedule(kernel, arraySize);
  }

  template <class CIn, class COut>
  VISKORES_CONT static void UpperBounds(
    const viskores::cont::ArrayHandle<viskores::Id, CIn>& input,
    viskores::cont::ArrayHandle<viskores::Id, COut>& values_output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    DeviceAdapterAlgorithmGeneral<DerivedAlgorithm, DeviceAdapterTag>::UpperBounds(
      input, values_output, values_output);
  }
};

} // namespace internal

/// \brief Class providing a device-specific support for selecting the optimal
/// Task type for a given worklet.
///
/// When worklets are launched inside the execution environment we need to
/// ask the device adapter what is the preferred execution style, be it
/// a tiled iteration pattern, or strided. This class
///
/// By default if not specialized for a device adapter the default
/// is to use viskores::exec::internal::TaskSingular
///
template <typename DeviceTag>
class DeviceTaskTypes
{
public:
  template <typename WorkletType, typename InvocationType>
  static viskores::exec::internal::TaskSingular<WorkletType, InvocationType> MakeTask(
    WorkletType& worklet,
    InvocationType& invocation,
    viskores::Id,
    viskores::Id globalIndexOffset = 0)
  {
    using Task = viskores::exec::internal::TaskSingular<WorkletType, InvocationType>;
    return Task(worklet, invocation, globalIndexOffset);
  }

  template <typename WorkletType, typename InvocationType>
  static viskores::exec::internal::TaskSingular<WorkletType, InvocationType> MakeTask(
    WorkletType& worklet,
    InvocationType& invocation,
    viskores::Id3,
    viskores::Id globalIndexOffset = 0)
  {
    using Task = viskores::exec::internal::TaskSingular<WorkletType, InvocationType>;
    return Task(worklet, invocation, globalIndexOffset);
  }
};
}
} // namespace viskores::cont

#endif //viskores_cont_internal_DeviceAdapterAlgorithmGeneral_h
