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
#ifndef viskores_cont_cuda_internal_DeviceAdapterAlgorithmCuda_h
#define viskores_cont_cuda_internal_DeviceAdapterAlgorithmCuda_h

#include <viskores/Math.h>
#include <viskores/TypeTraits.h>
#include <viskores/Types.h>
#include <viskores/UnaryPredicates.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleMultiplexer.h>
#include <viskores/cont/BitField.h>
#include <viskores/cont/DeviceAdapterAlgorithm.h>
#include <viskores/cont/ErrorExecution.h>
#include <viskores/cont/Logging.h>
#include <viskores/cont/Token.h>
#include <viskores/cont/viskores_cont_export.h>

#include <viskores/cont/internal/DeviceAdapterAlgorithmGeneral.h>

#include <viskores/cont/cuda/ErrorCuda.h>
#include <viskores/cont/cuda/internal/DeviceAdapterRuntimeDetectorCuda.h>
#include <viskores/cont/cuda/internal/DeviceAdapterTagCuda.h>
#include <viskores/cont/cuda/internal/DeviceAdapterTimerImplementationCuda.h>
#include <viskores/cont/cuda/internal/MakeThrustIterator.h>
#include <viskores/cont/cuda/internal/ThrustExceptionHandler.h>
#include <viskores/exec/cuda/internal/WrappedOperators.h>

#include <viskores/exec/cuda/internal/TaskStrided.h>
#include <viskores/exec/internal/ErrorMessageBuffer.h>

// Disable warnings we check viskores for but Thrust does not.
#include <viskores/exec/cuda/internal/ThrustPatches.h>
VISKORES_THIRDPARTY_PRE_INCLUDE
//needs to be first
#include <viskores/exec/cuda/internal/ExecutionPolicy.h>

#include <cooperative_groups.h>
#include <cuda.h>
#include <thrust/advance.h>
#include <thrust/binary_search.h>
#include <thrust/copy.h>
#include <thrust/count.h>
#include <thrust/iterator/counting_iterator.h>
#include <thrust/scan.h>
#include <thrust/sort.h>
#include <thrust/system/cpp/memory.h>
#include <thrust/system/cuda/vector.h>
#include <thrust/unique.h>

VISKORES_THIRDPARTY_POST_INCLUDE

#include <limits>
#include <memory>

namespace viskores
{
namespace cont
{
namespace cuda
{

/// \brief Represents how to schedule 1D, 2D, and 3D Cuda kernels
///
/// \c ScheduleParameters represents how Viskores should schedule different
/// cuda kernel types. By default Viskores uses a preset table based on the
/// GPU's found at runtime.
///
/// When these defaults are insufficient for certain projects it is possible
/// to override the defaults by using \c InitScheduleParameters.
///
///
struct VISKORES_CONT_EXPORT ScheduleParameters
{
  int one_d_blocks;
  int one_d_threads_per_block;

  int two_d_blocks;
  dim3 two_d_threads_per_block;

  int three_d_blocks;
  dim3 three_d_threads_per_block;
};

/// \brief Specify the custom scheduling to use for Viskores CUDA kernel launches
///
/// By default Viskores uses a preset table based on the GPU's found at runtime to
/// determine the best scheduling parameters for a worklet.  When these defaults
/// are insufficient for certain projects it is possible to override the defaults
/// by binding a custom function to \c InitScheduleParameters.
///
/// Note: The this function must be called before any invocation of any worklets
/// by Viskores.
///
/// Note: This function will be called for each GPU on a machine.
///
/// \code{.cpp}
///
///  ScheduleParameters CustomScheduleValues(char const* name,
///                                          int major,
///                                          int minor,
///                                          int multiProcessorCount,
///                                          int maxThreadsPerMultiProcessor,
///                                          int maxThreadsPerBlock)
///  {
///
///    ScheduleParameters params  {
///        64 * multiProcessorCount,  //1d blocks
///        64,                        //1d threads per block
///        64 * multiProcessorCount,  //2d blocks
///        { 8, 8, 1 },               //2d threads per block
///        64 * multiProcessorCount,  //3d blocks
///        { 4, 4, 4 } };             //3d threads per block
///    return params;
///  }
/// \endcode
///
///
VISKORES_CONT_EXPORT void InitScheduleParameters(
  viskores::cont::cuda::ScheduleParameters (*)(char const* name,
                                               int major,
                                               int minor,
                                               int multiProcessorCount,
                                               int maxThreadsPerMultiProcessor,
                                               int maxThreadsPerBlock));

namespace internal
{

#if (defined(VISKORES_GCC) || defined(VISKORES_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

template <typename TaskType>
__global__ void TaskStrided1DLaunch(TaskType task, viskores::Id size)
{
  //see https://devblogs.nvidia.com/cuda-pro-tip-write-flexible-kernels-grid-stride-loops/
  //for why our inc is grid-stride
  const viskores::Id start = blockIdx.x * blockDim.x + threadIdx.x;
  const viskores::Id inc = blockDim.x * gridDim.x;
  task(start, size, inc);
}

template <typename TaskType>
__global__ void TaskStrided3DLaunch(TaskType task, viskores::Id3 size)
{
  //This is the 3D version of executing in a grid-stride manner
  const dim3 start(blockIdx.x * blockDim.x + threadIdx.x,
                   blockIdx.y * blockDim.y + threadIdx.y,
                   blockIdx.z * blockDim.z + threadIdx.z);
  const dim3 inc(blockDim.x * gridDim.x, blockDim.y * gridDim.y, blockDim.z * gridDim.z);

  for (viskores::Id k = start.z; k < size[2]; k += inc.z)
  {
    for (viskores::Id j = start.y; j < size[1]; j += inc.y)
    {
      task(size, start.x, size[0], inc.x, j, k);
    }
  }
}

template <typename T, typename BinaryOperationType>
__global__ void SumExclusiveScan(T a, T b, T result, BinaryOperationType binary_op)
{
  result = binary_op(a, b);
}

#if (defined(VISKORES_GCC) || defined(VISKORES_CLANG))
#pragma GCC diagnostic pop
#endif

template <typename FunctorType, typename ArgType>
struct FunctorSupportsUnaryImpl
{
  template <typename F, typename A, typename = decltype(std::declval<F>()(std::declval<A>()))>
  static std::true_type has(int);
  template <typename F, typename A>
  static std::false_type has(...);
  using type = decltype(has<FunctorType, ArgType>(0));
};
template <typename FunctorType, typename ArgType>
using FunctorSupportsUnary = typename FunctorSupportsUnaryImpl<FunctorType, ArgType>::type;

template <typename PortalType,
          typename BinaryAndUnaryFunctor,
          typename = FunctorSupportsUnary<BinaryAndUnaryFunctor, typename PortalType::ValueType>>
struct CastPortal;

template <typename PortalType, typename BinaryAndUnaryFunctor>
struct CastPortal<PortalType, BinaryAndUnaryFunctor, std::true_type>
{
  using InputType = typename PortalType::ValueType;
  using ValueType = decltype(std::declval<BinaryAndUnaryFunctor>()(std::declval<InputType>()));

  PortalType Portal;
  BinaryAndUnaryFunctor Functor;

  VISKORES_CONT
  CastPortal(const PortalType& portal, const BinaryAndUnaryFunctor& functor)
    : Portal(portal)
    , Functor(functor)
  {
  }

  VISKORES_EXEC
  viskores::Id GetNumberOfValues() const { return this->Portal.GetNumberOfValues(); }

  VISKORES_EXEC
  ValueType Get(viskores::Id index) const { return this->Functor(this->Portal.Get(index)); }
};

template <typename PortalType, typename BinaryFunctor>
struct CastPortal<PortalType, BinaryFunctor, std::false_type>
{
  using InputType = typename PortalType::ValueType;
  using ValueType =
    decltype(std::declval<BinaryFunctor>()(std::declval<InputType>(), std::declval<InputType>()));

  PortalType Portal;

  VISKORES_CONT
  CastPortal(const PortalType& portal, const BinaryFunctor&)
    : Portal(portal)
  {
  }

  VISKORES_EXEC
  viskores::Id GetNumberOfValues() const { return this->Portal.GetNumberOfValues(); }

