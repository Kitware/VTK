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
// vtkMPIController vtkProcessGroup

#ifndef __vtkMPICommunicator_h
#define __vtkMPICommunicator_h

#include "vtkCommunicator.h"

class vtkMPIController;
class vtkProcessGroup;

class vtkMPICommunicatorOpaqueComm;
class vtkMPICommunicatorOpaqueRequest;
class vtkMPICommunicatorReceiveDataInfo;

class VTK_PARALLEL_EXPORT vtkMPICommunicator : public vtkCommunicator
{
public:
//BTX

  class VTK_PARALLEL_EXPORT Request
  {
  public:
    Request();
    Request( const Request& );
    ~Request();
    Request& operator = ( const Request& );
    int Test();
    void Cancel();
    void Wait();
    vtkMPICommunicatorOpaqueRequest* Req;
  };

//ETX

  vtkTypeMacro( vtkMPICommunicator,vtkCommunicator);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Creates an empty communicator.
  static vtkMPICommunicator* New();

  // Description:
  // Returns the singleton which behaves as the global
  // communicator (MPI_COMM_WORLD)
  static vtkMPICommunicator* GetWorldCommunicator();
  

  // Description:
  // Used to initialize the communicator (i.e. create the underlying MPI_Comm).
  // The group must be associated with a valid vtkMPICommunicator.
  int Initialize(vtkProcessGroup *group);

  // Description:
  // Used to initialize the communicator (i.e. create the underlying MPI_Comm)
  // using MPI_Comm_split on the given communicator.
  int SplitInitialize(vtkCommunicator *oldcomm, int color, int key);

  // Description:
  // Performs the actual communication.  You will usually use the convenience
  // Send functions defined in the superclass.
  virtual int SendVoidArray(const void *data, vtkIdType length, int type,
                            int remoteProcessId, int tag);
  virtual int ReceiveVoidArray(void *data, vtkIdType length, int type,
                               int remoteProcessId, int tag);

  // Description:
  // This method sends data to another process (non-blocking).  
  // Tag eliminates ambiguity when multiple sends or receives 
  // exist in the same process. The last argument,
  // vtkMPICommunicator::Request& req can later be used (with
  // req.Test() ) to test the success of the message.
  int NoBlockSend(const int* data, int length, int remoteProcessId, int tag,
                  Request& req);
  int NoBlockSend(const unsigned long* data, int length, int remoteProcessId,
                  int tag, Request& req);
  int NoBlockSend(const char* data, int length, int remoteProcessId, 
                  int tag, Request& req);
  int NoBlockSend(const float* data, int length, int remoteProcessId, 
                  int tag, Request& req);

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
#ifdef VTK_USE_64BIT_IDS
  int NoBlockReceive(vtkIdType* data, int length, int remoteProcessId, 
                     int tag, Request& req);
#endif


  // Description:
  // More efficient implementations of collective operations that use
  // the equivalent MPI commands.
  virtual void Barrier();
  virtual int BroadcastVoidArray(void *data, vtkIdType length, int type,
                                 int srcProcessId);
  virtual int GatherVoidArray(const void *sendBuffer, void *recvBuffer,
                              vtkIdType length, int type, int destProcessId);
  virtual int GatherVVoidArray(const void *sendBuffer, void *recvBuffer,
                               vtkIdType sendLength, vtkIdType *recvLengths,
                               vtkIdType *offsets, int type, int destProcessId);
  virtual int ScatterVoidArray(const void *sendBuffer, void *recvBuffer,
                               vtkIdType length, int type, int srcProcessId);
  virtual int ScatterVVoidArray(const void *sendBuffer, void *recvBuffer,
                                vtkIdType *sendLengths, vtkIdType *offsets,
                                vtkIdType recvLength, int type,
                                int srcProcessId);
  virtual int AllGatherVoidArray(const void *sendBuffer, void *recvBuffer,
                                 vtkIdType length, int type);
  virtual int AllGatherVVoidArray(const void *sendBuffer, void *recvBuffer,
                                  vtkIdType sendLength, vtkIdType *recvLengths,
                                  vtkIdType *offsets, int type);
  virtual int ReduceVoidArray(const void *sendBuffer, void *recvBuffer,
                              vtkIdType length, int type,
                              int operation, int destProcessId);
  virtual int ReduceVoidArray(const void *sendBuffer, void *recvBuffer,
                              vtkIdType length, int type,
                              Operation *operation, int destProcessId);
  virtual int AllReduceVoidArray(const void *sendBuffer, void *recvBuffer,
                                 vtkIdType length, int type,
                                 int operation);
  virtual int AllReduceVoidArray(const void *sendBuffer, void *recvBuffer,
                                 vtkIdType length, int type,
                                 Operation *operation);

//BTX

  friend class vtkMPIController;

  vtkMPICommunicatorOpaqueComm *GetMPIComm()
    {
    return this->MPIComm;
    }
//ETX

  static char* Allocate(size_t size);
  static void Free(char* ptr);


  // Description:
  // When set to 1, all MPI_Send calls are replaced by MPI_Ssend calls.
  // Default is 0.
  vtkSetClampMacro(UseSsend, int, 0, 1);
  vtkGetMacro(UseSsend, int);
  vtkBooleanMacro(UseSsend, int);

  // Description:
  // Copies all the attributes of source, deleting previously
  // stored data. The MPI communicator handle is also copied.
  // Normally, this should not be needed. It is used during
  // the construction of a new communicator for copying the
  // world communicator, keeping the same context.
  void CopyFrom(vtkMPICommunicator* source);
protected:
  vtkMPICommunicator();
  ~vtkMPICommunicator();

  // Obtain size and rank setting NumberOfProcesses and LocalProcessId Should
  // not be called if the current communicator does not include this process
  int InitializeNumberOfProcesses();

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
  // stored data EXCEPT the MPI communicator handle which is
  // duplicated with MPI_Comm_dup(). Therefore, although the
  // processes in the communicator remain the same, a new context
  // is created. This prevents the two communicators from 
  // intefering with each other during message send/receives even
  // if the tags are the same.
  void Duplicate(vtkMPICommunicator* source);

  // Description:
  // Implementation for receive data.
  virtual int ReceiveDataInternal(
    char* data, int length, int sizeoftype, 
    int remoteProcessId, int tag,
    vtkMPICommunicatorReceiveDataInfo* info,
    int useCopy, int& senderId);

  vtkMPICommunicatorOpaqueComm* MPIComm;

  int Initialized;
  int KeepHandle;

  int LastSenderId;
  int UseSsend;
  static int CheckForMPIError(int err);

private:
  vtkMPICommunicator(const vtkMPICommunicator&);  // Not implemented.
  void operator=(const vtkMPICommunicator&);  // Not implemented.
};

#endif
