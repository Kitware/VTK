// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef STDThreadvtkSMPToolsImpl_txx
#define STDThreadvtkSMPToolsImpl_txx

#include <algorithm>  // For std::sort
#include <functional> // For std::bind

#include "SMP/Common/vtkSMPToolsImpl.h"
#include "SMP/Common/vtkSMPToolsInternal.h" // For common vtk smp class
#include "SMP/STDThread/vtkSMPThreadPool.h" // For vtkSMPThreadPool
#include "vtkCommonCoreModule.h"            // For export macro

namespace vtk
{
namespace detail
{
namespace smp
{
VTK_ABI_NAMESPACE_BEGIN

int VTKCOMMONCORE_EXPORT GetNumberOfThreadsSTDThread();

//--------------------------------------------------------------------------------
template <>
template <typename FunctorInternal>
void vtkSMPToolsImpl<BackendType::STDThread>::For(
  vtkIdType first, vtkIdType last, vtkIdType grain, FunctorInternal& fi)
{
  vtkIdType n = last - first;
  if (n <= 0)
  {
    return;
  }

  if (grain >= n || (!this->NestedActivated && vtkSMPThreadPool::GetInstance().IsParallelScope()))
  {
    fi.Execute(first, last);
  }
  else
  {
    int threadNumber = GetNumberOfThreadsSTDThread();

    if (grain <= 0)
    {
      vtkIdType estimateGrain = (last - first) / (threadNumber * 4);
      grain = (estimateGrain > 0) ? estimateGrain : 1;
    }

    auto proxy = vtkSMPThreadPool::GetInstance().AllocateThreads(threadNumber);

    for (vtkIdType from = first; from < last; from += grain)
    {
      const auto to = (std::min)(from + grain, last);
      proxy.DoJob([&fi, from, to] { fi.Execute(from, to); });
    }

    proxy.Join();
  }
}

//--------------------------------------------------------------------------------
template <>
template <typename InputIt, typename OutputIt, typename Functor>
void vtkSMPToolsImpl<BackendType::STDThread>::Transform(
  InputIt inBegin, InputIt inEnd, OutputIt outBegin, Functor transform)
{
  auto size = std::distance(inBegin, inEnd);

  UnaryTransformCall<InputIt, OutputIt, Functor> exec(inBegin, outBegin, transform);
  this->For(0, size, 0, exec);
}

//--------------------------------------------------------------------------------
template <>
template <typename InputIt1, typename InputIt2, typename OutputIt, typename Functor>
void vtkSMPToolsImpl<BackendType::STDThread>::Transform(
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
void vtkSMPToolsImpl<BackendType::STDThread>::Fill(Iterator begin, Iterator end, const T& value)
{
  auto size = std::distance(begin, end);

  FillFunctor<T> fill(value);
  UnaryTransformCall<Iterator, Iterator, FillFunctor<T>> exec(begin, begin, fill);
  this->For(0, size, 0, exec);
}

//--------------------------------------------------------------------------------
template <>
template <typename RandomAccessIterator>
void vtkSMPToolsImpl<BackendType::STDThread>::Sort(
  RandomAccessIterator begin, RandomAccessIterator end)
{
  std::sort(begin, end);
}

//--------------------------------------------------------------------------------
template <>
template <typename RandomAccessIterator, typename Compare>
void vtkSMPToolsImpl<BackendType::STDThread>::Sort(
  RandomAccessIterator begin, RandomAccessIterator end, Compare comp)
{
  std::sort(begin, end, comp);
}

//--------------------------------------------------------------------------------
template <>
VTKCOMMONCORE_EXPORT void vtkSMPToolsImpl<BackendType::STDThread>::Initialize(int);

//--------------------------------------------------------------------------------
template <>
VTKCOMMONCORE_EXPORT int vtkSMPToolsImpl<BackendType::STDThread>::GetEstimatedNumberOfThreads();

//--------------------------------------------------------------------------------
template <>
VTKCOMMONCORE_EXPORT int
vtkSMPToolsImpl<BackendType::STDThread>::GetEstimatedDefaultNumberOfThreads();

//--------------------------------------------------------------------------------
template <>
VTKCOMMONCORE_EXPORT bool vtkSMPToolsImpl<BackendType::STDThread>::GetSingleThread();

//--------------------------------------------------------------------------------
template <>
VTKCOMMONCORE_EXPORT bool vtkSMPToolsImpl<BackendType::STDThread>::IsParallelScope();

VTK_ABI_NAMESPACE_END
} // namespace smp
} // namespace detail
} // namespace vtk

#endif