  VISKORES_EXEC
  ValueType Get(viskores::Id index) const
  {
    return static_cast<ValueType>(this->Portal.Get(index));
  }
};

struct CudaFreeFunctor
{
  void operator()(void* ptr) const { VISKORES_CUDA_CALL(cudaFree(ptr)); }
};

template <typename T>
using CudaUniquePtr = std::unique_ptr<T, CudaFreeFunctor>;

template <typename T>
CudaUniquePtr<T> make_CudaUniquePtr(std::size_t numElements)
{
  T* ptr;
  VISKORES_CUDA_CALL(cudaMalloc(&ptr, sizeof(T) * numElements));
  return CudaUniquePtr<T>(ptr);
}
}
} // end namespace cuda::internal

template <>
struct DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagCuda>
  : viskores::cont::internal::DeviceAdapterAlgorithmGeneral<
      viskores::cont::DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagCuda>,
      viskores::cont::DeviceAdapterTagCuda>
{
// Because of some funny code conversions in nvcc, kernels for devices have to
// be public.
#ifndef VISKORES_CUDA
private:
#endif

  using Superclass = viskores::cont::internal::DeviceAdapterAlgorithmGeneral<
    viskores::cont::DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagCuda>,
    viskores::cont::DeviceAdapterTagCuda>;

  template <typename BitsPortal, typename IndicesPortal, typename GlobalPopCountType>
  struct BitFieldToUnorderedSetFunctor : public viskores::exec::FunctorBase
  {
    VISKORES_STATIC_ASSERT_MSG(
      VISKORES_PASS_COMMAS(std::is_same<GlobalPopCountType, viskores::Int32>::value ||
                           std::is_same<GlobalPopCountType, viskores::UInt32>::value ||
                           std::is_same<GlobalPopCountType, viskores::UInt64>::value),
      "Unsupported GlobalPopCountType. Must support CUDA atomicAdd.");

    //Using typename BitsPortal::WordTypePreferred causes dependent type errors using GCC 4.8.5
    //which is the GCC required compiler for CUDA 9.2 on summit/power9
    using Word = viskores::AtomicTypePreferred;

    VISKORES_STATIC_ASSERT(
      VISKORES_PASS_COMMAS(std::is_same<typename IndicesPortal::ValueType, viskores::Id>::value));

    VISKORES_CONT
    BitFieldToUnorderedSetFunctor(const BitsPortal& input,
                                  const IndicesPortal& output,
                                  GlobalPopCountType* globalPopCount)
      : Input{ input }
      , Output{ output }
      , GlobalPopCount{ globalPopCount }
      , FinalWordIndex{ input.GetNumberOfWords() - 1 }
      , FinalWordMask(input.GetFinalWordMask())
    {
    }

    ~BitFieldToUnorderedSetFunctor() {}

    VISKORES_CONT void Initialize()
    {
      assert(this->GlobalPopCount != nullptr);
      VISKORES_CUDA_CALL(cudaMemset(this->GlobalPopCount, 0, sizeof(GlobalPopCountType)));
    }

    VISKORES_SUPPRESS_EXEC_WARNINGS
    __device__ void operator()(viskores::Id wordIdx) const
    {
      Word word = this->Input.GetWord(wordIdx);

      // The last word may be partial -- mask out trailing bits if needed.
      const Word mask = wordIdx == this->FinalWordIndex ? this->FinalWordMask : ~Word{ 0 };

      word &= mask;

      if (word != 0)
      {
        this->LocalPopCount = viskores::CountSetBits(word);
        this->ReduceAllocate();

        viskores::Id firstBitIdx = wordIdx * sizeof(Word) * CHAR_BIT;
        do
        {
          // Find next bit. FindFirstSetBit's result is indexed starting at 1.
          viskores::Int32 bit = viskores::FindFirstSetBit(word) - 1;
          viskores::Id outIdx = this->GetNextOutputIndex();
          // Write index of bit
          this->Output.Set(outIdx, firstBitIdx + bit);
          word ^= (1 << bit); // clear bit
        } while (word != 0);  // have bits
      }
    }

    VISKORES_CONT viskores::Id Finalize() const
    {
      assert(this->GlobalPopCount != nullptr);
      GlobalPopCountType result;
      VISKORES_CUDA_CALL(cudaMemcpy(
        &result, this->GlobalPopCount, sizeof(GlobalPopCountType), cudaMemcpyDeviceToHost));
      return static_cast<viskores::Id>(result);
    }

  private:
    // Every thread with a non-zero local popcount calls this function, which
    // computes the total popcount for the coalesced threads and allocates
    // a contiguous block in the output by atomically increasing the global
    // popcount.
    VISKORES_SUPPRESS_EXEC_WARNINGS
    __device__ void ReduceAllocate() const
    {
      const auto activeLanes = cooperative_groups::coalesced_threads();
      const int activeRank = activeLanes.thread_rank();
      const int activeSize = activeLanes.size();

      // Reduction value:
      viskores::Int32 rVal = this->LocalPopCount;
      for (int delta = 1; delta < activeSize; delta *= 2)
      {
        const viskores::Int32 shflVal = activeLanes.shfl_down(rVal, delta);
        if (activeRank + delta < activeSize)
        {
          rVal += shflVal;
        }
      }

      if (activeRank == 0)
      {
        this->AllocationHead =
          atomicAdd(this->GlobalPopCount, static_cast<GlobalPopCountType>(rVal));
      }

      this->AllocationHead = activeLanes.shfl(this->AllocationHead, 0);
    }

    // The global output allocation is written to by striding the writes across
    // the warp lanes, allowing the writes to global memory to be coalesced.
    VISKORES_SUPPRESS_EXEC_WARNINGS
    __device__ viskores::Id GetNextOutputIndex() const
    {
      // Only lanes with unwritten output indices left will call this method,
      // so just check the coalesced threads:
      const auto activeLanes = cooperative_groups::coalesced_threads();
      const int activeRank = activeLanes.thread_rank();
      const int activeSize = activeLanes.size();

      viskores::Id nextIdx = static_cast<viskores::Id>(this->AllocationHead + activeRank);
      this->AllocationHead += activeSize;

      return nextIdx;
    }

    const BitsPortal Input;
    const IndicesPortal Output;
    GlobalPopCountType* GlobalPopCount;
    mutable viskores::UInt64 AllocationHead{ 0 };
    mutable viskores::Int32 LocalPopCount{ 0 };
    // Used to mask trailing bits the in last word.
    viskores::Id FinalWordIndex{ 0 };
    Word FinalWordMask{ 0 };
  };

  template <class InputPortal, class OutputPortal>
  VISKORES_CONT static void CopyPortal(const InputPortal& input, const OutputPortal& output)
  {
    try
    {
      ::thrust::copy(ThrustCudaPolicyPerThread,
                     cuda::internal::IteratorBegin(input),
                     cuda::internal::IteratorEnd(input),
                     cuda::internal::IteratorBegin(output));
    }
    catch (...)
    {
      cuda::internal::throwAsViskoresException();
    }
  }

  template <class ValueIterator, class StencilPortal, class OutputPortal, class UnaryPredicate>
  VISKORES_CONT static viskores::Id CopyIfPortal(ValueIterator valuesBegin,
                                                 ValueIterator valuesEnd,
                                                 StencilPortal stencil,
                                                 OutputPortal output,
                                                 UnaryPredicate unary_predicate)
  {
    auto outputBegin = cuda::internal::IteratorBegin(output);

    using ValueType = typename StencilPortal::ValueType;

    viskores::exec::cuda::internal::WrappedUnaryPredicate<ValueType, UnaryPredicate> up(
      unary_predicate);

    try
    {
      auto newLast = ::thrust::copy_if(ThrustCudaPolicyPerThread,
                                       valuesBegin,
                                       valuesEnd,
                                       cuda::internal::IteratorBegin(stencil),
                                       outputBegin,
                                       up);
      return static_cast<viskores::Id>(::thrust::distance(outputBegin, newLast));
    }
    catch (...)
    {
      cuda::internal::throwAsViskoresException();
      return viskores::Id(0);
    }
  }

  template <class ValuePortal, class StencilPortal, class OutputPortal, class UnaryPredicate>
  VISKORES_CONT static viskores::Id CopyIfPortal(ValuePortal values,
                                                 StencilPortal stencil,
                                                 OutputPortal output,
                                                 UnaryPredicate unary_predicate)
  {
    return CopyIfPortal(cuda::internal::IteratorBegin(values),
                        cuda::internal::IteratorEnd(values),
                        stencil,
                        output,
                        unary_predicate);
  }

  template <class InputPortal, class OutputPortal>
  VISKORES_CONT static void CopySubRangePortal(const InputPortal& input,
                                               viskores::Id inputOffset,
                                               viskores::Id size,
                                               const OutputPortal& output,
                                               viskores::Id outputOffset)
  {
    try
    {
      ::thrust::copy_n(ThrustCudaPolicyPerThread,
                       cuda::internal::IteratorBegin(input) + inputOffset,
                       static_cast<std::size_t>(size),
                       cuda::internal::IteratorBegin(output) + outputOffset);
    }
    catch (...)
    {
      cuda::internal::throwAsViskoresException();
    }
  }


  template <typename BitsPortal, typename GlobalPopCountType>
  struct CountSetBitsFunctor : public viskores::exec::FunctorBase
  {
    VISKORES_STATIC_ASSERT_MSG(
      VISKORES_PASS_COMMAS(std::is_same<GlobalPopCountType, viskores::Int32>::value ||
                           std::is_same<GlobalPopCountType, viskores::UInt32>::value ||
                           std::is_same<GlobalPopCountType, viskores::UInt64>::value),
      "Unsupported GlobalPopCountType. Must support CUDA atomicAdd.");

    //Using typename BitsPortal::WordTypePreferred causes dependent type errors using GCC 4.8.5
    //which is the GCC required compiler for CUDA 9.2 on summit/power9
    using Word = viskores::AtomicTypePreferred;

    VISKORES_CONT
    CountSetBitsFunctor(const BitsPortal& portal, GlobalPopCountType* globalPopCount)
      : Portal{ portal }
      , GlobalPopCount{ globalPopCount }
      , FinalWordIndex{ portal.GetNumberOfWords() - 1 }
      , FinalWordMask{ portal.GetFinalWordMask() }
    {
    }

    ~CountSetBitsFunctor() {}

    VISKORES_CONT void Initialize()
    {
      assert(this->GlobalPopCount != nullptr);
      VISKORES_CUDA_CALL(cudaMemset(this->GlobalPopCount, 0, sizeof(GlobalPopCountType)));
    }

    VISKORES_SUPPRESS_EXEC_WARNINGS
    __device__ void operator()(viskores::Id wordIdx) const
    {
      Word word = this->Portal.GetWord(wordIdx);

      // The last word may be partial -- mask out trailing bits if needed.
      const Word mask = wordIdx == this->FinalWordIndex ? this->FinalWordMask : ~Word{ 0 };

      word &= mask;

      if (word != 0)
      {
        this->LocalPopCount = viskores::CountSetBits(word);
        this->Reduce();
      }
    }

    VISKORES_CONT viskores::Id Finalize() const
    {
      assert(this->GlobalPopCount != nullptr);
      GlobalPopCountType result;
      VISKORES_CUDA_CALL(cudaMemcpy(
        &result, this->GlobalPopCount, sizeof(GlobalPopCountType), cudaMemcpyDeviceToHost));
      return static_cast<viskores::Id>(result);
    }

  private:
    // Every thread with a non-zero local popcount calls this function, which
    // computes the total popcount for the coalesced threads and atomically
    // increasing the global popcount.
    VISKORES_SUPPRESS_EXEC_WARNINGS
    __device__ void Reduce() const
    {
      const auto activeLanes = cooperative_groups::coalesced_threads();
      const int activeRank = activeLanes.thread_rank();
      const int activeSize = activeLanes.size();

      // Reduction value:
      viskores::Int32 rVal = this->LocalPopCount;
      for (int delta = 1; delta < activeSize; delta *= 2)
      {
        const viskores::Int32 shflVal = activeLanes.shfl_down(rVal, delta);
        if (activeRank + delta < activeSize)
        {
          rVal += shflVal;
        }
      }

      if (activeRank == 0)
      {
        atomicAdd(this->GlobalPopCount, static_cast<GlobalPopCountType>(rVal));
      }
    }

    const BitsPortal Portal;
    GlobalPopCountType* GlobalPopCount;
    mutable viskores::Int32 LocalPopCount{ 0 };
    // Used to mask trailing bits the in last word.
    viskores::Id FinalWordIndex{ 0 };
    Word FinalWordMask{ 0 };
  };

  template <class InputPortal, class ValuesPortal, class OutputPortal>
  VISKORES_CONT static void LowerBoundsPortal(const InputPortal& input,
                                              const ValuesPortal& values,
                                              const OutputPortal& output)
  {
    using ValueType = typename ValuesPortal::ValueType;
    LowerBoundsPortal(input, values, output, ::thrust::less<ValueType>());
  }

  template <class InputPortal, class OutputPortal>
  VISKORES_CONT static void LowerBoundsPortal(const InputPortal& input,
                                              const OutputPortal& values_output)
  {
    using ValueType = typename InputPortal::ValueType;
    LowerBoundsPortal(input, values_output, values_output, ::thrust::less<ValueType>());
  }

  template <class InputPortal, class ValuesPortal, class OutputPortal, class BinaryCompare>
  VISKORES_CONT static void LowerBoundsPortal(const InputPortal& input,
                                              const ValuesPortal& values,
                                              const OutputPortal& output,
                                              BinaryCompare binary_compare)
  {
    using ValueType = typename InputPortal::ValueType;
    viskores::exec::cuda::internal::WrappedBinaryPredicate<ValueType, BinaryCompare> bop(
      binary_compare);

    try
    {
      ::thrust::lower_bound(ThrustCudaPolicyPerThread,
                            cuda::internal::IteratorBegin(input),
                            cuda::internal::IteratorEnd(input),
                            cuda::internal::IteratorBegin(values),
                            cuda::internal::IteratorEnd(values),
                            cuda::internal::IteratorBegin(output),
                            bop);
    }
    catch (...)
    {
      cuda::internal::throwAsViskoresException();
    }
  }

  template <class InputPortal, typename T>
  VISKORES_CONT static T ReducePortal(const InputPortal& input, T initialValue)
  {
    return ReducePortal(input, initialValue, ::thrust::plus<T>());
  }

  template <class InputPortal, typename T, class BinaryFunctor>
  VISKORES_CONT static T ReducePortal(const InputPortal& input,
                                      T initialValue,
                                      BinaryFunctor binary_functor)
  {
    using fast_path = std::is_same<typename InputPortal::ValueType, T>;
    return ReducePortalImpl(input, initialValue, binary_functor, fast_path());
  }

  template <class InputPortal, typename T, class BinaryFunctor>
  VISKORES_CONT static T ReducePortalImpl(const InputPortal& input,
                                          T initialValue,
                                          BinaryFunctor binary_functor,
                                          std::true_type)
  {
    //The portal type and the initial value are the same so we can use
    //the thrust reduction algorithm
    viskores::exec::cuda::internal::WrappedBinaryOperator<T, BinaryFunctor> bop(binary_functor);

    try
    {
      return ::thrust::reduce(ThrustCudaPolicyPerThread,
                              cuda::internal::IteratorBegin(input),
                              cuda::internal::IteratorEnd(input),
                              initialValue,
                              bop);
    }
    catch (...)
    {
      cuda::internal::throwAsViskoresException();
    }

    return initialValue;
  }

  template <class InputPortal, typename T, class BinaryFunctor>
  VISKORES_CONT static T ReducePortalImpl(const InputPortal& input,
                                          T initialValue,
                                          BinaryFunctor binary_functor,
                                          std::false_type)
  {
    //The portal type and the initial value AREN'T the same type so we have
    //to a slower approach, where we wrap the input portal inside a cast
    //portal
    viskores::cont::cuda::internal::CastPortal<InputPortal, BinaryFunctor> castPortal(
      input, binary_functor);

    viskores::exec::cuda::internal::WrappedBinaryOperator<T, BinaryFunctor> bop(binary_functor);

    try
    {
      return ::thrust::reduce(ThrustCudaPolicyPerThread,
                              cuda::internal::IteratorBegin(castPortal),
                              cuda::internal::IteratorEnd(castPortal),
                              initialValue,
                              bop);
    }
    catch (...)
    {
      cuda::internal::throwAsViskoresException();
    }

    return initialValue;
  }

  template <class KeysPortal,
            class ValuesPortal,
            class KeysOutputPortal,
            class ValueOutputPortal,
            class BinaryFunctor>
  VISKORES_CONT static viskores::Id ReduceByKeyPortal(const KeysPortal& keys,
                                                      const ValuesPortal& values,
                                                      const KeysOutputPortal& keys_output,
                                                      const ValueOutputPortal& values_output,
                                                      BinaryFunctor binary_functor)
  {
    auto keys_out_begin = cuda::internal::IteratorBegin(keys_output);
    auto values_out_begin = cuda::internal::IteratorBegin(values_output);

    ::thrust::pair<decltype(keys_out_begin), decltype(values_out_begin)> result_iterators;

    ::thrust::equal_to<typename KeysPortal::ValueType> binaryPredicate;

    using ValueType = typename ValuesPortal::ValueType;
    viskores::exec::cuda::internal::WrappedBinaryOperator<ValueType, BinaryFunctor> bop(
      binary_functor);

    try
    {
      result_iterators = ::thrust::reduce_by_key(viskores_cuda_policy(),
                                                 cuda::internal::IteratorBegin(keys),
                                                 cuda::internal::IteratorEnd(keys),
                                                 cuda::internal::IteratorBegin(values),
                                                 keys_out_begin,
                                                 values_out_begin,
                                                 binaryPredicate,
                                                 bop);
    }
    catch (...)
    {
      cuda::internal::throwAsViskoresException();
    }

    return static_cast<viskores::Id>(::thrust::distance(keys_out_begin, result_iterators.first));
  }

  template <class InputPortal, class OutputPortal>
  VISKORES_CONT static typename InputPortal::ValueType ScanExclusivePortal(
    const InputPortal& input,
    const OutputPortal& output)
  {
    using ValueType = typename OutputPortal::ValueType;

    return ScanExclusivePortal(input,
                               output,
                               (::thrust::plus<ValueType>()),
                               viskores::TypeTraits<ValueType>::ZeroInitialization());
  }

  template <class InputPortal, class OutputPortal, class BinaryFunctor>
  VISKORES_CONT static typename InputPortal::ValueType ScanExclusivePortal(
    const InputPortal& input,
    const OutputPortal& output,
    BinaryFunctor binaryOp,
    typename InputPortal::ValueType initialValue)
  {
    // Use iterator to get value so that thrust device_ptr has chance to handle
    // data on device.
    using ValueType = typename OutputPortal::ValueType;

    //we have size three so that we can store the origin end value, the
    //new end value, and the sum of those two
    ::thrust::system::cuda::vector<ValueType> sum(3);
    try
    {

      //store the current value of the last position array in a separate cuda
      //memory location since the exclusive_scan will overwrite that value
      //once run
      ::thrust::copy_n(
        ThrustCudaPolicyPerThread, cuda::internal::IteratorEnd(input) - 1, 1, sum.begin());

      viskores::exec::cuda::internal::WrappedBinaryOperator<ValueType, BinaryFunctor> bop(binaryOp);

      auto end = ::thrust::exclusive_scan(ThrustCudaPolicyPerThread,
                                          cuda::internal::IteratorBegin(input),
                                          cuda::internal::IteratorEnd(input),
                                          cuda::internal::IteratorBegin(output),
                                          initialValue,
                                          bop);

      //Store the new value for the end of the array. This is done because
      //with items such as the transpose array it is unsafe to pass the
      //portal to the SumExclusiveScan
      ::thrust::copy_n(ThrustCudaPolicyPerThread, (end - 1), 1, sum.begin() + 1);

      //execute the binaryOp one last time on the device.
      cuda::internal::SumExclusiveScan<<<1, 1, 0, cudaStreamPerThread>>>(
        sum[0], sum[1], sum[2], bop);
    }
    catch (...)
    {
      cuda::internal::throwAsViskoresException();
    }
    return sum[2];
  }

  template <class InputPortal, class OutputPortal>
  VISKORES_CONT static typename InputPortal::ValueType ScanInclusivePortal(
    const InputPortal& input,
    const OutputPortal& output)
  {
    using ValueType = typename OutputPortal::ValueType;
    return ScanInclusivePortal(input, output, ::thrust::plus<ValueType>());
  }

  template <class InputPortal, class OutputPortal, class BinaryFunctor>
  VISKORES_CONT static typename InputPortal::ValueType ScanInclusivePortal(
    const InputPortal& input,
    const OutputPortal& output,
    BinaryFunctor binary_functor)
  {
    using ValueType = typename OutputPortal::ValueType;
    viskores::exec::cuda::internal::WrappedBinaryOperator<ValueType, BinaryFunctor> bop(
      binary_functor);

    try
    {
      ::thrust::system::cuda::vector<ValueType> result(1);
      auto end = ::thrust::inclusive_scan(ThrustCudaPolicyPerThread,
                                          cuda::internal::IteratorBegin(input),
                                          cuda::internal::IteratorEnd(input),
                                          cuda::internal::IteratorBegin(output),
                                          bop);

      ::thrust::copy_n(ThrustCudaPolicyPerThread, end - 1, 1, result.begin());
      return result[0];
    }
    catch (...)
    {
      cuda::internal::throwAsViskoresException();
      return typename InputPortal::ValueType();
    }

    //return the value at the last index in the array, as that is the sum
  }

  template <typename KeysPortal, typename ValuesPortal, typename OutputPortal>
  VISKORES_CONT static void ScanInclusiveByKeyPortal(const KeysPortal& keys,
                                                     const ValuesPortal& values,
                                                     const OutputPortal& output)
  {
    using KeyType = typename KeysPortal::ValueType;
    using ValueType = typename OutputPortal::ValueType;
    ScanInclusiveByKeyPortal(
      keys, values, output, ::thrust::equal_to<KeyType>(), ::thrust::plus<ValueType>());
  }

  template <typename KeysPortal,
            typename ValuesPortal,
            typename OutputPortal,
            typename BinaryPredicate,
            typename AssociativeOperator>
  VISKORES_CONT static void ScanInclusiveByKeyPortal(const KeysPortal& keys,
                                                     const ValuesPortal& values,
                                                     const OutputPortal& output,
                                                     BinaryPredicate binary_predicate,
                                                     AssociativeOperator binary_operator)
  {
    using KeyType = typename KeysPortal::ValueType;
    viskores::exec::cuda::internal::WrappedBinaryOperator<KeyType, BinaryPredicate> bpred(
      binary_predicate);
    using ValueType = typename OutputPortal::ValueType;
    viskores::exec::cuda::internal::WrappedBinaryOperator<ValueType, AssociativeOperator> bop(
      binary_operator);

    try
    {
      ::thrust::inclusive_scan_by_key(ThrustCudaPolicyPerThread,
                                      cuda::internal::IteratorBegin(keys),
                                      cuda::internal::IteratorEnd(keys),
                                      cuda::internal::IteratorBegin(values),
                                      cuda::internal::IteratorBegin(output),
                                      bpred,
                                      bop);
    }
    catch (...)
    {
      cuda::internal::throwAsViskoresException();
    }
  }

  template <typename KeysPortal, typename ValuesPortal, typename OutputPortal>
  VISKORES_CONT static void ScanExclusiveByKeyPortal(const KeysPortal& keys,
                                                     const ValuesPortal& values,
                                                     const OutputPortal& output)
  {
    using KeyType = typename KeysPortal::ValueType;
    using ValueType = typename OutputPortal::ValueType;
    ScanExclusiveByKeyPortal(keys,
                             values,
                             output,
                             viskores::TypeTraits<ValueType>::ZeroInitialization(),
                             ::thrust::equal_to<KeyType>(),
                             ::thrust::plus<ValueType>());
  }

  template <typename KeysPortal,
            typename ValuesPortal,
            typename OutputPortal,
            typename T,
            typename BinaryPredicate,
            typename AssociativeOperator>
  VISKORES_CONT static void ScanExclusiveByKeyPortal(const KeysPortal& keys,
                                                     const ValuesPortal& values,
                                                     const OutputPortal& output,
                                                     T initValue,
                                                     BinaryPredicate binary_predicate,
                                                     AssociativeOperator binary_operator)
  {
    using KeyType = typename KeysPortal::ValueType;
    viskores::exec::cuda::internal::WrappedBinaryOperator<KeyType, BinaryPredicate> bpred(
      binary_predicate);
    using ValueType = typename OutputPortal::ValueType;
    viskores::exec::cuda::internal::WrappedBinaryOperator<ValueType, AssociativeOperator> bop(
      binary_operator);
    try
    {
      ::thrust::exclusive_scan_by_key(ThrustCudaPolicyPerThread,
                                      cuda::internal::IteratorBegin(keys),
                                      cuda::internal::IteratorEnd(keys),
                                      cuda::internal::IteratorBegin(values),
                                      cuda::internal::IteratorBegin(output),
                                      initValue,
                                      bpred,
                                      bop);
    }
    catch (...)
    {
      cuda::internal::throwAsViskoresException();
    }
  }

  template <class ValuesPortal>
  VISKORES_CONT static void SortPortal(const ValuesPortal& values)
  {
    using ValueType = typename ValuesPortal::ValueType;
    SortPortal(values, ::thrust::less<ValueType>());
  }

  template <class ValuesPortal, class BinaryCompare>
  VISKORES_CONT static void SortPortal(const ValuesPortal& values, BinaryCompare binary_compare)
  {
    using ValueType = typename ValuesPortal::ValueType;
    viskores::exec::cuda::internal::WrappedBinaryPredicate<ValueType, BinaryCompare> bop(
      binary_compare);
    try
    {
      ::thrust::sort(viskores_cuda_policy(),
                     cuda::internal::IteratorBegin(values),
                     cuda::internal::IteratorEnd(values),
                     bop);
    }
    catch (...)
    {
      cuda::internal::throwAsViskoresException();
    }
  }

  template <class KeysPortal, class ValuesPortal>
  VISKORES_CONT static void SortByKeyPortal(const KeysPortal& keys, const ValuesPortal& values)
  {
    using ValueType = typename KeysPortal::ValueType;
    SortByKeyPortal(keys, values, ::thrust::less<ValueType>());
  }

  template <class KeysPortal, class ValuesPortal, class BinaryCompare>
  VISKORES_CONT static void SortByKeyPortal(const KeysPortal& keys,
                                            const ValuesPortal& values,
                                            BinaryCompare binary_compare)
  {
    using ValueType = typename KeysPortal::ValueType;
    viskores::exec::cuda::internal::WrappedBinaryPredicate<ValueType, BinaryCompare> bop(
      binary_compare);
    try
    {
      ::thrust::sort_by_key(viskores_cuda_policy(),
                            cuda::internal::IteratorBegin(keys),
                            cuda::internal::IteratorEnd(keys),
                            cuda::internal::IteratorBegin(values),
                            bop);
    }
    catch (...)
    {
      cuda::internal::throwAsViskoresException();
    }
  }

  template <class ValuesPortal>
  VISKORES_CONT static viskores::Id UniquePortal(const ValuesPortal values)
  {
    try
    {
      auto begin = cuda::internal::IteratorBegin(values);
      auto newLast =
        ::thrust::unique(ThrustCudaPolicyPerThread, begin, cuda::internal::IteratorEnd(values));
      return static_cast<viskores::Id>(::thrust::distance(begin, newLast));
    }
    catch (...)
    {
      cuda::internal::throwAsViskoresException();
      return viskores::Id(0);
    }
  }

  template <class ValuesPortal, class BinaryCompare>
  VISKORES_CONT static viskores::Id UniquePortal(const ValuesPortal values,
                                                 BinaryCompare binary_compare)
  {
    using ValueType = typename ValuesPortal::ValueType;
    viskores::exec::cuda::internal::WrappedBinaryPredicate<ValueType, BinaryCompare> bop(
      binary_compare);
    try
    {
      auto begin = cuda::internal::IteratorBegin(values);
      auto newLast = ::thrust::unique(
        ThrustCudaPolicyPerThread, begin, cuda::internal::IteratorEnd(values), bop);
      return static_cast<viskores::Id>(::thrust::distance(begin, newLast));
    }
    catch (...)
    {
      cuda::internal::throwAsViskoresException();
      return viskores::Id(0);
    }
  }

  template <class InputPortal, class ValuesPortal, class OutputPortal>
  VISKORES_CONT static void UpperBoundsPortal(const InputPortal& input,
                                              const ValuesPortal& values,
                                              const OutputPortal& output)
  {
    try
    {
      ::thrust::upper_bound(ThrustCudaPolicyPerThread,
                            cuda::internal::IteratorBegin(input),
                            cuda::internal::IteratorEnd(input),
                            cuda::internal::IteratorBegin(values),
                            cuda::internal::IteratorEnd(values),
                            cuda::internal::IteratorBegin(output));
    }
    catch (...)
    {
      cuda::internal::throwAsViskoresException();
    }
  }

  template <class InputPortal, class ValuesPortal, class OutputPortal, class BinaryCompare>
  VISKORES_CONT static void UpperBoundsPortal(const InputPortal& input,
                                              const ValuesPortal& values,
                                              const OutputPortal& output,
                                              BinaryCompare binary_compare)
  {
    using ValueType = typename OutputPortal::ValueType;

    viskores::exec::cuda::internal::WrappedBinaryPredicate<ValueType, BinaryCompare> bop(
      binary_compare);
    try
    {
      ::thrust::upper_bound(ThrustCudaPolicyPerThread,
                            cuda::internal::IteratorBegin(input),
                            cuda::internal::IteratorEnd(input),
                            cuda::internal::IteratorBegin(values),
                            cuda::internal::IteratorEnd(values),
                            cuda::internal::IteratorBegin(output),
                            bop);
    }
    catch (...)
    {
      cuda::internal::throwAsViskoresException();
    }
  }

  template <class InputPortal, class OutputPortal>
  VISKORES_CONT static void UpperBoundsPortal(const InputPortal& input,
                                              const OutputPortal& values_output)
  {
    try
    {
      ::thrust::upper_bound(ThrustCudaPolicyPerThread,
                            cuda::internal::IteratorBegin(input),
                            cuda::internal::IteratorEnd(input),
                            cuda::internal::IteratorBegin(values_output),
                            cuda::internal::IteratorEnd(values_output),
                            cuda::internal::IteratorBegin(values_output));
    }
    catch (...)
    {
      cuda::internal::throwAsViskoresException();
    }
  }

  template <typename GlobalPopCountType, typename BitsPortal, typename IndicesPortal>
  VISKORES_CONT static viskores::Id BitFieldToUnorderedSetPortal(const BitsPortal& bits,
                                                                 const IndicesPortal& indices)
  {
    using Functor = BitFieldToUnorderedSetFunctor<BitsPortal, IndicesPortal, GlobalPopCountType>;

    // RAII for the global atomic counter.
    auto globalCount = cuda::internal::make_CudaUniquePtr<GlobalPopCountType>(1);
    Functor functor{ bits, indices, globalCount.get() };

    functor.Initialize();
    Schedule(functor, bits.GetNumberOfWords());
    Synchronize(); // Ensure kernel is done before checking final atomic count
    return functor.Finalize();
  }

  template <typename GlobalPopCountType, typename BitsPortal>
  VISKORES_CONT static viskores::Id CountSetBitsPortal(const BitsPortal& bits)
  {
    using Functor = CountSetBitsFunctor<BitsPortal, GlobalPopCountType>;

    // RAII for the global atomic counter.
    auto globalCount = cuda::internal::make_CudaUniquePtr<GlobalPopCountType>(1);
    Functor functor{ bits, globalCount.get() };

    functor.Initialize();
    Schedule(functor, bits.GetNumberOfWords());
    Synchronize(); // Ensure kernel is done before checking final atomic count
    return functor.Finalize();
  }

  //-----------------------------------------------------------------------------

public:
  template <typename IndicesStorage>
  VISKORES_CONT static viskores::Id BitFieldToUnorderedSet(
    const viskores::cont::BitField& bits,
    viskores::cont::ArrayHandle<Id, IndicesStorage>& indices)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::Id numBits = bits.GetNumberOfBits();

    {
      viskores::cont::Token token;
      auto bitsPortal = bits.PrepareForInput(DeviceAdapterTagCuda{}, token);
      auto indicesPortal = indices.PrepareForOutput(numBits, DeviceAdapterTagCuda{}, token);

      // Use a uint64 for accumulator, as atomicAdd does not support signed int64.
      numBits = BitFieldToUnorderedSetPortal<viskores::UInt64>(bitsPortal, indicesPortal);
    }

    indices.Allocate(numBits, viskores::CopyFlag::On);
    return numBits;
  }

