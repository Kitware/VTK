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
#include "vtkNew.h"

#ifdef _MSC_VER
#  pragma push_macro("__TBB_NO_IMPLICIT_LINKAGE")
#  define __TBB_NO_IMPLICIT_LINKAGE 1
#endif

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_sort.h>

#ifdef _MSC_VER
#  pragma pop_macro("__TBB_NO_IMPLICIT_LINKAGE")
#endif

namespace vtk
{
namespace detail
{
namespace smp
{

//--------------------------------------------------------------------------------
template <typename T>
class FuncCall
{
  T& o;

  void operator=(const FuncCall&) = delete;

public:
  void operator() (const tbb::blocked_range<vtkIdType>& r) const
  {
      o.Execute(r.begin(), r.end());
  }

  FuncCall (T& _o) : o(_o)
  {
  }
};

//--------------------------------------------------------------------------------
template <typename FunctorInternal>
void vtkSMPTools_Impl_For(
  vtkIdType first, vtkIdType last, vtkIdType grain,
  FunctorInternal& fi)
{
  vtkIdType range = last - first;
  if (range <= 0)
  {
    return;
  }
  if (grain > 0)
  {
    tbb::parallel_for(tbb::blocked_range<vtkIdType>(first, last, grain), FuncCall<FunctorInternal>(fi));
  }
  else
  {
    // When the grain is not specified, automatically calculate an appropriate grain size so
    // most of the time will still be spent running the calculation and not task overhead.
    const vtkIdType numberThreadsEstimate = 40; // Estimate of how many threads we might be able to run
    const vtkIdType batchesPerThread = 5; // Plan for a few batches per thread so one busy core doesn't stall the whole system
    const vtkIdType batches = numberThreadsEstimate * batchesPerThread;

    if (range >= batches)
    {
      vtkIdType calculatedGrain = ((range - 1) / batches) + 1; // std::ceil round up for systems without cmath
      tbb::parallel_for(tbb::blocked_range<vtkIdType>(first, last, calculatedGrain), FuncCall<FunctorInternal>(fi));
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
template<typename RandomAccessIterator>
void vtkSMPTools_Impl_Sort(RandomAccessIterator begin,
                                  RandomAccessIterator end)
{
  tbb::parallel_sort(begin, end);
}

//--------------------------------------------------------------------------------
template<typename RandomAccessIterator, typename Compare>
void vtkSMPTools_Impl_Sort(RandomAccessIterator begin,
                                  RandomAccessIterator end,
                                  Compare comp)
{
  tbb::parallel_sort(begin, end, comp);
}


}//namespace smp
}//namespace detail
}//namespace vtk
