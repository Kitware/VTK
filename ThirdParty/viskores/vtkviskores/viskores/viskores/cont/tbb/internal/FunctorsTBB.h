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
#ifndef viskores_cont_tbb_internal_FunctorsTBB_h
#define viskores_cont_tbb_internal_FunctorsTBB_h

#include <viskores/TypeTraits.h>
#include <viskores/Types.h>
#include <viskores/cont/ArrayPortalToIterators.h>
#include <viskores/cont/Error.h>
#include <viskores/cont/internal/FunctorsGeneral.h>
#include <viskores/exec/internal/ErrorMessageBuffer.h>

#include <algorithm>
#include <iterator>
#include <sstream>
#include <type_traits>

VISKORES_THIRDPARTY_PRE_INCLUDE

#if defined(VISKORES_MSVC)

// TBB's header include a #pragma comment(lib,"tbb.lib") line to make all
// consuming libraries link to tbb, this is bad behavior in a header
// based project
#pragma push_macro("__TBB_NO_IMPLICITLINKAGE")
#define __TBB_NO_IMPLICIT_LINKAGE 1

#endif // defined(VISKORES_MSVC)

// TBB includes windows.h, so instead we want to include windows.h with the
// correct settings so that we don't clobber any existing function
#include <viskores/internal/Windows.h>


#include <numeric>
#include <tbb/blocked_range.h>
#include <tbb/blocked_range3d.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <tbb/parallel_scan.h>
#include <tbb/parallel_sort.h>
#include <tbb/partitioner.h>
#include <tbb/tick_count.h>

#if defined(VISKORES_MSVC)
#pragma pop_macro("__TBB_NO_IMPLICITLINKAGE")
#endif

VISKORES_THIRDPARTY_POST_INCLUDE

namespace viskores
{
namespace cont
{
namespace tbb
{

namespace internal
{
template <typename ResultType, typename Function>
using WrappedBinaryOperator = viskores::cont::internal::WrappedBinaryOperator<ResultType, Function>;
}

// The "grain size" of scheduling with TBB.  Not a lot of thought has gone
// into picking this size.
static constexpr viskores::Id TBB_GRAIN_SIZE = 1024;

template <typename InputPortalType, typename OutputPortalType>
struct CopyBody
{
  InputPortalType InputPortal;
  OutputPortalType OutputPortal;
  viskores::Id InputOffset;
  viskores::Id OutputOffset;

  CopyBody(const InputPortalType& inPortal,
           const OutputPortalType& outPortal,
           viskores::Id inOffset,
           viskores::Id outOffset)
    : InputPortal(inPortal)
    , OutputPortal(outPortal)
    , InputOffset(inOffset)
    , OutputOffset(outOffset)
  {
  }

  // MSVC likes complain about narrowing type conversions in std::copy and
  // provides no reasonable way to disable the warning. As a work-around, this
  // template calls std::copy if and only if the types match, otherwise falls
  // back to a iterative casting approach. Since std::copy can only really
  // optimize same-type copies, this shouldn't affect performance.
  template <typename InIter, typename OutIter>
  void DoCopy(InIter src, InIter srcEnd, OutIter dst, std::false_type) const
  {
    using InputType = typename InputPortalType::ValueType;
    using OutputType = typename OutputPortalType::ValueType;
    while (src != srcEnd)
    {
      // The conversion to InputType and then OutputType looks weird, but it is necessary.
      // *src actually returns an ArrayPortalValueReference, which can automatically convert
      // itself to InputType but not necessarily OutputType. Thus, we first convert to
      // InputType, and then allow the conversion to OutputType.
      *dst = static_cast<OutputType>(static_cast<InputType>(*src));
      ++src;
      ++dst;
    }
  }


  template <typename InIter, typename OutIter>
  void DoCopy(InIter src, InIter srcEnd, OutIter dst, std::true_type) const
  {
    std::copy(src, srcEnd, dst);
  }



  void operator()(const ::tbb::blocked_range<viskores::Id>& range) const
  {
    if (range.empty())
    {
      return;
    }

    auto inIter = viskores::cont::ArrayPortalToIteratorBegin(this->InputPortal);
    auto outIter = viskores::cont::ArrayPortalToIteratorBegin(this->OutputPortal);

    using InputType = typename InputPortalType::ValueType;
    using OutputType = typename OutputPortalType::ValueType;

    this->DoCopy(inIter + this->InputOffset + range.begin(),
                 inIter + this->InputOffset + range.end(),
                 outIter + this->OutputOffset + range.begin(),
                 std::is_same<InputType, OutputType>());
  }
};

template <typename InputPortalType, typename OutputPortalType>
void CopyPortals(const InputPortalType& inPortal,
                 const OutputPortalType& outPortal,
                 viskores::Id inOffset,
                 viskores::Id outOffset,
                 viskores::Id numValues)
{
  using Kernel = CopyBody<InputPortalType, OutputPortalType>;
  Kernel kernel(inPortal, outPortal, inOffset, outOffset);
  ::tbb::blocked_range<viskores::Id> range(0, numValues, TBB_GRAIN_SIZE);
  ::tbb::parallel_for(range, kernel);
}

template <typename InputPortalType,
          typename StencilPortalType,
          typename OutputPortalType,
          typename UnaryPredicateType>
struct CopyIfBody
{
  using ValueType = typename InputPortalType::ValueType;
  using StencilType = typename StencilPortalType::ValueType;

  struct Range
  {
    viskores::Id InputBegin;
    viskores::Id InputEnd;
    viskores::Id OutputBegin;
    viskores::Id OutputEnd;


    Range()
      : InputBegin(-1)
      , InputEnd(-1)
      , OutputBegin(-1)
      , OutputEnd(-1)
    {
    }


    Range(viskores::Id inputBegin,
          viskores::Id inputEnd,
          viskores::Id outputBegin,
          viskores::Id outputEnd)
      : InputBegin(inputBegin)
      , InputEnd(inputEnd)
      , OutputBegin(outputBegin)
      , OutputEnd(outputEnd)
    {
      this->AssertSane();
    }


