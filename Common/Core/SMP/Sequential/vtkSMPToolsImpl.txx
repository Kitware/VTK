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
void vtkSMPToolsImpl<BackendType::Sequential>::Initialize(int);

//--------------------------------------------------------------------------------
template <>
int vtkSMPToolsImpl<BackendType::Sequential>::GetEstimatedNumberOfThreads();

} // namespace smp
} // namespace detail
} // namespace vtk

#endif
