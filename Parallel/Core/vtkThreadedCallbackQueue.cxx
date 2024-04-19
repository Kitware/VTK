// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkThreadedCallbackQueue.h"
#include "vtkObjectFactory.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkThreadedCallbackQueue);

//=============================================================================
class vtkThreadedCallbackQueue::ThreadWorker
{
public:
  ThreadWorker(vtkThreadedCallbackQueue* queue, std::shared_ptr<std::atomic_int>& threadIndex)
    : Queue(queue)
    , ThreadIndex(threadIndex)
  {
  }

  ThreadWorker(ThreadWorker&& other) noexcept
    : Queue(other.Queue)
    , ThreadIndex(std::move(other.ThreadIndex))
  {
  }

  void operator()()
  {
    while (this->Pop())
    {
    }
    std::lock_guard<std::mutex> lock(this->Queue->ControlMutex);
    this->Queue->ThreadIdToIndex.erase(std::this_thread::get_id());
  }

private:
  /**
   * Pops an invoker from the queue and runs it if the queue is running and if the thread
   * is in service (meaning its thread id is still higher than Queue->NumberOfThreads).
   * It returns true if the queue has been able to be popped and false otherwise.
   */
  bool Pop()
  {
    std::unique_lock<std::mutex> lock(this->Queue->Mutex);

    if (this->OnHold())
    {
      this->Queue->ConditionVariable.wait(lock, [this] { return !this->OnHold(); });
    }

    // Note that if the queue is empty at this point, it means that either the current thread id
    // is now out of bound, or the queue is being destroyed.
    if (!this->Continue())
    {
      return false;
    }

    auto& invokerQueue = this->Queue->InvokerQueue;

    SharedFutureBasePointer invoker = std::move(invokerQueue.front());
    invokerQueue.pop_front();

    // invoker was in the queue which is locked. No one will be touching Status, so we do not need
    // to lock invoker->Mutex
    invoker->Status.store(RUNNING, std::memory_order_release);

    this->Queue->PopFrontNullptr();
    lock.unlock();

    this->Queue->Invoke(std::move(invoker));

    return true;
  }

  /**
   * Thread is on hold if its thread id is not out of bounds, while the queue is not calling
   * its destructor, while the queue is running, while the queue is empty.
   */
  bool OnHold() const
  {
    return *this->ThreadIndex < this->Queue->NumberOfThreads && !this->Queue->Destroying &&
      this->Queue->InvokerQueue.empty();
  }

  /**
   * We can continue popping elements if the thread id is not out of bounds while
   * the queue is running and the queue is not empty.
   */
  bool Continue() const
  {
    return *this->ThreadIndex < this->Queue->NumberOfThreads && !this->Queue->InvokerQueue.empty();
  }

  vtkThreadedCallbackQueue* Queue;
  std::shared_ptr<std::atomic_int> ThreadIndex;
};

//-----------------------------------------------------------------------------
vtkThreadedCallbackQueue::vtkThreadedCallbackQueue()
{
  this->SetNumberOfThreads(1);
}

//-----------------------------------------------------------------------------
vtkThreadedCallbackQueue::~vtkThreadedCallbackQueue()
{
  {
    std::lock_guard<std::mutex> destroyLock(this->DestroyMutex);
    {
      std::lock_guard<std::mutex> lock(this->Mutex);
      this->Destroying = true;
    }
  }

  this->ConditionVariable.notify_all();
  this->Sync();
}

//-----------------------------------------------------------------------------
void vtkThreadedCallbackQueue::SetNumberOfThreads(int numberOfThreads)
{
  this->PushControl([this, numberOfThreads]() {
    int size = static_cast<int>(this->Threads.size());

    std::lock_guard<std::mutex> destroyLock(this->DestroyMutex);
    if (this->Destroying)
    {
      return;
    }
    if (size == numberOfThreads)
    {
      // Nothing to do
      return;
    }
    // If we are expanding the number of threads, then we just need to spawn
    // the missing threads.
    else if (size < numberOfThreads)
    {
      this->NumberOfThreads = numberOfThreads;

      std::generate_n(std::back_inserter(this->Threads), numberOfThreads - size, [this] {
        auto threadIndex =
          std::make_shared<std::atomic_int>(static_cast<int>(this->Threads.size()));
        auto thread = std::thread(ThreadWorker(this, threadIndex));
        {
          std::lock_guard<std::mutex> threadIdLock(this->ThreadIdToIndexMutex);
          this->ThreadIdToIndex.emplace(thread.get_id(), threadIndex);
        }
        return thread;
      });
    }
    // If we are shrinking the number of threads, let's notify all threads
    // so the threads whose id is more than the updated NumberOfThreads terminate.
    else
    {
      // If we have a thread index larger than the new number of threads, we swap ourself with
      // thread 0. We now know we will live after this routine and can synchronize terminating
      // threads ourselves.
      {
        std::unique_lock<std::mutex> lock(this->ThreadIdToIndexMutex);
        std::atomic_int& threadIndex = *this->ThreadIdToIndex.at(std::this_thread::get_id());
        if (threadIndex && threadIndex >= numberOfThreads)
        {
          std::atomic_int& thread0Index = *this->ThreadIdToIndex.at(this->Threads[0].get_id());
          lock.unlock();

          std::swap(this->Threads[threadIndex], this->Threads[0]);

          // Swapping the value of atomic ThreadIndex inside ThreadWorker.
          int tmp = thread0Index;
          thread0Index.exchange(threadIndex);
          threadIndex = tmp;
        }
      }

      {
        std::lock_guard<std::mutex> lock(this->Mutex);
        this->NumberOfThreads = numberOfThreads;
      }
      this->ConditionVariable.notify_all();
      this->Sync(this->NumberOfThreads);

      // Excess threads are done, we can resize
      this->Threads.resize(numberOfThreads);
    }
  });
}

