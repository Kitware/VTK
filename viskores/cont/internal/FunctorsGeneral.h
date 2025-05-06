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
#ifndef viskores_cont_internal_FunctorsGeneral_h
#define viskores_cont_internal_FunctorsGeneral_h

#include <viskores/BinaryOperators.h>
#include <viskores/BinaryPredicates.h>
#include <viskores/LowerBound.h>
#include <viskores/TypeTraits.h>
#include <viskores/UnaryPredicates.h>
#include <viskores/UpperBound.h>
#include <viskores/cont/ArrayPortalToIterators.h>

#include <viskores/exec/FunctorBase.h>

#include <algorithm>
#include <atomic>
#include <iterator>

namespace viskores
{
namespace cont
{
namespace internal
{

// Binary function object wrapper which can detect and handle calling the
// wrapped operator with complex value types such as
// ArrayPortalValueReference which happen when passed an input array that
// is implicit.
template <typename ResultType, typename Function>
struct WrappedBinaryOperator
{
  Function m_f;

  VISKORES_CONT
  WrappedBinaryOperator(const Function& f)
    : m_f(f)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename Argument1, typename Argument2>
  VISKORES_EXEC_CONT ResultType operator()(const Argument1& x, const Argument2& y) const
  {
    return static_cast<ResultType>(m_f(x, y));
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename Argument1, typename Argument2>
  VISKORES_EXEC_CONT ResultType
  operator()(const viskores::internal::ArrayPortalValueReference<Argument1>& x,
             const viskores::internal::ArrayPortalValueReference<Argument2>& y) const
  {
    using ValueTypeX = typename viskores::internal::ArrayPortalValueReference<Argument1>::ValueType;
    using ValueTypeY = typename viskores::internal::ArrayPortalValueReference<Argument2>::ValueType;
    return static_cast<ResultType>(m_f((ValueTypeX)x, (ValueTypeY)y));
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename Argument1, typename Argument2>
  VISKORES_EXEC_CONT ResultType
  operator()(const Argument1& x,
             const viskores::internal::ArrayPortalValueReference<Argument2>& y) const
  {
    using ValueTypeY = typename viskores::internal::ArrayPortalValueReference<Argument2>::ValueType;
    return static_cast<ResultType>(m_f(x, (ValueTypeY)y));
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename Argument1, typename Argument2>
  VISKORES_EXEC_CONT ResultType
  operator()(const viskores::internal::ArrayPortalValueReference<Argument1>& x,
             const Argument2& y) const
  {
    using ValueTypeX = typename viskores::internal::ArrayPortalValueReference<Argument1>::ValueType;
    return static_cast<ResultType>(m_f((ValueTypeX)x, y));
  }
};

using DefaultCompareFunctor = viskores::SortLess;

//needs to be in a location that TBB DeviceAdapterAlgorithm can reach
template <typename T, typename U, class BinaryCompare = DefaultCompareFunctor>
struct KeyCompare
{
  KeyCompare()
    : CompareFunctor()
  {
  }
  explicit KeyCompare(BinaryCompare c)
    : CompareFunctor(c)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  bool operator()(const viskores::Pair<T, U>& a, const viskores::Pair<T, U>& b) const
  {
    return CompareFunctor(a.first, b.first);
  }

private:
  BinaryCompare CompareFunctor;
};

template <typename PortalConstType, typename T, typename BinaryFunctor>
struct ReduceKernel : viskores::exec::FunctorBase
{
  PortalConstType Portal;
  T InitialValue;
  BinaryFunctor BinaryOperator;
  viskores::Id PortalLength;

  VISKORES_EXEC_CONT
  ReduceKernel()
    : Portal()
    , InitialValue()
    , BinaryOperator()
    , PortalLength(0)
  {
  }

  VISKORES_CONT
  ReduceKernel(const PortalConstType& portal, T initialValue, BinaryFunctor binary_functor)
    : Portal(portal)
    , InitialValue(initialValue)
    , BinaryOperator(binary_functor)
    , PortalLength(portal.GetNumberOfValues())
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  T operator()(viskores::Id index) const
  {
    const viskores::Id reduceWidth = 16;
    const viskores::Id offset = index * reduceWidth;

    if (offset + reduceWidth >= this->PortalLength)
    {
      //This will only occur for a single index value, so this is the case
      //that needs to handle the initialValue
      T partialSum = static_cast<T>(BinaryOperator(this->InitialValue, this->Portal.Get(offset)));
      viskores::Id currentIndex = offset + 1;
      while (currentIndex < this->PortalLength)
      {
        partialSum = static_cast<T>(BinaryOperator(partialSum, this->Portal.Get(currentIndex)));
        ++currentIndex;
      }
      return partialSum;
    }
    else
    {
      //optimize the usecase where all values are valid and we don't
      //need to check that we might go out of bounds
      T partialSum =
        static_cast<T>(BinaryOperator(this->Portal.Get(offset), this->Portal.Get(offset + 1)));
      for (int i = 2; i < reduceWidth; ++i)
      {
        partialSum = static_cast<T>(BinaryOperator(partialSum, this->Portal.Get(offset + i)));
      }
      return partialSum;
    }
  }
};

struct ReduceKeySeriesStates
{
  bool fStart; // START of a segment
  bool fEnd;   // END of a segment

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  ReduceKeySeriesStates(bool start = false, bool end = false)
    : fStart(start)
    , fEnd(end)
  {
  }
};

template <typename InputPortalType, typename KeyStatePortalType>
struct ReduceStencilGeneration : viskores::exec::FunctorBase
{
  InputPortalType Input;
  KeyStatePortalType KeyState;

  VISKORES_CONT
  ReduceStencilGeneration(const InputPortalType& input, const KeyStatePortalType& kstate)
    : Input(input)
    , KeyState(kstate)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  void operator()(viskores::Id centerIndex) const
  {
    using ValueType = typename InputPortalType::ValueType;
    using KeyStateType = typename KeyStatePortalType::ValueType;

    const viskores::Id leftIndex = centerIndex - 1;
    const viskores::Id rightIndex = centerIndex + 1;

    //we need to determine which of three states this
    //index is. It can be:
    // 1. Middle of a set of equivalent keys.
    // 2. Start of a set of equivalent keys.
    // 3. End of a set of equivalent keys.
    // 4. Both the start and end of a set of keys

    //we don't have to worry about an array of length 1, as
    //the calling code handles that use case

    if (centerIndex == 0)
    {
      //this means we are at the start of the array
      //means we are automatically START
      //just need to check if we are END
      const ValueType centerValue = this->Input.Get(centerIndex);
      const ValueType rightValue = this->Input.Get(rightIndex);
      const KeyStateType state = ReduceKeySeriesStates(true, rightValue != centerValue);
      this->KeyState.Set(centerIndex, state);
    }
    else if (rightIndex == this->Input.GetNumberOfValues())
    {
      //this means we are at the end, so we are at least END
      //just need to check if we are START
      const ValueType centerValue = this->Input.Get(centerIndex);
      const ValueType leftValue = this->Input.Get(leftIndex);
      const KeyStateType state = ReduceKeySeriesStates(leftValue != centerValue, true);
      this->KeyState.Set(centerIndex, state);
    }
    else
    {
      const ValueType centerValue = this->Input.Get(centerIndex);
      const bool leftMatches(this->Input.Get(leftIndex) == centerValue);
      const bool rightMatches(this->Input.Get(rightIndex) == centerValue);

      //assume it is the middle, and check for the other use-case
      KeyStateType state = ReduceKeySeriesStates(!leftMatches, !rightMatches);
      this->KeyState.Set(centerIndex, state);
    }
  }
};

template <typename BinaryFunctor>
struct ReduceByKeyAdd
{
  BinaryFunctor BinaryOperator;

  ReduceByKeyAdd(BinaryFunctor binary_functor)
    : BinaryOperator(binary_functor)
  {
  }

  template <typename T>
  VISKORES_EXEC viskores::Pair<T, ReduceKeySeriesStates> operator()(
    const viskores::Pair<T, ReduceKeySeriesStates>& a,
    const viskores::Pair<T, ReduceKeySeriesStates>& b) const
  {
    using ReturnType = viskores::Pair<T, ReduceKeySeriesStates>;
    //need too handle how we are going to add two numbers together
    //based on the keyStates that they have

    // Make it work for parallel inclusive scan.  Will end up with all start bits = 1
    // the following logic should change if you use a different parallel scan algorithm.
    if (!b.second.fStart)
    {
      // if b is not START, then it's safe to sum a & b.
      // Propagate a's start flag to b
      // so that later when b's START bit is set, it means there must exists a START between a and b
      return ReturnType(this->BinaryOperator(a.first, b.first),
                        ReduceKeySeriesStates(a.second.fStart, b.second.fEnd));
    }
    return b;
  }
};

struct ReduceByKeyUnaryStencilOp
{
  VISKORES_EXEC
  bool operator()(ReduceKeySeriesStates keySeriesState) const { return keySeriesState.fEnd; }
};

template <typename T,
          typename InputPortalType,
          typename KeyStatePortalType,
          typename OutputPortalType>
struct ShiftCopyAndInit : viskores::exec::FunctorBase
{
  InputPortalType Input;
  KeyStatePortalType KeyState;
  OutputPortalType Output;
  T initValue;

  ShiftCopyAndInit(const InputPortalType& _input,
                   const KeyStatePortalType& kstate,
                   OutputPortalType& _output,
                   T _init)
    : Input(_input)
    , KeyState(kstate)
    , Output(_output)
    , initValue(_init)
  {
  }

  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    if (this->KeyState.Get(index).fStart)
    {
      Output.Set(index, initValue);
    }
    else
    {
      Output.Set(index, Input.Get(index - 1));
    }
  }
};

template <class BitsPortal, class IndicesPortal>
struct BitFieldToUnorderedSetFunctor : public viskores::exec::FunctorBase
{
  using WordType = typename BitsPortal::WordTypePreferred;

  // This functor executes a number of instances, where each instance handles
  // two cachelines worth of data. Figure out how many words that is:
  static constexpr viskores::Id CacheLineSize = VISKORES_ALLOCATION_ALIGNMENT;
  static constexpr viskores::Id WordsPerCacheLine =
    CacheLineSize / static_cast<viskores::Id>(sizeof(WordType));
  static constexpr viskores::Id CacheLinesPerInstance = 2;
  static constexpr viskores::Id WordsPerInstance = CacheLinesPerInstance * WordsPerCacheLine;

  VISKORES_STATIC_ASSERT(
    VISKORES_PASS_COMMAS(std::is_same<typename IndicesPortal::ValueType, viskores::Id>::value));

  VISKORES_CONT
  BitFieldToUnorderedSetFunctor(const BitsPortal& input,
                                IndicesPortal& output,
                                std::atomic<viskores::UInt64>& popCount)
    : Input{ input }
    , Output{ output }
    , PopCount(popCount)
    , FinalWordIndex{ input.GetNumberOfWords() - 1 }
    , FinalWordMask(input.GetFinalWordMask())
  {
  }

  VISKORES_CONT viskores::Id GetNumberOfInstances() const
  {
    const auto numWords = this->Input.GetNumberOfWords();
    return (numWords + WordsPerInstance - 1) / WordsPerInstance;
  }

  VISKORES_EXEC void operator()(viskores::Id instanceIdx) const
  {
    const viskores::Id numWords = this->Input.GetNumberOfWords();
    const viskores::Id wordStart = viskores::Min(instanceIdx * WordsPerInstance, numWords);
    const viskores::Id wordEnd = viskores::Min(wordStart + WordsPerInstance, numWords);

    if (wordStart != wordEnd) // range is valid
    {
      this->ExecuteRange(wordStart, wordEnd);
    }
  }

  VISKORES_EXEC void ExecuteRange(viskores::Id wordStart, viskores::Id wordEnd) const
  {
#ifndef VISKORES_CUDA_DEVICE_PASS // for std::atomic call from VISKORES_EXEC function:
    // Count bits and allocate space for output:
    viskores::UInt64 chunkBits = this->CountChunkBits(wordStart, wordEnd);
    if (chunkBits > 0)
    {
      viskores::UInt64 outIdx = this->PopCount.fetch_add(chunkBits, std::memory_order_relaxed);

      this->ProcessWords(wordStart, wordEnd, static_cast<viskores::Id>(outIdx));
    }
#else
    (void)wordStart;
    (void)wordEnd;
#endif
  }

  VISKORES_CONT viskores::UInt64 GetPopCount() const
  {
    return PopCount.load(std::memory_order_relaxed);
  }

private:
  VISKORES_EXEC viskores::UInt64 CountChunkBits(viskores::Id wordStart, viskores::Id wordEnd) const
  {
    // Need to mask out trailing bits from the final word:
    const bool isFinalChunk = wordEnd == (this->FinalWordIndex + 1);

    if (isFinalChunk)
    {
      wordEnd = this->FinalWordIndex;
    }

    viskores::Int32 tmp = 0;
    for (viskores::Id i = wordStart; i < wordEnd; ++i)
    {
      tmp += viskores::CountSetBits(this->Input.GetWord(i));
    }

    if (isFinalChunk)
    {
      tmp +=
        viskores::CountSetBits(this->Input.GetWord(this->FinalWordIndex) & this->FinalWordMask);
    }

    return static_cast<viskores::UInt64>(tmp);
  }

  VISKORES_EXEC void ProcessWords(viskores::Id wordStart,
                                  viskores::Id wordEnd,
                                  viskores::Id outputStartIdx) const
  {
    // Need to mask out trailing bits from the final word:
    const bool isFinalChunk = wordEnd == (this->FinalWordIndex + 1);

    if (isFinalChunk)
    {
      wordEnd = this->FinalWordIndex;
    }

    for (viskores::Id i = wordStart; i < wordEnd; ++i)
    {
      const viskores::Id firstBitIdx = i * static_cast<viskores::Id>(sizeof(WordType)) * CHAR_BIT;
      WordType word = this->Input.GetWord(i);
      while (word != 0) // have bits
      {
        // Find next bit. FindFirstSetBit starts counting at 1.
        viskores::Int32 bit = viskores::FindFirstSetBit(word) - 1;
        this->Output.Set(outputStartIdx++, firstBitIdx + bit); // Write index of bit
        word ^= (1 << bit);                                    // clear bit
      }
    }

    if (isFinalChunk)
    {
      const viskores::Id i = this->FinalWordIndex;
      const viskores::Id firstBitIdx = i * static_cast<viskores::Id>(sizeof(WordType)) * CHAR_BIT;
      WordType word = this->Input.GetWord(i) & this->FinalWordMask;
      while (word != 0) // have bits
      {
        // Find next bit. FindFirstSetBit starts counting at 1.
        viskores::Int32 bit = viskores::FindFirstSetBit(word) - 1;
        this->Output.Set(outputStartIdx++, firstBitIdx + bit); // Write index of bit
        word ^= (1 << bit);                                    // clear bit
      }
    }
  }

  BitsPortal Input;
  IndicesPortal Output;
  std::atomic<viskores::UInt64>& PopCount;
  // Used to mask trailing bits the in last word.
  viskores::Id FinalWordIndex{ 0 };
  WordType FinalWordMask{ 0 };
};

template <class InputPortalType, class OutputPortalType>
struct CopyKernel
{
  InputPortalType InputPortal;
  OutputPortalType OutputPortal;
  viskores::Id InputOffset;
  viskores::Id OutputOffset;

  VISKORES_CONT
  CopyKernel(InputPortalType inputPortal,
             OutputPortalType outputPortal,
             viskores::Id inputOffset = 0,
             viskores::Id outputOffset = 0)
    : InputPortal(inputPortal)
    , OutputPortal(outputPortal)
    , InputOffset(inputOffset)
    , OutputOffset(outputOffset)
  {
  }

  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    using ValueType = typename OutputPortalType::ValueType;
    this->OutputPortal.Set(
      index + this->OutputOffset,
      static_cast<ValueType>(this->InputPortal.Get(index + this->InputOffset)));
  }

  VISKORES_CONT
  void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer&) {}
};

template <typename BitsPortal>
struct CountSetBitsFunctor : public viskores::exec::FunctorBase
{
  using WordType = typename BitsPortal::WordTypePreferred;

