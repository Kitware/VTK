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

#ifndef OpenMPvtkSMPToolsImpl_txx
#define OpenMPvtkSMPToolsImpl_txx

#include <algorithm> // For std::sort

#include "SMP/Common/vtkSMPToolsImpl.h"
#include "SMP/Common/vtkSMPToolsInternal.h" // For common vtk smp class
#include "vtkCommonCoreModule.h"            // For export macro

namespace vtk
{
namespace detail
{
namespace smp
{

int VTKCOMMONCORE_EXPORT GetNumberOfThreadsOpenMP();
void VTKCOMMONCORE_EXPORT vtkSMPToolsImplForOpenMP(vtkIdType first, vtkIdType last, vtkIdType grain,
  ExecuteFunctorPtrType functorExecuter, void* functor, bool nestedActivated);

//--------------------------------------------------------------------------------
template <typename FunctorInternal>
void ExecuteFunctorOpenMP(void* functor, vtkIdType from, vtkIdType grain, vtkIdType last)
{
  const vtkIdType to = std::min(from + grain, last);

  FunctorInternal& fi = *reinterpret_cast<FunctorInternal*>(functor);
  fi.Execute(from, to);
}

//--------------------------------------------------------------------------------
template <>
template <typename FunctorInternal>
void vtkSMPToolsImpl<BackendType::OpenMP>::For(
  vtkIdType first, vtkIdType last, vtkIdType grain, FunctorInternal& fi)
{
  vtkIdType n = last - first;
  if (n <= 0)
  {
    return;
  }

  if (grain >= n)
  {
    fi.Execute(first, last);
  }
  else
  {
    // this->IsParallel may have threads conficts but it will be always between true and true,
    // it is set to false only in sequential code.
    // /!\ This behaviour should be changed if we want more control on nested
    // (e.g only the 2 first nested For are in parallel)
    bool fromParallelCode = this->IsParallel;
    this->IsParallel = true;

    vtkSMPToolsImplForOpenMP(
      first, last, grain, ExecuteFunctorOpenMP<FunctorInternal>, &fi, this->NestedActivated);

    this->IsParallel &= fromParallelCode;
  }
}

//--------------------------------------------------------------------------------
template <>
template <typename InputIt, typename OutputIt, typename Functor>
void vtkSMPToolsImpl<BackendType::OpenMP>::Transform(
  InputIt inBegin, InputIt inEnd, OutputIt outBegin, Functor transform)
{
  auto size = std::distance(inBegin, inEnd);

  UnaryTransformCall<InputIt, OutputIt, Functor> exec(inBegin, outBegin, transform);
  this->For(0, size, 0, exec);
}

//--------------------------------------------------------------------------------
template <>
template <typename InputIt1, typename InputIt2, typename OutputIt, typename Functor>
void vtkSMPToolsImpl<BackendType::OpenMP>::Transform(
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
void vtkSMPToolsImpl<BackendType::OpenMP>::Fill(Iterator begin, Iterator end, const T& value)
{
  auto size = std::distance(begin, end);

  FillFunctor<T> fill(value);
  UnaryTransformCall<Iterator, Iterator, FillFunctor<T>> exec(begin, begin, fill);
  this->For(0, size, 0, exec);
}

//--------------------------------------------------------------------------------
template <>
template <typename RandomAccessIterator>
void vtkSMPToolsImpl<BackendType::OpenMP>::Sort(
  RandomAccessIterator begin, RandomAccessIterator end)
{
  std::sort(begin, end);
}

//--------------------------------------------------------------------------------
template <>
template <typename RandomAccessIterator, typename Compare>
void vtkSMPToolsImpl<BackendType::OpenMP>::Sort(
  RandomAccessIterator begin, RandomAccessIterator end, Compare comp)
{
  std::sort(begin, end, comp);
}

//--------------------------------------------------------------------------------
template <>
void vtkSMPToolsImpl<BackendType::OpenMP>::Initialize(int);

//--------------------------------------------------------------------------------
template <>
int vtkSMPToolsImpl<BackendType::OpenMP>::GetEstimatedNumberOfThreads();

} // namespace smp
} // namespace detail
} // namespace vtk

#endif
