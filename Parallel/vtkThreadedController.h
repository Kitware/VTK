/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreadedController.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

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
  vtkTypeRevisionMacro(vtkThreadedController,vtkMultiProcessController);
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


