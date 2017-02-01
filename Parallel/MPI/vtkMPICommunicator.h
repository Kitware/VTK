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
/**
 * @class   vtkMPICommunicator
 * @brief   Class for creating user defined MPI communicators.
 *
 *
 * This class can be used to create user defined MPI communicators.
 * The actual creation (with MPI_Comm_create) occurs in Initialize
 * which takes as arguments a super-communicator and a group of
 * process ids. The new communicator is created by including the
 * processes contained in the group. The global communicator
 * (equivalent to MPI_COMM_WORLD) can be obtained using the class
 * method GetWorldCommunicator. It is important to note that
 * this communicator should not be used on the processes not contained
 * in the group. For example, if the group contains processes 0 and 1,
 * controller->SetCommunicator(communicator) would cause an MPI error
 * on any other process.
 *
 * @sa
 * vtkMPIController vtkProcessGroup
*/

#ifndef vtkMPICommunicator_h
#define vtkMPICommunicator_h

#include "vtkParallelMPIModule.h" // For export macro
#include "vtkCommunicator.h"

class vtkMPIController;
class vtkProcessGroup;

class vtkMPICommunicatorOpaqueComm;
class vtkMPICommunicatorOpaqueRequest;
class vtkMPICommunicatorReceiveDataInfo;

class VTKPARALLELMPI_EXPORT vtkMPICommunicator : public vtkCommunicator
{
public:

  class VTKPARALLELMPI_EXPORT Request
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

  vtkTypeMacro( vtkMPICommunicator,vtkCommunicator);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Creates an empty communicator.
   */
  static vtkMPICommunicator* New();

  /**
   * Returns the singleton which behaves as the global
   * communicator (MPI_COMM_WORLD)
   */
  static vtkMPICommunicator* GetWorldCommunicator();


  /**
   * Used to initialize the communicator (i.e. create the underlying MPI_Comm).
   * The group must be associated with a valid vtkMPICommunicator.
   */
  int Initialize(vtkProcessGroup *group);

  /**
   * Used to initialize the communicator (i.e. create the underlying MPI_Comm)
   * using MPI_Comm_split on the given communicator. Return values are 1 for success
   * and 0 otherwise.
   */
  int SplitInitialize(vtkCommunicator *oldcomm, int color, int key);

  //@{
  /**
   * Performs the actual communication.  You will usually use the convenience
   * Send functions defined in the superclass. Return values are 1 for success
   * and 0 otherwise.
   */
  virtual int SendVoidArray(const void *data, vtkIdType length, int type,
                            int remoteProcessId, int tag) VTK_OVERRIDE;
  virtual int ReceiveVoidArray(void *data, vtkIdType length, int type,
                               int remoteProcessId, int tag) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * This method sends data to another process (non-blocking).
   * Tag eliminates ambiguity when multiple sends or receives
   * exist in the same process. The last argument,
   * vtkMPICommunicator::Request& req can later be used (with
   * req.Test() ) to test the success of the message. Return values are 1
   * for success and 0 otherwise.
   */
  int NoBlockSend(const int* data, int length, int remoteProcessId, int tag,
                  Request& req);
  int NoBlockSend(const unsigned long* data, int length, int remoteProcessId,
                  int tag, Request& req);
  int NoBlockSend(const char* data, int length, int remoteProcessId,
                  int tag, Request& req);
  int NoBlockSend(const unsigned char* data, int length, int remoteProcessId,
                    int tag, Request& req);
  int NoBlockSend(const float* data, int length, int remoteProcessId,
                  int tag, Request& req);
  int NoBlockSend(const double* data, int length, int remoteProcessId,
                  int tag, Request& req);
#ifdef VTK_USE_64BIT_IDS
  int NoBlockSend(const vtkIdType* data, int length, int remoteProcessId,
                  int tag, Request& req);
#endif
  //@}