    void AssertSane() const
    {
      VISKORES_ASSERT("Input begin precedes end" && this->InputBegin <= this->InputEnd);
      VISKORES_ASSERT("Output begin precedes end" && this->OutputBegin <= this->OutputEnd);
      VISKORES_ASSERT("Output not past input" && this->OutputBegin <= this->InputBegin &&
                      this->OutputEnd <= this->InputEnd);
      VISKORES_ASSERT("Output smaller than input" &&
                      (this->OutputEnd - this->OutputBegin) <= (this->InputEnd - this->InputBegin));
    }


    bool IsNext(const Range& next) const { return this->InputEnd == next.InputBegin; }
  };

  InputPortalType InputPortal;
  StencilPortalType StencilPortal;
  OutputPortalType OutputPortal;
  UnaryPredicateType UnaryPredicate;
  Range Ranges;

  VISKORES_CONT
  CopyIfBody(const InputPortalType& inputPortal,
             const StencilPortalType& stencilPortal,
             const OutputPortalType& outputPortal,
             UnaryPredicateType unaryPredicate)
    : InputPortal(inputPortal)
    , StencilPortal(stencilPortal)
    , OutputPortal(outputPortal)
    , UnaryPredicate(unaryPredicate)
  {
  }


  CopyIfBody(const CopyIfBody& body, ::tbb::split)
    : InputPortal(body.InputPortal)
    , StencilPortal(body.StencilPortal)
    , OutputPortal(body.OutputPortal)
    , UnaryPredicate(body.UnaryPredicate)
  {
  }



  void operator()(const ::tbb::blocked_range<viskores::Id>& range)
  {
    if (range.empty())
    {
      return;
    }

    bool firstRun = this->Ranges.OutputBegin < 0; // First use of this body object
    if (firstRun)
    {
      this->Ranges.InputBegin = range.begin();
    }
    else
    {
      // Must be a continuation of the previous input range:
      VISKORES_ASSERT(this->Ranges.InputEnd == range.begin());
    }
    this->Ranges.InputEnd = range.end();
    this->Ranges.AssertSane();

    using InputIteratorsType = viskores::cont::ArrayPortalToIterators<InputPortalType>;
    using StencilIteratorsType = viskores::cont::ArrayPortalToIterators<StencilPortalType>;
    using OutputIteratorsType = viskores::cont::ArrayPortalToIterators<OutputPortalType>;

    InputIteratorsType inputIters(this->InputPortal);
    StencilIteratorsType stencilIters(this->StencilPortal);
    OutputIteratorsType outputIters(this->OutputPortal);

    using InputIteratorType = typename InputIteratorsType::IteratorType;
    using StencilIteratorType = typename StencilIteratorsType::IteratorType;
    using OutputIteratorType = typename OutputIteratorsType::IteratorType;

    InputIteratorType inIter = inputIters.GetBegin();
    StencilIteratorType stencilIter = stencilIters.GetBegin();
    OutputIteratorType outIter = outputIters.GetBegin();

    viskores::Id readPos = range.begin();
    const viskores::Id readEnd = range.end();

    // Determine output index. If we're reusing the body, pick up where the
    // last block left off. If not, use the input range.
    viskores::Id writePos;
    if (firstRun)
    {
      this->Ranges.OutputBegin = range.begin();
      this->Ranges.OutputEnd = range.begin();
      writePos = range.begin();
    }
    else
    {
      writePos = this->Ranges.OutputEnd;
    }
    this->Ranges.AssertSane();

    // We're either writing at the end of a previous block, or at the input
    // location. Either way, the write position will never be greater than
    // the read position.
    VISKORES_ASSERT(writePos <= readPos);

    UnaryPredicateType predicate(this->UnaryPredicate);
    for (; readPos < readEnd; ++readPos)
    {
      if (predicate(stencilIter[readPos]))
      {
        outIter[writePos] = inIter[readPos];
        ++writePos;
      }
    }

    this->Ranges.OutputEnd = writePos;
  }



  void join(const CopyIfBody& rhs)
  {
    using OutputIteratorsType = viskores::cont::ArrayPortalToIterators<OutputPortalType>;
    using OutputIteratorType = typename OutputIteratorsType::IteratorType;

    OutputIteratorsType outputIters(this->OutputPortal);
    OutputIteratorType outIter = outputIters.GetBegin();

    this->Ranges.AssertSane();
    rhs.Ranges.AssertSane();
    // Ensure that we're joining two consecutive subsets of the input:
    VISKORES_ASSERT(this->Ranges.IsNext(rhs.Ranges));

    viskores::Id srcBegin = rhs.Ranges.OutputBegin;
    const viskores::Id srcEnd = rhs.Ranges.OutputEnd;
    const viskores::Id dstBegin = this->Ranges.OutputEnd;

    // move data:
    if (srcBegin != dstBegin && srcBegin != srcEnd)
    {
      // Sanity check:
      VISKORES_ASSERT(srcBegin < srcEnd);
      std::copy(outIter + srcBegin, outIter + srcEnd, outIter + dstBegin);
    }

    this->Ranges.InputEnd = rhs.Ranges.InputEnd;
    this->Ranges.OutputEnd += srcEnd - srcBegin;
    this->Ranges.AssertSane();
  }
};


template <typename InputPortalType,
          typename StencilPortalType,
          typename OutputPortalType,
          typename UnaryPredicateType>
VISKORES_CONT viskores::Id CopyIfPortals(InputPortalType inputPortal,
                                         StencilPortalType stencilPortal,
                                         OutputPortalType outputPortal,
                                         UnaryPredicateType unaryPredicate)
{
  const viskores::Id inputLength = inputPortal.GetNumberOfValues();
  VISKORES_ASSERT(inputLength == stencilPortal.GetNumberOfValues());

  if (inputLength == 0)
  {
    return 0;
  }

  CopyIfBody<InputPortalType, StencilPortalType, OutputPortalType, UnaryPredicateType> body(
    inputPortal, stencilPortal, outputPortal, unaryPredicate);
  ::tbb::blocked_range<viskores::Id> range(0, inputLength, TBB_GRAIN_SIZE);

  ::tbb::parallel_reduce(range, body);

  body.Ranges.AssertSane();
  VISKORES_ASSERT(body.Ranges.InputBegin == 0 && body.Ranges.InputEnd == inputLength &&
                  body.Ranges.OutputBegin == 0 && body.Ranges.OutputEnd <= inputLength);

  return body.Ranges.OutputEnd;
}

template <class InputPortalType, class T, class BinaryOperationType>
struct ReduceBody
{
  T Sum;
  T InitialValue;
  bool FirstCall;
  InputPortalType InputPortal;
  BinaryOperationType BinaryOperation;

