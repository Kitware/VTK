/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPToolsInternal.h.in

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <algorithm> //for std::sort()

#include "vtkSMPToolsInternalCommon.h" // For common vtk smp class

namespace vtk
{
namespace detail
{
namespace smp
{
template <typename FunctorInternal>
void vtkSMPTools_Impl_For(vtkIdType first, vtkIdType last, vtkIdType grain, FunctorInternal& fi)
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
template <typename InputIt, typename OutputIt, typename Functor>
void vtkSMPTools_Impl_Transform(InputIt inBegin, InputIt inEnd, OutputIt outBegin, Functor& transform)
{
  std::transform(inBegin, inEnd, outBegin, outBegin, transform);
}

//--------------------------------------------------------------------------------
template <typename Iterator, typename T>
void vtkSMPTools_Impl_Fill(Iterator begin, Iterator end, const T& value)
{
  std::fill(begin, end, value);
}

//--------------------------------------------------------------------------------
template <typename RandomAccessIterator>
void vtkSMPTools_Impl_Sort(RandomAccessIterator begin, RandomAccessIterator end)
{
  std::sort(begin, end);
}

//--------------------------------------------------------------------------------
template <typename RandomAccessIterator, typename Compare>
void vtkSMPTools_Impl_Sort(RandomAccessIterator begin, RandomAccessIterator end, Compare comp)
{
  std::sort(begin, end, comp);
}

} // namespace smp
} // namespace detail
} // namespace vtk
