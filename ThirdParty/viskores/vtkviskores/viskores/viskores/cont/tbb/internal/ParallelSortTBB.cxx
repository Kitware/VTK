//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

// Copyright 2010, Takuya Akiba
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Takuya Akiba nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <viskores/cont/internal/ParallelRadixSort.h>

#if defined(VISKORES_MSVC)

// TBB's header include a #pragma comment(lib,"tbb.lib") line to make all
// consuming libraries link to tbb, this is bad behavior in a header
// based project
#pragma push_macro("__TBB_NO_IMPLICITLINKAGE")
#define __TBB_NO_IMPLICIT_LINKAGE 1

#endif // defined(VISKORES_MSVC)

// TBB includes windows.h, so instead we want to include windows.h with the
// correct settings so that we don't clobber any existing function
#include <viskores/internal/Windows.h>

#if TBB_VERSION_MAJOR >= 2020
#include <tbb/task.h>
#include <tbb/task_group.h>
#else
#include <tbb/tbb.h>
#endif

#include <thread>

#if defined(VISKORES_MSVC)
#pragma pop_macro("__TBB_NO_IMPLICITLINKAGE")
#endif

namespace viskores
{
namespace cont
{
namespace tbb
{
namespace sort
{

const size_t MAX_CORES = std::thread::hardware_concurrency();

#if TBB_VERSION_MAJOR < 2020
// Simple TBB task wrapper around a generic functor.
template <typename FunctorType>
struct TaskWrapper : public ::tbb::task
{
  FunctorType Functor;

  TaskWrapper(FunctorType f)
    : Functor(f)
  {
  }

  ::tbb::task* execute()
  {
    this->Functor(this);
    return nullptr;
  }
};

struct RadixThreaderTBB
{
  size_t GetAvailableCores() const { return MAX_CORES; }

  template <typename TaskType>
  void RunParentTask(TaskType task) const
  {
    using Task = TaskWrapper<TaskType>;
    Task& root = *new (::tbb::task::allocate_root()) Task(task);
    ::tbb::task::spawn_root_and_wait(root);
  }

  template <typename TaskType>
  void RunChildTasks(TaskWrapper<TaskType>* wrapper, TaskType left, TaskType right) const
  {
    using Task = TaskWrapper<TaskType>;
    ::tbb::empty_task& p = *new (wrapper->allocate_continuation())::tbb::empty_task();

    Task& lchild = *new (p.allocate_child()) Task(left);
    Task& rchild = *new (p.allocate_child()) Task(right);
    p.set_ref_count(2);
    ::tbb::task::spawn(lchild);
    ::tbb::task::spawn(rchild);
  }
};

#else // TBB_VERSION_MAJOR >= 2020

// In TBB version 2020, the task class was deprecated. Instead, we use the simpler task_group.

struct RadixThreaderTBB
{
  std::shared_ptr<::tbb::task_group> TaskGroup =
    std::shared_ptr<::tbb::task_group>(new ::tbb::task_group);

  size_t GetAvailableCores() const { return MAX_CORES; }

  template <typename TaskType>
  void RunParentTask(TaskType task) const
  {
    this->TaskGroup->run_and_wait(task);
    // All tasks should be complete at this point.
  }

  template <typename TaskType>
  void RunChildTasks(void*, TaskType left, TaskType right) const
  {
    this->TaskGroup->run(left);
    this->TaskGroup->run(right);
  }
};

#endif

VISKORES_INSTANTIATE_RADIX_SORT_FOR_THREADER(RadixThreaderTBB)
}
}
}
} // viskores::cont::tbb::sort