  VISKORES_CONT
  ReduceBody(const InputPortalType& inputPortal,
             T initialValue,
             BinaryOperationType binaryOperation)
    : Sum(viskores::TypeTraits<T>::ZeroInitialization())
    , InitialValue(initialValue)
    , FirstCall(true)
    , InputPortal(inputPortal)
    , BinaryOperation(binaryOperation)
  {
  }


  ReduceBody(const ReduceBody& body, ::tbb::split)
    : Sum(viskores::TypeTraits<T>::ZeroInitialization())
    , InitialValue(body.InitialValue)
    , FirstCall(true)
    , InputPortal(body.InputPortal)
    , BinaryOperation(body.BinaryOperation)
  {
  }



  void operator()(const ::tbb::blocked_range<viskores::Id>& range)
  {
    using InputIteratorsType = viskores::cont::ArrayPortalToIterators<InputPortalType>;
    InputIteratorsType inputIterators(this->InputPortal);

    //use temp, and iterators instead of member variable to reduce false sharing
    typename InputIteratorsType::IteratorType inIter =
      inputIterators.GetBegin() + static_cast<std::ptrdiff_t>(range.begin());

    T temp = this->BinaryOperation(*inIter, *(inIter + 1));
    ++inIter;
    ++inIter;
    for (viskores::Id index = range.begin() + 2; index != range.end(); ++index, ++inIter)
    {
      temp = this->BinaryOperation(temp, *inIter);
    }

    //determine if we also have to add the initial value to temp
    if (range.begin() == 0)
    {
      temp = this->BinaryOperation(temp, this->InitialValue);
    }

    //Now we can save temp back to sum, taking into account if
    //this task has been called before, and the sum value needs
    //to also be reduced.
    if (this->FirstCall)
    {
      this->Sum = temp;
    }
    else
    {
      this->Sum = this->BinaryOperation(this->Sum, temp);
    }

    this->FirstCall = false;
  }



  void join(const ReduceBody& left)
  {
    // std::cout << "join" << std::endl;
    this->Sum = this->BinaryOperation(left.Sum, this->Sum);
  }
};


template <class InputPortalType, typename T, class BinaryOperationType>
VISKORES_CONT static auto ReducePortals(InputPortalType inputPortal,
                                        T initialValue,
                                        BinaryOperationType binaryOperation)
  -> decltype(binaryOperation(initialValue, inputPortal.Get(0)))
{
  using ResultType = decltype(binaryOperation(initialValue, inputPortal.Get(0)));
  using WrappedBinaryOp = internal::WrappedBinaryOperator<ResultType, BinaryOperationType>;

  WrappedBinaryOp wrappedBinaryOp(binaryOperation);
  ReduceBody<InputPortalType, ResultType, WrappedBinaryOp> body(
    inputPortal, initialValue, wrappedBinaryOp);
  viskores::Id arrayLength = inputPortal.GetNumberOfValues();

  if (arrayLength > 1)
  {
    ::tbb::blocked_range<viskores::Id> range(0, arrayLength, TBB_GRAIN_SIZE);
    ::tbb::parallel_reduce(range, body);
    return body.Sum;
  }
  else if (arrayLength == 1)
  {
    //ReduceBody does not work with an array of size 1.
    return binaryOperation(initialValue, inputPortal.Get(0));
  }
  else // arrayLength == 0
  {
    // ReduceBody does not work with an array of size 0.
    return static_cast<ResultType>(initialValue);
  }
}

// Define this to print out timing information from the reduction and join
// operations in the tbb ReduceByKey algorithm:
//#define VISKORES_DEBUG_TBB_RBK

template <typename KeysInPortalType,
          typename ValuesInPortalType,
          typename KeysOutPortalType,
          typename ValuesOutPortalType,
          class BinaryOperationType>
struct ReduceByKeyBody
{
  using KeyType = typename KeysInPortalType::ValueType;
  using ValueType = typename ValuesInPortalType::ValueType;

  struct Range
  {
    viskores::Id InputBegin;
    viskores::Id InputEnd;
    viskores::Id OutputBegin;
    viskores::Id OutputEnd;


    Range()
      : InputBegin(-1)
      , InputEnd(-1)
      , OutputBegin(-1)
      , OutputEnd(-1)
    {
    }


    Range(viskores::Id inputBegin,
          viskores::Id inputEnd,
          viskores::Id outputBegin,
          viskores::Id outputEnd)
      : InputBegin(inputBegin)
      , InputEnd(inputEnd)
      , OutputBegin(outputBegin)
      , OutputEnd(outputEnd)
    {
      this->AssertSane();
    }


    void AssertSane() const
    {
      VISKORES_ASSERT("Input begin precedes end" && this->InputBegin <= this->InputEnd);
      VISKORES_ASSERT("Output begin precedes end" && this->OutputBegin <= this->OutputEnd);
      VISKORES_ASSERT("Output not past input" && this->OutputBegin <= this->InputBegin &&
                      this->OutputEnd <= this->InputEnd);
      VISKORES_ASSERT("Output smaller than input" &&
                      (this->OutputEnd - this->OutputBegin) <= (this->InputEnd - this->InputBegin));
    }


    bool IsNext(const Range& next) const { return this->InputEnd == next.InputBegin; }
  };

  KeysInPortalType KeysInPortal;
  ValuesInPortalType ValuesInPortal;
  KeysOutPortalType KeysOutPortal;
  ValuesOutPortalType ValuesOutPortal;
  BinaryOperationType BinaryOperation;
  Range Ranges;
#ifdef VISKORES_DEBUG_TBB_RBK
  double ReduceTime;
  double JoinTime;
#endif

