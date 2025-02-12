// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLogger.h"

#include "vtkMultiThreader.h"

#include <cassert>
#include <queue>
#include <utility>

//=============================================================================
namespace vtkThreadedTaskQueueInternals
{
VTK_ABI_NAMESPACE_BEGIN

template <typename R>
class TaskQueue
{
public:
  TaskQueue(int buffer_size)
    : Done(false)
    , BufferSize(buffer_size)
    , NextTaskId(0)
  {
  }

  ~TaskQueue() = default;

  void MarkDone()
  {
    {
      std::lock_guard<std::mutex> lock(this->TasksMutex);
      this->Done = true;
    }
    this->TasksCV.notify_all();
  }

  std::uint64_t GetNextTaskId() const { return this->NextTaskId; }

  void Push(std::function<R()>&& task)
  {
    if (this->Done)
    {
      return;
    }
    else
    {
      std::lock_guard<std::mutex> lk(this->TasksMutex);
      // vtkLogF(INFO, "pushing-task %d", (int)this->NextTaskId);
      this->Tasks.push(std::make_pair(this->NextTaskId++, std::move(task)));
      while (this->BufferSize > 0 && static_cast<int>(this->Tasks.size()) > this->BufferSize)
      {
        this->Tasks.pop();
      }
    }
    this->TasksCV.notify_one();
  }

  bool Pop(std::uint64_t& task_id, std::function<R()>& task)
  {
    std::unique_lock<std::mutex> lk(this->TasksMutex);
    this->TasksCV.wait(lk, [this] { return this->Done || !this->Tasks.empty(); });
    if (!this->Tasks.empty())
    {
      auto task_pair = this->Tasks.front();
      // vtkLogF(TRACE, "popping-task %d", (int)task_pair.first);
      this->Tasks.pop();
      lk.unlock();

      task_id = task_pair.first;
      task = std::move(task_pair.second);
      return true;
    }
    assert(this->Done);
    return false;
  }

private:
  std::atomic_bool Done;
  int BufferSize;
  std::atomic<std::uint64_t> NextTaskId;
  std::queue<std::pair<std::uint64_t, std::function<R()>>> Tasks;
  std::mutex TasksMutex;
  std::condition_variable TasksCV;
};

//=============================================================================
template <typename R>
class ResultQueue
{
public:
  ResultQueue(bool strict_ordering)
    : NextResultId(0)
    , StrictOrdering(strict_ordering)
  {
  }

  ~ResultQueue() = default;

  std::uint64_t GetNextResultId() const { return this->NextResultId; }

  void Push(std::uint64_t task_id, const R&& result)
  {
    std::unique_lock<std::mutex> lk(this->ResultsMutex);
    // don't save this result if it's obsolete.
    if (task_id >= this->NextResultId)
    {
      this->Results.push(std::make_pair(task_id, std::move(result)));
    }
    lk.unlock();
    this->ResultsCV.notify_one();
  }

  bool TryPop(R& result)
  {
    std::unique_lock<std::mutex> lk(this->ResultsMutex);
    if (this->Results.empty() ||
      (this->StrictOrdering && this->Results.top().first != this->NextResultId))
    {
      // results are not available or of strict-ordering is requested, the
      // result available is not the next one in sequence, hence don't pop
      // anything.
      return false;
    }

    auto result_pair = this->Results.top();
    this->NextResultId = (result_pair.first + 1);
    this->Results.pop();
    lk.unlock();

    result = std::move(result_pair.second);
    return true;
  }

  bool Pop(R& result)
  {
    std::unique_lock<std::mutex> lk(this->ResultsMutex);
    this->ResultsCV.wait(lk,
      [this]
      {
        return !this->Results.empty() &&
          (!this->StrictOrdering || this->Results.top().first == this->NextResultId);
      });
    lk.unlock();
    return this->TryPop(result);
  }

private:
  template <typename T>
  struct Comparator
  {
    bool operator()(const T& left, const T& right) const { return left.first > right.first; }
  };
  std::priority_queue<std::pair<std::uint64_t, R>, std::vector<std::pair<std::uint64_t, R>>,
    Comparator<std::pair<std::uint64_t, R>>>
    Results;
  std::mutex ResultsMutex;
  std::condition_variable ResultsCV;
  std::atomic<std::uint64_t> NextResultId;
  bool StrictOrdering;
};

VTK_ABI_NAMESPACE_END
}

VTK_ABI_NAMESPACE_BEGIN
//-----------------------------------------------------------------------------
template <typename R, typename... Args>
vtkThreadedTaskQueue<R, Args...>::vtkThreadedTaskQueue(
  std::function<R(Args...)> worker, bool strict_ordering, int buffer_size, int max_concurrent_tasks)
  : Worker(worker)
  , Tasks(new vtkThreadedTaskQueueInternals::TaskQueue<R>(
      std::max(0, strict_ordering ? 0 : buffer_size)))
  , Results(new vtkThreadedTaskQueueInternals::ResultQueue<R>(strict_ordering))
  , NumberOfThreads(max_concurrent_tasks <= 0 ? vtkMultiThreader::GetGlobalDefaultNumberOfThreads()
                                              : max_concurrent_tasks)
  , Threads{ new std::thread[this->NumberOfThreads] }
{
  auto f = [this](int thread_id)
  {
    vtkLogger::SetThreadName("ttq::worker" + std::to_string(thread_id));
    while (true)
    {
      std::function<R()> task;
      std::uint64_t task_id;
      if (this->Tasks->Pop(task_id, task))
      {
        this->Results->Push(task_id, task());
        continue;
      }
      else
      {
        break;
      }
    }
    // vtkLogF(INFO, "done");
  };

  for (int cc = 0; cc < this->NumberOfThreads; ++cc)
  {
    this->Threads[cc] = std::thread(f, cc);
  }
}

