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

#include "vtkSMP.h"

#include <mutex>

#ifdef _MSC_VER
#pragma push_macro("__TBB_NO_IMPLICIT_LINKAGE")
#define __TBB_NO_IMPLICIT_LINKAGE 1
#endif

#include <tbb/task_arena.h>

#ifdef _MSC_VER
#pragma pop_macro("__TBB_NO_IMPLICIT_LINKAGE")
#endif

static tbb::task_arena taskArena;
static std::mutex vtkSMPToolsCS;

//------------------------------------------------------------------------------
const char* vtkSMPTools::GetBackend()
{
  return VTK_SMP_BACKEND;
}

//------------------------------------------------------------------------------
void vtkSMPTools::Initialize(int numThreads)
{
  vtkSMPToolsCS.lock();

  if (numThreads == 0)
  {
    const char* vtkSmpNumThreads = std::getenv("VTK_SMP_MAX_THREADS");
    if (vtkSmpNumThreads)
    {
      numThreads = std::atoi(vtkSmpNumThreads);
    }
  }
  if (numThreads > 0 && numThreads != taskArena.max_concurrency())
  {
    if (taskArena.is_active())
    {
      taskArena.terminate();
    }
    taskArena.initialize(numThreads);
  }

  vtkSMPToolsCS.unlock();
}

//------------------------------------------------------------------------------
int vtkSMPTools::GetEstimatedNumberOfThreads()
{
  return taskArena.max_concurrency();
}

//------------------------------------------------------------------------------
void vtk::detail::smp::vtkSMPTools_Impl_For_TBB(vtkIdType first, vtkIdType last, vtkIdType grain,
  ExecuteFunctorPtrType functorExecuter, void* functor)
{
  if (taskArena.is_active())
  {
    taskArena.execute([&] { functorExecuter(functor, first, last, grain); });
  }
  else
  {
    functorExecuter(functor, first, last, grain);
  }
}
