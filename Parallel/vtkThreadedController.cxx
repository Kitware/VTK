/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkThreadedController.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkThreadedController.h"
#include "vtkObjectFactory.h"

#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkOutputWindow.h"
#include "vtkCriticalSection.h"

#ifdef VTK_USE_SPROC
#include <sys/prctl.h>
#endif

static vtkSimpleCriticalSection vtkOutputWindowCritSect;

// Output window which prints out the process id
// with the error or warning messages
class VTK_EXPORT vtkThreadedControllerOutputWindow : public vtkOutputWindow
{
public:
  vtkTypeMacro(vtkThreadedControllerOutputWindow,vtkOutputWindow);

  void DisplayText(const char* t)
  {
    // Need to use critical section because the output window
    // is global. For the same reason, the process id has to
    // be obtained by calling GetGlobalController
    vtkOutputWindowCritSect.Lock();
    vtkMultiProcessController* cont = 
      vtkMultiProcessController::GetGlobalController();
    if (cont)
      {
      cout << "Process id: " << cont->GetLocalProcessId()
	   << " >> ";
      }
    cout << t;
    cout.flush();
    vtkOutputWindowCritSect.Unlock();
  }

  vtkThreadedControllerOutputWindow()
  {
    vtkObject* ret = vtkObjectFactory::CreateInstance("vtkThreadedControllerOutputWindow");
    if (ret)
      ret->Delete();
  }

  friend class vtkThreadedController;

protected:
  vtkThreadedControllerOutputWindow(const vtkThreadedControllerOutputWindow&);
  void operator=(const vtkThreadedControllerOutputWindow&);

};


void vtkThreadedController::CreateOutputWindow()
{
#if defined(VTK_USE_PTHREADS) || defined(VTK_USE_SPROC) || defined(VTK_USE_WIN32_THREADS)
  vtkThreadedControllerOutputWindow* window = new vtkThreadedControllerOutputWindow;
  this->OutputWindow = window;
  vtkOutputWindow::SetInstance(this->OutputWindow);
#endif
}