  // This functor executes a number of instances, where each instance handles
  // two cachelines worth of data. This reduces the number of atomic operations.
  // Figure out how many words that is:
  static constexpr viskores::Id CacheLineSize = VISKORES_ALLOCATION_ALIGNMENT;
  static constexpr viskores::Id WordsPerCacheLine =
    CacheLineSize / static_cast<viskores::Id>(sizeof(WordType));
  static constexpr viskores::Id CacheLinesPerInstance = 2;
  static constexpr viskores::Id WordsPerInstance = CacheLinesPerInstance * WordsPerCacheLine;

  VISKORES_CONT
  CountSetBitsFunctor(const BitsPortal& input, std::atomic<viskores::UInt64>& popCount)
    : Input{ input }
    , PopCount(popCount)
    , FinalWordIndex{ input.GetNumberOfWords() - 1 }
    , FinalWordMask{ input.GetFinalWordMask() }
  {
  }

  VISKORES_CONT viskores::Id GetNumberOfInstances() const
  {
    const auto numWords = this->Input.GetNumberOfWords();
    return (numWords + WordsPerInstance - 1) / WordsPerInstance;
  }

  VISKORES_EXEC void operator()(viskores::Id instanceIdx) const
  {
    const viskores::Id numWords = this->Input.GetNumberOfWords();
    const viskores::Id wordStart = viskores::Min(instanceIdx * WordsPerInstance, numWords);
    const viskores::Id wordEnd = viskores::Min(wordStart + WordsPerInstance, numWords);

    if (wordStart != wordEnd) // range is valid
    {
      this->ExecuteRange(wordStart, wordEnd);
    }
  }

