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

#include <omp.h>

#include <algorithm>

namespace
{
int vtkSMPNumberOfSpecifiedThreads = 0;
}

void vtkSMPTools::Initialize(int numThreads)
{
# pragma omp single
  if (numThreads)
  {
    vtkSMPNumberOfSpecifiedThreads = numThreads;
    omp_set_num_threads(numThreads);
  }
}

int vtkSMPTools::GetEstimatedNumberOfThreads()
{
  return vtk::detail::smp::GetNumberOfThreads();
}

int vtk::detail::smp::GetNumberOfThreads()
{
  return vtkSMPNumberOfSpecifiedThreads ? vtkSMPNumberOfSpecifiedThreads :
         omp_get_max_threads();
}

void vtk::detail::smp::vtkSMPTools_Impl_For_OpenMP(vtkIdType first,
  vtkIdType last, vtkIdType grain, ExecuteFunctorPtrType functorExecuter,
  void *functor)
{
  if (grain <= 0)
  {
    vtkIdType estimateGrain = (last - first)/(omp_get_max_threads() * 4);
    grain = (estimateGrain > 0) ? estimateGrain : 1;
  }

# pragma omp parallel for schedule(runtime)
  for (vtkIdType from = first; from < last; from += grain)
  {
    functorExecuter(functor, from, grain, last);
  }
}