  VISKORES_CONT
  ReduceByKeyBody(const KeysInPortalType& keysInPortal,
                  const ValuesInPortalType& valuesInPortal,
                  const KeysOutPortalType& keysOutPortal,
                  const ValuesOutPortalType& valuesOutPortal,
                  BinaryOperationType binaryOperation)
    : KeysInPortal(keysInPortal)
    , ValuesInPortal(valuesInPortal)
    , KeysOutPortal(keysOutPortal)
    , ValuesOutPortal(valuesOutPortal)
    , BinaryOperation(binaryOperation)
#ifdef VISKORES_DEBUG_TBB_RBK
    , ReduceTime(0)
    , JoinTime(0)
#endif
  {
  }


  ReduceByKeyBody(const ReduceByKeyBody& body, ::tbb::split)
    : KeysInPortal(body.KeysInPortal)
    , ValuesInPortal(body.ValuesInPortal)
    , KeysOutPortal(body.KeysOutPortal)
    , ValuesOutPortal(body.ValuesOutPortal)
    , BinaryOperation(body.BinaryOperation)
#ifdef VISKORES_DEBUG_TBB_RBK
    , ReduceTime(0)
    , JoinTime(0)
#endif
  {
  }



  void operator()(const ::tbb::blocked_range<viskores::Id>& range)
  {
#ifdef VISKORES_DEBUG_TBB_RBK
    ::tbb::tick_count startTime = ::tbb::tick_count::now();
#endif // VISKORES_DEBUG_TBB_RBK
    if (range.empty())
    {
      return;
    }

    bool firstRun = this->Ranges.OutputBegin < 0; // First use of this body object
    if (firstRun)
    {
      this->Ranges.InputBegin = range.begin();
    }
    else
    {
      // Must be a continuation of the previous input range:
      VISKORES_ASSERT(this->Ranges.InputEnd == range.begin());
    }
    this->Ranges.InputEnd = range.end();
    this->Ranges.AssertSane();

    using KeysInIteratorsType = viskores::cont::ArrayPortalToIterators<KeysInPortalType>;
    using ValuesInIteratorsType = viskores::cont::ArrayPortalToIterators<ValuesInPortalType>;
    using KeysOutIteratorsType = viskores::cont::ArrayPortalToIterators<KeysOutPortalType>;
    using ValuesOutIteratorsType = viskores::cont::ArrayPortalToIterators<ValuesOutPortalType>;

    KeysInIteratorsType keysInIters(this->KeysInPortal);
    ValuesInIteratorsType valuesInIters(this->ValuesInPortal);
    KeysOutIteratorsType keysOutIters(this->KeysOutPortal);
    ValuesOutIteratorsType valuesOutIters(this->ValuesOutPortal);

    using KeysInIteratorType = typename KeysInIteratorsType::IteratorType;
    using ValuesInIteratorType = typename ValuesInIteratorsType::IteratorType;
    using KeysOutIteratorType = typename KeysOutIteratorsType::IteratorType;
    using ValuesOutIteratorType = typename ValuesOutIteratorsType::IteratorType;

    KeysInIteratorType keysIn = keysInIters.GetBegin();
    ValuesInIteratorType valuesIn = valuesInIters.GetBegin();
    KeysOutIteratorType keysOut = keysOutIters.GetBegin();
    ValuesOutIteratorType valuesOut = valuesOutIters.GetBegin();

    viskores::Id readPos = range.begin();
    const viskores::Id readEnd = range.end();

    // Determine output index. If we're reusing the body, pick up where the
    // last block left off. If not, use the input range.
    viskores::Id writePos;
    if (firstRun)
    {
      this->Ranges.OutputBegin = range.begin();
      this->Ranges.OutputEnd = range.begin();
      writePos = range.begin();
    }
    else
    {
      writePos = this->Ranges.OutputEnd;
    }
    this->Ranges.AssertSane();

    // We're either writing at the end of a previous block, or at the input
    // location. Either way, the write position will never be greater than
    // the read position.
    VISKORES_ASSERT(writePos <= readPos);

    // Initialize reduction variables:
    BinaryOperationType functor(this->BinaryOperation);
    KeyType currentKey = keysIn[readPos];
    ValueType currentValue = valuesIn[readPos];
    ++readPos;

    // If the start of the current range continues a previous key block,
    // initialize with the previous result and decrement the write index.
    VISKORES_ASSERT(firstRun || writePos > 0);
    if (!firstRun && keysOut[writePos - 1] == currentKey)
    {
      // Ensure that we'll overwrite the continued key values:
      --writePos;

      // Update our accumulator with the partial value:
      currentValue = functor(valuesOut[writePos], currentValue);
    }

    // Special case: single value in range
    if (readPos >= readEnd)
    {
      keysOut[writePos] = currentKey;
      valuesOut[writePos] = currentValue;
      ++writePos;

      this->Ranges.OutputEnd = writePos;
      return;
    }

    for (;;)
    {
      while (readPos < readEnd && currentKey == keysIn[readPos])
      {
        currentValue = functor(currentValue, valuesIn[readPos]);
        ++readPos;
      }

      VISKORES_ASSERT(writePos <= readPos);
      keysOut[writePos] = currentKey;
      valuesOut[writePos] = currentValue;
      ++writePos;

      if (readPos < readEnd)
      {
        currentKey = keysIn[readPos];
        currentValue = valuesIn[readPos];
        ++readPos;
        continue;
      }

      break;
    }

    this->Ranges.OutputEnd = writePos;

#ifdef VISKORES_DEBUG_TBB_RBK
    ::tbb::tick_count endTime = ::tbb::tick_count::now();
    double time = (endTime - startTime).seconds();
    this->ReduceTime += time;
    std::ostringstream out;
    out << "Reduced " << range.size() << " key/value pairs in " << time << "s. "
        << "InRange: " << this->Ranges.InputBegin << " " << this->Ranges.InputEnd << " "
        << "OutRange: " << this->Ranges.OutputBegin << " " << this->Ranges.OutputEnd << "\n";
    std::cerr << out.str();
#endif
  }