  VISKORES_CONT viskores::UInt64 GetPopCount() const
  {
    return PopCount.load(std::memory_order_relaxed);
  }

private:
  VISKORES_EXEC void ExecuteRange(viskores::Id wordStart, viskores::Id wordEnd) const
  {
#ifndef VISKORES_CUDA_DEVICE_PASS // for std::atomic call from VISKORES_EXEC function:
    // Count bits and allocate space for output:
    viskores::UInt64 chunkBits = this->CountChunkBits(wordStart, wordEnd);
    this->PopCount.fetch_add(chunkBits, std::memory_order_relaxed);
#else
    (void)wordStart;
    (void)wordEnd;
#endif
  }

  VISKORES_EXEC viskores::UInt64 CountChunkBits(viskores::Id wordStart, viskores::Id wordEnd) const
  {
    // Need to mask out trailing bits from the final word:
    const bool isFinalChunk = wordEnd == (this->FinalWordIndex + 1);

    if (isFinalChunk)
    {
      wordEnd = this->FinalWordIndex;
    }

    viskores::Int32 tmp = 0;
    for (viskores::Id i = wordStart; i < wordEnd; ++i)
    {
      tmp += viskores::CountSetBits(this->Input.GetWord(i));
    }

    if (isFinalChunk)
    {
      tmp +=
        viskores::CountSetBits(this->Input.GetWord(this->FinalWordIndex) & this->FinalWordMask);
    }

    return static_cast<viskores::UInt64>(tmp);
  }

