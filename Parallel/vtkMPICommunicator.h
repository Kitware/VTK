/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPICommunicator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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

#include "vtkCommunicator.h"

class vtkMPIController;
class vtkMPIGroup;

class vtkMPICommunicatorOpaqueRequest;
class vtkMPICommunicatorOpaqueComm;

class VTK_PARALLEL_EXPORT vtkMPICommunicator : public vtkCommunicator
{
public:
  vtkTypeRevisionMacro( vtkMPICommunicator,vtkCommunicator);
  
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
  virtual int Send(unsigned char* data, int length, int remoteProcessId, 
                   int tag);
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
    Request();
    ~Request();
    int Test();
    void Cancel();
    void Wait();
    vtkMPICommunicatorOpaqueRequest* Req;
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


  // Description:
  // Broadcast an array from the given root process.
  int Broadcast(int* data          , int length, int root);
  int Broadcast(unsigned long* data, int length, int root);
  int Broadcast(char* data         , int length, int root);
  int Broadcast(float* data        , int length, int root);
  int Broadcast(double* data        , int length, int root);

  
  // Description:
  // Gather an array to the given root process (the "to" pointer
  // must point to an array of length length*numProcesses
  int Gather(int* data          , int* to          , int length, int root);
  int Gather(unsigned long* data, unsigned long* to, int length, int root);
  int Gather(char* data         , char* to         , int length, int root);
  int Gather(float* data        , float* to        , int length, int root);
  int Gather(double* data       , double* to       , int length, int root);

  // Description:
  // Gather an array to the given root process.  This method
  // allows for arrays of different sizes on all processes
  // to be gathered into a single array on the root process.  For the
  // root process, all arguments are significant, on non-root
  // processes, only data, sendlength, and root are significant
  // (all other args can be NULL).  The argument offsets is an array
  // of integers describing offset for each sent array.
  int GatherV(int* data, int* to, 
              int sendlength, int* recvlengths, int* offsets, int root);
  int GatherV(unsigned long* data, unsigned long* to, 
              int sendlength, int* recvlengths, int* offsets, int root);
  int GatherV(char* data, char* to, 
              int sendlength, int* recvlengths, int* offsets, int root);
  int GatherV(float* data, float* to, 
              int sendlength, int* recvlengths, int* offsets, int root);
  int GatherV(double* data, double* to, 
              int sendlength, int* recvlengths, int* offsets, int root);


  // Description:
  // Same as Gather, but result ends up on all processes.
  // Length is the length of the data array (input).  This length
  // has to be the same on all processes.  The recv/output array 'to'
  // has to be length (length*numprocs).
  int AllGather(int* data          , int* to          , int length);
  int AllGather(unsigned long* data, unsigned long* to, int length);
  int AllGather(char* data         , char* to         , int length);
  int AllGather(float* data        , float* to        , int length);
  int AllGather(double* data       , double* to       , int length);
  
  // Description:
  // same as gather, but the arrays can be different lengths on 
  // different processes.  ProcId is the index into recvLengths 
  // and recvOffsets.  These arrays determine how all the the arrays
  // are packed into the received arra 'to'.
  int AllGatherV(int* data, int* to, 
                 int sendlength, int* recvlengths, int* recvOffsets);
  int AllGatherV(unsigned long* data, unsigned long* to, 
                 int sendlength, int* recvlengths, int* recvOffsets);
  int AllGatherV(char* data, char* to, 
                 int sendlength, int* recvlengths, int* recvOffsets);
  int AllGatherV(float* data, float* to, 
                 int sendlength, int* recvlengths, int* recvOffsets);
  int AllGatherV(double* data, double* to, 
                 int sendlength, int* recvlengths, int* recvOffsets);

  // Description:
  // Reduce an array to the given root process.  
  int ReduceMax(int* data, int* to, int size, int root);
  int ReduceMax(unsigned long* data, unsigned long* to, int size, int root);
  int ReduceMax(float* data, float* to, int size, int root);
  int ReduceMax(double* data, double* to, int size, int root);

  int ReduceMin(int* data, int* to, int size, int root);
  int ReduceMin(unsigned long* data, unsigned long* to, int size, int root);
  int ReduceMin(float* data, float* to, int size, int root);
  int ReduceMin(double* data, double* to, int size, int root);

  int ReduceSum(int* data, int* to, int size, int root);
  int ReduceSum(unsigned long* data, unsigned long* to, int size, int root);
  int ReduceSum(float* data, float* to, int size, int root);
  int ReduceSum(double* data, double* to, int size, int root);

  int ReduceAnd(bool* data, bool* to, int size, int root);
  int ReduceOr(bool* data, bool* to, int size, int root);

  // Description:
  // This method receives a data object from a corresponding send. It blocks
  // until the receive is finished. 
  virtual int Receive(vtkDataObject* data, int remoteHandle, int tag);

//BTX

  friend class vtkMPIController;

  vtkMPICommunicatorOpaqueComm *GetMPIComm()
    {
    return this->MPIComm;
    }
//ETX

  static char* Allocate(size_t size);
  static void Free(char* ptr);


protected:
  vtkMPICommunicator();
  ~vtkMPICommunicator();

  virtual void SetGroup(vtkMPIGroup*);

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

  vtkMPICommunicatorOpaqueComm* MPIComm;
  vtkMPIGroup* Group;

  int Initialized;
  int KeepHandle;

  int LastSenderId;

  static int CheckForMPIError(int err);

private:
  vtkMPICommunicator(const vtkMPICommunicator&);  // Not implemented.
  void operator=(const vtkMPICommunicator&);  // Not implemented.
};

#endif
