/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPICommunicator.h
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
// .NAME vtkMPICommunicator - Class for creating user defined MPI communicators.

// .SECTION Description
// This class can be used to create user defined MPI communicators.
// The actual creation (with MPI_Comm_create) occurs in Initialize
// which takes as arguments a super-communicator and a group of
// process ids. The new communicator is created by including the
// processes contained in the group. The global communicator 
// (equivalent to MPI_COMM_WORLD) can be obtained using the class
// method GetWorldCommunicator. It is important to note that 
// this communicator should not be used on the processes not contained
// in the group. For example, if the group contains processes 0 and 1,
// controller->SetCommunicator(communicator) would cause an MPI error
// on any other process.

// .SECTION See Also
// vtkMPIController vtkMPIGroup

#ifndef __vtkMPICommunicator_h
#define __vtkMPICommunicator_h

#include "mpi.h"
#include "vtkCommunicator.h"
#include "vtkMPIGroup.h"

class vtkMPIController;

class VTK_EXPORT vtkMPICommunicator : public vtkCommunicator
{

public:

  vtkTypeMacro( vtkMPICommunicator,vtkObject);
  
  // Description:
  // Creates an empty communicator.
  static vtkMPICommunicator* New();

  // Description:
  // Returns the singleton which behaves as the global
  // communicator (MPI_COMM_WORLD)
  static vtkMPICommunicator* GetWorldCommunicator();
  
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Used to initialize (i.e. create the underlying MPI_Comm)
  // the communicator. Note that group is also stored in
  // an instance variable (the reference count of group
  // is increased by 1). group->Delete() can be safely invoked after 
  // this.
  int Initialize(vtkMPICommunicator* mpiComm, vtkMPIGroup* group);

  // Description:
  // This method sends data to another process.  Tag eliminates ambiguity
  // when multiple sends or receives exist in the same process.
  virtual int Send(int* data, int length, int remoteProcessId, int tag);
  virtual int Send(unsigned long* data, int length, int remoteProcessId,
		   int tag);
  virtual int Send(char* data, int length, int remoteProcessId, int tag);
  virtual int Send(unsigned char* data, int length, int remoteProcessId, int tag);
  virtual int Send(float* data, int length, int remoteProcessId, 
		   int tag);
  virtual int Send(double* data, int length, int remoteProcessId, 
		   int tag);
  virtual int Send(vtkIdType* data, int length, int remoteProcessId, 
		   int tag);
  virtual int Send(vtkDataObject* data, int remoteProcessId, int tag)
    { return this->vtkCommunicator::Send(data, remoteProcessId, tag); }
  virtual int Send(vtkDataArray* data, int remoteProcessId, int tag)
    { return this->vtkCommunicator::Send(data, remoteProcessId, tag); }

//BTX

  class VTK_EXPORT Request
  {
  public:
    int Test();
    void Cancel();
    void Wait();
    MPI_Request Req;
  };

//ETX

  // Description:
  // This method sends data to another process (non-blocking).  
  // Tag eliminates ambiguity when multiple sends or receives 
  // exist in the same process. The last argument,
  // vtkMPICommunicator::Request& req can later be used (with
  // req.Test() ) to test the success of the message.
  int NoBlockSend(int* data, int length, int remoteProcessId, int tag,
		  Request& req);
  int NoBlockSend(unsigned long* data, int length, int remoteProcessId,
		  int tag, Request& req);
  int NoBlockSend(char* data, int length, int remoteProcessId, 
		  int tag, Request& req);
  int NoBlockSend(float* data, int length, int remoteProcessId, 
		  int tag, Request& req);

  // Description:
  // This method receives data from a corresponding send. It blocks
  // until the receive is finished.
  virtual int Receive(int* data, int length, int remoteProcessId, 
		      int tag);
  virtual int Receive(unsigned long* data, int length, 
		      int remoteProcessId, int tag);
  virtual int Receive(char* data, int length, int remoteProcessId, 
		      int tag);
  virtual int Receive(unsigned char* data, int length, int remoteProcessId, 
		      int tag);
  virtual int Receive(float* data, int length, int remoteProcessId, 
		      int tag);
  virtual int Receive(double* data, int length, int remoteProcessId, 
		      int tag);
  virtual int Receive(vtkIdType* data, int length, int remoteProcessId, 
		      int tag);
  virtual int Receive(vtkDataObject* data, int remoteProcessId, int tag)
    { return this->vtkCommunicator::Receive(data, remoteProcessId, tag); }
  virtual int Receive(vtkDataArray* data, int remoteProcessId, int tag)
    { return this->vtkCommunicator::Receive(data, remoteProcessId, tag); }

  // Description:
  // This method receives data from a corresponding send (non-blocking). 
  // The last argument,
  // vtkMPICommunicator::Request& req can later be used (with
  // req.Test() ) to test the success of the message.
  int NoBlockReceive(int* data, int length, int remoteProcessId, 
		     int tag, Request& req);
  int NoBlockReceive(unsigned long* data, int length, 
		     int remoteProcessId, int tag, Request& req);
  int NoBlockReceive(char* data, int length, int remoteProcessId, 
		     int tag, Request& req);
  int NoBlockReceive(float* data, int length, int remoteProcessId, 
		     int tag, Request& req);

//BTX

  friend vtkMPIController;

//ETX

protected:

  vtkSetObjectMacro(Group, vtkMPIGroup);

  // Description:
  // KeepHandle is normally off. This means that the MPI
  // communicator handle will be freed at the destruction
  // of the object. However, if the handle was copied from
  // another object (via CopyFrom() not Duplicate()), this
  // has to be turned on otherwise the handle will be freed
  // multiple times causing MPI failure. The alternative to
  // this is using reference counting but it is unnecessarily
  // complicated for this case.
  vtkSetMacro(KeepHandle, int);
  vtkBooleanMacro(KeepHandle, int);

  static vtkMPICommunicator* WorldCommunicator;

  void InitializeCopy(vtkMPICommunicator* source);

  // Description:
  // Copies all the attributes of source, deleting previously
  // stored data. The MPI communicator handle is also copied.
  // Normally, this should not be needed. It is used during
  // the construction of a new communicator for copying the
  // world communicator, keeping the same context.
  void CopyFrom(vtkMPICommunicator* source);

  // Description:
  // Copies all the attributes of source, deleting previously
  // stored data EXCEPT the MPI communicator handle which is
  // duplicated with MPI_Comm_dup(). Therefore, although the
  // processes in the communicator remain the same, a new context
  // is created. This prevents the two communicators from 
  // intefering with each other during message send/receives even
  // if the tags are the same.
  void Duplicate(vtkMPICommunicator* source);

  MPI_Comm* Handle;
  vtkMPIGroup* Group;

  int Initialized;
  int KeepHandle;

  vtkMPICommunicator();
  ~vtkMPICommunicator();
  vtkMPICommunicator(const vtkMPICommunicator&);
  void operator=(const vtkMPICommunicator&);

 private:
  static int CheckForMPIError(int err);

};


#endif //  __vtkMPICommunicator_h