  template <typename T, typename U, class SIn, class SOut>
  VISKORES_CONT static void Copy(const viskores::cont::ArrayHandle<T, SIn>& input,
                                 viskores::cont::ArrayHandle<U, SOut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    const viskores::Id inSize = input.GetNumberOfValues();
    if (inSize <= 0)
    {
      output.Allocate(inSize, viskores::CopyFlag::On);
      return;
    }
    viskores::cont::Token token;
    CopyPortal(input.PrepareForInput(DeviceAdapterTagCuda(), token),
               output.PrepareForOutput(inSize, DeviceAdapterTagCuda(), token));
  }

  template <typename T, typename U, class SIn, class SStencil, class SOut>
  VISKORES_CONT static void CopyIf(const viskores::cont::ArrayHandle<U, SIn>& input,
                                   const viskores::cont::ArrayHandle<T, SStencil>& stencil,
                                   viskores::cont::ArrayHandle<U, SOut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::Id size = stencil.GetNumberOfValues();
    if (size <= 0)
    {
      output.Allocate(size, viskores::CopyFlag::On);
      return;
    }

    viskores::Id newSize;

    {
      viskores::cont::Token token;

      newSize = CopyIfPortal(input.PrepareForInput(DeviceAdapterTagCuda(), token),
                             stencil.PrepareForInput(DeviceAdapterTagCuda(), token),
                             output.PrepareForOutput(size, DeviceAdapterTagCuda(), token),
                             ::viskores::NotZeroInitialized()); //yes on the stencil
    }

    output.Allocate(newSize, viskores::CopyFlag::On);
  }

