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
// vtkMPIController is a concrete class which implements the
// abstract multi-process control methods defined in vtkMultiProcessController
// using MPI (Message Passing Interface) 
// cf. Using MPI / Portable Parallel Programming with the Message-Passing
// Interface, Gropp et al, MIT Press.
// It also provide functionality specific to MPI and not present in 
// vtkMultiProcessController.
// Before any MPI communication can occur Initialize() must be called
// by all processes. It is required to be called once, controllers
// created after this need not call Initialize().
// At the end of the program Finalize() must be called by all
// processes. 
// The use of user-defined communicators are supported with 
// vtkMPICommunicator and vtkMPIGroup. Note that a duplicate of
// the user defined communicator is used for internal communications (RMIs).
// This communicator has the same properties as the user one except that
// it has a new context which prevents the two communicators from
// interfering with each other.

// .SECTION see also
// vtkOutputPort vtkInputPort  vtkMultiProcessController
// vtkMPICommunicator vtkMPIGroup

#ifndef __vtkMPIController_h
#define __vtkMPIController_h

#include "mpi.h"

#include "vtkMultiProcessController.h"
#include "vtkMPICommunicator.h"


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
  // The addresses of argc and argv should be passed to this method
  // otherwise command line arguments will not be correct (because
  // usually MPI implementations add their own arguments during
  // startup).
  virtual void Initialize(int* argc, char*** arcv);

  // Description:
  // This method is for cleaning up and has to be called before
  // the end of the program if MPI was initialized with
  //Initialize()
  virtual void Finalize();

  // Description:
  // Execute the SingleMethod (as define by SetSingleMethod) using
  // this->NumberOfProcesses processes.  
  virtual void SingleMethodExecute();
  
  // Description:
  // Execute the MultipleMethods (as define by calling SetMultipleMethod
  // for each of the required this->NumberOfProcesses methods) using
  // this->NumberOfProcesses processes.
  virtual void MultipleMethodExecute();

  // Description:
  // This method can be used to synchronize MPI processes in the
  // current communicator. This uses the user communicator.
  void Barrier();

  // Description:
  // This method can be used to tell the controller to create
  // a special output window in which all messages are preceded
  // by the process id.
  virtual void CreateOutputWindow();

  // Description:
  // Given an MPI error code, return a string which contains
  // an error message. This string has to be freed by the user.
  static char* ErrorString(int err);


  // Description:
  // MPIController uses this communicator in all sends and
  // receives. By default, MPI_COMM_WORLD is used.
  // THIS SHOULD ONLY BE CALLED ON THE PROCESSES INCLUDED
  // IN THE COMMUNICATOR. FOR EXAMPLE, IF THE COMMUNICATOR
  // CONTAINES PROCESSES 0 AND 1, INVOKING THIS METHOD ON
  // ANY OTHER PROCESS WILL CAUSE AN MPI ERROR AND POSSIBLY
  // LEAD TO A CRASH.
  void SetCommunicator(vtkMPICommunicator* comm);

//BTX

  // Description:
  // This method sends data to another process (non-blocking).  
  // Tag eliminates ambiguity when multiple sends or receives 
  // exist in the same process. The last argument,
  // vtkMPICommunicator::Request& req can later be used (with
  // req.Test() ) to test the success of the message.
  // Note: These methods delegate to the communicator
  int NoBlockSend(int* data, int length, int remoteProcessId, int tag,
		  vtkMPICommunicator::Request& req)
    { return ((vtkMPICommunicator*)this->Communicator)->NoBlockSend
	(data ,length, remoteProcessId, tag, req); }
  int NoBlockSend(unsigned long* data, int length, int remoteProcessId,
		  int tag, vtkMPICommunicator::Request& req)
    { return ((vtkMPICommunicator*)this->Communicator)->NoBlockSend
	(data, length, remoteProcessId, tag, req); }
  int NoBlockSend(char* data, int length, int remoteProcessId, 
		  int tag, vtkMPICommunicator::Request& req)
    { return ((vtkMPICommunicator*)this->Communicator)->NoBlockSend
	(data, length, remoteProcessId, tag, req); }
  int NoBlockSend(float* data, int length, int remoteProcessId, 
		  int tag, vtkMPICommunicator::Request& req)
    { return ((vtkMPICommunicator*)this->Communicator)->NoBlockSend
	(data, length, remoteProcessId, tag, req); }

  // Description:
  // This method receives data from a corresponding send (non-blocking). 
  // The last argument,
  // vtkMPICommunicator::Request& req can later be used (with
  // req.Test() ) to test the success of the message.
  // Note: These methods delegate to the communicator
  int NoBlockReceive(int* data, int length, int remoteProcessId, 
		     int tag, vtkMPICommunicator::Request& req)
    { return ((vtkMPICommunicator*)this->Communicator)->NoBlockReceive
	(data, length, remoteProcessId, tag, req); }
  int NoBlockReceive(unsigned long* data, int length, 
		     int remoteProcessId, int tag, 
		     vtkMPICommunicator::Request& req)
    { return ((vtkMPICommunicator*)this->Communicator)->NoBlockReceive
	(data, length, remoteProcessId, tag, req); }
  int NoBlockReceive(char* data, int length, int remoteProcessId, 
		     int tag, vtkMPICommunicator::Request& req)
    { return ((vtkMPICommunicator*)this->Communicator)->NoBlockReceive
	(data, length, remoteProcessId, tag, req); }
  int NoBlockReceive(float* data, int length, int remoteProcessId, 
		     int tag, vtkMPICommunicator::Request& req)
    { return ((vtkMPICommunicator*)this->Communicator)->NoBlockReceive
	(data, length, remoteProcessId, tag, req); }

//ETX

protected:

  vtkMPIController();
  ~vtkMPIController();
  vtkMPIController(const vtkMPIController&);
  void operator=(const vtkMPIController&);

  // Given a communicator, obtain size and rank
  // setting NumberOfProcesses and LocalProcessId
  // Should not be called if the current communicator
  // does not include this process
  int InitializeNumberOfProcesses();

  // Set the communicator to comm and call InitializeNumberOfProcesses()
  void InitializeCommunicator(vtkMPICommunicator* comm);

  // Duplicate the current communicator, creating RMICommunicator
  void InitializeRMICommunicator();

  // MPI communicator created when Initialize() called.
  // This is a copy of MPI_COMM_WORLD but uses a new
  // context, i.e. even if the tags are the same, the
  // RMI messages will not interfere with user level
  // messages.
  static vtkMPICommunicator* WorldRMICommunicator;

  // Initialize only once.
  static int Initialized;


};


#endif