  BitsPortal Input;
  std::atomic<viskores::UInt64>& PopCount;
  // Used to mask trailing bits the in last word.
  viskores::Id FinalWordIndex{ 0 };
  WordType FinalWordMask{ 0 };
};

// For a given unsigned integer less than 32 bits, repeat its bits until we
// have a 32 bit pattern. This is used to make all fill patterns at least
// 32 bits in size, since concurrently writing to adjacent locations smaller
// than 32 bits may race on some platforms.
template <typename WordType, typename = typename std::enable_if<(sizeof(WordType) >= 4)>::type>
static constexpr VISKORES_CONT WordType RepeatTo32BitsIfNeeded(WordType pattern)
{ // for 32 bits or more, just pass the type through.
  return pattern;
}

static inline constexpr VISKORES_CONT viskores::UInt32 RepeatTo32BitsIfNeeded(
  viskores::UInt16 pattern)
{
  return static_cast<viskores::UInt32>(pattern << 16 | pattern);
}

static inline constexpr VISKORES_CONT viskores::UInt32 RepeatTo32BitsIfNeeded(
  viskores::UInt8 pattern)
{
  return RepeatTo32BitsIfNeeded(static_cast<viskores::UInt16>(pattern << 8 | pattern));
}

template <typename BitsPortal, typename WordType>
struct FillBitFieldFunctor : public viskores::exec::FunctorBase
{
  VISKORES_CONT
  FillBitFieldFunctor(const BitsPortal& portal, WordType mask)
    : Portal{ portal }
    , Mask{ mask }
  {
  }