  template <typename T, typename U, class SIn, class SStencil, class SOut, class UnaryPredicate>
  VISKORES_CONT static void CopyIf(const viskores::cont::ArrayHandle<U, SIn>& input,
                                   const viskores::cont::ArrayHandle<T, SStencil>& stencil,
                                   viskores::cont::ArrayHandle<U, SOut>& output,
                                   UnaryPredicate unary_predicate)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::Id size = stencil.GetNumberOfValues();
    if (size <= 0)
    {
      output.Allocate(size, viskores::CopyFlag::On);
      return;
    }

    viskores::Id newSize;

    {
      viskores::cont::Token token;
      newSize = CopyIfPortal(input.PrepareForInput(DeviceAdapterTagCuda(), token),
                             stencil.PrepareForInput(DeviceAdapterTagCuda(), token),
                             output.PrepareForOutput(size, DeviceAdapterTagCuda(), token),
                             unary_predicate);
    }

    output.Allocate(newSize, viskores::CopyFlag::On);
  }

  template <typename T, typename U, class SIn, class SOut>
  VISKORES_CONT static bool CopySubRange(const viskores::cont::ArrayHandle<T, SIn>& input,
                                         viskores::Id inputStartIndex,
                                         viskores::Id numberOfElementsToCopy,
                                         viskores::cont::ArrayHandle<U, SOut>& output,
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
        viskores::cont::ArrayHandle<U, SOut> temp;
        temp.Allocate(copyOutEnd);
        CopySubRange(output, 0, outSize, temp);
        output = temp;
      }
    }
    viskores::cont::Token token;
    CopySubRangePortal(input.PrepareForInput(DeviceAdapterTagCuda(), token),
                       inputStartIndex,
                       numberOfElementsToCopy,
                       output.PrepareForInPlace(DeviceAdapterTagCuda(), token),
                       outputIndex);
    return true;
  }

  VISKORES_CONT static viskores::Id CountSetBits(const viskores::cont::BitField& bits)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);
    viskores::cont::Token token;
    auto bitsPortal = bits.PrepareForInput(DeviceAdapterTagCuda{}, token);
    // Use a uint64 for accumulator, as atomicAdd does not support signed int64.
    return CountSetBitsPortal<viskores::UInt64>(bitsPortal);
  }

  template <typename T, class SIn, class SVal, class SOut>
  VISKORES_CONT static void LowerBounds(const viskores::cont::ArrayHandle<T, SIn>& input,
                                        const viskores::cont::ArrayHandle<T, SVal>& values,
                                        viskores::cont::ArrayHandle<viskores::Id, SOut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::Id numberOfValues = values.GetNumberOfValues();
    viskores::cont::Token token;
    LowerBoundsPortal(input.PrepareForInput(DeviceAdapterTagCuda(), token),
                      values.PrepareForInput(DeviceAdapterTagCuda(), token),
                      output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda(), token));
  }

  template <typename T, class SIn, class SVal, class SOut, class BinaryCompare>
  VISKORES_CONT static void LowerBounds(const viskores::cont::ArrayHandle<T, SIn>& input,
                                        const viskores::cont::ArrayHandle<T, SVal>& values,
                                        viskores::cont::ArrayHandle<viskores::Id, SOut>& output,
                                        BinaryCompare binary_compare)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::Id numberOfValues = values.GetNumberOfValues();
    viskores::cont::Token token;
    LowerBoundsPortal(input.PrepareForInput(DeviceAdapterTagCuda(), token),
                      values.PrepareForInput(DeviceAdapterTagCuda(), token),
                      output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda(), token),
                      binary_compare);
  }

  template <class SIn, class SOut>
  VISKORES_CONT static void LowerBounds(
    const viskores::cont::ArrayHandle<viskores::Id, SIn>& input,
    viskores::cont::ArrayHandle<viskores::Id, SOut>& values_output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::cont::Token token;
    LowerBoundsPortal(input.PrepareForInput(DeviceAdapterTagCuda(), token),
                      values_output.PrepareForInPlace(DeviceAdapterTagCuda(), token));
  }

  template <typename T, typename U, class SIn>
  VISKORES_CONT static U Reduce(const viskores::cont::ArrayHandle<T, SIn>& input, U initialValue)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    const viskores::Id numberOfValues = input.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      return initialValue;
    }
    viskores::cont::Token token;
    return ReducePortal(input.PrepareForInput(DeviceAdapterTagCuda(), token), initialValue);
  }

  template <typename T, typename U, class SIn, class BinaryFunctor>
  VISKORES_CONT static U Reduce(const viskores::cont::ArrayHandle<T, SIn>& input,
                                U initialValue,
                                BinaryFunctor binary_functor)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    const viskores::Id numberOfValues = input.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      return initialValue;
    }
    viskores::cont::Token token;
    return ReducePortal(
      input.PrepareForInput(DeviceAdapterTagCuda(), token), initialValue, binary_functor);
  }

  // At least some versions of Thrust/nvcc result in compile errors when calling Thrust's
  // reduce with sufficiently complex iterators, which can happen with some versions of
  // ArrayHandleMultiplexer. Thus, don't use the Thrust version for ArrayHandleMultiplexer.
  template <typename T, typename U, typename... SIns>
  VISKORES_CONT static U Reduce(
    const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagMultiplexer<SIns...>>& input,
    U initialValue)
  {
    return Superclass::Reduce(input, initialValue);
  }
  template <typename T, typename U, typename BinaryFunctor, typename... SIns>
  VISKORES_CONT static U Reduce(
    const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagMultiplexer<SIns...>>& input,
    U initialValue,
    BinaryFunctor binary_functor)
  {
    return Superclass::Reduce(input, initialValue, binary_functor);
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

    //there is a concern that by default we will allocate too much
    //space for the keys/values output. 1 option is to
    const viskores::Id numberOfValues = keys.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      return;
    }

    viskores::Id reduced_size;
    {
      viskores::cont::Token token;
      reduced_size = ReduceByKeyPortal(
        keys.PrepareForInput(DeviceAdapterTagCuda(), token),
        values.PrepareForInput(DeviceAdapterTagCuda(), token),
        keys_output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda(), token),
        values_output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda(), token),
        binary_functor);
    }

    keys_output.Allocate(reduced_size, viskores::CopyFlag::On);
    values_output.Allocate(reduced_size, viskores::CopyFlag::On);
  }

  template <typename T, class SIn, class SOut>
  VISKORES_CONT static T ScanExclusive(const viskores::cont::ArrayHandle<T, SIn>& input,
                                       viskores::cont::ArrayHandle<T, SOut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    const viskores::Id numberOfValues = input.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      output.Allocate(0);
      return viskores::TypeTraits<T>::ZeroInitialization();
    }

    //We need call PrepareForInput on the input argument before invoking a
    //function. The order of execution of parameters of a function is undefined
    //so we need to make sure input is called before output, or else in-place
    //use case breaks.
    viskores::cont::Token token;
    auto inputPortal = input.PrepareForInput(DeviceAdapterTagCuda(), token);
    return ScanExclusivePortal(
      inputPortal, output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda(), token));
  }

  template <typename T, class SIn, class SOut, class BinaryFunctor>
  VISKORES_CONT static T ScanExclusive(const viskores::cont::ArrayHandle<T, SIn>& input,
                                       viskores::cont::ArrayHandle<T, SOut>& output,
                                       BinaryFunctor binary_functor,
                                       const T& initialValue)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    const viskores::Id numberOfValues = input.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      output.Allocate(0);
      return viskores::TypeTraits<T>::ZeroInitialization();
    }

    //We need call PrepareForInput on the input argument before invoking a
    //function. The order of execution of parameters of a function is undefined
    //so we need to make sure input is called before output, or else in-place
    //use case breaks.
    viskores::cont::Token token;
    auto inputPortal = input.PrepareForInput(DeviceAdapterTagCuda(), token);
    return ScanExclusivePortal(
      inputPortal,
      output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda(), token),
      binary_functor,
      initialValue);
  }

  template <typename T, class SIn, class SOut>
  VISKORES_CONT static T ScanInclusive(const viskores::cont::ArrayHandle<T, SIn>& input,
                                       viskores::cont::ArrayHandle<T, SOut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    const viskores::Id numberOfValues = input.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      output.Allocate(0);
      return viskores::TypeTraits<T>::ZeroInitialization();
    }

    //We need call PrepareForInput on the input argument before invoking a
    //function. The order of execution of parameters of a function is undefined
    //so we need to make sure input is called before output, or else in-place
    //use case breaks.
    viskores::cont::Token token;
    auto inputPortal = input.PrepareForInput(DeviceAdapterTagCuda(), token);
    return ScanInclusivePortal(
      inputPortal, output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda(), token));
  }

  template <typename T, class SIn, class SOut, class BinaryFunctor>
  VISKORES_CONT static T ScanInclusive(const viskores::cont::ArrayHandle<T, SIn>& input,
                                       viskores::cont::ArrayHandle<T, SOut>& output,
                                       BinaryFunctor binary_functor)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    const viskores::Id numberOfValues = input.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      output.Allocate(0);
      return viskores::TypeTraits<T>::ZeroInitialization();
    }

    //We need call PrepareForInput on the input argument before invoking a
    //function. The order of execution of parameters of a function is undefined
    //so we need to make sure input is called before output, or else in-place
    //use case breaks.
    viskores::cont::Token token;
    auto inputPortal = input.PrepareForInput(DeviceAdapterTagCuda(), token);
    return ScanInclusivePortal(
      inputPortal,
      output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda(), token),
      binary_functor);
  }

  template <typename T, typename U, typename KIn, typename VIn, typename VOut>
  VISKORES_CONT static void ScanInclusiveByKey(const viskores::cont::ArrayHandle<T, KIn>& keys,
                                               const viskores::cont::ArrayHandle<U, VIn>& values,
                                               viskores::cont::ArrayHandle<U, VOut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    const viskores::Id numberOfValues = keys.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      output.Allocate(0);
      return;
    }

    //We need call PrepareForInput on the input argument before invoking a
    //function. The order of execution of parameters of a function is undefined
    //so we need to make sure input is called before output, or else in-place
    //use case breaks.
    viskores::cont::Token token;
    auto keysPortal = keys.PrepareForInput(DeviceAdapterTagCuda(), token);
    auto valuesPortal = values.PrepareForInput(DeviceAdapterTagCuda(), token);
    ScanInclusiveByKeyPortal(
      keysPortal,
      valuesPortal,
      output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda(), token));
  }

  template <typename T,
            typename U,
            typename KIn,
            typename VIn,
            typename VOut,
            typename BinaryFunctor>
  VISKORES_CONT static void ScanInclusiveByKey(const viskores::cont::ArrayHandle<T, KIn>& keys,
                                               const viskores::cont::ArrayHandle<U, VIn>& values,
                                               viskores::cont::ArrayHandle<U, VOut>& output,
                                               BinaryFunctor binary_functor)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    const viskores::Id numberOfValues = keys.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      output.Allocate(0);
      return;
    }

    //We need call PrepareForInput on the input argument before invoking a
    //function. The order of execution of parameters of a function is undefined
    //so we need to make sure input is called before output, or else in-place
    //use case breaks.
    viskores::cont::Token token;
    auto keysPortal = keys.PrepareForInput(DeviceAdapterTagCuda(), token);
    auto valuesPortal = values.PrepareForInput(DeviceAdapterTagCuda(), token);
    ScanInclusiveByKeyPortal(keysPortal,
                             valuesPortal,
                             output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda(), token),
                             ::thrust::equal_to<T>(),
                             binary_functor);
  }

  template <typename T, typename U, typename KIn, typename VIn, typename VOut>
  VISKORES_CONT static void ScanExclusiveByKey(const viskores::cont::ArrayHandle<T, KIn>& keys,
                                               const viskores::cont::ArrayHandle<U, VIn>& values,
                                               viskores::cont::ArrayHandle<U, VOut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    const viskores::Id numberOfValues = keys.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      output.Allocate(0);
      return;
    }

    //We need call PrepareForInput on the input argument before invoking a
    //function. The order of execution of parameters of a function is undefined
    //so we need to make sure input is called before output, or else in-place
    //use case breaks.
    viskores::cont::Token token;
    auto keysPortal = keys.PrepareForInput(DeviceAdapterTagCuda(), token);
    auto valuesPortal = values.PrepareForInput(DeviceAdapterTagCuda(), token);
    ScanExclusiveByKeyPortal(keysPortal,
                             valuesPortal,
                             output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda(), token),
                             viskores::TypeTraits<T>::ZeroInitialization(),
                             ::thrust::equal_to<T>(),
                             viskores::Add());
  }

  template <typename T,
            typename U,
            typename KIn,
            typename VIn,
            typename VOut,
            typename BinaryFunctor>
  VISKORES_CONT static void ScanExclusiveByKey(const viskores::cont::ArrayHandle<T, KIn>& keys,
                                               const viskores::cont::ArrayHandle<U, VIn>& values,
                                               viskores::cont::ArrayHandle<U, VOut>& output,
                                               const U& initialValue,
                                               BinaryFunctor binary_functor)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    const viskores::Id numberOfValues = keys.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      output.Allocate(0);
      return;
    }

    //We need call PrepareForInput on the input argument before invoking a
    //function. The order of execution of parameters of a function is undefined
    //so we need to make sure input is called before output, or else in-place
    //use case breaks.
    viskores::cont::Token token;
    auto keysPortal = keys.PrepareForInput(DeviceAdapterTagCuda(), token);
    auto valuesPortal = values.PrepareForInput(DeviceAdapterTagCuda(), token);
    ScanExclusiveByKeyPortal(keysPortal,
                             valuesPortal,
                             output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda(), token),
                             initialValue,
                             ::thrust::equal_to<T>(),
                             binary_functor);
  }

  // we use cuda pinned memory to reduce the amount of synchronization
  // and mem copies between the host and device.
  struct VISKORES_CONT_EXPORT PinnedErrorArray
  {
    char* HostPtr = nullptr;
    char* DevicePtr = nullptr;
    viskores::Id Size = 0;
  };

  VISKORES_CONT_EXPORT
  static const PinnedErrorArray& GetPinnedErrorArray();

  VISKORES_CONT_EXPORT
  static void CheckForErrors(); // throws viskores::cont::ErrorExecution

  VISKORES_CONT_EXPORT
  static void SetupErrorBuffer(viskores::exec::cuda::internal::TaskStrided& functor);

  VISKORES_CONT_EXPORT
  static void GetBlocksAndThreads(viskores::UInt32& blocks,
                                  viskores::UInt32& threadsPerBlock,
                                  viskores::Id size,
                                  viskores::IdComponent maxThreadsPerBlock);

  VISKORES_CONT_EXPORT
  static void GetBlocksAndThreads(viskores::UInt32& blocks,
                                  dim3& threadsPerBlock,
                                  const dim3& size,
                                  viskores::IdComponent maxThreadsPerBlock);

  template <typename... Hints, typename... Args>
  static void GetBlocksAndThreads(viskores::cont::internal::HintList<Hints...>, Args&&... args)
  {
    using ThreadsPerBlock =
      viskores::cont::internal::HintFind<viskores::cont::internal::HintList<Hints...>,
                                         viskores::cont::internal::HintThreadsPerBlock<0>,
                                         viskores::cont::DeviceAdapterTagCuda>;
    GetBlocksAndThreads(std::forward<Args>(args)..., ThreadsPerBlock::MaxThreads);
  }

  VISKORES_CONT_EXPORT
  static void LogKernelLaunch(const cudaFuncAttributes& func_attrs,
                              const std::type_info& worklet_info,
                              viskores::UInt32 blocks,
                              viskores::UInt32 threadsPerBlock,
                              viskores::Id size);

  VISKORES_CONT_EXPORT
  static void LogKernelLaunch(const cudaFuncAttributes& func_attrs,
                              const std::type_info& worklet_info,
                              viskores::UInt32 blocks,
                              dim3 threadsPerBlock,
                              const dim3& size);