//-----------------------------------------------------------------------------
void vtkThreadedCallbackQueue::Sync(int startId)
{
  std::for_each(this->Threads.begin() + startId, this->Threads.end(),
    [](std::thread& thread) { thread.join(); });
}

//-----------------------------------------------------------------------------
void vtkThreadedCallbackQueue::PopFrontNullptr()
{
  while (!this->InvokerQueue.empty() && !this->InvokerQueue.front())
  {
    this->InvokerQueue.pop_front();
  }
}

//-----------------------------------------------------------------------------
void vtkThreadedCallbackQueue::Invoke(vtkSharedFutureBase* invoker)
{
  (*invoker)();
  this->SignalDependentSharedFutures(invoker);
}

//-----------------------------------------------------------------------------
void vtkThreadedCallbackQueue::SignalDependentSharedFutures(vtkSharedFutureBase* invoker)
{
  // We put invokers to launch in a separate container so we can separate the usage of mutexes as
  // much as possible
  std::vector<SharedFutureBasePointer> invokersToLaunch;
  {
    // We are iterating on our dependents, which mean we cannot let any dependent add themselves to
    // this container. At this point we're "ready" anyway so no dependents should be waiting in most
    // cases.
    std::lock_guard<std::mutex> lock(invoker->Mutex);

    for (auto& dependent : invoker->Dependents)
    {
      // We're locking the dependent future. When the lock is released, either the future is not
      // done constructing and we have nothing to do, we can let it run itself, or the future is
      // done constructing, in which case if we hit zero prior futures remaining, we've gotta move
      // its associated invoker in the running queue.
      std::unique_lock<std::mutex> dependentLock(dependent->Mutex);
      --dependent->NumberOfPriorSharedFuturesRemaining;
      if (dependent->Status.load(std::memory_order_acquire) == ON_HOLD &&
        !dependent->NumberOfPriorSharedFuturesRemaining)
      {
        // Invoker is high priority if it comes from vtkThreadedCallbackQueue::Wait for example.
        if (dependent->IsHighPriority)
        {
          dependent->Status.store(RUNNING, std::memory_order_release);
          dependentLock.unlock();
          this->Invoke(dependent);
        }
        else
        {
          // We can unlock at this point, we don't touch the future anymore
          dependentLock.unlock();
          invokersToLaunch.emplace_back(std::move(dependent));
        }
      }
    }
  }
  if (!invokersToLaunch.empty())
  {
    std::lock_guard<std::mutex> lock(this->Mutex);
    // We need to handle the invoker index.
    // If the InvokerQueue is empty, then we set it such that after this look, the front has index
    // 0.
    vtkIdType index = this->InvokerQueue.empty() ? static_cast<vtkIdType>(invokersToLaunch.size())
                                                 : this->InvokerQueue.front()->InvokerIndex;
    for (SharedFutureBasePointer& inv : invokersToLaunch)
    {
      assert(inv->Status.load(std::memory_order_acquire) == ON_HOLD && "Status should be ON_HOLD");
      inv->InvokerIndex = --index;

      std::lock_guard<std::mutex> stateLock(inv->Mutex);
      inv->Status.store(ENQUEUED, std::memory_order_release);

      // This dependent has been waiting enough, let's give him some priority.
      // Anyway, the invoker is past due if it was put inside InvokersOnHold.
      this->InvokerQueue.emplace_front(std::move(inv));
    }
  }
  for (std::size_t i = 0; i < invokersToLaunch.size(); ++i)
  {
    this->ConditionVariable.notify_one();
  }
}

//-----------------------------------------------------------------------------
bool vtkThreadedCallbackQueue::TryInvoke(vtkSharedFutureBase* invoker)
{
  if (![this, &invoker] {
        // We need to check again if we cannot run in case the thread worker just popped this
        // invoker. We are guarded by this->Mutex so there cannot be a conflict here.
        if (invoker->Status.load(std::memory_order_relaxed) != ENQUEUED)
        {
          // Someone picked up the invoker right before us, we can abort.
          return false;
        }

        std::lock_guard<std::mutex> lock(this->Mutex);

        if (this->InvokerQueue.empty())
        {
          return false;
        }

        std::lock_guard<std::mutex> invLock(invoker->Mutex);

        if (invoker->Status.load(std::memory_order_acquire) != ENQUEUED)
        {
          // Recheck if someone picked up the invoker right before us.
          return false;
        }

        // There has to be a front if we are here.
        vtkIdType index = invoker->InvokerIndex - this->InvokerQueue.front()->InvokerIndex;

        // When index is negative, it means that the invoker we want to run is already being
        // handled by the "normal" path of the queue. The invoker is locked by a mutex, we must
        // release it.
        if (index < 0)
        {
          return false;
        }

        SharedFutureBasePointer& result = this->InvokerQueue[index];

        // Someone has reinserted in the front another invoker. invoker is already running.
        if (result != invoker)
        {
          return false;
        }

        // If we just picked the front invoker, let's pop the queue.
        if (index == 0)
        {
          this->InvokerQueue.pop_front();
          this->PopFrontNullptr();
        }
        invoker->Status.store(RUNNING, std::memory_order_release);
        return true;
      }())
  {
    return false;
  }

  this->Invoke(invoker);
  return true;
}

//-----------------------------------------------------------------------------
void vtkThreadedCallbackQueue::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  std::lock_guard<std::mutex> lock1(this->Mutex);
  os << indent << "Threads: " << this->NumberOfThreads << std::endl;
  os << indent << "Callback queue size: " << this->InvokerQueue.size() << std::endl;
}

VTK_ABI_NAMESPACE_END