  void join(const ReduceByKeyBody& rhs)
  {
    using KeysIteratorsType = viskores::cont::ArrayPortalToIterators<KeysOutPortalType>;
    using ValuesIteratorsType = viskores::cont::ArrayPortalToIterators<ValuesOutPortalType>;
    using KeysIteratorType = typename KeysIteratorsType::IteratorType;
    using ValuesIteratorType = typename ValuesIteratorsType::IteratorType;

#ifdef VISKORES_DEBUG_TBB_RBK
    ::tbb::tick_count startTime = ::tbb::tick_count::now();
#endif

    this->Ranges.AssertSane();
    rhs.Ranges.AssertSane();
    // Ensure that we're joining two consecutive subsets of the input:
    VISKORES_ASSERT(this->Ranges.IsNext(rhs.Ranges));

    KeysIteratorsType keysIters(this->KeysOutPortal);
    ValuesIteratorsType valuesIters(this->ValuesOutPortal);
    KeysIteratorType keys = keysIters.GetBegin();
    ValuesIteratorType values = valuesIters.GetBegin();

    const viskores::Id dstBegin = this->Ranges.OutputEnd;
    const viskores::Id lastDstIdx = this->Ranges.OutputEnd - 1;

    viskores::Id srcBegin = rhs.Ranges.OutputBegin;
    const viskores::Id srcEnd = rhs.Ranges.OutputEnd;

    // Merge boundaries if needed:
    if (keys[srcBegin] == keys[lastDstIdx])
    {
      values[lastDstIdx] = this->BinaryOperation(values[lastDstIdx], values[srcBegin]);
      ++srcBegin; // Don't copy the key/value we just reduced
    }

    // move data:
    if (srcBegin != dstBegin && srcBegin != srcEnd)
    {
      // Sanity check:
      VISKORES_ASSERT(srcBegin < srcEnd);
      std::copy(keys + srcBegin, keys + srcEnd, keys + dstBegin);
      std::copy(values + srcBegin, values + srcEnd, values + dstBegin);
    }

    this->Ranges.InputEnd = rhs.Ranges.InputEnd;
    this->Ranges.OutputEnd += srcEnd - srcBegin;
    this->Ranges.AssertSane();

#ifdef VISKORES_DEBUG_TBB_RBK
    ::tbb::tick_count endTime = ::tbb::tick_count::now();
    double time = (endTime - startTime).seconds();
    this->JoinTime += rhs.JoinTime + time;
    std::ostringstream out;
    out << "Joined " << (srcEnd - srcBegin) << " rhs values into body in " << time << "s. "
        << "InRange: " << this->Ranges.InputBegin << " " << this->Ranges.InputEnd << " "
        << "OutRange: " << this->Ranges.OutputBegin << " " << this->Ranges.OutputEnd << "\n";
    std::cerr << out.str();
#endif
  }
};


template <typename KeysInPortalType,
          typename ValuesInPortalType,
          typename KeysOutPortalType,
          typename ValuesOutPortalType,
          typename BinaryOperationType>
VISKORES_CONT viskores::Id ReduceByKeyPortals(KeysInPortalType keysInPortal,
                                              ValuesInPortalType valuesInPortal,
                                              KeysOutPortalType keysOutPortal,
                                              ValuesOutPortalType valuesOutPortal,
                                              BinaryOperationType binaryOperation)
{
  const viskores::Id inputLength = keysInPortal.GetNumberOfValues();
  VISKORES_ASSERT(inputLength == valuesInPortal.GetNumberOfValues());

  if (inputLength == 0)
  {
    return 0;
  }

  using ValueType = typename ValuesInPortalType::ValueType;
  using WrappedBinaryOp = internal::WrappedBinaryOperator<ValueType, BinaryOperationType>;
  WrappedBinaryOp wrappedBinaryOp(binaryOperation);

  ReduceByKeyBody<KeysInPortalType,
                  ValuesInPortalType,
                  KeysOutPortalType,
                  ValuesOutPortalType,
                  WrappedBinaryOp>
    body(keysInPortal, valuesInPortal, keysOutPortal, valuesOutPortal, wrappedBinaryOp);
  ::tbb::blocked_range<viskores::Id> range(0, inputLength, TBB_GRAIN_SIZE);

#ifdef VISKORES_DEBUG_TBB_RBK
  std::cerr << "\n\nTBB ReduceByKey:\n";
#endif

  ::tbb::parallel_reduce(range, body);

#ifdef VISKORES_DEBUG_TBB_RBK
  std::cerr << "Total reduce time: " << body.ReduceTime << "s\n";
  std::cerr << "Total join time:   " << body.JoinTime << "s\n";
  std::cerr << "\nend\n";
#endif

  body.Ranges.AssertSane();
  VISKORES_ASSERT(body.Ranges.InputBegin == 0 && body.Ranges.InputEnd == inputLength &&
                  body.Ranges.OutputBegin == 0 && body.Ranges.OutputEnd <= inputLength);

  return body.Ranges.OutputEnd;
}

#ifdef VISKORES_DEBUG_TBB_RBK
#undef VISKORES_DEBUG_TBB_RBK
#endif

template <class InputPortalType, class OutputPortalType, class BinaryOperationType>
struct ScanInclusiveBody
{
  using ValueType = typename std::remove_reference<typename OutputPortalType::ValueType>::type;
  ValueType Sum;
  bool FirstCall;
  InputPortalType InputPortal;
  OutputPortalType OutputPortal;
  BinaryOperationType BinaryOperation;

  VISKORES_CONT
  ScanInclusiveBody(const InputPortalType& inputPortal,
                    const OutputPortalType& outputPortal,
                    BinaryOperationType binaryOperation)
    : Sum(viskores::TypeTraits<ValueType>::ZeroInitialization())
    , FirstCall(true)
    , InputPortal(inputPortal)
    , OutputPortal(outputPortal)
    , BinaryOperation(binaryOperation)
  {
  }


