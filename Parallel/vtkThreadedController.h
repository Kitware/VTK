/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkThreadedController.h
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
// .NAME vtkThreadedController - Allows communication between running threads.
// .SECTION Description
// vtkThreadedController uses a vtkMultiThreader to spawn threads.
// The communication is accomplished using a vtkSharedMemoryCommunicator.
// The RMI communicator is identical to the user communicator.
// Note that each thread gets its own vtkThreadedController to
// accomplish thread safety.

// .SECTION see also
// vtkMultiProcessController vtkMultiThreader vtkSharedMemoryCommunicator
// vtkInputPort vtkOutputPort

#ifndef __vtkThreadedController_h
#define __vtkThreadedController_h

#include "vtkObject.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiThreader.h"
#include "vtkSharedMemoryCommunicator.h"
#include "vtkCriticalSection.h"

class VTK_PARALLEL_EXPORT vtkThreadedController : public vtkMultiProcessController
{
public:
  static vtkThreadedController *New();
  vtkTypeMacro(vtkThreadedController,vtkMultiProcessController);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method is for setting up the processes.
  virtual void Initialize(int* argc, char*** arcv);
  virtual void Finalize() {}

  // Description:
  // This method returns an integer from 0 to (NumberOfProcesses-1)
  // indicating which process we are in.  
  // Note: The correct controller is passed as an argument to
  // the initial function (SingleMethod/MultipleMethod).  Calling this
  // method on another controller may give wrong results.
  vtkGetMacro(LocalProcessId, int);

  // Description:
  // Execute the SingleMethod (as define by SetSingleMethod) using
  // this->NumberOfProcesses processes.  This will only return when
  // all the processes finish executing their methods.
  virtual void SingleMethodExecute();
  
  // Description:
  // Execute the MultipleMethods (as define by calling SetMultipleMethod
  // for each of the required this->NumberOfProcesses methods) using
  // this->NumberOfProcesses processes.
  virtual void MultipleMethodExecute();

  // Description:
  // This method can be used to synchronize the threads.
  virtual void Barrier();

  // Description:
  // This method can be used to tell the controller to create
  // a special output window in which all messages are preceded
  // by the process id.
  virtual void CreateOutputWindow();


protected:

  vtkThreadedController();
  ~vtkThreadedController();
  
  void CreateProcessControllers();
  
  // Description:
  // First method called after threads are spawned.
  void Start(int threadIdx);

  void ResetControllers();

  static VTK_THREAD_RETURN_TYPE vtkThreadedControllerStart( void *arg );

  // Each Process/Thread has its own controller.
  vtkThreadedController** Controllers;

//BTX

// Required only for static access to threadId (GetLocalController).
#ifdef VTK_USE_PTHREADS
  typedef pthread_t ThreadIdType;
#elif defined VTK_USE_SPROC
  typedef pid_t ThreadIdType;
#elif defined VTK_USE_WIN32_THREADS
  typedef DWORD ThreadIdType;
#else
  typedef int ThreadIdType;
#endif

//ETX
 
  // Used in barrier
  static vtkSimpleCriticalSection CounterLock;
  static int Counter;
  static int IsBarrierInProgress;
  static void WaitForPreviousBarrierToEnd();
  static void BarrierStarted();
  static void BarrierEnded();
  static void SignalNextThread();
  static void InitializeBarrier();
  static void WaitForNextThread();
#ifdef VTK_USE_WIN32_THREADS
  static HANDLE BarrierEndedEvent;
  static HANDLE NextThread;
#else
  static vtkSimpleCriticalSection BarrierLock;
  static vtkSimpleCriticalSection BarrierInProgress;
#endif
  
  ThreadIdType* ThreadIds;

  int LastNumberOfProcesses;

  vtkMultiThreader *MultiThreader;
  // Used internally to switch between multiple and single method execution.
  int MultipleMethodFlag;
  
  // For static GetGlobalController.  Translates controller for thread0
  // to controller for local thread.
  vtkMultiProcessController *GetLocalController();

private:
  vtkThreadedController(const vtkThreadedController&);  // Not implemented.
  void operator=(const vtkThreadedController&);  // Not implemented.
};

#endif


