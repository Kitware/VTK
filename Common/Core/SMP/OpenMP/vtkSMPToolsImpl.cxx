/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPToolsImpl.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "SMP/Common/vtkSMPToolsImpl.h"
#include "SMP/OpenMP/vtkSMPToolsImpl.txx"

#include <cstdlib> // For std::getenv()
#include <omp.h>

namespace vtk
{
namespace detail
{
namespace smp
{
static int specifiedNumThreads = 0;

//------------------------------------------------------------------------------
template <>
void vtkSMPToolsImpl<BackendType::OpenMP>::Initialize(int numThreads)
{
  const int maxThreads = omp_get_max_threads();
  if (numThreads == 0)
  {
    const char* vtkSmpNumThreads = std::getenv("VTK_SMP_MAX_THREADS");
    if (vtkSmpNumThreads)
    {
      numThreads = std::atoi(vtkSmpNumThreads);
    }
    else if (specifiedNumThreads)
    {
      specifiedNumThreads = 0;
      omp_set_num_threads(maxThreads);
    }
  }
#pragma omp single
  if (numThreads > 0)
  {
    numThreads = std::min(numThreads, maxThreads);
    specifiedNumThreads = numThreads;
    omp_set_num_threads(numThreads);
  }
}

//------------------------------------------------------------------------------
int GetNumberOfThreadsOpenMP()
{
  return specifiedNumThreads ? specifiedNumThreads : omp_get_max_threads();
}

//------------------------------------------------------------------------------
template <>
int vtkSMPToolsImpl<BackendType::OpenMP>::GetEstimatedNumberOfThreads()
{
  return GetNumberOfThreadsOpenMP();
}

//------------------------------------------------------------------------------
void vtkSMPToolsImplForOpenMP(vtkIdType first, vtkIdType last, vtkIdType grain,
  ExecuteFunctorPtrType functorExecuter, void* functor, bool nestedActivated)
{
  if (grain <= 0)
  {
    vtkIdType estimateGrain = (last - first) / (GetNumberOfThreadsOpenMP() * 4);
    grain = (estimateGrain > 0) ? estimateGrain : 1;
  }

  omp_set_nested(nestedActivated);

#pragma omp parallel for schedule(runtime)
  for (vtkIdType from = first; from < last; from += grain)
  {
    functorExecuter(functor, from, grain, last);
  }
}

} // namespace smp
} // namespace detail
} // namespace vtk
