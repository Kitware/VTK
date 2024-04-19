// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "SMP/Common/vtkSMPToolsImpl.h"
#include "SMP/STDThread/vtkSMPToolsImpl.txx"

#include <cstdlib> // For std::getenv()
#include <thread>  // For std::thread::hardware_concurrency()

namespace vtk
{
namespace detail
{
namespace smp
{
VTK_ABI_NAMESPACE_BEGIN
static int specifiedNumThreadsSTD; // Default initialized to zero

//------------------------------------------------------------------------------
int GetNumberOfThreadsSTDThread()
{
  return specifiedNumThreadsSTD ? specifiedNumThreadsSTD : std::thread::hardware_concurrency();
}

//------------------------------------------------------------------------------
template <>
void vtkSMPToolsImpl<BackendType::STDThread>::Initialize(int numThreads)
{
  const int maxThreads = std::thread::hardware_concurrency();
  if (numThreads == 0)
  {
    const char* vtkSmpNumThreads = std::getenv("VTK_SMP_MAX_THREADS");
    if (vtkSmpNumThreads)
    {
      numThreads = std::atoi(vtkSmpNumThreads);
    }
    else
    {
      specifiedNumThreadsSTD = 0;
    }
  }
  if (numThreads > 0)
  {
    numThreads = std::min(numThreads, maxThreads);
    specifiedNumThreadsSTD = numThreads;
  }
}

//------------------------------------------------------------------------------
template <>
int vtkSMPToolsImpl<BackendType::STDThread>::GetEstimatedNumberOfThreads()
{
  return specifiedNumThreadsSTD > 0 ? specifiedNumThreadsSTD : std::thread::hardware_concurrency();
}

//------------------------------------------------------------------------------
template <>
int vtkSMPToolsImpl<BackendType::STDThread>::GetEstimatedDefaultNumberOfThreads()
{
  return std::thread::hardware_concurrency();
}

//------------------------------------------------------------------------------
template <>
bool vtkSMPToolsImpl<BackendType::STDThread>::GetSingleThread()
{
  return vtkSMPThreadPool::GetInstance().GetSingleThread();
}

//------------------------------------------------------------------------------
template <>
bool vtkSMPToolsImpl<BackendType::STDThread>::IsParallelScope()
{
  return vtkSMPThreadPool::GetInstance().IsParallelScope();
}

VTK_ABI_NAMESPACE_END
} // namespace smp
} // namespace detail
} // namespace vtk