public:
  template <typename WType, typename IType, typename Hints>
  static void ScheduleTask(
    viskores::exec::cuda::internal::TaskStrided1D<WType, IType, Hints>& functor,
    viskores::Id numInstances)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    VISKORES_ASSERT(numInstances >= 0);
    if (numInstances < 1)
    {
      // No instances means nothing to run. Just return.
      return;
    }

    CheckForErrors();
    SetupErrorBuffer(functor);

    viskores::UInt32 blocks, threadsPerBlock;
    GetBlocksAndThreads(Hints{}, blocks, threadsPerBlock, numInstances);

#ifdef VISKORES_ENABLE_LOGGING
    if (GetStderrLogLevel() >= viskores::cont::LogLevel::KernelLaunches)
    {
      using FunctorType = std::decay_t<decltype(functor)>;
      cudaFuncAttributes empty_kernel_attrs;
      VISKORES_CUDA_CALL(cudaFuncGetAttributes(&empty_kernel_attrs,
                                               cuda::internal::TaskStrided1DLaunch<FunctorType>));
      LogKernelLaunch(empty_kernel_attrs, typeid(WType), blocks, threadsPerBlock, numInstances);
    }
#endif

    cuda::internal::TaskStrided1DLaunch<<<blocks, threadsPerBlock, 0, cudaStreamPerThread>>>(
      functor, numInstances);
  }

  template <typename WType, typename IType, typename Hints>
  static void ScheduleTask(
    viskores::exec::cuda::internal::TaskStrided3D<WType, IType, Hints>& functor,
    viskores::Id3 rangeMax)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    VISKORES_ASSERT((rangeMax[0] >= 0) && (rangeMax[1] >= 0) && (rangeMax[2] >= 0));
    if ((rangeMax[0] < 1) || (rangeMax[1] < 1) || (rangeMax[2] < 1))
    {
      // No instances means nothing to run. Just return.
      return;
    }

    CheckForErrors();
    SetupErrorBuffer(functor);

    const dim3 ranges(static_cast<viskores::UInt32>(rangeMax[0]),
                      static_cast<viskores::UInt32>(rangeMax[1]),
                      static_cast<viskores::UInt32>(rangeMax[2]));

    viskores::UInt32 blocks;
    dim3 threadsPerBlock;
    GetBlocksAndThreads(Hints{}, blocks, threadsPerBlock, ranges);

