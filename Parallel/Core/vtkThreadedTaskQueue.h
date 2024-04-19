// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkThreadedTaskQueue
 * @brief simple threaded task queue
 *
 * vtkThreadedTaskQueue provides a simple task queue that can use threads to
 * execute individual tasks. It is intended for use applications such as data
 * compression, encoding etc. where the task may be completed concurrently
 * without blocking the main thread.
 *
 * vtkThreadedTaskQueue's API is intended to called from the same main thread.
 * The constructor defines the work (or task) to be performed. `Push` allows the
 * caller to enqueue a task with specified input arguments. The call will return
 * immediately without blocking. The task is enqueued and will be executed
 * concurrently when resources become available.  `Pop` will block until the
 * result is available. To avoid waiting for results to be available, use
 * `TryPop`.
 *
 * The constructor allows mechanism to customize the queue. `strict_ordering`
 * implies that results should be popped in the same order that tasks were
 * pushed without dropping any task. If the caller is only concerned with
 * obtaining the latest available result where intermediate results that take
 * longer to compute may be dropped, then `strict_ordering` can be set to `false`.
 *
 * `max_concurrent_tasks` controls how many threads are used to process tasks in
 * the queue. Default is same as
 * `vtkMultiThreader::GetGlobalDefaultNumberOfThreads()`.
 *
 * `buffer_size` indicates how many tasks may be queued for processing. Default
 * is infinite size. If a positive number is provided, then pushing additional
 * tasks will result in discarding of older tasks that haven't begun processing
 * from the queue. Note, this does not impact tasks that may already be in
 * progress. Also, if `strict_ordering` is true, this is ignored; the
 * buffer_size will be set to unlimited.
 *
 */

#ifndef vtkThreadedTaskQueue_h
#define vtkThreadedTaskQueue_h

#include "vtkObject.h"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

#if !defined(__WRAP__)
namespace vtkThreadedTaskQueueInternals
{
VTK_ABI_NAMESPACE_BEGIN
template <typename R>
class TaskQueue;

template <typename R>
class ResultQueue;
VTK_ABI_NAMESPACE_END
}

VTK_ABI_NAMESPACE_BEGIN

template <typename R, typename... Args>
class vtkThreadedTaskQueue
{
public:
  vtkThreadedTaskQueue(std::function<R(Args...)> worker, bool strict_ordering = true,
    int buffer_size = -1, int max_concurrent_tasks = -1);
  ~vtkThreadedTaskQueue();

  /**
   * Push arguments for the work
   */
  void Push(Args&&... args);

  /**
   * Pop the last result. Returns true on success. May fail if called on an
   * empty queue. This will wait for result to be available.
   */
  bool Pop(R& result);

  /**
   * Attempt to pop without waiting. If not results are available, returns
   * false.
   */
  bool TryPop(R& result);

  /**
   * Returns false if there's some result that may be popped right now or in the
   * future.
   */
  bool IsEmpty() const;

  /**
   * Blocks till the queue becomes empty.
   */
  void Flush();

private:
  vtkThreadedTaskQueue(const vtkThreadedTaskQueue&) = delete;
  void operator=(const vtkThreadedTaskQueue&) = delete;

  std::function<R(Args...)> Worker;

  std::unique_ptr<vtkThreadedTaskQueueInternals::TaskQueue<R>> Tasks;
  std::unique_ptr<vtkThreadedTaskQueueInternals::ResultQueue<R>> Results;

  int NumberOfThreads;
  std::unique_ptr<std::thread[]> Threads;
};

template <typename... Args>
class vtkThreadedTaskQueue<void, Args...>
{
public:
  vtkThreadedTaskQueue(std::function<void(Args...)> worker, bool strict_ordering = true,
    int buffer_size = -1, int max_concurrent_tasks = -1);
  ~vtkThreadedTaskQueue();

  /**
   * Push arguments for the work
   */
  void Push(Args&&... args);

  /**
   * Returns false if there's some result that may be popped right now or in the
   * future.
   */
  bool IsEmpty() const;

  /**
   * Blocks till the queue becomes empty.
   */
  void Flush();

private:
  vtkThreadedTaskQueue(const vtkThreadedTaskQueue&) = delete;
  void operator=(const vtkThreadedTaskQueue&) = delete;

  std::function<void(Args...)> Worker;

  std::unique_ptr<vtkThreadedTaskQueueInternals::TaskQueue<void>> Tasks;

  std::condition_variable ResultsCV;
  std::mutex NextResultIdMutex;
  std::atomic<std::uint64_t> NextResultId;

  int NumberOfThreads;
  std::unique_ptr<std::thread[]> Threads;
};

VTK_ABI_NAMESPACE_END
#include "vtkThreadedTaskQueue.txx"

#endif // !defined(__WRAP__)

#endif
// VTK-HeaderTest-Exclude: vtkThreadedTaskQueue.h