//----------------------------------------------------------------------------
vtkThreadedController* vtkThreadedController::New()
{
  // First try to create the object from the vtkObjectactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkThreadedController");
  if(ret)
    {
    return (vtkThreadedController*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkThreadedController;
}


//----------------------------------------------------------------------------
vtkThreadedController::vtkThreadedController()
{
  this->LocalProcessId = 0;

  vtkMultiThreader::SetGlobalMaximumNumberOfThreads(0);

  this->MultiThreader = 0;
  this->NumberOfProcesses = 0;
  this->MultipleMethodFlag = 0;
    
  this->LastNumberOfProcesses = 0;
  this->Controllers = 0;
  this->ThreadIds = 0;

  this->OutputWindow = 0;
}

//----------------------------------------------------------------------------
vtkThreadedController::~vtkThreadedController()
{
  if (this->MultiThreader)
    {
    this->MultiThreader->Delete();
    }
  
   if(this->Communicator)
     {
     this->Communicator->Delete();
     }

   this->NumberOfProcesses = 0;
   this->ResetControllers();
}

//----------------------------------------------------------------------------
void vtkThreadedController::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMultiProcessController::PrintSelf(os,indent);
  if (this->MultiThreader)
    {
    os << indent << "MultiThreader:\n";
    this->MultiThreader->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "MultiThreader: (none)\n";
    }
  os << indent << "LocalProcessId: " << this->LocalProcessId << endl;
  os << indent << "Barrier in progress: " 
     << (vtkThreadedController::IsBarrierInProgress ? "(yes)" : "(no)")
     << endl;
  os << indent << "Barrier counter: " << vtkThreadedController::Counter
     << endl;
  os << indent << "Last number of processes: " << this->LastNumberOfProcesses
     << endl;  
}

//----------------------------------------------------------------------------
void vtkThreadedController::Initialize(int* vtkNotUsed(argc), 
				       char*** vtkNotUsed(argv))
{
}
  
void vtkThreadedController::ResetControllers()
{
  int i;

  for(i=1; i < this->LastNumberOfProcesses; i++)
    {
    this->Controllers[i]->Delete();
    }

  if (this->NumberOfProcesses == this->LastNumberOfProcesses)
    {
    return;
    }
  
  delete[] this->Controllers;
  delete[] this->ThreadIds;

  if (this->NumberOfProcesses > 0 )
    {
    this->Controllers = new vtkThreadedController*[this->NumberOfProcesses];
    this->ThreadIds = new ThreadIdType[this->NumberOfProcesses];
    }
}


//----------------------------------------------------------------------------
// Called before threads are spawned to create the "process objecs".
void vtkThreadedController::CreateProcessControllers()
{

  // Delete previous controllers.
  this->ResetControllers();

  // Create the controllers.
  // The original controller will be assigned thread 0.
  this->Controllers[0] = this;
  this->LocalProcessId = 0;

  // Create a new communicator.
  if (this->Communicator)
    {
    this->Communicator->Delete();
    }
  this->Communicator = vtkSharedMemoryCommunicator::New();
  ((vtkSharedMemoryCommunicator*)this->Communicator)->Initialize(
    this->NumberOfProcesses, 
    this->ForceDeepCopy);
  this->RMICommunicator = this->Communicator;

  // Initialize the new controllers.
  for (int i = 1; i < this->NumberOfProcesses; ++i)
    {
    this->Controllers[i] = vtkThreadedController::New();
    this->Controllers[i]->LocalProcessId = i;
    this->Controllers[i]->NumberOfProcesses = this->NumberOfProcesses;
    this->Controllers[i]->Communicator = 
      ((vtkSharedMemoryCommunicator*)this->Communicator)->Communicators[i];
    this->Controllers[i]->RMICommunicator = 
      ((vtkSharedMemoryCommunicator*)this->RMICommunicator)->Communicators[i];
    }

  // Stored in case someone changes the number of processes.
  // Needed to delete the controllers properly.
  this->LastNumberOfProcesses = this->NumberOfProcesses;
}

vtkSimpleCriticalSection vtkThreadedController::CounterLock;
int vtkThreadedController::Counter;

#ifdef VTK_USE_WIN32_THREADS
HANDLE vtkThreadedController::BarrierEndedEvent = 0;
HANDLE vtkThreadedController::NextThread = 0;
#else
vtkSimpleCriticalSection vtkThreadedController::BarrierLock(1);
vtkSimpleCriticalSection vtkThreadedController::BarrierInProgress;
#endif
int vtkThreadedController::IsBarrierInProgress=0;


void vtkThreadedController::Barrier()
{
  vtkThreadedController::InitializeBarrier();

  // If there was a barrier before this one, we need to
  // wait until that is cleaned up
  if (vtkThreadedController::IsBarrierInProgress)
    {
    vtkThreadedController::WaitForPreviousBarrierToEnd();
    }

  // All processes increment the counter (which is initially 0) by 1
  vtkThreadedController::CounterLock.Lock();
  int count = ++vtkThreadedController::Counter;
  vtkThreadedController::CounterLock.Unlock();

  if (count == this->NumberOfProcesses)
    {
    // If you are the last process, unlock the barrier
    vtkThreadedController::BarrierStarted();
    vtkThreadedController::SignalNextThread();
    }
  else
    {
    // If you are not the last process, wait until someone unlocks 
    // the barrier
    vtkThreadedController::WaitForNextThread();
    vtkThreadedController::Counter--;

    if (vtkThreadedController::Counter == 1)
      {
      // If you are the last process to pass the barrier
      // Set the counter to 0 and leave the barrier locked
      vtkThreadedController::Counter = 0;
      // Barrier is over, another one can start
      vtkThreadedController::BarrierEnded();
      }
    else
      {
      //  unlock the barrier for the next guy
      vtkThreadedController::SignalNextThread();
      }

    }
}

//----------------------------------------------------------------------------
VTK_THREAD_RETURN_TYPE vtkThreadedController::vtkThreadedControllerStart( 
  void *arg )
{
  ThreadInfoStruct* info = (ThreadInfoStruct*)(arg);
  int threadId = info->ThreadID;
  vtkThreadedController *controller0 =(vtkThreadedController*)(info->UserData);

  controller0->Start(threadId);
  return VTK_THREAD_RETURN_VALUE;
}

//----------------------------------------------------------------------------
// We are going to try something new.  We will pass the local controller
// as the argument.
void vtkThreadedController::Start(int threadId)
{
  vtkThreadedController* localController = this->Controllers[threadId];

    // Store threadId in a table.
#ifdef VTK_USE_PTHREADS  
  this->ThreadIds[threadId] = pthread_self();
#elif defined VTK_USE_SPROC
  this->ThreadIds[threadId] = PRDA->sys_prda.prda_sys.t_pid;
#elif defined VTK_USE_WIN32_THREADS
  this->ThreadIds[threadId] = GetCurrentThreadId();
#endif
  
  if (this->MultipleMethodFlag)
    {
    if (this->MultipleMethod[threadId])
      {
      (this->MultipleMethod[threadId])(localController, 
				       this->MultipleData[threadId]);
      }
    else
      {
      vtkWarningMacro("MultipleMethod " << threadId << " not set");
      }
    }
  else
    {
    if (this->SingleMethod)
      {
      (this->SingleMethod)(localController, this->SingleData);
      }
    else
      {
      vtkErrorMacro("SingleMethod not set");
      } 
    }
}

//----------------------------------------------------------------------------
// Execute the method set as the SingleMethod on NumberOfThreads threads.
void vtkThreadedController::SingleMethodExecute()
{
  if (!this->MultiThreader)
    {
    this->MultiThreader = vtkMultiThreader::New();
    }
  this->CreateProcessControllers();
  this->MultipleMethodFlag = 0;

  this->MultiThreader->SetSingleMethod(vtkThreadedControllerStart, 
				       (void*)this);
  this->MultiThreader->SetNumberOfThreads(this->NumberOfProcesses);

  // GLOBAL_CONTROLLER will be from thread0 always.
  // GetLocalController will translate to the local controller.
  vtkMultiProcessController::SetGlobalController(this);
  
  this->MultiThreader->SingleMethodExecute();
}
//----------------------------------------------------------------------------
// Execute the methods set as the MultipleMethods.
void vtkThreadedController::MultipleMethodExecute()
{
  if (!this->MultiThreader)
    {
    this->MultiThreader = vtkMultiThreader::New();
    }
  this->CreateProcessControllers();
  this->MultipleMethodFlag = 1;

  this->MultiThreader->SetSingleMethod(vtkThreadedControllerStart, 
				       (void*)this);
  this->MultiThreader->SetNumberOfThreads(this->NumberOfProcesses);

  // GLOBAL_CONTROLLER will be from thread0 always.
  // GetLocalController will translate to the local controller.
  vtkMultiProcessController::SetGlobalController(this);

  this->MultiThreader->SingleMethodExecute();
}

vtkMultiProcessController *vtkThreadedController::GetLocalController()
{
#ifdef VTK_USE_PTHREADS  
  int idx;
  pthread_t pid = pthread_self();
  for (idx = 0; idx < this->NumberOfProcesses; ++idx)
    {
    if (pthread_equal(pid, this->ThreadIds[idx]))
      {
      return this->Controllers[idx];
      }
    }

  // Need to use cerr instead of error macro here to prevent
  // recursion (the controller's output window calls GetLocalController)
  cerr << "Could Not Find my process id." << endl;
  return NULL;
#elif defined VTK_USE_SPROC
  int idx;
  pid_t pid = PRDA->sys_prda.prda_sys.t_pid;
  for (idx = 0; idx < this->NumberOfProcesses; ++idx)
    {
    if (pid == this->ThreadIds[idx])
      {
      return this->Controllers[idx];
      }
    }
  
  // Need to use cerr instead of error macro here to prevent
  // recursion (the controller's output window calls GetLocalController)
  cerr << "Could Not Find my process id." << endl;
  return NULL;
#elif defined VTK_USE_WIN32_THREADS

  int idx;
  DWORD pid = GetCurrentThreadId();
  for (idx = 0; idx < this->NumberOfProcesses; ++idx)
    {
    if (pid == this->ThreadIds[idx])
      {
      return this->Controllers[idx];
      }
    }
  
  // Need to use cerr instead of error macro here to prevent
  // recursion (the controller's output window calls GetLocalController)
  cerr << "Could Not Find my process id." << endl;
  return NULL;
  
#else

  vtkErrorMacro("ThreadedController only works with windows api, pthreads or sproc");
  return this;
  
#endif  
}

// Note that the Windows and Unix implementations of
// these methods are completely different. This is because,
// in Windows, if the same thread locks the same mutex/critical
// section twice, it will not block. Therefore, this method
// can not be used to make the threads wait until all of them
// reach the barrier


// If there was a barrier before this one, we need to
// wait until that is cleaned up or bad things happen.
void vtkThreadedController::WaitForPreviousBarrierToEnd()
{
#ifdef VTK_USE_WIN32_THREADS
  WaitForSingleObject(vtkThreadedController::BarrierEndedEvent, INFINITE);
#else
  vtkThreadedController::BarrierInProgress.Lock();
  vtkThreadedController::BarrierInProgress.Unlock();
#endif
}

void vtkThreadedController::BarrierStarted()
{
  vtkThreadedController::IsBarrierInProgress = 1;
#ifdef VTK_USE_WIN32_THREADS

#else
  vtkThreadedController::BarrierInProgress.Lock();
#endif
}

// A new barrier can now start
void vtkThreadedController::BarrierEnded()
{
  vtkThreadedController::IsBarrierInProgress = 0;
#ifdef VTK_USE_WIN32_THREADS
  SetEvent(vtkThreadedController::BarrierEndedEvent);
#else
  vtkThreadedController::BarrierInProgress.Unlock();
#endif
}

// Tell the next guy that it is ok to continue with the barrier
void vtkThreadedController::SignalNextThread()
{
#ifdef VTK_USE_WIN32_THREADS
  SetEvent(vtkThreadedController::NextThread);
#else
  vtkThreadedController::BarrierLock.Unlock();
#endif
}

// Create the windows event necessary for waiting  
void vtkThreadedController::InitializeBarrier()
{
#ifdef VTK_USE_WIN32_THREADS
  if (!BarrierEndedEvent)
    {
    vtkThreadedController::BarrierEndedEvent = CreateEvent(0,FALSE,FALSE,0);
    vtkThreadedController::NextThread = CreateEvent(0,FALSE,FALSE,0);
    }
#endif
}

// Wait until the previous thread says it's ok to continue
void vtkThreadedController::WaitForNextThread()
{
#ifdef VTK_USE_WIN32_THREADS
  WaitForSingleObject(vtkThreadedController::NextThread,INFINITE);
#else
  vtkThreadedController::BarrierLock.Lock();
#endif
}
