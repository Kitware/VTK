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
// vtkThreadedController just uses a vtkMultiThreader to spawn threads.
// It the implements sends and receives using shared memory and reference 
// counting.

// Unfortunately, as this is written, it is not thread safe.  All threads
// use the same controller object, so operations like adding an RMI could
// potentially conflict.  We need to have our own RegisterAndGetGlobalController
// method to create different controllers for each thread.  This would also
// simplify the GetLocalProcessId methods.


// .SECTION see also
// vtkDownStreamPort vtkUpStreamPort vtkMultiThreader vtkMultiProcessController

#ifndef __vtkThreadedController_h
#define __vtkThreadedController_h

#include "vtkObject.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiThreader.h"


class vtkThreadedControllerMessage;


class VTK_EXPORT vtkThreadedController : public vtkMultiProcessController
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
    
  //------------------ Communication --------------------
  
  // Description:
  // This method sends data to another process.  Tag eliminates ambiguity
  // and is used to match sends with receives.
  virtual int Send(vtkDataObject *data, int remoteProcessId, int tag);
  virtual int Send(int *data, int length, int remoteProcessId, int tag);
  virtual int Send(unsigned long *data, int length, int remoteProcessId, int tag);
  virtual int Send(char *data, int length, int remoteProcessId, int tag);
  virtual int Send(float *data, int length, int remoteProcessId, int tag);

  // Description:
  // This method receives data from a corresponding send. It blocks
  // until the receive is finished.  
  virtual int Receive(vtkDataObject *data, int remoteProcessId, int tag);
  virtual int Receive(int *data, int length, int remoteProcessId, int tag);
  virtual int Receive(unsigned long *data, int length, int remoteProcessId, int tag);
  virtual int Receive(char *data, int length, int remoteProcessId, int tag);
  virtual int Receive(float *data, int length, int remoteProcessId, int tag);
  
  // Description:
  // First method called after threads are spawned.
  // It is public because the function vtkThreadedControllerStart
  // is not a friend yet.  You should not call this method.
  void Start(int threadIdx);

protected:
  vtkThreadedController();
  ~vtkThreadedController();
  vtkThreadedController(const vtkThreadedController&) {};
  void operator=(const vtkThreadedController&) {};
  
  void CreateProcessControllers();
  vtkThreadedControllerMessage *NewMessage(vtkDataObject *object,
                                           void *data, int dataLength);
  void DeleteMessage(vtkThreadedControllerMessage *message);
  void AddMessage(vtkThreadedControllerMessage *message);
  vtkThreadedControllerMessage *FindMessage(int sendId, int tag);

  // Each Process/Thread has its own controller.
  vtkThreadedController *Controllers[VTK_MP_CONTROLLER_MAX_PROCESSES];

  // Required only for static access to threadId (GetLocalController).
#ifdef VTK_USE_PTHREADS
  pthread_t ThreadIds[VTK_MP_CONTROLLER_MAX_PROCESSES];
#endif
#ifdef VTK_USE_SPROC
  pid_t ThreadIds[VTK_MP_CONTROLLER_MAX_PROCESSES];  
#endif  
  
  // The id for this objects process.
  int LocalProcessId;
  int WaitingForId;

  vtkMultiThreader *MultiThreader;
  // Used internally to switch between multiple and single method execution.
  int MultipleMethodFlag;
  
  // It is not enough to block on the messages, we have to mutex 
  // the whole send interaction.  I was trying to avoid a central 
  // mutex (oh well).
  vtkMutexLock *MessageListLock;

  // This mutex is normally locked.  It is used to block the execution 
  // of the receiving process when the send has not been called yet.
  vtkMutexLock *Gate;

  // Double linked list.
  vtkThreadedControllerMessage *MessageListStart;
  vtkThreadedControllerMessage *MessageListEnd;
  
  // Trying to track down lockups.
  FILE *LogFile;
  

  // The generic send and receive methods.
  int Send(vtkDataObject *object, void *data, int dataLength, 
	   int remoteProcessId, int tag);
  int Receive(vtkDataObject *object, void *data, int dataLength, 
	      int remoteProcessId, int tag);

  // For static GetGlobalController.  Translates controller for thread0
  // to controller for local thread.
  vtkMultiProcessController *GetLocalController();
  
};


#endif