  //@{
  /**
   * This method receives data from a corresponding send (non-blocking).
   * The last argument,
   * vtkMPICommunicator::Request& req can later be used (with
   * req.Test() ) to test the success of the message. Return values
   * are 1 for success and 0 otherwise.
   */
  int NoBlockReceive(int* data, int length, int remoteProcessId,
                     int tag, Request& req);
  int NoBlockReceive(unsigned long* data, int length,
                     int remoteProcessId, int tag, Request& req);
  int NoBlockReceive(char* data, int length, int remoteProcessId,
                     int tag, Request& req);
  int NoBlockReceive(unsigned char* data, int length, int remoteProcessId,
                     int tag, Request& req);
  int NoBlockReceive(float* data, int length, int remoteProcessId,
                     int tag, Request& req);
  int NoBlockReceive(double* data, int length, int remoteProcessId,
                     int tag, Request& req);
#ifdef VTK_USE_64BIT_IDS
  int NoBlockReceive(vtkIdType* data, int length, int remoteProcessId,
                     int tag, Request& req);
#endif
  //@}


  //@{
  /**
   * More efficient implementations of collective operations that use
   * the equivalent MPI commands. Return values are 1 for success
   * and 0 otherwise.
   */
  virtual void Barrier() VTK_OVERRIDE;
  virtual int BroadcastVoidArray(void *data, vtkIdType length, int type,
                                 int srcProcessId) VTK_OVERRIDE;
  virtual int GatherVoidArray(const void *sendBuffer, void *recvBuffer,
                              vtkIdType length, int type, int destProcessId) VTK_OVERRIDE;
  virtual int GatherVVoidArray(const void *sendBuffer, void *recvBuffer,
                               vtkIdType sendLength, vtkIdType *recvLengths,
                               vtkIdType *offsets, int type, int destProcessId) VTK_OVERRIDE;
  virtual int ScatterVoidArray(const void *sendBuffer, void *recvBuffer,
                               vtkIdType length, int type, int srcProcessId) VTK_OVERRIDE;
  virtual int ScatterVVoidArray(const void *sendBuffer, void *recvBuffer,
                                vtkIdType *sendLengths, vtkIdType *offsets,
                                vtkIdType recvLength, int type,
                                int srcProcessId) VTK_OVERRIDE;
  virtual int AllGatherVoidArray(const void *sendBuffer, void *recvBuffer,
                                 vtkIdType length, int type) VTK_OVERRIDE;
  virtual int AllGatherVVoidArray(const void *sendBuffer, void *recvBuffer,
                                  vtkIdType sendLength, vtkIdType *recvLengths,
                                  vtkIdType *offsets, int type) VTK_OVERRIDE;
  virtual int ReduceVoidArray(const void *sendBuffer, void *recvBuffer,
                              vtkIdType length, int type,
                              int operation, int destProcessId) VTK_OVERRIDE;
  virtual int ReduceVoidArray(const void *sendBuffer, void *recvBuffer,
                              vtkIdType length, int type,
                              Operation *operation, int destProcessId) VTK_OVERRIDE;
  virtual int AllReduceVoidArray(const void *sendBuffer, void *recvBuffer,
                                 vtkIdType length, int type,
                                 int operation) VTK_OVERRIDE;
  virtual int AllReduceVoidArray(const void *sendBuffer, void *recvBuffer,
                                 vtkIdType length, int type,
                                 Operation *operation) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Nonblocking test for a message.  Inputs are: source -- the source rank
   * or ANY_SOURCE; tag -- the tag value.  Outputs are:
   * flag -- True if a message matches; actualSource -- the rank
   * sending the message (useful if ANY_SOURCE is used) if flag is True
   * and actualSource isn't NULL; size -- the length of the message in
   * bytes if flag is true (only set if size isn't NULL). The return
   * value is 1 for success and 0 otherwise.
   */
  int Iprobe(int source, int tag, int* flag, int* actualSource);
  int Iprobe(int source, int tag, int* flag, int* actualSource,
             int* type, int* size);
  int Iprobe(int source, int tag, int* flag, int* actualSource,
             unsigned long* type, int* size);
  int Iprobe(int source, int tag, int* flag, int* actualSource,
             const char* type, int* size);
  int Iprobe(int source, int tag, int* flag, int* actualSource,
             float* type, int* size);
  int Iprobe(int source, int tag, int* flag, int* actualSource,
             double* type, int* size);
  //@}

  /**
   * Given the request objects of a set of non-blocking operations
   * (send and/or receive) this method blocks until all requests are complete.
   */
  int WaitAll(const int count, Request requests[]);

  /**
   * Blocks until *one* of the specified requests in the given request array
   * completes. Upon return, the index in the array of the completed request
   * object is returned through the argument list.
   */
  int WaitAny(const int count, Request requests[], int& idx);

  /**
   * Blocks until *one or more* of the specified requests in the given request
   * request array completes. Upon return, the list of handles that have
   * completed is stored in the completed vtkIntArray.
   */
  int WaitSome(
      const int count, Request requests[], int &NCompleted, int *completed );

  /**
   * Checks if the given communication request objects are complete. Upon
   * return, flag evaluates to true iff *all* of the communication request
   * objects are complete.
   */
  int TestAll( const int count, Request requests[], int& flag );

  /**
   * Check if at least *one* of the specified requests has completed.
   */
  int TestAny(const int count, Request requests[], int &idx, int &flag );

  /**
   * Checks the status of *all* the given request communication object handles.
   * Upon return, NCompleted holds the count of requests that have completed
   * and the indices of the completed requests, w.r.t. the requests array is
   * given the by the pre-allocated completed array.
   */
  int TestSome(const int count,Request requests[],
               int& NCompleted,int *completed);

  friend class vtkMPIController;

  vtkMPICommunicatorOpaqueComm *GetMPIComm()
  {
    return this->MPIComm;
  }

  int InitializeExternal(vtkMPICommunicatorOpaqueComm *comm);

  static char* Allocate(size_t size);
  static void Free(char* ptr);


  //@{
  /**
   * When set to 1, all MPI_Send calls are replaced by MPI_Ssend calls.
   * Default is 0.
   */
  vtkSetClampMacro(UseSsend, int, 0, 1);
  vtkGetMacro(UseSsend, int);
  vtkBooleanMacro(UseSsend, int);
  //@}

  /**
   * Copies all the attributes of source, deleting previously
   * stored data. The MPI communicator handle is also copied.
   * Normally, this should not be needed. It is used during
   * the construction of a new communicator for copying the
   * world communicator, keeping the same context.
   */
  void CopyFrom(vtkMPICommunicator* source);

protected:
  vtkMPICommunicator();
  ~vtkMPICommunicator();

  // Obtain size and rank setting NumberOfProcesses and LocalProcessId Should
  // not be called if the current communicator does not include this process
  int InitializeNumberOfProcesses();

  //@{
  /**
   * KeepHandle is normally off. This means that the MPI
   * communicator handle will be freed at the destruction
   * of the object. However, if the handle was copied from
   * another object (via CopyFrom() not Duplicate()), this
   * has to be turned on otherwise the handle will be freed
   * multiple times causing MPI failure. The alternative to
   * this is using reference counting but it is unnecessarily
   * complicated for this case.
   */
  vtkSetMacro(KeepHandle, int);
  vtkBooleanMacro(KeepHandle, int);
  //@}


  static vtkMPICommunicator* WorldCommunicator;

  void InitializeCopy(vtkMPICommunicator* source);

  /**
   * Copies all the attributes of source, deleting previously
   * stored data EXCEPT the MPI communicator handle which is
   * duplicated with MPI_Comm_dup(). Therefore, although the
   * processes in the communicator remain the same, a new context
   * is created. This prevents the two communicators from
   * intefering with each other during message send/receives even
   * if the tags are the same.
   */
  void Duplicate(vtkMPICommunicator* source);

  /**
   * Implementation for receive data.
   */
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
  vtkMPICommunicator(const vtkMPICommunicator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMPICommunicator&) VTK_DELETE_FUNCTION;
};

#endif
