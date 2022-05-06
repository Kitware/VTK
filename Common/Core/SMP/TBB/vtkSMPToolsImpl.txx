/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPToolsImpl.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef TBBvtkSMPToolsImpl_txx
#define TBBvtkSMPToolsImpl_txx

#include "SMP/Common/vtkSMPToolsImpl.h"
#include "SMP/Common/vtkSMPToolsInternal.h" // For common vtk smp class
#include "vtkCommonCoreModule.h"            // For export macro

#ifdef _MSC_VER
#pragma push_macro("__TBB_NO_IMPLICIT_LINKAGE")
#define __TBB_NO_IMPLICIT_LINKAGE 1
#endif

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_sort.h>

#ifdef _MSC_VER
#pragma pop_macro("__TBB_NO_IMPLICIT_LINKAGE")
#endif

namespace vtk
{
namespace detail
{
namespace smp
{

void VTKCOMMONCORE_EXPORT vtkSMPToolsImplForTBB(vtkIdType first, vtkIdType last, vtkIdType grain,
  ExecuteFunctorPtrType functorExecuter, void* functor);

//--------------------------------------------------------------------------------
template <typename T>
class FuncCall
{
  T& o;

  void operator=(const FuncCall&) = delete;

public:
  void operator()(const tbb::blocked_range<vtkIdType>& r) const { o.Execute(r.begin(), r.end()); }

  FuncCall(T& _o)
    : o(_o)
  {
  }
};

//--------------------------------------------------------------------------------
template <typename FunctorInternal>
void ExecuteFunctorTBB(void* functor, vtkIdType first, vtkIdType last, vtkIdType grain)
{
  FunctorInternal& fi = *reinterpret_cast<FunctorInternal*>(functor);

  vtkIdType range = last - first;
  if (range <= 0)
  {
    return;
  }
  if (grain > 0)
  {
    tbb::parallel_for(
      tbb::blocked_range<vtkIdType>(first, last, grain), FuncCall<FunctorInternal>(fi));
  }
  else
  {
    // When the grain is not specified, automatically calculate an appropriate grain size so
    // most of the time will still be spent running the calculation and not task overhead.

    // Estimate of how many threads we might be able to run
    const vtkIdType numberThreadsEstimate = 40;
    // Plan for a few batches per thread so one busy core doesn't stall the whole system
    const vtkIdType batchesPerThread = 5;
    const vtkIdType batches = numberThreadsEstimate * batchesPerThread;

    if (range >= batches)
    {
      // std::ceil round up for systems without cmath
      vtkIdType calculatedGrain = ((range - 1) / batches) + 1;
      tbb::parallel_for(
        tbb::blocked_range<vtkIdType>(first, last, calculatedGrain), FuncCall<FunctorInternal>(fi));
    }
    else
    {
      // Data is too small to generate a reasonable grain. Fallback to default so data still runs
      // on as many threads as possible (Jan 2020: Default is one index per tbb task).
      tbb::parallel_for(tbb::blocked_range<vtkIdType>(first, last), FuncCall<FunctorInternal>(fi));
    }
  }
}

//--------------------------------------------------------------------------------
template <>
template <typename FunctorInternal>
void vtkSMPToolsImpl<BackendType::TBB>::For(
  vtkIdType first, vtkIdType last, vtkIdType grain, FunctorInternal& fi)
{
  if (!this->NestedActivated && this->IsParallel)
  {
    fi.Execute(first, last);
  }
  else
  {
    // /!\ This behaviour should be changed if we want more control on nested
    // (e.g only the 2 first nested For are in parallel)
    bool fromParallelCode = this->IsParallel.exchange(true);

    vtkSMPToolsImplForTBB(first, last, grain, ExecuteFunctorTBB<FunctorInternal>, &fi);

    // Atomic contortion to achieve this->IsParallel &= fromParallelCode.
    // This compare&exchange basically boils down to:
    // if (IsParallel == trueFlag)
    //   IsParallel = fromParallelCode;
    // else
    //   trueFlag = IsParallel;
    // Which either leaves IsParallel as false or sets it to fromParallelCode (i.e. &=).
    // Note that the return value of compare_exchange_weak() is not needed,
    // and that no looping is necessary.
    bool trueFlag = true;
    this->IsParallel.compare_exchange_weak(trueFlag, fromParallelCode);
  }
}

//--------------------------------------------------------------------------------
template <>
template <typename InputIt, typename OutputIt, typename Functor>
void vtkSMPToolsImpl<BackendType::TBB>::Transform(
  InputIt inBegin, InputIt inEnd, OutputIt outBegin, Functor transform)
{
  auto size = std::distance(inBegin, inEnd);

  UnaryTransformCall<InputIt, OutputIt, Functor> exec(inBegin, outBegin, transform);
  this->For(0, size, 0, exec);
}

//--------------------------------------------------------------------------------
template <>
template <typename InputIt1, typename InputIt2, typename OutputIt, typename Functor>
void vtkSMPToolsImpl<BackendType::TBB>::Transform(
  InputIt1 inBegin1, InputIt1 inEnd, InputIt2 inBegin2, OutputIt outBegin, Functor transform)
{
  auto size = std::distance(inBegin1, inEnd);

  BinaryTransformCall<InputIt1, InputIt2, OutputIt, Functor> exec(
    inBegin1, inBegin2, outBegin, transform);
  this->For(0, size, 0, exec);
}

//--------------------------------------------------------------------------------
template <>
template <typename Iterator, typename T>
void vtkSMPToolsImpl<BackendType::TBB>::Fill(Iterator begin, Iterator end, const T& value)
{
  auto size = std::distance(begin, end);

  FillFunctor<T> fill(value);
  UnaryTransformCall<Iterator, Iterator, FillFunctor<T>> exec(begin, begin, fill);
  this->For(0, size, 0, exec);
}

//--------------------------------------------------------------------------------
template <>
template <typename RandomAccessIterator>
void vtkSMPToolsImpl<BackendType::TBB>::Sort(RandomAccessIterator begin, RandomAccessIterator end)
{
  tbb::parallel_sort(begin, end);
}

//--------------------------------------------------------------------------------
template <>
template <typename RandomAccessIterator, typename Compare>
void vtkSMPToolsImpl<BackendType::TBB>::Sort(
  RandomAccessIterator begin, RandomAccessIterator end, Compare comp)
{
  tbb::parallel_sort(begin, end, comp);
}

//--------------------------------------------------------------------------------
template <>
void vtkSMPToolsImpl<BackendType::TBB>::Initialize(int);

//--------------------------------------------------------------------------------
template <>
int vtkSMPToolsImpl<BackendType::TBB>::GetEstimatedNumberOfThreads();

} // namespace smp
} // namespace detail
} // namespace vtk

#endif