  ScanInclusiveBody(const ScanInclusiveBody& body, ::tbb::split)
    : Sum(viskores::TypeTraits<ValueType>::ZeroInitialization())
    , FirstCall(true)
    , InputPortal(body.InputPortal)
    , OutputPortal(body.OutputPortal)
    , BinaryOperation(body.BinaryOperation)
  {
  }



  void operator()(const ::tbb::blocked_range<viskores::Id>& range, ::tbb::pre_scan_tag)
  {
    using InputIteratorsType = viskores::cont::ArrayPortalToIterators<InputPortalType>;
    InputIteratorsType inputIterators(this->InputPortal);

    //use temp, and iterators instead of member variable to reduce false sharing
    typename InputIteratorsType::IteratorType inIter =
      inputIterators.GetBegin() + static_cast<std::ptrdiff_t>(range.begin());
    ValueType temp = this->FirstCall ? *inIter++ : this->BinaryOperation(this->Sum, *inIter++);
    this->FirstCall = false;
    for (viskores::Id index = range.begin() + 1; index != range.end(); ++index, ++inIter)
    {
      temp = this->BinaryOperation(temp, *inIter);
    }
    this->Sum = temp;
  }



  void operator()(const ::tbb::blocked_range<viskores::Id>& range, ::tbb::final_scan_tag)
  {
    using InputIteratorsType = viskores::cont::ArrayPortalToIterators<InputPortalType>;
    using OutputIteratorsType = viskores::cont::ArrayPortalToIterators<OutputPortalType>;

    InputIteratorsType inputIterators(this->InputPortal);
    OutputIteratorsType outputIterators(this->OutputPortal);

    //use temp, and iterators instead of member variable to reduce false sharing
    typename InputIteratorsType::IteratorType inIter =
      inputIterators.GetBegin() + static_cast<std::ptrdiff_t>(range.begin());
    typename OutputIteratorsType::IteratorType outIter =
      outputIterators.GetBegin() + static_cast<std::ptrdiff_t>(range.begin());
    ValueType temp = this->FirstCall ? *inIter++ : this->BinaryOperation(this->Sum, *inIter++);
    this->FirstCall = false;
    *outIter++ = temp;
    for (viskores::Id index = range.begin() + 1; index != range.end(); ++index, ++inIter, ++outIter)
    {
      *outIter = temp = this->BinaryOperation(temp, *inIter);
    }
    this->Sum = temp;
  }



  void reverse_join(const ScanInclusiveBody& left)
  {
    this->Sum = this->BinaryOperation(left.Sum, this->Sum);
  }



  void assign(const ScanInclusiveBody& src) { this->Sum = src.Sum; }
};

template <class InputPortalType, class OutputPortalType, class BinaryOperationType>
struct ScanExclusiveBody
{
  using ValueType = typename std::remove_reference<typename OutputPortalType::ValueType>::type;

  ValueType Sum;
  bool FirstCall;
  InputPortalType InputPortal;
  OutputPortalType OutputPortal;
  BinaryOperationType BinaryOperation;

  VISKORES_CONT
  ScanExclusiveBody(const InputPortalType& inputPortal,
                    const OutputPortalType& outputPortal,
                    BinaryOperationType binaryOperation,
                    const ValueType& initialValue)
    : Sum(initialValue)
    , FirstCall(true)
    , InputPortal(inputPortal)
    , OutputPortal(outputPortal)
    , BinaryOperation(binaryOperation)
  {
  }


  ScanExclusiveBody(const ScanExclusiveBody& body, ::tbb::split)
    : Sum(body.Sum)
    , FirstCall(true)
    , InputPortal(body.InputPortal)
    , OutputPortal(body.OutputPortal)
    , BinaryOperation(body.BinaryOperation)
  {
  }



  void operator()(const ::tbb::blocked_range<viskores::Id>& range, ::tbb::pre_scan_tag)
  {
    using InputIteratorsType = viskores::cont::ArrayPortalToIterators<InputPortalType>;
    InputIteratorsType inputIterators(this->InputPortal);

    //move the iterator to the first item
    typename InputIteratorsType::IteratorType iter =
      inputIterators.GetBegin() + static_cast<std::ptrdiff_t>(range.begin());

    ValueType temp = *iter;
    ++iter;
    if (!(this->FirstCall && range.begin() > 0))
    {
      temp = this->BinaryOperation(this->Sum, temp);
    }
    for (viskores::Id index = range.begin() + 1; index != range.end(); ++index, ++iter)
    {
      temp = this->BinaryOperation(temp, *iter);
    }
    this->Sum = temp;
    this->FirstCall = false;
  }



  void operator()(const ::tbb::blocked_range<viskores::Id>& range, ::tbb::final_scan_tag)
  {
    using InputIteratorsType = viskores::cont::ArrayPortalToIterators<InputPortalType>;
    using OutputIteratorsType = viskores::cont::ArrayPortalToIterators<OutputPortalType>;

    InputIteratorsType inputIterators(this->InputPortal);
    OutputIteratorsType outputIterators(this->OutputPortal);

    //move the iterators to the first item
    typename InputIteratorsType::IteratorType inIter =
      inputIterators.GetBegin() + static_cast<std::ptrdiff_t>(range.begin());
    typename OutputIteratorsType::IteratorType outIter =
      outputIterators.GetBegin() + static_cast<std::ptrdiff_t>(range.begin());

    ValueType temp = this->Sum;
    for (viskores::Id index = range.begin(); index != range.end(); ++index, ++inIter, ++outIter)
    {
      //copy into a local reference since Input and Output portal
      //could point to the same memory location
      ValueType v = *inIter;
      *outIter = temp;
      temp = this->BinaryOperation(temp, v);
    }
    this->Sum = temp;
    this->FirstCall = false;
  }



  void reverse_join(const ScanExclusiveBody& left)
  {
    //The contract we have with TBB is that they will only join
    //two objects that have been scanned, or two objects which
    //haven't been scanned
    VISKORES_ASSERT(left.FirstCall == this->FirstCall);
    if (!left.FirstCall && !this->FirstCall)
    {
      this->Sum = this->BinaryOperation(left.Sum, this->Sum);
    }
  }



