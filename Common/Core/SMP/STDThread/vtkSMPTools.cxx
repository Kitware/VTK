/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPTools.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMPTools.h"
#include "vtkSMPThreadPool.h"

#include <thread>
#include <vector>

static int specifiedNumThreads = 0;

//------------------------------------------------------------------------------
void vtkSMPTools::Initialize(int numThreads)
{
  if (numThreads == 0)
  {
    const char* vtkSmpNumThreads = std::getenv("VTK_SMP_MAX_THREADS");
    if (vtkSmpNumThreads)
    {
      numThreads = std::atoi(vtkSmpNumThreads);
    }
  }
  if (numThreads > 0)
  {
    specifiedNumThreads = numThreads;
  }
}

//------------------------------------------------------------------------------
int vtkSMPTools::GetEstimatedNumberOfThreads()
{
  return vtk::detail::smp::GetNumberOfThreads();
}

int vtk::detail::smp::GetNumberOfThreads()
{
  return specifiedNumThreads ? specifiedNumThreads : std::thread::hardware_concurrency();
}

void vtk::detail::smp::vtkSMPTools_Impl_For_STD(vtkIdType first, vtkIdType last, vtkIdType grain,
  ExecuteFunctorPtrType functorExecuter, void* functor)
{
  int threadNumber = vtkSMPTools::GetEstimatedNumberOfThreads();

  if (grain <= 0)
  {
    vtkIdType estimateGrain = (last - first) / (threadNumber * 4);
    grain = (estimateGrain > 0) ? estimateGrain : 1;
  }

  vtkSMPThreadPool pool(threadNumber);

  for (vtkIdType from = first; from < last; from += grain)
  {
    auto job = std::bind(functorExecuter, functor, from, grain, last);
    pool.DoJob(job);
  }
}
