/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPThreadPool.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "SMP/STDThread/vtkSMPThreadPool.h"

#include <iostream>

namespace vtk
{
namespace detail
{
namespace smp
{
VTK_ABI_NAMESPACE_BEGIN

vtkSMPThreadPool::vtkSMPThreadPool(int threadNumber)
{
  this->Threads.reserve(threadNumber);
  for (int i = 0; i < threadNumber; ++i)
  {
    this->Threads.emplace_back(std::bind(&vtkSMPThreadPool::ThreadJob, this));
  }
}

void vtkSMPThreadPool::Join()
{
  {
    std::unique_lock<std::mutex> lock(this->Mutex);

    this->Joining = true;
    this->ConditionVariable.notify_all();
  }

  for (auto& it : this->Threads)
  {
    it.join();
  }
}

void vtkSMPThreadPool::DoJob(std::function<void(void)> job)
{
  std::unique_lock<std::mutex> lock(this->Mutex);

  this->JobQueue.emplace(std::move(job));
  this->ConditionVariable.notify_one();
}

void vtkSMPThreadPool::ThreadJob()
{
  std::function<void(void)> job;

  while (true)
  {
    {
      std::unique_lock<std::mutex> lock(this->Mutex);

      this->ConditionVariable.wait(
        lock, [this] { return (!this->JobQueue.empty() || this->Joining); });

      if (this->JobQueue.empty())
      {
        return;
      }

      job = std::move(this->JobQueue.front());
      this->JobQueue.pop();
    }
    job();
  }
}

std::vector<std::thread>* vtk::detail::smp::vtkSMPThreadPool::GetThreads()
{
  return &(this->Threads);
}
VTK_ABI_NAMESPACE_END
} // namespace smp
} // namespace detail
} // namespace vtk