  VISKORES_EXEC void operator()(viskores::Id wordIdx) const
  {
    this->Portal.SetWord(wordIdx, this->Mask);
  }

private:
  BitsPortal Portal;
  WordType Mask;
};

template <typename PortalType>
struct FillArrayHandleFunctor : public viskores::exec::FunctorBase
{
  using ValueType = typename PortalType::ValueType;

  VISKORES_CONT
  FillArrayHandleFunctor(const PortalType& portal, ValueType value)
    : Portal{ portal }
    , Value{ value }
  {
  }

  VISKORES_EXEC void operator()(viskores::Id idx) const { this->Portal.Set(idx, this->Value); }

private:
  PortalType Portal;
  ValueType Value;
};

template <typename Iterator, typename IteratorTag>
VISKORES_EXEC static inline viskores::Id IteratorDistanceImpl(const Iterator& from,
                                                              const Iterator& to,
                                                              IteratorTag)
{
  viskores::Id dist = 0;
  for (auto it = from; it != to; ++it)
  {
    ++dist;
  }
  return dist;
}

template <typename Iterator>
VISKORES_EXEC static inline viskores::Id IteratorDistanceImpl(const Iterator& from,
                                                              const Iterator& to,
                                                              std::random_access_iterator_tag)
{
  return static_cast<viskores::Id>(to - from);
}

#if defined(VISKORES_HIP)

template <typename Iterator>
__host__ static inline viskores::Id IteratorDistance(const Iterator& from, const Iterator& to)
{
  return static_cast<viskores::Id>(std::distance(from, to));
}

template <typename Iterator>
__device__ static inline viskores::Id IteratorDistance(const Iterator& from, const Iterator& to)
{
  return IteratorDistanceImpl(
    from, to, typename std::iterator_traits<Iterator>::iterator_category{});
}

#else

template <typename Iterator>
VISKORES_EXEC static inline viskores::Id IteratorDistance(const Iterator& from, const Iterator& to)
{
#ifndef VISKORES_CUDA_DEVICE_PASS
  return static_cast<viskores::Id>(std::distance(from, to));
#else
  return IteratorDistanceImpl(
    from, to, typename std::iterator_traits<Iterator>::iterator_category{});
#endif
}

#endif

template <class InputPortalType, class ValuesPortalType, class OutputPortalType>
struct LowerBoundsKernel
{
  InputPortalType InputPortal;
  ValuesPortalType ValuesPortal;
  OutputPortalType OutputPortal;

  VISKORES_CONT
  LowerBoundsKernel(InputPortalType inputPortal,
                    ValuesPortalType valuesPortal,
                    OutputPortalType outputPortal)
    : InputPortal(inputPortal)
    , ValuesPortal(valuesPortal)
    , OutputPortal(outputPortal)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    // This method assumes that (1) InputPortalType can return working
    // iterators in the execution environment and that (2) methods not
    // specified with VISKORES_EXEC (such as the STL algorithms) can be
    // called from the execution environment. Neither one of these is
    // necessarily true, but it is true for the current uses of this general
    // function and I don't want to compete with STL if I don't have to.

    using InputIteratorsType = viskores::cont::ArrayPortalToIterators<InputPortalType>;
    InputIteratorsType inputIterators(this->InputPortal);
    auto resultPos = viskores::LowerBound(
      inputIterators.GetBegin(), inputIterators.GetEnd(), this->ValuesPortal.Get(index));

    viskores::Id resultIndex = IteratorDistance(inputIterators.GetBegin(), resultPos);
    this->OutputPortal.Set(index, resultIndex);
  }

  VISKORES_CONT
  void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer&) {}
};

template <class InputPortalType,
          class ValuesPortalType,
          class OutputPortalType,
          class BinaryCompare>
struct LowerBoundsComparisonKernel
{
  InputPortalType InputPortal;
  ValuesPortalType ValuesPortal;
  OutputPortalType OutputPortal;
  BinaryCompare CompareFunctor;