  void assign(const ScanExclusiveBody& src) { this->Sum = src.Sum; }
};


template <class InputPortalType, class OutputPortalType, class BinaryOperationType>
VISKORES_CONT static typename std::remove_reference<typename OutputPortalType::ValueType>::type
ScanInclusivePortals(InputPortalType inputPortal,
                     OutputPortalType outputPortal,
                     BinaryOperationType binaryOperation)
{
  using ValueType = typename std::remove_reference<typename OutputPortalType::ValueType>::type;

  using WrappedBinaryOp = internal::WrappedBinaryOperator<ValueType, BinaryOperationType>;

  WrappedBinaryOp wrappedBinaryOp(binaryOperation);
  ScanInclusiveBody<InputPortalType, OutputPortalType, WrappedBinaryOp> body(
    inputPortal, outputPortal, wrappedBinaryOp);
  viskores::Id arrayLength = inputPortal.GetNumberOfValues();

  ::tbb::blocked_range<viskores::Id> range(0, arrayLength, TBB_GRAIN_SIZE);
  ::tbb::parallel_scan(range, body);
  return body.Sum;
}


template <class InputPortalType, class OutputPortalType, class BinaryOperationType>
VISKORES_CONT static typename std::remove_reference<typename OutputPortalType::ValueType>::type
ScanExclusivePortals(
  InputPortalType inputPortal,
  OutputPortalType outputPortal,
  BinaryOperationType binaryOperation,
  typename std::remove_reference<typename OutputPortalType::ValueType>::type initialValue)
{
  using ValueType = typename std::remove_reference<typename OutputPortalType::ValueType>::type;

  using WrappedBinaryOp = internal::WrappedBinaryOperator<ValueType, BinaryOperationType>;

  WrappedBinaryOp wrappedBinaryOp(binaryOperation);
  ScanExclusiveBody<InputPortalType, OutputPortalType, WrappedBinaryOp> body(
    inputPortal, outputPortal, wrappedBinaryOp, initialValue);
  viskores::Id arrayLength = inputPortal.GetNumberOfValues();

  ::tbb::blocked_range<viskores::Id> range(0, arrayLength, TBB_GRAIN_SIZE);
  ::tbb::parallel_scan(range, body);

  // Seems a little weird to me that we would return the last value in the
  // array rather than the sum, but that is how the function is specified.
  return body.Sum;
}

template <typename InputPortalType, typename IndexPortalType, typename OutputPortalType>
class ScatterKernel
{
public:
  VISKORES_CONT ScatterKernel(InputPortalType inputPortal,
                              IndexPortalType indexPortal,
                              OutputPortalType outputPortal)
    : ValuesPortal(inputPortal)
    , IndexPortal(indexPortal)
    , OutputPortal(outputPortal)
  {
  }

  VISKORES_CONT
  void operator()(const ::tbb::blocked_range<viskores::Id>& range) const
  {
    // The TBB device adapter causes array classes to be shared between
    // control and execution environment. This means that it is possible for
    // an exception to be thrown even though this is typically not allowed.
    // Throwing an exception from here is bad because there are several
    // simultaneous threads running. Get around the problem by catching the
    // error and setting the message buffer as expected.
    try
    {
      VISKORES_VECTORIZATION_PRE_LOOP
      for (viskores::Id i = range.begin(); i < range.end(); i++)
      {
        VISKORES_VECTORIZATION_IN_LOOP
        OutputPortal.Set(i, ValuesPortal.Get(IndexPortal.Get(i)));
      }
    }
    catch (viskores::cont::Error& error)
    {
      this->ErrorMessage.RaiseError(error.GetMessage().c_str());
    }
    catch (...)
    {
      this->ErrorMessage.RaiseError("Unexpected error in execution environment.");
    }
  }

private:
  InputPortalType ValuesPortal;
  IndexPortalType IndexPortal;
  OutputPortalType OutputPortal;
  viskores::exec::internal::ErrorMessageBuffer ErrorMessage;
};


template <typename InputPortalType, typename IndexPortalType, typename OutputPortalType>
VISKORES_CONT static void ScatterPortal(InputPortalType inputPortal,
                                        IndexPortalType indexPortal,
                                        OutputPortalType outputPortal)
{
  const viskores::Id size = inputPortal.GetNumberOfValues();
  VISKORES_ASSERT(size == indexPortal.GetNumberOfValues());

  ScatterKernel<InputPortalType, IndexPortalType, OutputPortalType> scatter(
    inputPortal, indexPortal, outputPortal);

  ::tbb::blocked_range<viskores::Id> range(0, size, TBB_GRAIN_SIZE);
  ::tbb::parallel_for(range, scatter);
}

template <typename PortalType, typename BinaryOperationType>
struct UniqueBody
{
  using ValueType = typename PortalType::ValueType;

  struct Range
  {
    viskores::Id InputBegin;
    viskores::Id InputEnd;
    viskores::Id OutputBegin;
    viskores::Id OutputEnd;



    Range()
      : InputBegin(-1)
      , InputEnd(-1)
      , OutputBegin(-1)
      , OutputEnd(-1)
    {
    }



    Range(viskores::Id inputBegin,
          viskores::Id inputEnd,
          viskores::Id outputBegin,
          viskores::Id outputEnd)
      : InputBegin(inputBegin)
      , InputEnd(inputEnd)
      , OutputBegin(outputBegin)
      , OutputEnd(outputEnd)
    {
      this->AssertSane();
    }



    void AssertSane() const
    {
      VISKORES_ASSERT("Input begin precedes end" && this->InputBegin <= this->InputEnd);
      VISKORES_ASSERT("Output begin precedes end" && this->OutputBegin <= this->OutputEnd);
      VISKORES_ASSERT("Output not past input" && this->OutputBegin <= this->InputBegin &&
                      this->OutputEnd <= this->InputEnd);
      VISKORES_ASSERT("Output smaller than input" &&
                      (this->OutputEnd - this->OutputBegin) <= (this->InputEnd - this->InputBegin));
    }



