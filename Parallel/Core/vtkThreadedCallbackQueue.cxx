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
void vtkThreadedCallbackQueue::SetNumberOfThreads(int numberOfThreads)
{
  ::Execute(
    this->Controller,
    [](vtkThreadedCallbackQueue* self, int n) {
      if (static_cast<std::size_t>(n) == self->Threads.size())
      {
        return;
      }

      // We need to use SerialStop so it is not pushed into the controller's queue.
      // If someone calls Start after SetNumberOfThreads and Start is inserted before Stop,
      // then we enter a deadlock. Serializing the Stop call prevents that.
      self->SerialStop(self);
      self->NumberOfThreads = n;
      self->Threads.resize(n);
    },
    this, numberOfThreads);
}

//-----------------------------------------------------------------------------
void vtkThreadedCallbackQueue::SerialStop(vtkThreadedCallbackQueue* self)
{
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
}

//-----------------------------------------------------------------------------
void vtkThreadedCallbackQueue::Stop()
{
  ::Execute(this->Controller, this->SerialStop, this);
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

      for (std::thread& thread : self->Threads)
      {
        thread = std::thread([self] {
          while (self->Running && (!self->Destroying || !self->Empty))
          {
            self->Pop();
          }
        });
      }
    },
    this);
}

//-----------------------------------------------------------------------------
void vtkThreadedCallbackQueue::Sync()
{
  for (std::thread& thread : this->Threads)
  {
    thread.join();
  }
}

//-----------------------------------------------------------------------------
void vtkThreadedCallbackQueue::Pop()
{
  std::unique_lock<std::mutex> lock(this->Mutex);

  if (!this->Destroying && this->Workers.empty())
  {
    this->ConditionVariable.wait(
      lock, [this] { return !this->Workers.empty() || !this->Running || this->Destroying; });
  }

  if (!this->Running || this->Workers.empty())
  {
    return;
  }

  WorkerPointer worker = std::move(this->Workers.front());
  this->Workers.pop();
  this->Empty = this->Workers.empty();

  lock.unlock();

  (*worker)();
}

//-----------------------------------------------------------------------------
void vtkThreadedCallbackQueue::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  std::lock_guard<std::mutex> lock(this->Mutex);
  os << indent << "Threads: " << this->NumberOfThreads << std::endl;
  os << indent << "Functions to execute: " << this->Workers.size() << std::endl;
  os << indent << "Queue is" << (this->Running ? " not" : "") << " running" << std::endl;
}

VTK_ABI_NAMESPACE_END
