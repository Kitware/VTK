/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPICommunicator.h
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

class VTK_PARALLEL_EXPORT vtkMPICommunicator : public vtkCommunicator
{

public:

  vtkTypeRevisionMacro( vtkMPICommunicator,vtkObject);
  
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
#ifdef VTK_USE_64BIT_IDS
  virtual int Send(vtkIdType* data, int length, int remoteProcessId, 
                   int tag);
#endif
  virtual int Send(vtkDataObject* data, int remoteProcessId, int tag)
    { return this->vtkCommunicator::Send(data, remoteProcessId, tag); }
  virtual int Send(vtkDataArray* data, int remoteProcessId, int tag)
    { return this->vtkCommunicator::Send(data, remoteProcessId, tag); }

//BTX

  class VTK_PARALLEL_EXPORT Request
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
#ifdef VTK_USE_64BIT_IDS
  virtual int Receive(vtkIdType* data, int length, int remoteProcessId, 
                      int tag);
#endif
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

  friend class vtkMPIController;

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

 private:
  static int CheckForMPIError(int err);

private:
  vtkMPICommunicator(const vtkMPICommunicator&);  // Not implemented.
  void operator=(const vtkMPICommunicator&);  // Not implemented.
};


#endif //  __vtkMPICommunicator_h