//-----------------------------------------------------------------------------
template <typename R, typename... Args>
vtkThreadedTaskQueue<R, Args...>::~vtkThreadedTaskQueue()
{
  this->Tasks->MarkDone();
  for (int cc = 0; cc < this->NumberOfThreads; ++cc)
  {
    this->Threads[cc].join();
  }
}

//-----------------------------------------------------------------------------
template <typename R, typename... Args>
void vtkThreadedTaskQueue<R, Args...>::Push(Args&&... args)
{
  this->Tasks->Push([this, arguments = std::make_tuple(std::forward<Args>(args)...)]()
    { return std::apply(this->Worker, arguments); });
}

//-----------------------------------------------------------------------------
template <typename R, typename... Args>
bool vtkThreadedTaskQueue<R, Args...>::TryPop(R& result)
{
  return this->Results->TryPop(result);
}

//-----------------------------------------------------------------------------
template <typename R, typename... Args>
bool vtkThreadedTaskQueue<R, Args...>::Pop(R& result)
{
  if (this->IsEmpty())
  {
    return false;
  }

  return this->Results->Pop(result);
}

//-----------------------------------------------------------------------------
template <typename R, typename... Args>
bool vtkThreadedTaskQueue<R, Args...>::IsEmpty() const
{
  return this->Results->GetNextResultId() == this->Tasks->GetNextTaskId();
}

//-----------------------------------------------------------------------------
template <typename R, typename... Args>
void vtkThreadedTaskQueue<R, Args...>::Flush()
{
  R tmp;
  while (!this->IsEmpty())
  {
    this->Pop(tmp);
  }
}

//=============================================================================
// ** specialization for `void` returns types.
//=============================================================================

//-----------------------------------------------------------------------------
template <typename... Args>
vtkThreadedTaskQueue<void, Args...>::vtkThreadedTaskQueue(std::function<void(Args...)> worker,
  bool strict_ordering, int buffer_size, int max_concurrent_tasks)
  : Worker(worker)
  , Tasks(new vtkThreadedTaskQueueInternals::TaskQueue<void>(
      std::max(0, strict_ordering ? 0 : buffer_size)))
  , NextResultId(0)
  , NumberOfThreads(max_concurrent_tasks <= 0 ? vtkMultiThreader::GetGlobalDefaultNumberOfThreads()
                                              : max_concurrent_tasks)
  , Threads{ new std::thread[this->NumberOfThreads] }
{
  auto f = [this](int thread_id)
  {
    vtkLogger::SetThreadName("ttq::worker" + std::to_string(thread_id));
    while (true)
    {
      std::function<void()> task;
      std::uint64_t task_id;
      if (this->Tasks->Pop(task_id, task))
      {
        task();

        std::unique_lock<std::mutex> lk(this->NextResultIdMutex);
        this->NextResultId = std::max(static_cast<std::uint64_t>(this->NextResultId), task_id + 1);
        lk.unlock();
        this->ResultsCV.notify_all();
        continue;
      }
      else
      {
        break;
      }
    }
    this->ResultsCV.notify_all();
    // vtkLogF(INFO, "done");
  };

  for (int cc = 0; cc < this->NumberOfThreads; ++cc)
  {
    this->Threads[cc] = std::thread(f, cc);
  }
}

//-----------------------------------------------------------------------------
template <typename... Args>
vtkThreadedTaskQueue<void, Args...>::~vtkThreadedTaskQueue()
{
  this->Tasks->MarkDone();
  for (int cc = 0; cc < this->NumberOfThreads; ++cc)
  {
    this->Threads[cc].join();
  }
}

//-----------------------------------------------------------------------------
template <typename... Args>
void vtkThreadedTaskQueue<void, Args...>::Push(Args&&... args)
{
  this->Tasks->Push([this, arguments = std::make_tuple(std::forward<Args>(args)...)]()
    { std::apply(this->Worker, arguments); });
}

//-----------------------------------------------------------------------------
template <typename... Args>
bool vtkThreadedTaskQueue<void, Args...>::IsEmpty() const
{
  return this->NextResultId == this->Tasks->GetNextTaskId();
}

//-----------------------------------------------------------------------------
template <typename... Args>
void vtkThreadedTaskQueue<void, Args...>::Flush()
{
  if (this->IsEmpty())
  {
    return;
  }
  std::unique_lock<std::mutex> lk(this->NextResultIdMutex);
  this->ResultsCV.wait(lk, [this] { return this->IsEmpty(); });
}
VTK_ABI_NAMESPACE_END