  VISKORES_CONT
  LowerBoundsComparisonKernel(InputPortalType inputPortal,
                              ValuesPortalType valuesPortal,
                              OutputPortalType outputPortal,
                              BinaryCompare binary_compare)
    : InputPortal(inputPortal)
    , ValuesPortal(valuesPortal)
    , OutputPortal(outputPortal)
    , CompareFunctor(binary_compare)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    // This method assumes that (1) InputPortalType can return working
    // iterators in the execution environment and that (2) methods not
    // specified with VISKORES_EXEC (such as the STL algorithms) can be
    // called from the execution environment. Neither one of these is
    // necessarily true, but it is true for the current uses of this general
    // function and I don't want to compete with STL if I don't have to.

    using InputIteratorsType = viskores::cont::ArrayPortalToIterators<InputPortalType>;
    InputIteratorsType inputIterators(this->InputPortal);
    auto resultPos = viskores::LowerBound(inputIterators.GetBegin(),
                                          inputIterators.GetEnd(),
                                          this->ValuesPortal.Get(index),
                                          this->CompareFunctor);

    viskores::Id resultIndex = IteratorDistance(inputIterators.GetBegin(), resultPos);
    this->OutputPortal.Set(index, resultIndex);
  }

  VISKORES_CONT
  void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer&) {}
};

template <typename PortalType>
struct SetConstantKernel
{
  using ValueType = typename PortalType::ValueType;
  PortalType Portal;
  ValueType Value;

  VISKORES_CONT
  SetConstantKernel(const PortalType& portal, ValueType value)
    : Portal(portal)
    , Value(value)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  void operator()(viskores::Id index) const { this->Portal.Set(index, this->Value); }

  VISKORES_CONT
  void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer&) {}
};

template <typename PortalType, typename BinaryCompare>
struct BitonicSortMergeKernel : viskores::exec::FunctorBase
{
  PortalType Portal;
  BinaryCompare Compare;
  viskores::Id GroupSize;

  VISKORES_CONT
  BitonicSortMergeKernel(const PortalType& portal,
                         const BinaryCompare& compare,
                         viskores::Id groupSize)
    : Portal(portal)
    , Compare(compare)
    , GroupSize(groupSize)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    using ValueType = typename PortalType::ValueType;

    viskores::Id groupIndex = index % this->GroupSize;
    viskores::Id blockSize = 2 * this->GroupSize;
    viskores::Id blockIndex = index / this->GroupSize;

    viskores::Id lowIndex = blockIndex * blockSize + groupIndex;
    viskores::Id highIndex = lowIndex + this->GroupSize;

    if (highIndex < this->Portal.GetNumberOfValues())
    {
      ValueType lowValue = this->Portal.Get(lowIndex);
      ValueType highValue = this->Portal.Get(highIndex);
      if (this->Compare(highValue, lowValue))
      {
        this->Portal.Set(highIndex, lowValue);
        this->Portal.Set(lowIndex, highValue);
      }
    }
  }
};

template <typename PortalType, typename BinaryCompare>
struct BitonicSortCrossoverKernel : viskores::exec::FunctorBase
{
  PortalType Portal;
  BinaryCompare Compare;
  viskores::Id GroupSize;

  VISKORES_CONT
  BitonicSortCrossoverKernel(const PortalType& portal,
                             const BinaryCompare& compare,
                             viskores::Id groupSize)
    : Portal(portal)
    , Compare(compare)
    , GroupSize(groupSize)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    using ValueType = typename PortalType::ValueType;

    viskores::Id groupIndex = index % this->GroupSize;
    viskores::Id blockSize = 2 * this->GroupSize;
    viskores::Id blockIndex = index / this->GroupSize;

    viskores::Id lowIndex = blockIndex * blockSize + groupIndex;
    viskores::Id highIndex = blockIndex * blockSize + (blockSize - groupIndex - 1);

    if (highIndex < this->Portal.GetNumberOfValues())
    {
      ValueType lowValue = this->Portal.Get(lowIndex);
      ValueType highValue = this->Portal.Get(highIndex);
      if (this->Compare(highValue, lowValue))
      {
        this->Portal.Set(highIndex, lowValue);
        this->Portal.Set(lowIndex, highValue);
      }
    }
  }
};

template <class StencilPortalType, class OutputPortalType, class UnaryPredicate>
struct StencilToIndexFlagKernel
{
  using StencilValueType = typename StencilPortalType::ValueType;
  StencilPortalType StencilPortal;
  OutputPortalType OutputPortal;
  UnaryPredicate Predicate;

  VISKORES_CONT
  StencilToIndexFlagKernel(StencilPortalType stencilPortal,
                           OutputPortalType outputPortal,
                           UnaryPredicate unary_predicate)
    : StencilPortal(stencilPortal)
    , OutputPortal(outputPortal)
    , Predicate(unary_predicate)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    StencilValueType value = this->StencilPortal.Get(index);
    this->OutputPortal.Set(index, this->Predicate(value) ? 1 : 0);
  }

  VISKORES_CONT
  void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer&) {}
};

template <class InputPortalType,
          class StencilPortalType,
          class IndexPortalType,
          class OutputPortalType,
          class PredicateOperator>
