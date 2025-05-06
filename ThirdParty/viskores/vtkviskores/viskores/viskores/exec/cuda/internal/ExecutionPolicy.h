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
#ifndef viskores_exec_cuda_internal_ExecutionPolicy_h
#define viskores_exec_cuda_internal_ExecutionPolicy_h

#include <viskores/BinaryPredicates.h>
#include <viskores/cont/cuda/ErrorCuda.h>
#include <viskores/exec/cuda/internal/WrappedOperators.h>

#include <viskores/exec/cuda/internal/ThrustPatches.h>
VISKORES_THIRDPARTY_PRE_INCLUDE
#include <thrust/execution_policy.h>
#include <thrust/sort.h>
#include <thrust/system/cuda/execution_policy.h>
#include <thrust/system/cuda/memory.h>
VISKORES_THIRDPARTY_POST_INCLUDE

#define ThrustCudaPolicyPerThread ::thrust::cuda::par.on(cudaStreamPerThread)

struct viskores_cuda_policy : thrust::device_execution_policy<viskores_cuda_policy>
{
};

//Specialize the sort call for cuda pointers using less/greater operators.
//The purpose of this is that for 32bit types (UInt32,Int32,Float32) thrust
//will call a super fast radix sort only if the operator is thrust::less
//or thrust::greater.
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename T>
__host__ __device__ void sort(
  const viskores_cuda_policy& exec,
  T* first,
  T* last,
  viskores::exec::cuda::internal::WrappedBinaryPredicate<T, viskores::SortLess> comp)
{ //sort for concrete pointers and less than op
  //this makes sure that we invoke the thrust radix sort and not merge sort
  return thrust::sort(ThrustCudaPolicyPerThread, first, last, thrust::less<T>());
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename T, typename RandomAccessIterator>
__host__ __device__ void sort_by_key(
  const viskores_cuda_policy& exec,
  T* first,
  T* last,
  RandomAccessIterator values_first,
  viskores::exec::cuda::internal::WrappedBinaryPredicate<T, viskores::SortLess> comp)
{ //sort for concrete pointers and less than op
  //this makes sure that we invoke the thrust radix sort and not merge sort
  return thrust::sort_by_key(
    ThrustCudaPolicyPerThread, first, last, values_first, thrust::less<T>());
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename T>
__host__ __device__ void sort(
  const viskores_cuda_policy& exec,
  T* first,
  T* last,
  viskores::exec::cuda::internal::WrappedBinaryPredicate<T, ::thrust::less<T>> comp)
{ //sort for concrete pointers and less than op
  //this makes sure that we invoke the thrust radix sort and not merge sort
  return thrust::sort(ThrustCudaPolicyPerThread, first, last, thrust::less<T>());
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename T, typename RandomAccessIterator>
__host__ __device__ void sort_by_key(
  const viskores_cuda_policy& exec,
  T* first,
  T* last,
  RandomAccessIterator values_first,
  viskores::exec::cuda::internal::WrappedBinaryPredicate<T, ::thrust::less<T>> comp)
{ //sort for concrete pointers and less than op
  //this makes sure that we invoke the thrust radix sort and not merge sort
  return thrust::sort_by_key(
    ThrustCudaPolicyPerThread, first, last, values_first, thrust::less<T>());
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename T>
__host__ __device__ void sort(
  const viskores_cuda_policy& exec,
  T* first,
  T* last,
  viskores::exec::cuda::internal::WrappedBinaryPredicate<T, viskores::SortGreater> comp)
{ //sort for concrete pointers and greater than op
  //this makes sure that we invoke the thrust radix sort and not merge sort
  return thrust::sort(ThrustCudaPolicyPerThread, first, last, thrust::greater<T>());
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename T, typename RandomAccessIterator>
__host__ __device__ void sort_by_key(
  const viskores_cuda_policy& exec,
  T* first,
  T* last,
  RandomAccessIterator values_first,
  viskores::exec::cuda::internal::WrappedBinaryPredicate<T, viskores::SortGreater> comp)
{ //sort for concrete pointers and greater than op
  //this makes sure that we invoke the thrust radix sort and not merge sort
  return thrust::sort_by_key(
    ThrustCudaPolicyPerThread, first, last, values_first, thrust::greater<T>());
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename T>
__host__ __device__ void sort(
  const viskores_cuda_policy& exec,
  T* first,
  T* last,
  viskores::exec::cuda::internal::WrappedBinaryPredicate<T, ::thrust::greater<T>> comp)
{ //sort for concrete pointers and greater than op
  //this makes sure that we invoke the thrust radix sort and not merge sort
  return thrust::sort(ThrustCudaPolicyPerThread, first, last, thrust::greater<T>());
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename T, typename RandomAccessIterator>
__host__ __device__ void sort_by_key(
  const viskores_cuda_policy& exec,
  T* first,
  T* last,
  RandomAccessIterator values_first,
  viskores::exec::cuda::internal::WrappedBinaryPredicate<T, ::thrust::greater<T>> comp)
{ //sort for concrete pointers and greater than op
  //this makes sure that we invoke the thrust radix sort and not merge sort
  return thrust::sort_by_key(
    ThrustCudaPolicyPerThread, first, last, values_first, thrust::greater<T>());
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename RandomAccessIterator, typename StrictWeakOrdering>
__host__ __device__ void sort(const viskores_cuda_policy& exec,
                              RandomAccessIterator first,
                              RandomAccessIterator last,
                              StrictWeakOrdering comp)
{
  //At this point the pointer type is not a cuda pointers and/or
  //the operator is not an approved less/greater operator.
  //This most likely will cause thrust to internally determine that
  //the best sort implementation is merge sort.
  return thrust::sort(ThrustCudaPolicyPerThread, first, last, comp);
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename RandomAccessIteratorKeys,
          typename RandomAccessIteratorValues,
          typename StrictWeakOrdering>
__host__ __device__ void sort_by_key(const viskores_cuda_policy& exec,
                                     RandomAccessIteratorKeys first,
                                     RandomAccessIteratorKeys last,
                                     RandomAccessIteratorValues values_first,
                                     StrictWeakOrdering comp)
{
  //At this point the pointer type is not a cuda pointers and/or
  //the operator is not an approved less/greater operator.
  //This most likely will cause thrust to internally determine that
  //the best sort implementation is merge sort.
  return thrust::sort_by_key(ThrustCudaPolicyPerThread, first, last, values_first, comp);
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename T,
          typename InputIterator2,
          typename OutputIterator1,
          typename OutputIterator2,
          typename BinaryPredicate,
          typename BinaryFunction>
__host__ __device__::thrust::pair<OutputIterator1, OutputIterator2> reduce_by_key(
  const viskores_cuda_policy& exec,
  T* keys_first,
  T* keys_last,
  InputIterator2 values_first,
  OutputIterator1 keys_output,
  OutputIterator2 values_output,
  BinaryPredicate binary_pred,
  BinaryFunction binary_op)

{
#if defined(VISKORES_CUDA_VERSION_MAJOR) && (VISKORES_CUDA_VERSION_MAJOR == 7) && \
  (VISKORES_CUDA_VERSION_MINOR >= 5)
  ::thrust::pair<OutputIterator1, OutputIterator2> result =
    thrust::reduce_by_key(ThrustCudaPolicyPerThread,
                          keys_first,
                          keys_last,
                          values_first,
                          keys_output,
                          values_output,
                          binary_pred,
                          binary_op);

//only sync if we are being invoked from the host
#ifndef VISKORES_CUDA_DEVICE_PASS
  VISKORES_CUDA_CALL(cudaStreamSynchronize(cudaStreamPerThread));
#endif

  return result;
#else
  return thrust::reduce_by_key(ThrustCudaPolicyPerThread,
                               keys_first,
                               keys_last,
                               values_first,
                               keys_output,
                               values_output,
                               binary_pred,
                               binary_op);
#endif
}

#endif
