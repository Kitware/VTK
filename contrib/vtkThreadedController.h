/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkThreadedController.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkThreadedController - Allows communication between running threads.
// .SECTION Description
// vtkThreadedController just uses a vtkMultiThreader to spawn threads.
// It the implements sends and receives using shared memory and reference 
// counting.

// Unfortunately, as this is written, it is not thread safe.  All threads
// use the same controller object, so opperations like adding an RMI could
// potentially conflict.  We need to have our own RegisterAndGetGlobalController
// method to create different controllers for each thread.  This would also
// simplifiy the GetLocalProcessId methods.


// .SECTION see also
// vtkDownStreamPort vtkUpStreamPort vtkMultiThreader vtkMultiProcessController

#ifndef __vtkThreadedController_h
#define __vtkThreadedController_h

#include "vtkObject.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiThreader.h"

class vtkThreadedControllerProcessInfo;


class VTK_EXPORT vtkThreadedController : public vtkMultiProcessController
{
public:
  static vtkThreadedController *New();
  vtkTypeMacro(vtkThreadedController,vtkMultiProcessController);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method is for setting up the processes.
  void Initialize(int argc, char *arcv[]);

  // Description:
  // Execute the SingleMethod (as define by SetSingleMethod) using
  // this->NumberOfProcesses processes.  You should not expect this to return.
  void SingleMethodExecute();
  
  // Description:
  // Execute the MultipleMethods (as define by calling SetMultipleMethod
  // for each of the required this->NumberOfProcesses methods) using
  // this->NumberOfProcesses processes.
  void MultipleMethodExecute();
  
  // Description:
  // This method returns an integer from 0 to (NumberOfProcesses-1)
  // indicating which process we are int.  It should not be called
  // until ExecuteSingleMethod or ExecuteMultipleMethod has been
  // called.
  int GetLocalProcessId();
  
  //------------------ Communication --------------------
  
  // Description:
  // This method sends data to another process.  Tag eliminates ambiguity
  // when multiple sends ar receives exist in the same process.
  int Send(vtkObject *data, int remoteProcessId, int tag);
  int Send(int *data, int length, int remoteProcessId, int tag);
  int Send(unsigned long *data, int length, int remoteProcessId, int tag);
  int Send(char *data, int length, int remoteProcessId, int tag);
  int Send(float *data, int length, int remoteProcessId, int tag);

  // Description:
  // This method receives data from a corresponding send. It blocks
  // until the receive is finished.  It calls methods in "data"
  // to communicate the sending data.
  int Receive(vtkObject *data, int remoteProcessId, int tag);
  int Receive(int *data, int length, int remoteProcessId, int tag);
  int Receive(unsigned long *data, int length, int remoteProcessId, int tag);
  int Receive(char *data, int length, int remoteProcessId, int tag);
  int Receive(float *data, int length, int remoteProcessId, int tag);
  
  // Description:
  // First method called after threads are spawned.
  // It is public because the function vtkThreadedControllerStart
  // is not a friend yet.  You should not call this method.
  void Start(int threadIdx);

protected:
  
  vtkMultiThreader *MultiThreader;
  // Used internally to switch between mutliple and single method execution.
  int MultipleMethodFlag;
  
#ifdef VTK_USE_PTHREADS
  pthread_t ThreadIds[VTK_MP_CONTROLLER_MAX_PROCESSES];
#endif
#ifdef VTK_USE_SPROC
  pid_t ThreadIds[VTK_MP_CONTROLLER_MAX_PROCESSES];  
#endif

  // Locks and pointers for communication.
  vtkThreadedControllerProcessInfo *Processes[VTK_MP_CONTROLLER_MAX_PROCESSES];
  
  // It is not enough to block on the messages, we have to mutex 
  // the whole send interaction.  I was trying to avoid a central 
  // mutex (oh well).
  vtkMutexLock *MessageLock;
  
  // Trying to track down lockups.
  FILE *LogFile;
  
  vtkThreadedController();
  ~vtkThreadedController();
  vtkThreadedController(const vtkThreadedController&) {};
  void operator=(const vtkThreadedController&) {};

  // Initialize and clean up in main thread.
  void CreateThreadInfoObjects();
  void DeleteThreadInfoObjects();
  
  int Send(vtkObject *object, void *data, int length, 
	   int remoteProcessId, int tag);
  int Receive(vtkObject *object, void *data, int length, 
	      int remoteProcessId, int tag);

  // For shallow copy on object sends / receives.
  void CopyObject(vtkObject *src, vtkObject *dest);
};


#endif