    bool IsNext(const Range& next) const { return this->InputEnd == next.InputBegin; }
  };

  PortalType Portal;
  BinaryOperationType BinaryOperation;
  Range Ranges;

  VISKORES_CONT
  UniqueBody(const PortalType& portal, BinaryOperationType binaryOperation)
    : Portal(portal)
    , BinaryOperation(binaryOperation)
  {
  }

  VISKORES_CONT
  UniqueBody(const UniqueBody& body, ::tbb::split)
    : Portal(body.Portal)
    , BinaryOperation(body.BinaryOperation)
  {
  }



  void operator()(const ::tbb::blocked_range<viskores::Id>& range)
  {
    if (range.empty())
    {
      return;
    }

    bool firstRun = this->Ranges.OutputBegin < 0; // First use of this body object
    if (firstRun)
    {
      this->Ranges.InputBegin = range.begin();
    }
    else
    {
      // Must be a continuation of the previous input range:
      VISKORES_ASSERT(this->Ranges.InputEnd == range.begin());
    }
    this->Ranges.InputEnd = range.end();
    this->Ranges.AssertSane();

    using IteratorsType = viskores::cont::ArrayPortalToIterators<PortalType>;
    using IteratorType = typename IteratorsType::IteratorType;

    IteratorsType iters(this->Portal);
    IteratorType data = iters.GetBegin();

    viskores::Id readPos = range.begin();
    const viskores::Id readEnd = range.end();

    // Determine output index. If we're reusing the body, pick up where the
    // last block left off. If not, use the input range.
    viskores::Id writePos;
    if (firstRun)
    {
      this->Ranges.OutputBegin = range.begin();
      this->Ranges.OutputEnd = range.begin();
      writePos = range.begin();
    }
    else
    {
      writePos = this->Ranges.OutputEnd;
    }
    this->Ranges.AssertSane();

    // We're either writing at the end of a previous block, or at the input
    // location. Either way, the write position will never be greater than
    // the read position.
    VISKORES_ASSERT(writePos <= readPos);

    // Initialize loop variables:
    BinaryOperationType functor(this->BinaryOperation);
    ValueType current = data[readPos];
    ++readPos;

    // If the start of the current range continues a previous block of
    // identical elements, initialize with the previous result and decrement
    // the write index.
    VISKORES_ASSERT(firstRun || writePos > 0);
    if (!firstRun && functor(data[writePos - 1], current))
    {
      // Ensure that we'll overwrite the duplicate value:
      --writePos;

      // Copy the last data value into current. TestingDeviceAdapter's test
      // using the FuseAll functor requires that the first value in a set of
      // duplicates must be used, and this ensures that condition is met.
      current = data[writePos];
    }

    // Special case: single value in range
    if (readPos >= readEnd)
    {
      data[writePos] = current;
      ++writePos;

      this->Ranges.OutputEnd = writePos;
      return;
    }

    for (;;)
    {
      // Advance readPos until the value changes
      while (readPos < readEnd && functor(current, data[readPos]))
      {
        ++readPos;
      }

      // Write out the unique value
      VISKORES_ASSERT(writePos <= readPos);
      data[writePos] = current;
      ++writePos;

      // Update the current value if there are more entries
      if (readPos < readEnd)
      {
        current = data[readPos];
        ++readPos;
        continue;
      }

      // Input range is exhausted if we reach this point.
      break;
    }

    this->Ranges.OutputEnd = writePos;
  }



  void join(const UniqueBody& rhs)
  {
    using IteratorsType = viskores::cont::ArrayPortalToIterators<PortalType>;
    using IteratorType = typename IteratorsType::IteratorType;

    this->Ranges.AssertSane();
    rhs.Ranges.AssertSane();

    // Ensure that we're joining two consecutive subsets of the input:
    VISKORES_ASSERT(this->Ranges.IsNext(rhs.Ranges));

    IteratorsType iters(this->Portal);
    IteratorType data = iters.GetBegin();
    BinaryOperationType functor(this->BinaryOperation);

    const viskores::Id dstBegin = this->Ranges.OutputEnd;
    const viskores::Id lastDstIdx = this->Ranges.OutputEnd - 1;

    viskores::Id srcBegin = rhs.Ranges.OutputBegin;
    const viskores::Id srcEnd = rhs.Ranges.OutputEnd;

    // Merge boundaries if needed:
    if (functor(data[srcBegin], data[lastDstIdx]))
    {
      ++srcBegin; // Don't copy the duplicate value
    }

    // move data:
    if (srcBegin != dstBegin && srcBegin != srcEnd)
    {
      // Sanity check:
      VISKORES_ASSERT(srcBegin < srcEnd);
      std::copy(data + srcBegin, data + srcEnd, data + dstBegin);
    }

    this->Ranges.InputEnd = rhs.Ranges.InputEnd;
    this->Ranges.OutputEnd += srcEnd - srcBegin;
    this->Ranges.AssertSane();
  }
};


template <typename PortalType, typename BinaryOperationType>
VISKORES_CONT viskores::Id UniquePortals(PortalType portal, BinaryOperationType binaryOperation)
{
  const viskores::Id inputLength = portal.GetNumberOfValues();
  if (inputLength == 0)
  {
    return 0;
  }

  using WrappedBinaryOp = internal::WrappedBinaryOperator<bool, BinaryOperationType>;
  WrappedBinaryOp wrappedBinaryOp(binaryOperation);

  UniqueBody<PortalType, WrappedBinaryOp> body(portal, wrappedBinaryOp);
  ::tbb::blocked_range<viskores::Id> range(0, inputLength, TBB_GRAIN_SIZE);

  ::tbb::parallel_reduce(range, body);

  body.Ranges.AssertSane();
  VISKORES_ASSERT(body.Ranges.InputBegin == 0 && body.Ranges.InputEnd == inputLength &&
                  body.Ranges.OutputBegin == 0 && body.Ranges.OutputEnd <= inputLength);

  return body.Ranges.OutputEnd;
}
}
}
}
#endif //viskores_cont_tbb_internal_FunctorsTBB_h
