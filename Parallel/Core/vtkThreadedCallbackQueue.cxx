/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreadedCallbackQueue.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkThreadedCallbackQueue.h"
#include "vtkObjectFactory.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkThreadedCallbackQueue);

//-----------------------------------------------------------------------------
vtkThreadedCallbackQueue* vtkThreadedCallbackQueue::New(vtkThreadedCallbackQueue* controller)
{
  vtkThreadedCallbackQueue* result = new vtkThreadedCallbackQueue(controller);
  result->InitializeObjectBase();
  return result;
}

//=============================================================================
class vtkThreadedCallbackQueue::ThreadWorker
{
public:
  ThreadWorker(vtkThreadedCallbackQueue* queue, int threadId)
    : Queue(queue)
    , ThreadId(threadId)
  {
  }

  void operator()()
  {
    while (this->Pop())
      ;
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
    InvokerBasePointer invoker = std::move(invokerQueue.front());
    invokerQueue.pop_front();

    lock.unlock();

    (*invoker)();

    this->Queue->SignalDependentSharedFutures(invoker.get());

    return true;
  }

  /**
   * Thread is on hold if its thread id is not out of bounds, while the queue is not calling
   * its destructor, while the queue is running, while the queue is empty.
   */
  bool OnHold() const
  {
    return this->ThreadId < this->Queue->NumberOfThreads && !this->Queue->Destroying &&
      this->Queue->InvokerQueue.empty();
  }

  /**
   * We can continue popping elements if the thread id is not out of bounds while
   * the queue is running and the queue is not empty.
   */
  bool Continue() const
  {
    return this->ThreadId < this->Queue->NumberOfThreads && !this->Queue->InvokerQueue.empty();
  }

  vtkThreadedCallbackQueue* Queue;
  int ThreadId;
};

namespace
{
//-----------------------------------------------------------------------------
template <class FT, class... ArgsT>
void Execute(vtkThreadedCallbackQueue* controller, FT&& f, ArgsT&&... args)
{
  if (controller)
  {
    controller->Push(std::forward<FT>(f), std::forward<ArgsT>(args)...);
  }
  else
  {
    f(std::forward<ArgsT>(args)...);
  }
}
} // anonymous namespace

//-----------------------------------------------------------------------------
vtkThreadedCallbackQueue::vtkThreadedCallbackQueue()
  : vtkThreadedCallbackQueue(
      vtkSmartPointer<vtkThreadedCallbackQueue>::Take(vtkThreadedCallbackQueue::New(nullptr)))
{
}

//-----------------------------------------------------------------------------
vtkThreadedCallbackQueue::vtkThreadedCallbackQueue(
  vtkSmartPointer<vtkThreadedCallbackQueue>&& controller)
  : NumberOfThreads(1)
  , Threads(NumberOfThreads)
  , Controller(controller)
{
  this->Start();
}

//-----------------------------------------------------------------------------
vtkThreadedCallbackQueue::~vtkThreadedCallbackQueue()
{
  // By deleting the controller, we ensure that all the Start()
  // and SetNumberOfThreads() calls are terminated and that we have a sane state
  // of our queue.
  this->Controller = nullptr;

  {
    std::lock_guard<std::mutex> lock(this->Mutex);
    this->Destroying = true;
  }

  this->ConditionVariable.notify_all();
  this->Sync();
}

