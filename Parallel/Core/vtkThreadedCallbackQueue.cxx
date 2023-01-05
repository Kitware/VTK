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
#include <functional>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkThreadedCallbackQueue);

//=============================================================================
// This class is basically vtkThreadedCallbackQueue.
// Its uses the nullptr constructor of its parent to avoid infinite recursion
// on the Controller member.
// It is the only way to call the contructor with a parameter because New() doesn't accept
// arguments.
class vtkThreadedCallbackQueue::vtkInternalController : public vtkThreadedCallbackQueue
{
public:
  static vtkInternalController* New();
  vtkTypeMacro(vtkInternalController, vtkThreadedCallbackQueue);

  vtkInternalController()
    : vtkThreadedCallbackQueue(nullptr)
  {
  }
  ~vtkInternalController() override = default;

private:
  vtkInternalController(const vtkInternalController&) = delete;
  void operator=(const vtkInternalController&) = delete;
};

vtkStandardNewMacro(vtkThreadedCallbackQueue::vtkInternalController);

namespace
{
//-----------------------------------------------------------------------------
template <class FT, class... ArgsT>
void Execute(vtkThreadedCallbackQueue* controller, FT&& f, ArgsT&&... args)
{
  controller ? controller->Push(std::forward<FT>(f), std::forward<ArgsT>(args)...)
             : f(std::forward<ArgsT>(args)...);
}
} // anonymous namespace

//-----------------------------------------------------------------------------
vtkThreadedCallbackQueue::vtkThreadedCallbackQueue()
  : vtkThreadedCallbackQueue(vtkSmartPointer<vtkInternalController>::New())
{
}

//-----------------------------------------------------------------------------
vtkThreadedCallbackQueue::vtkThreadedCallbackQueue(
  vtkSmartPointer<vtkInternalController>&& controller)
  : Empty(true)
  , Destroying(false)
  , Running(false)
  , NumberOfThreads(1)
  , Threads(NumberOfThreads)
  , Controller(controller)
{
  if (this->Controller)
  {
    this->Controller->SetNumberOfThreads(1);
    this->Controller->Start();
  }
}

//-----------------------------------------------------------------------------
vtkThreadedCallbackQueue::~vtkThreadedCallbackQueue()
{
  // By deleting the controller, we ensure that all the Start(), Stop()
  // and SetNumberOfThreads() calls are terminated and that we have a sane state
  // of our queue.
  this->Controller = nullptr;

  if (this->Running)
  {
    {
      std::lock_guard<std::mutex> lock(this->Mutex);
      this->Destroying = true;
    }

    this->ConditionVariable.notify_all();
    this->Sync();
  }
}

//-----------------------------------------------------------------------------
void vtkThreadedCallbackQueue::ThreadRoutine(int threadId)
{
  while (threadId < this->NumberOfThreads && this->Running && (!this->Destroying || !this->Empty))
  {
    this->Pop(threadId);
  }
}

//-----------------------------------------------------------------------------
void vtkThreadedCallbackQueue::SetNumberOfThreads(int numberOfThreads)
{
  ::Execute(
    this->Controller,
    [](vtkThreadedCallbackQueue* self, int n) {
      int size = static_cast<int>(self->Threads.size());

      if (size == n)
      {
        // Nothing to do
        return;
      }
      // We only need to protect the shared atomic NumberOfThreads if we are shrinking.
      else if (size < n || !self->Running)
      {
        self->NumberOfThreads = n;
      }
      else
      {
        std::lock_guard<std::mutex> lock(self->Mutex);
        self->NumberOfThreads = n;
      }

      // If there are no threads running, we can just allocate the vector of threads.
      if (!self->Running)
      {
        self->Threads.resize(n);
        return;
      }

      // If we are expanding the number of threads, then we just need to spawn
      // the missing threads.
      if (size < n)
      {
        std::generate_n(std::back_inserter(self->Threads), n - size, [&self] {
          return std::thread(
            std::bind(&vtkThreadedCallbackQueue::ThreadRoutine, self, self->Threads.size() - 1));
        });
      }
      // If we are shrinking the number of threads, let's notify all threads
      // so the threads whose id is more than the updated NumberOfThreads terminate.
      else
      {
        self->ConditionVariable.notify_all();
        self->Sync(self->NumberOfThreads);
        self->Threads.resize(n);
      }
    },
    this, numberOfThreads);
}

//-----------------------------------------------------------------------------
void vtkThreadedCallbackQueue::Stop()
{
  ::Execute(
    this->Controller,
    [](vtkThreadedCallbackQueue* self) {
      if (!self->Running)
      {
        return;
      }

      {
        std::lock_guard<std::mutex> lock(self->Mutex);
        self->Running = false;
      }

      self->ConditionVariable.notify_all();
      self->Sync();
    },
    this);
}

//-----------------------------------------------------------------------------
void vtkThreadedCallbackQueue::Start()
{
  ::Execute(
    this->Controller,
    [](vtkThreadedCallbackQueue* self) {
      if (self->Running)
      {
        return;
      }

      self->Running = true;

      int threadId = -1;
      std::generate(self->Threads.begin(), self->Threads.end(), [&self, &threadId] {
        return std::thread(std::bind(&vtkThreadedCallbackQueue::ThreadRoutine, self, ++threadId));
      });
    },
    this);
}

//-----------------------------------------------------------------------------
void vtkThreadedCallbackQueue::Sync(int startId)
{
  std::for_each(this->Threads.begin() + startId, this->Threads.end(),
    [](std::thread& thread) { thread.join(); });
}

//-----------------------------------------------------------------------------
void vtkThreadedCallbackQueue::Pop(int threadId)
{
  std::unique_lock<std::mutex> lock(this->Mutex);

  if (threadId < this->NumberOfThreads && !this->Destroying && this->InvokerQueue.empty())
  {
    this->ConditionVariable.wait(lock, [this, &threadId] {
      return threadId >= this->NumberOfThreads || !this->InvokerQueue.empty() || !this->Running ||
        this->Destroying;
    });
  }

  if (threadId >= this->NumberOfThreads || !this->Running || this->InvokerQueue.empty())
  {
    return;
  }

  InvokerPointer worker = std::move(this->InvokerQueue.front());
  this->InvokerQueue.pop();
  this->Empty = this->InvokerQueue.empty();

  lock.unlock();

  (*worker)();
}

//-----------------------------------------------------------------------------
void vtkThreadedCallbackQueue::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  std::lock_guard<std::mutex> lock(this->Mutex);
  os << indent << "Threads: " << this->NumberOfThreads << std::endl;
  os << indent << "Callback queue size: " << this->InvokerQueue.size() << std::endl;
  os << indent << "Queue is" << (this->Running ? " not" : "") << " running" << std::endl;
}

VTK_ABI_NAMESPACE_END
