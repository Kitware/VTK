// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef SequentialvtkSMPToolsImpl_txx
#define SequentialvtkSMPToolsImpl_txx

#include <algorithm> // For std::sort, std::transform, std::fill

#include "SMP/Common/vtkSMPToolsImpl.h"
#include "SMP/Common/vtkSMPToolsInternal.h" // For common vtk smp class

namespace vtk
{
namespace detail
{
namespace smp
{
VTK_ABI_NAMESPACE_BEGIN

//--------------------------------------------------------------------------------
template <>
template <typename FunctorInternal>
void vtkSMPToolsImpl<BackendType::Sequential>::For(
  vtkIdType first, vtkIdType last, vtkIdType grain, FunctorInternal& fi)
{
  vtkIdType n = last - first;
  if (!n)
  {
    return;
  }

  if (grain == 0 || grain >= n)
  {
    fi.Execute(first, last);
  }
  else
  {
    vtkIdType b = first;
    while (b < last)
    {
      vtkIdType e = b + grain;
      if (e > last)
      {
        e = last;
      }
      fi.Execute(b, e);
      b = e;
    }
  }
}

//--------------------------------------------------------------------------------
template <>
template <typename InputIt, typename OutputIt, typename Functor>
void vtkSMPToolsImpl<BackendType::Sequential>::Transform(
  InputIt inBegin, InputIt inEnd, OutputIt outBegin, Functor transform)
{
  std::transform(inBegin, inEnd, outBegin, transform);
}

//--------------------------------------------------------------------------------
template <>
template <typename InputIt1, typename InputIt2, typename OutputIt, typename Functor>
void vtkSMPToolsImpl<BackendType::Sequential>::Transform(
  InputIt1 inBegin1, InputIt1 inEnd, InputIt2 inBegin2, OutputIt outBegin, Functor transform)
{
  std::transform(inBegin1, inEnd, inBegin2, outBegin, transform);
}

//--------------------------------------------------------------------------------
template <>
template <typename Iterator, typename T>
void vtkSMPToolsImpl<BackendType::Sequential>::Fill(Iterator begin, Iterator end, const T& value)
{
  std::fill(begin, end, value);
}

//--------------------------------------------------------------------------------
template <>
template <typename RandomAccessIterator>
void vtkSMPToolsImpl<BackendType::Sequential>::Sort(
  RandomAccessIterator begin, RandomAccessIterator end)
{
  std::sort(begin, end);
}

//--------------------------------------------------------------------------------
template <>
template <typename RandomAccessIterator, typename Compare>
void vtkSMPToolsImpl<BackendType::Sequential>::Sort(
  RandomAccessIterator begin, RandomAccessIterator end, Compare comp)
{
  std::sort(begin, end, comp);
}

//--------------------------------------------------------------------------------
template <>
VTKCOMMONCORE_EXPORT void vtkSMPToolsImpl<BackendType::Sequential>::Initialize(int);

//--------------------------------------------------------------------------------
template <>
VTKCOMMONCORE_EXPORT int vtkSMPToolsImpl<BackendType::Sequential>::GetEstimatedNumberOfThreads();

//--------------------------------------------------------------------------------
template <>
VTKCOMMONCORE_EXPORT bool vtkSMPToolsImpl<BackendType::Sequential>::GetSingleThread();

VTK_ABI_NAMESPACE_END
} // namespace smp
} // namespace detail
} // namespace vtk

#endif
