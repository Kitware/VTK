/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkMPIController.h
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
// .NAME vtkMPIController - Process communication using MPI
// .SECTION Description
// vtkMPIController supplies a minimal set of communication methods as an
// abstract interface through a variety of multi-processing communication
// techniques.  It accepts Sends and Receives as well as implements
// remove method invocations (RMI)
// The "RegisterAndGetGlobalController" ensures that at most one 
// controller exists per process.  In most cases, the controller will
// be created automatically by a higher level object.
// The intent is to generalize this class to have different multiprocessing
// options: Threads, forking processes with shared memory or pipes.
// The initialization is modeled after vtkMultiThreader, and may merge with
// vtkMultiThreader in the future.

// .SECTION see also
// vtkDownStreamPort vtkUpStreamPort vtkMultiThreader vtkMultiProcessController

#ifndef __vtkMPIController_h
#define __vtkMPIController_h

#include "vtkObject.h"
#include "vtkMultiProcessController.h"
#include "mpi.h"

class VTK_EXPORT vtkMPIController : public vtkMultiProcessController
{
public:
  static vtkMPIController *New();
  vtkTypeMacro(vtkMPIController,vtkMultiProcessController);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method is for setting up the processes.
  // It needs to be called only once during program execution. 
  // Calling it more than once will have no effect. Controllers
  // created after this call will be initialized automatically
  // (i.e. they will have the proper LocalProcessId and NumberOfProcesses).
  virtual void Initialize(int* argc, char*** arcv);

  // Description:
  // This method is for cleaning up and has to be called before
  // the end of the program if MPI was initialized with
  // vtkMPIController::Initialize
  virtual void Finalize();

  // Description:
  // Execute the SingleMethod (as define by SetSingleMethod) using
  // this->NumberOfProcesses processes.  You should not expect this to return.
  virtual void SingleMethodExecute();
  
  // Description:
  // Execute the MultipleMethods (as define by calling SetMultipleMethod
  // for each of the required this->NumberOfProcesses methods) using
  // this->NumberOfProcesses processes.
  virtual void MultipleMethodExecute();

  //------------------ Communication --------------------
  
  // Description:
  // This method sends data to another process.  Tag eliminates ambiguity
  // when multiple sends or receives exist in the same process.
  virtual int Send(int *data, int length, int remoteProcessId, int tag);
  virtual int Send(unsigned long *data, int length, int remoteProcessId, int tag);
  virtual int Send(char *data, int length, int remoteProcessId, int tag);
  virtual int Send(float *data, int length, int remoteProcessId, int tag);
  virtual int Send(vtkDataObject *data, int remoteId, int tag)
    {return this->vtkMultiProcessController::Send(data,remoteId,tag);}

  // Description:
  // This method receives data from a corresponding send. It blocks
  // until the receive is finished.  It calls methods in "data"
  // to communicate the sending data.
  virtual int Receive(int *data, int length, int remoteProcessId, int tag);
  virtual int Receive(unsigned long *data, int length, int remoteProcessId, int tag);
  virtual int Receive(char *data, int length, int remoteProcessId, int tag);
  virtual int Receive(float *data, int length, int remoteProcessId, int tag);
  virtual int Receive(vtkDataObject *data, int remoteId, int tag)
    {return this->vtkMultiProcessController::Receive(data, remoteId, tag);}

protected:

  vtkMPIController();
  ~vtkMPIController();
  vtkMPIController(const vtkMPIController&) {};
  void operator=(const vtkMPIController&) {};

  void InitializeNumberOfProcesses();

  // Initialize only once.
  static int Initialized;

};


#endif