#ifdef VISKORES_ENABLE_LOGGING
    if (GetStderrLogLevel() >= viskores::cont::LogLevel::KernelLaunches)
    {
      using FunctorType = std::decay_t<decltype(functor)>;
      cudaFuncAttributes empty_kernel_attrs;
      VISKORES_CUDA_CALL(cudaFuncGetAttributes(&empty_kernel_attrs,
                                               cuda::internal::TaskStrided3DLaunch<FunctorType>));
      LogKernelLaunch(empty_kernel_attrs, typeid(WType), blocks, threadsPerBlock, ranges);
    }
#endif

    cuda::internal::TaskStrided3DLaunch<<<blocks, threadsPerBlock, 0, cudaStreamPerThread>>>(
      functor, rangeMax);
  }

  template <typename Hints, typename Functor>
  VISKORES_CONT static void Schedule(Hints, Functor functor, viskores::Id numInstances)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::exec::cuda::internal::TaskStrided1D<Functor, viskores::internal::NullType, Hints>
      kernel(functor);

    ScheduleTask(kernel, numInstances);
  }

  template <typename FunctorType>
  VISKORES_CONT static inline void Schedule(FunctorType&& functor, viskores::Id numInstances)
  {
    Schedule(viskores::cont::internal::HintList<>{}, functor, numInstances);
  }

  template <typename Hints, typename Functor>
  VISKORES_CONT static void Schedule(Hints, Functor functor, const viskores::Id3& rangeMax)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::exec::cuda::internal::TaskStrided3D<Functor, viskores::internal::NullType, Hints>
      kernel(functor);
    ScheduleTask(kernel, rangeMax);
  }

  template <typename FunctorType>
  VISKORES_CONT static inline void Schedule(FunctorType&& functor, viskores::Id3 rangeMax)
  {
    Schedule(viskores::cont::internal::HintList<>{}, functor, rangeMax);
  }

  template <typename T, class Storage>
  VISKORES_CONT static void Sort(viskores::cont::ArrayHandle<T, Storage>& values)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::cont::Token token;
    SortPortal(values.PrepareForInPlace(DeviceAdapterTagCuda(), token));
  }

  template <typename T, class Storage, class BinaryCompare>
  VISKORES_CONT static void Sort(viskores::cont::ArrayHandle<T, Storage>& values,
                                 BinaryCompare binary_compare)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::cont::Token token;
    SortPortal(values.PrepareForInPlace(DeviceAdapterTagCuda(), token), binary_compare);
  }

  template <typename T, typename U, class StorageT, class StorageU>
  VISKORES_CONT static void SortByKey(viskores::cont::ArrayHandle<T, StorageT>& keys,
                                      viskores::cont::ArrayHandle<U, StorageU>& values)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::cont::Token token;
    SortByKeyPortal(keys.PrepareForInPlace(DeviceAdapterTagCuda(), token),
                    values.PrepareForInPlace(DeviceAdapterTagCuda(), token));
  }

  template <typename T, typename U, class StorageT, class StorageU, class BinaryCompare>
  VISKORES_CONT static void SortByKey(viskores::cont::ArrayHandle<T, StorageT>& keys,
                                      viskores::cont::ArrayHandle<U, StorageU>& values,
                                      BinaryCompare binary_compare)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::cont::Token token;
    SortByKeyPortal(keys.PrepareForInPlace(DeviceAdapterTagCuda(), token),
                    values.PrepareForInPlace(DeviceAdapterTagCuda(), token),
                    binary_compare);
  }

  template <typename T, class Storage>
  VISKORES_CONT static void Unique(viskores::cont::ArrayHandle<T, Storage>& values)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::Id newSize;

    {
      viskores::cont::Token token;
      newSize = UniquePortal(values.PrepareForInPlace(DeviceAdapterTagCuda(), token));
    }

    values.Allocate(newSize, viskores::CopyFlag::On);
  }

  template <typename T, class Storage, class BinaryCompare>
  VISKORES_CONT static void Unique(viskores::cont::ArrayHandle<T, Storage>& values,
                                   BinaryCompare binary_compare)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::Id newSize;
    {
      viskores::cont::Token token;
      newSize =
        UniquePortal(values.PrepareForInPlace(DeviceAdapterTagCuda(), token), binary_compare);
    }

    values.Allocate(newSize, viskores::CopyFlag::On);
  }

  template <typename T, class SIn, class SVal, class SOut>
  VISKORES_CONT static void UpperBounds(const viskores::cont::ArrayHandle<T, SIn>& input,
                                        const viskores::cont::ArrayHandle<T, SVal>& values,
                                        viskores::cont::ArrayHandle<viskores::Id, SOut>& output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::Id numberOfValues = values.GetNumberOfValues();
    viskores::cont::Token token;
    UpperBoundsPortal(input.PrepareForInput(DeviceAdapterTagCuda(), token),
                      values.PrepareForInput(DeviceAdapterTagCuda(), token),
                      output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda(), token));
  }

  template <typename T, class SIn, class SVal, class SOut, class BinaryCompare>
  VISKORES_CONT static void UpperBounds(const viskores::cont::ArrayHandle<T, SIn>& input,
                                        const viskores::cont::ArrayHandle<T, SVal>& values,
                                        viskores::cont::ArrayHandle<viskores::Id, SOut>& output,
                                        BinaryCompare binary_compare)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::Id numberOfValues = values.GetNumberOfValues();
    viskores::cont::Token token;
    UpperBoundsPortal(input.PrepareForInput(DeviceAdapterTagCuda(), token),
                      values.PrepareForInput(DeviceAdapterTagCuda(), token),
                      output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda(), token),
                      binary_compare);
  }

  template <class SIn, class SOut>
  VISKORES_CONT static void UpperBounds(
    const viskores::cont::ArrayHandle<viskores::Id, SIn>& input,
    viskores::cont::ArrayHandle<viskores::Id, SOut>& values_output)
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    viskores::cont::Token token;
    UpperBoundsPortal(input.PrepareForInput(DeviceAdapterTagCuda(), token),
                      values_output.PrepareForInPlace(DeviceAdapterTagCuda(), token));
  }

  VISKORES_CONT static void Synchronize()
  {
    VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

    VISKORES_CUDA_CALL(cudaStreamSynchronize(cudaStreamPerThread));
    CheckForErrors();
  }
};

template <>
class DeviceTaskTypes<viskores::cont::DeviceAdapterTagCuda>
{
public:
  template <typename Hints, typename WorkletType, typename InvocationType>
  static viskores::exec::cuda::internal::TaskStrided1D<WorkletType, InvocationType, Hints>
  MakeTask(WorkletType& worklet, InvocationType& invocation, viskores::Id, Hints = Hints{})
  {
    return { worklet, invocation };
  }

  template <typename Hints, typename WorkletType, typename InvocationType>
  static viskores::exec::cuda::internal::TaskStrided3D<WorkletType, InvocationType, Hints>
  MakeTask(WorkletType& worklet, InvocationType& invocation, viskores::Id3, Hints = Hints{})
  {
    return { worklet, invocation };
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

#endif //viskores_cont_cuda_internal_DeviceAdapterAlgorithmCuda_h