struct CopyIfKernel
{
  InputPortalType InputPortal;
  StencilPortalType StencilPortal;
  IndexPortalType IndexPortal;
  OutputPortalType OutputPortal;
  PredicateOperator Predicate;

  VISKORES_CONT
  CopyIfKernel(InputPortalType inputPortal,
               StencilPortalType stencilPortal,
               IndexPortalType indexPortal,
               OutputPortalType outputPortal,
               PredicateOperator unary_predicate)
    : InputPortal(inputPortal)
    , StencilPortal(stencilPortal)
    , IndexPortal(indexPortal)
    , OutputPortal(outputPortal)
    , Predicate(unary_predicate)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    using StencilValueType = typename StencilPortalType::ValueType;
    StencilValueType stencilValue = this->StencilPortal.Get(index);
    if (Predicate(stencilValue))
    {
      viskores::Id outputIndex = this->IndexPortal.Get(index);

      using OutputValueType = typename OutputPortalType::ValueType;
      OutputValueType value = this->InputPortal.Get(index);

      this->OutputPortal.Set(outputIndex, value);
    }
  }

  VISKORES_CONT
  void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer&) {}
};

template <class InputPortalType, class StencilPortalType>
struct ClassifyUniqueKernel
{
  InputPortalType InputPortal;
  StencilPortalType StencilPortal;

  VISKORES_CONT
  ClassifyUniqueKernel(InputPortalType inputPortal, StencilPortalType stencilPortal)
    : InputPortal(inputPortal)
    , StencilPortal(stencilPortal)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    using ValueType = typename StencilPortalType::ValueType;
    if (index == 0)
    {
      // Always copy first value.
      this->StencilPortal.Set(index, ValueType(1));
    }
    else
    {
      ValueType flag = ValueType(this->InputPortal.Get(index - 1) != this->InputPortal.Get(index));
      this->StencilPortal.Set(index, flag);
    }
  }

  VISKORES_CONT
  void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer&) {}
};

template <class InputPortalType, class StencilPortalType, class BinaryCompare>
struct ClassifyUniqueComparisonKernel
{
  InputPortalType InputPortal;
  StencilPortalType StencilPortal;
  BinaryCompare CompareFunctor;

  VISKORES_CONT
  ClassifyUniqueComparisonKernel(InputPortalType inputPortal,
                                 StencilPortalType stencilPortal,
                                 BinaryCompare binary_compare)
    : InputPortal(inputPortal)
    , StencilPortal(stencilPortal)
    , CompareFunctor(binary_compare)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    using ValueType = typename StencilPortalType::ValueType;
    if (index == 0)
    {
      // Always copy first value.
      this->StencilPortal.Set(index, ValueType(1));
    }
    else
    {
      //comparison predicate returns true when they match
      const bool same =
        !(this->CompareFunctor(this->InputPortal.Get(index - 1), this->InputPortal.Get(index)));
      ValueType flag = ValueType(same);
      this->StencilPortal.Set(index, flag);
    }
  }

  VISKORES_CONT
  void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer&) {}
};

template <class InputPortalType, class ValuesPortalType, class OutputPortalType>
struct UpperBoundsKernel
{
  InputPortalType InputPortal;
  ValuesPortalType ValuesPortal;
  OutputPortalType OutputPortal;

  VISKORES_CONT
  UpperBoundsKernel(InputPortalType inputPortal,
                    ValuesPortalType valuesPortal,
                    OutputPortalType outputPortal)
    : InputPortal(inputPortal)
    , ValuesPortal(valuesPortal)
    , OutputPortal(outputPortal)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    // This method assumes that (1) InputPortalType can return working
    // iterators in the execution environment and that (2) methods not
    // specified with VISKORES_EXEC (such as the STL algorithms) can be
    // called from the execution environment. Neither one of these is
    // necessarily true, but it is true for the current uses of this general
    // function and I don't want to compete with STL if I don't have to.

    using InputIteratorsType = viskores::cont::ArrayPortalToIterators<InputPortalType>;
    InputIteratorsType inputIterators(this->InputPortal);
    auto resultPos = viskores::UpperBound(
      inputIterators.GetBegin(), inputIterators.GetEnd(), this->ValuesPortal.Get(index));

    viskores::Id resultIndex = IteratorDistance(inputIterators.GetBegin(), resultPos);
    this->OutputPortal.Set(index, resultIndex);
  }

  VISKORES_CONT
  void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer&) {}
};

template <class InputPortalType,
          class ValuesPortalType,
          class OutputPortalType,
          class BinaryCompare>
struct UpperBoundsKernelComparisonKernel
{
  InputPortalType InputPortal;
  ValuesPortalType ValuesPortal;
  OutputPortalType OutputPortal;
  BinaryCompare CompareFunctor;

  VISKORES_CONT
  UpperBoundsKernelComparisonKernel(InputPortalType inputPortal,
                                    ValuesPortalType valuesPortal,
                                    OutputPortalType outputPortal,
                                    BinaryCompare binary_compare)
    : InputPortal(inputPortal)
    , ValuesPortal(valuesPortal)
    , OutputPortal(outputPortal)
    , CompareFunctor(binary_compare)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    // This method assumes that (1) InputPortalType can return working
    // iterators in the execution environment and that (2) methods not
    // specified with VISKORES_EXEC (such as the STL algorithms) can be
    // called from the execution environment. Neither one of these is
    // necessarily true, but it is true for the current uses of this general
    // function and I don't want to compete with STL if I don't have to.