//-----------------------------------------------------------------------------
void vtkThreadedCallbackQueue::SetNumberOfThreads(int numberOfThreads)
{
  ::Execute(this->Controller, [this, numberOfThreads]() {
    int size = static_cast<int>(this->Threads.size());

    if (size == numberOfThreads)
    {
      // Nothing to do
      return;
    }
    // We only need to protect the shared atomic NumberOfThreads if we are shrinking.
    else if (size < numberOfThreads)
    {
      this->NumberOfThreads = numberOfThreads;
    }
    else
    {
      std::lock_guard<std::mutex> lock(this->Mutex);
      this->NumberOfThreads = numberOfThreads;
    }

    // If we are expanding the number of threads, then we just need to spawn
    // the missing threads.
    if (size < numberOfThreads)
    {
      std::generate_n(std::back_inserter(this->Threads), numberOfThreads - size,
        [this] { return std::thread(ThreadWorker(this, static_cast<int>(this->Threads.size()))); });
    }
    // If we are shrinking the number of threads, let's notify all threads
    // so the threads whose id is more than the updated NumberOfThreads terminate.
    else
    {
      this->ConditionVariable.notify_all();
      this->Sync(this->NumberOfThreads);
      this->Threads.resize(numberOfThreads);
    }
  });
}

//-----------------------------------------------------------------------------
void vtkThreadedCallbackQueue::Start()
{
  ::Execute(this->Controller, [this]() {
    int threadId = -1;
    std::generate(this->Threads.begin(), this->Threads.end(),
      [this, &threadId] { return std::thread(ThreadWorker(this, ++threadId)); });
  });
}

//-----------------------------------------------------------------------------
void vtkThreadedCallbackQueue::Sync(int startId)
{
  std::for_each(this->Threads.begin() + startId, this->Threads.end(),
    [](std::thread& thread) { thread.join(); });
}

//-----------------------------------------------------------------------------
void vtkThreadedCallbackQueue::SignalDependentSharedFutures(const InvokerBase* invoker)
{
  // We put invokers to launch in a separate container so we can separate the usage of mutexes as
  // much as possible
  std::vector<InvokerBasePointer> invokersToLaunch;
  {
    auto invokerState = invoker->GetSharedState();

    // We are iterating on our dependents, which mean we cannot let any dependent add themselves to
    // this container. At this point we're "ready" anyway so no dependents should be waiting in most
    // cases.
    std::lock_guard<std::mutex> lock(invokerState->Mutex);
    for (auto& future : invokerState->DependentSharedFutures)
    {
      auto futureState = future->GetSharedState();

      // We're locking the dependent future. When the lock is released, either the future is not
      // done constructing and we have nothing to do, we can let it run itself, or the future is
      // done constructing, in which case if we hit zero prior futures remaining, we've gotta move
      // its associated invoker in the running queue.
      std::unique_lock<std::mutex> futureLock(futureState->Mutex);
      --futureState->NumberOfPriorSharedFuturesRemaining;
      if (!futureState->Constructing && !futureState->NumberOfPriorSharedFuturesRemaining)
      {
        // We can unlock at this point, we don't touch the future anymore
        futureLock.unlock();
        InvokerBasePointer waitingInvoker = [this, &future] {
          std::lock_guard<std::mutex> onHoldLock(this->OnHoldMutex);
          auto it = this->InvokersOnHold.find(future);
          InvokerBasePointer tmp = std::move(it->second);
          this->InvokersOnHold.erase(it);
          return tmp;
        }();

        // Invoker is high priority if it comes from vtkThreadedCallbackQueue::Wait for example.
        if (waitingInvoker->IsHighPriority)
        {
          (*waitingInvoker)();
          this->SignalDependentSharedFutures(waitingInvoker.get());
        }
        else
        {
          invokersToLaunch.emplace_back(std::move(waitingInvoker));
        }
      }
    }
  }
  if (!invokersToLaunch.empty())
  {
    std::lock_guard<std::mutex> lock(this->Mutex);
    for (InvokerBasePointer& inv : invokersToLaunch)
    {
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
void vtkThreadedCallbackQueue::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  std::lock_guard<std::mutex> lock1(this->Mutex), lock2(this->OnHoldMutex);
  os << indent << "Threads: " << this->NumberOfThreads << std::endl;
  os << indent << "Callback queue size: " << this->InvokerQueue.size() << std::endl;
  os << indent << "Number of functions on hold: " << this->InvokersOnHold.size() << std::endl;
}

VTK_ABI_NAMESPACE_END