    using InputIteratorsType = viskores::cont::ArrayPortalToIterators<InputPortalType>;
    InputIteratorsType inputIterators(this->InputPortal);
    auto resultPos = viskores::UpperBound(inputIterators.GetBegin(),
                                          inputIterators.GetEnd(),
                                          this->ValuesPortal.Get(index),
                                          this->CompareFunctor);

    viskores::Id resultIndex = IteratorDistance(inputIterators.GetBegin(), resultPos);
    this->OutputPortal.Set(index, resultIndex);
  }

  VISKORES_CONT
  void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer&) {}
};

template <typename InPortalType, typename OutPortalType, typename BinaryFunctor>
struct InclusiveToExclusiveKernel : viskores::exec::FunctorBase
{
  using ValueType = typename InPortalType::ValueType;

  InPortalType InPortal;
  OutPortalType OutPortal;
  BinaryFunctor BinaryOperator;
  ValueType InitialValue;

  VISKORES_CONT
  InclusiveToExclusiveKernel(const InPortalType& inPortal,
                             const OutPortalType& outPortal,
                             BinaryFunctor& binaryOperator,
                             ValueType initialValue)
    : InPortal(inPortal)
    , OutPortal(outPortal)
    , BinaryOperator(binaryOperator)
    , InitialValue(initialValue)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    const ValueType result = (index == 0)
      ? this->InitialValue
      : this->BinaryOperator(this->InitialValue, this->InPortal.Get(index - 1));

    this->OutPortal.Set(index, result);
  }
};

template <typename InPortalType, typename OutPortalType, typename BinaryFunctor>
struct InclusiveToExtendedKernel : viskores::exec::FunctorBase
{
  using ValueType = typename InPortalType::ValueType;

  InPortalType InPortal;
  OutPortalType OutPortal;
  BinaryFunctor BinaryOperator;
  ValueType InitialValue;
  ValueType FinalValue;

  VISKORES_CONT
  InclusiveToExtendedKernel(const InPortalType& inPortal,
                            const OutPortalType& outPortal,
                            BinaryFunctor& binaryOperator,
                            ValueType initialValue,
                            ValueType finalValue)
    : InPortal(inPortal)
    , OutPortal(outPortal)
    , BinaryOperator(binaryOperator)
    , InitialValue(initialValue)
    , FinalValue(finalValue)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    // The output array has one more value than the input, which holds the
    // total sum.
    const ValueType result = (index == 0) ? this->InitialValue
      : (index == this->InPortal.GetNumberOfValues())
      ? this->FinalValue
      : this->BinaryOperator(this->InitialValue, this->InPortal.Get(index - 1));

    this->OutPortal.Set(index, result);
  }
};

template <typename PortalType, typename BinaryFunctor>
struct ScanKernel : viskores::exec::FunctorBase
{
  PortalType Portal;
  BinaryFunctor BinaryOperator;
  viskores::Id Stride;
  viskores::Id Offset;
  viskores::Id Distance;

  VISKORES_CONT
  ScanKernel(const PortalType& portal,
             BinaryFunctor binary_functor,
             viskores::Id stride,
             viskores::Id offset)
    : Portal(portal)
    , BinaryOperator(binary_functor)
    , Stride(stride)
    , Offset(offset)
    , Distance(stride / 2)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    using ValueType = typename PortalType::ValueType;

    viskores::Id leftIndex = this->Offset + index * this->Stride;
    viskores::Id rightIndex = leftIndex + this->Distance;

    if (rightIndex < this->Portal.GetNumberOfValues())
    {
      ValueType leftValue = this->Portal.Get(leftIndex);
      ValueType rightValue = this->Portal.Get(rightIndex);
      this->Portal.Set(rightIndex, BinaryOperator(leftValue, rightValue));
    }
  }
};

template <typename InPortalType1,
          typename InPortalType2,
          typename OutPortalType,
          typename BinaryFunctor>
struct BinaryTransformKernel : viskores::exec::FunctorBase
{
  InPortalType1 InPortal1;
  InPortalType2 InPortal2;
  OutPortalType OutPortal;
  BinaryFunctor BinaryOperator;

  VISKORES_CONT
  BinaryTransformKernel(const InPortalType1& inPortal1,
                        const InPortalType2& inPortal2,
                        const OutPortalType& outPortal,
                        BinaryFunctor binaryOperator)
    : InPortal1(inPortal1)
    , InPortal2(inPortal2)
    , OutPortal(outPortal)
    , BinaryOperator(binaryOperator)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    this->OutPortal.Set(
      index, this->BinaryOperator(this->InPortal1.Get(index), this->InPortal2.Get(index)));
  }
};
}
}
} // namespace viskores::cont::internal

#endif //viskores_cont_internal_FunctorsGeneral_h
