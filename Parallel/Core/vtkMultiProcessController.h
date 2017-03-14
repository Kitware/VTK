/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiProcessController.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMultiProcessController
 * @brief   Multiprocessing communication superclass
 *
 * vtkMultiProcessController is used to control multiple processes
 * in a distributed computing environment. It has
 * methods for executing single/multiple method(s) on multiple processors,
 * triggering registered callbacks (Remote Methods) (AddRMI(), TriggerRMI())
 * and communication. Please note that the communication is done using
 * the communicator which is accessible to the user. Therefore it is
 * possible to get the communicator with GetCommunicator() and use
 * it to send and receive data. This is the encouraged communication method.
 * The internal (RMI) communications are done using a second internal
 * communicator (called RMICommunicator).
 *
 * There are two modes for RMI communication: (1) Send/Receive mode and
 * (2) Broadcast (collective) mode. The Send/Receive mode arranges processes
 * in a binary tree using post-order traversal and propagates the RMI trigger
 * starting from the root (rank 0) to the children. It is commonly employed to
 * communicate between client/server over TCP. Although, the Send/Receive mode
 * can be employed transparently over TCP or MPI, it is not optimal for
 * triggering the RMIs on the satellite ranks. The Broadcast mode provides a
 * more desirable alternative, namely, it uses MPI_Broadcast for communication,
 * which is the nominal way of achieving this in an MPI context. The underlying
 * communication mode used for triggering RMIs is controlled by the
 * "BroadcastTriggerRMI" variable. Note, that mixing between the two modes
 * for RMI communication is not correct behavior. All processes within the
 * vtkMultiProcessController must use the same mode for triggering RMI.
 *
 * @sa
 * vtkMPIController
 * vtkCommunicator vtkMPICommunicator
*/

#ifndef vtkMultiProcessController_h
#define vtkMultiProcessController_h

#include "vtkParallelCoreModule.h" // For export macro
#include "vtkObject.h"

#include "vtkCommunicator.h" // Needed for direct access to communicator

class vtkCollection;
class vtkDataObject;
class vtkDataSet;
class vtkImageData;
class vtkMultiProcessController;
class vtkMultiProcessStream;
class vtkOutputWindow;
class vtkProcessGroup;
class vtkProcess;

// The type of function that gets called when new processes are initiated.
typedef void (*vtkProcessFunctionType)(vtkMultiProcessController *controller,
                                       void *userData);

// The type of function that gets called when an RMI is triggered.
typedef void (*vtkRMIFunctionType)(void *localArg,
                                   void *remoteArg, int remoteArgLength,
                                   int remoteProcessId);

class VTKPARALLELCORE_EXPORT vtkMultiProcessController : public vtkObject
{
public:
  vtkTypeMacro(vtkMultiProcessController,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * This method is for setting up the processes.
   * If a subclass needs to initialize process communication (i.e. MPI)
   * it would over ride this method.
   */
  virtual void Initialize(int* vtkNotUsed(argc), char*** vtkNotUsed(argv))=0;

  /**
   * This method is for setting up the processes.
   * If a subclass needs to initialize process communication (i.e. MPI)
   * it would over ride this method.  Provided for initialization outside vtk.
   */
  virtual void Initialize(int* vtkNotUsed(argc), char*** vtkNotUsed(argv),
                          int initializedExternally)=0;

  /**
   * This method is for cleaning up.
   * If a subclass needs to clean up process communication (i.e. MPI)
   * it would over ride this method.
   */
  virtual void Finalize()=0;

  /**
   * This method is for cleaning up.
   * If a subclass needs to clean up process communication (i.e. MPI)
   * it would over ride this method.  Provided for finalization outside vtk.
   */
  virtual void Finalize(int finalizedExternally)=0;

  //@{
  /**
   * Set the number of processes you will be using.  This defaults
   * to the maximum number available.  If you set this to a value
   * higher than the default, you will get an error.
   */
  void SetNumberOfProcesses(int num);
  int GetNumberOfProcesses();
  //@}

  /**
   * Set the SingleMethod to f() and the UserData of the
   * for the method to be executed by all of the processes
   * when SingleMethodExecute is called.  All the processes will
   * start by calling this function.
   */
  void SetSingleMethod(vtkProcessFunctionType, void *data);

  /**
   * Object-oriented flavor of SetSingleMethod(). Instead of passing
   * some function pointer and user data, a vtkProcess object is passed
   * where the method to execute is Execute() and the data the object itself.
   */
  void SetSingleProcessObject(vtkProcess *p);

  /**
   * Execute the SingleMethod (as define by SetSingleMethod) using
   * this->NumberOfProcesses processes.  This will only return when
   * all the processes finish executing their methods.
   */
  virtual void SingleMethodExecute() = 0;

  /**
   * Set the MultipleMethod to f() and the UserData of the
   * for the method to be executed by the process index
   * when MultipleMethodExecute is called.  This is for having each
   * process start with a different function and data argument.
   */
  void SetMultipleMethod(int index, vtkProcessFunctionType, void *data);

  /**
   * Execute the MultipleMethods (as define by calling SetMultipleMethod
   * for each of the required this->NumberOfProcesses methods) using
   * this->NumberOfProcesses processes.
   */
  virtual void MultipleMethodExecute() = 0;

  /**
   * Tells you which process [0, NumProcess) you are in.
   */
  int GetLocalProcessId();

  /**
   * This convenience method returns the controller associated with the
   * local process.  It returns NULL until the processes are spawned.
   * It is better if you hang on to the controller passed as an argument to the
   * SingleMethod or MultipleMethod functions.
   */
  static vtkMultiProcessController *GetGlobalController();

  /**
   * This method can be used to tell the controller to create
   * a special output window in which all messages are preceded
   * by the process id.
   */
  virtual void CreateOutputWindow() = 0;

  /**
   * Creates a new controller with the processes specified by the given group.
   * The new controller will already be initialized for you.  You are
   * responsible for deleting the controller once you are done.  It is invalid
   * to pass this method a group with a different communicator than is used by
   * this controller.  This operation is collective across all processes
   * defined in the group.  It is undefined what will happen if the group is not
   * the same on all processes.  This method must be called by all processes in
   * the controller regardless of whether they are in the group.  NULL is
   * returned on all process not in the group.
   */
  virtual vtkMultiProcessController *CreateSubController(
                                                        vtkProcessGroup *group);

  /**
   * Partitions this controller based on a coloring.  That is, each process
   * passes in a color.  All processes with the same color are grouped into the
   * same partition.  The processes are ordered by their self-assigned key.
   * Lower keys have lower process ids.  Ties are broken by the current process
   * ids.  (For example, if all the keys are 0, then the resulting processes
   * will be ordered in the same way.)  This method returns a new controller to
   * each process that represents the local partition.  This is basically the
   * same operation as MPI_Comm_split.
   */
  virtual vtkMultiProcessController *PartitionController(int localColor,
                                                         int localKey);

  //------------------ RMIs --------------------

  /**
   * Register remote method invocation in the receiving process
   * which makes the call.  It must have a unique tag as an RMI id.
   * The vtkRMIFunctionType has several arguments: localArg (same as passed in),
   * remoteArg, remoteArgLength (memory passed by process triggering the RMI),
   * remoteProcessId.
   * Since only one callback can be registered per tag, this method will remove
   * any previously registered callback for the given tag.
   * Returns a unique Id for the RMI registration which can be used to
   * unregister the callback. RemoveRMI() should be preferred over
   * RemoveFirstRMI() since it avoid accidental removal of callbacks.
   */
  virtual unsigned long AddRMI(vtkRMIFunctionType, void *localArg, int tag);

  /**
   * Remove the first RMI matching the tag.
   */
  virtual int RemoveFirstRMI(int tag);

  /**
   * Remove the  RMI matching the id. The id is the same id returned by
   * AddRMI().
   */
  virtual int RemoveRMI(unsigned long id);

  /**
   * Take an RMI away.
   */
  virtual void RemoveRMI(vtkRMIFunctionType f, void *arg, int tag)
    {(void)f; (void)arg; (void)tag; vtkErrorMacro("RemoveRMI Not Implemented Yet");};

  /**
   * These methods are a part of the newer API to add multiple rmi callbacks.
   * When the RMI is triggered, all the callbacks are called
   * Adds a new callback for an RMI. Returns the identifier for the callback.
   */
  virtual unsigned long AddRMICallback(vtkRMIFunctionType, void* localArg, int tag);

  /**
   * These methods are a part of the newer API to add multiple rmi callbacks.
   * When the RMI is triggered, all the callbacks are called
   * Removes all callbacks for the tag.
   */
  virtual void RemoveAllRMICallbacks(int tag);

  /**
   * Remove a callback. Returns true is the remove was successful.
   */
  virtual bool RemoveRMICallback(unsigned long id);

  /**
   * A method to trigger a method invocation in another process.
   */
  void TriggerRMI(int remoteProcessId, void *arg, int argLength, int tag);

  /**
   * A conveniance method.  Called on process 0 to break "ProcessRMIs" loop
   * on all other processes.
   */
  void TriggerBreakRMIs();

  /**
   * Convenience method when the arg is a string.
   */
  void TriggerRMI(int remoteProcessId, const char *arg, int tag)
    { this->TriggerRMI(remoteProcessId, (void*)arg,
                       static_cast<int>(strlen(arg))+1, tag); }

  /**
   * Convenience method when there is no argument.
   */
  void TriggerRMI(int remoteProcessId, int tag)
    { this->TriggerRMI(remoteProcessId, NULL, 0, tag); }

  //@{
  /**
   * This is a convenicence method to trigger an RMI call on all the "children"
   * of the current node. The children of the current node can be determined by
   * drawing a binary tree starting at node 0 and then assigned nodes ids
   * incrementally in a breadth-first fashion from left to right. This is
   * designed to be used when trigger an RMI call on all satellites from the
   * root node.
   */
  void TriggerRMIOnAllChildren(void *arg, int argLength, int tag);
  void TriggerRMIOnAllChildren(const char *arg, int tag)
  {
    this->TriggerRMIOnAllChildren(
      (void*)arg, static_cast<int>(strlen(arg))+1, tag);
  }
  void TriggerRMIOnAllChildren(int tag)
  {
    this->TriggerRMIOnAllChildren(NULL, 0, tag);
  }
  void BroadcastTriggerRMIOnAllChildren(void* arg, int argLength, int tag);
  //@}

  //@{
  /**
   * Calling this method gives control to the controller to start
   * processing RMIs. Possible return values are:
   * RMI_NO_ERROR,
   * RMI_TAG_ERROR : rmi tag could not be received,
   * RMI_ARG_ERROR : rmi arg could not be received.
   * If reportErrors is false, no vtkErrorMacro is called.
   * ProcessRMIs() calls ProcessRMIs(int) with reportErrors = 0.
   * If dont_loop is 1, this call just process one RMI message
   * and exits.
   */
  int ProcessRMIs(int reportErrors, int dont_loop = 0);
  int ProcessRMIs();
  int BroadcastProcessRMIs(int reportErrors, int dont_loop=0);
  //@}

  //@{
  /**
   * Setting this flag to 1 will cause the ProcessRMIs loop to return.
   * This also causes vtkUpStreamPorts to return from
   * their WaitForUpdate loops.
   */
  vtkSetMacro(BreakFlag, int);
  vtkGetMacro(BreakFlag, int);
  //@}

  //@{
  /**
   * Setting this flag to 1 will cause the TriggerRMIOnAllChildren to use
   * a collective broadcast operation to communicate the RMI tag to the
   * satellites.
   */
  vtkSetMacro(BroadcastTriggerRMI,bool);
  vtkGetMacro(BroadcastTriggerRMI,bool);
  vtkBooleanMacro(BroadcastTriggerRMI,bool);
  //@}

  //@{
  /**
   * Returns the communicator associated with this controller.
   * A default communicator is created in constructor.
   */
  vtkGetObjectMacro(Communicator, vtkCommunicator);
  //@}

  /**
   * Accessor to some default tags.
   */
  static int GetBreakRMITag() { return BREAK_RMI_TAG; }
  static int GetRMITag() { return RMI_TAG; }
  static int GetRMIArgTag() { return RMI_ARG_TAG; }

  enum Errors
  {
    RMI_NO_ERROR,
    RMI_TAG_ERROR,
    RMI_ARG_ERROR
  };

  enum Consts
  {
    ANY_SOURCE     = -1,
    INVALID_SOURCE = -2
  };

  enum Tags
  {
    RMI_TAG        = 1,
    RMI_ARG_TAG    = 2,
    BREAK_RMI_TAG  = 3,
    XML_WRITER_DATA_INFO = 4
  };

  /**
   * This method can be used to synchronize processes.
   */
  void Barrier();

  static void SetGlobalController(vtkMultiProcessController *controller);

  //------------------ Communication --------------------

  //@{
  /**
   * This method sends data to another process.  Tag eliminates ambiguity
   * when multiple sends or receives exist in the same process.
   * It is recommended to use custom tag number over 100.
   * vtkMultiProcessController has reserved tags between 1 and 4.
   * vtkCommunicator has reserved tags between 10 and 16.
   */
  int Send(const int* data, vtkIdType length, int remoteProcessId, int tag);
  int Send(const short* data, vtkIdType length, int remoteProcessId, int tag);
  int Send(const unsigned short* data, vtkIdType length, int remoteProcessId, int tag);
  int Send(const unsigned int* data, vtkIdType length, int remoteProcessId, int tag);
  int Send(const unsigned long* data, vtkIdType length, int remoteProcessId,
           int tag);
  int Send(const long* data, vtkIdType length, int remoteProcessId,
           int tag);
  int Send(const signed char* data, vtkIdType length, int remoteProcessId, int tag);
  int Send(const char* data, vtkIdType length, int remoteProcessId, int tag);
  int Send(const unsigned char* data, vtkIdType length, int remoteProcessId, int tag);
  int Send(const float* data, vtkIdType length, int remoteProcessId, int tag);
  int Send(const double* data, vtkIdType length, int remoteProcessId, int tag);
  int Send(const long long* data, vtkIdType length, int remoteProcessId, int tag);
  int Send(const unsigned long long* data, vtkIdType length, int remoteProcessId, int tag);
  int Send(vtkDataObject *data, int remoteId, int tag);
  int Send(vtkDataArray *data, int remoteId, int tag);
  //@}

  /**
   * Send a stream to another process. vtkMultiProcessStream makes it possible
   * to send data with arbitrary length and different base types to the other
   * process(es). Instead of making several Send() requests for each type of
   * arguments, it's generally more efficient to push the arguments into the
   * stream and the send the stream over.
   */
  int Send(const vtkMultiProcessStream& stream, int remoteId, int tag);

  //@{
  /**
   * This method receives data from a corresponding send. It blocks
   * until the receive is finished.  It calls methods in "data"
   * to communicate the sending data. In the overrloads that take in a \c
   * maxlength argument, this length is the maximum length of the message to
   * receive. If the maxlength is less than the length of the message sent by
   * the sender, an error will be flagged. Once a message is received, use the
   * GetCount() method to determine the actual size of the data received.
   */
  int Receive(int* data, vtkIdType maxlength, int remoteProcessId, int tag);
  int Receive(unsigned int* data, vtkIdType maxlength, int remoteProcessId, int tag);
  int Receive(short* data, vtkIdType maxlength, int remoteProcessId, int tag);
  int Receive(unsigned short* data, vtkIdType maxlength, int remoteProcessId, int tag);
  int Receive(long* data, vtkIdType maxlength, int remoteProcessId, int tag);
  int Receive(unsigned long* data, vtkIdType maxlength, int remoteProcessId,
              int tag);
  int Receive(char* data, vtkIdType maxlength, int remoteProcessId, int tag);
  int Receive(unsigned char* data, vtkIdType maxlength, int remoteProcessId, int tag);
  int Receive(signed char* data, vtkIdType maxlength, int remoteProcessId, int tag);
  int Receive(float* data, vtkIdType maxlength, int remoteProcessId, int tag);
  int Receive(double* data, vtkIdType maxlength, int remoteProcessId, int tag);
  int Receive(long long* data, vtkIdType maxLength, int remoteProcessId, int tag);
  int Receive(unsigned long long* data, vtkIdType maxLength, int remoteProcessId, int tag);
  int Receive(vtkDataObject* data, int remoteId, int tag);
  int Receive(vtkDataArray* data, int remoteId, int tag);
  //@}

  /**
   * Receive a stream from the other processes.
   */
  int Receive(vtkMultiProcessStream& stream, int remoteId, int tag);

  vtkDataObject *ReceiveDataObject(int remoteId, int tag);

  /**
   * Returns the number of words received by the most recent Receive().
   * Note that this is not the number of bytes received, but the number of items
   * of the data-type received by the most recent Receive() eg. if
   * Receive(int*,..) was used, then this returns the number of ints received;
   * if Receive(double*,..) was used, then this returns the number of doubles
   * received etc. The return value is valid only after a successful Receive().
   */
  vtkIdType GetCount();


  //---------------------- Collective Operations ----------------------

  //@{
  /**
   * Broadcast sends the array in the process with id \c srcProcessId to all of
   * the other processes.  All processes must call these method with the same
   * arguments in order for it to complete.
   */
  int Broadcast(int *data, vtkIdType length, int srcProcessId) {
    return this->Communicator->Broadcast(data, length, srcProcessId);
  }
  int Broadcast(unsigned int *data, vtkIdType length, int srcProcessId) {
    return this->Communicator->Broadcast(data, length, srcProcessId);
  }
  int Broadcast(short *data, vtkIdType length, int srcProcessId) {
    return this->Communicator->Broadcast(data, length, srcProcessId);
  }
  int Broadcast(unsigned short *data, vtkIdType length, int srcProcessId) {
    return this->Communicator->Broadcast(data, length, srcProcessId);
  }
  int Broadcast(long *data, vtkIdType length, int srcProcessId) {
    return this->Communicator->Broadcast(data, length, srcProcessId);
  }
  int Broadcast(unsigned long *data, vtkIdType length, int srcProcessId) {
    return this->Communicator->Broadcast(data, length, srcProcessId);
  }
  int Broadcast(unsigned char *data, vtkIdType length, int srcProcessId) {
    return this->Communicator->Broadcast(data, length, srcProcessId);
  }
  int Broadcast(char *data, vtkIdType length, int srcProcessId) {
    return this->Communicator->Broadcast(data, length, srcProcessId);
  }
  int Broadcast(signed char *data, vtkIdType length, int srcProcessId) {
    return this->Communicator->Broadcast(data, length, srcProcessId);
  }
  int Broadcast(float *data, vtkIdType length, int srcProcessId) {
    return this->Communicator->Broadcast(data, length, srcProcessId);
  }
  int Broadcast(double *data, vtkIdType length, int srcProcessId) {
    return this->Communicator->Broadcast(data, length, srcProcessId);
  }
  int Broadcast(long long *data, vtkIdType length, int srcProcessId) {
    return this->Communicator->Broadcast(data, length, srcProcessId);
  }
  int Broadcast(unsigned long long *data, vtkIdType length, int srcProcessId) {
    return this->Communicator->Broadcast(data, length, srcProcessId);
  }
  int Broadcast(vtkDataObject *data, int srcProcessId) {
    return this->Communicator->Broadcast(data, srcProcessId);
  }
  int Broadcast(vtkDataArray *data, int srcProcessId) {
    return this->Communicator->Broadcast(data, srcProcessId);
  }
  //@}

  int Broadcast(vtkMultiProcessStream& stream, int srcProcessId) {
    return this->Communicator->Broadcast(stream, srcProcessId);
  }

  //@{
  /**
   * Gather collects arrays in the process with id \c destProcessId.  Each
   * process (including the destination) sends the contents of its send buffer
   * to the destination process.  The destination process receives the
   * messages and stores them in rank order.  The \c length argument
   * (which must be the same on all processes) is the length of the
   * sendBuffers.  The \c recvBuffer (on te destination process) must be of
   * length length*numProcesses.  Gather is the inverse operation of Scatter.
   */
  int Gather(const int *sendBuffer, int *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->Communicator->Gather(sendBuffer, recvBuffer, length,
                                      destProcessId);
  }
  int Gather(const unsigned int *sendBuffer, unsigned int *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->Communicator->Gather(sendBuffer, recvBuffer, length,
                                      destProcessId);
  }
  int Gather(const short *sendBuffer, short *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->Communicator->Gather(sendBuffer, recvBuffer, length,
                                      destProcessId);
  }
  int Gather(const unsigned short *sendBuffer, unsigned short *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->Communicator->Gather(sendBuffer, recvBuffer, length,
                                      destProcessId);
  }
  int Gather(const long *sendBuffer, long *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->Communicator->Gather(sendBuffer, recvBuffer, length,
                                      destProcessId);
  }
  int Gather(const unsigned long *sendBuffer, unsigned long *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->Communicator->Gather(sendBuffer, recvBuffer, length,
                                      destProcessId);
  }
  int Gather(const unsigned char *sendBuffer, unsigned char *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->Communicator->Gather(sendBuffer, recvBuffer, length,
                                      destProcessId);
  }
  int Gather(const char *sendBuffer, char *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->Communicator->Gather(sendBuffer, recvBuffer, length,
                                      destProcessId);
  }
  int Gather(const signed char *sendBuffer, signed char *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->Communicator->Gather(sendBuffer, recvBuffer, length,
                                      destProcessId);
  }
  int Gather(const float *sendBuffer, float *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->Communicator->Gather(sendBuffer, recvBuffer, length,
                                      destProcessId);
  }
  int Gather(const double *sendBuffer, double *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->Communicator->Gather(sendBuffer, recvBuffer, length,
                                      destProcessId);
  }
  int Gather(const long long *sendBuffer, long long *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->Communicator->Gather(sendBuffer, recvBuffer, length,
                                      destProcessId);
  }
  int Gather(const unsigned long long *sendBuffer, unsigned long long *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->Communicator->Gather(sendBuffer, recvBuffer, length,
                                      destProcessId);
  }
  int Gather(vtkDataArray *sendBuffer, vtkDataArray *recvBuffer,
             int destProcessId) {
    return this->Communicator->Gather(sendBuffer, recvBuffer, destProcessId);
  }
  //@}

  /**
   * Gathers vtkDataObject (\c sendBuffer) from all ranks to the \c destProcessId.
   * @param[in] sendBuffer - data object to send from local process. Can be null if
   * not sending any data from the current process.
   * @param[out] recvBuffer - vector of data objects to receive data on the receiving
   * rank (identified by \c destProcessId). This may be
   * empty or filled with data object instances. If empty,
   * data objects will be created as needed. If not empty,
   * existing data object will be used.
   * @param[in] destProcessId - process id to gather on.
   * @return - 1 on success, 0 on failure.
   */
  int Gather(vtkDataObject* sendBuffer,
    std::vector<vtkSmartPointer<vtkDataObject> >& recvBuffer,
    int destProcessId)
  {
    return this->Communicator->Gather(sendBuffer, recvBuffer, destProcessId);
  }

  //@{
  /**
   * GatherV is the vector variant of Gather.  It extends the functionality of
   * Gather by allowing a varying count of data from each process.
   * GatherV collects arrays in the process with id \c destProcessId.  Each
   * process (including the destination) sends the contents of its send buffer
   * to the destination process.  The destination process receives the
   * messages and stores them in rank order.  The \c sendLength argument
   * defines how much the local process sends to \c destProcessId and
   * \c recvLengths is an array containing the amount \c destProcessId
   * receives from each process, in rank order.
   */
  int GatherV(const int* sendBuffer, int* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->Communicator->GatherV(sendBuffer, recvBuffer,
                                       sendLength, recvLengths,
                                       offsets, destProcessId);
  }
  int GatherV(const unsigned int* sendBuffer, unsigned int* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->Communicator->GatherV(sendBuffer, recvBuffer,
                                       sendLength, recvLengths,
                                       offsets, destProcessId);
  }
  int GatherV(const short* sendBuffer, short* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->Communicator->GatherV(sendBuffer, recvBuffer,
                                       sendLength, recvLengths,
                                       offsets, destProcessId);
  }
  int GatherV(const unsigned short* sendBuffer, unsigned short* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->Communicator->GatherV(sendBuffer, recvBuffer,
                                       sendLength, recvLengths,
                                       offsets, destProcessId);
  }
  int GatherV(const long* sendBuffer, long* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->Communicator->GatherV(sendBuffer, recvBuffer,
                                       sendLength, recvLengths,
                                       offsets, destProcessId);
  }
  int GatherV(const unsigned long* sendBuffer, unsigned long* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->Communicator->GatherV(sendBuffer, recvBuffer,
                                       sendLength, recvLengths,
                                       offsets, destProcessId);
  }
  int GatherV(const unsigned char* sendBuffer, unsigned char* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->Communicator->GatherV(sendBuffer, recvBuffer,
                                       sendLength, recvLengths,
                                       offsets, destProcessId);
  }
  int GatherV(const char* sendBuffer, char* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->Communicator->GatherV(sendBuffer, recvBuffer,
                                       sendLength, recvLengths,
                                       offsets, destProcessId);
  }
  int GatherV(const signed char* sendBuffer, signed char* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->Communicator->GatherV(sendBuffer, recvBuffer,
                                       sendLength, recvLengths,
                                       offsets, destProcessId);
  }
  int GatherV(const float* sendBuffer, float* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->Communicator->GatherV(sendBuffer, recvBuffer,
                                       sendLength, recvLengths,
                                       offsets, destProcessId);
  }
  int GatherV(const double* sendBuffer, double* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->Communicator->GatherV(sendBuffer, recvBuffer,
                                       sendLength, recvLengths,
                                       offsets, destProcessId);
  }
  int GatherV(const long long* sendBuffer, long long* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->Communicator->GatherV(sendBuffer, recvBuffer,
                                       sendLength, recvLengths,
                                       offsets, destProcessId);
  }
  int GatherV(const unsigned long long* sendBuffer, unsigned long long* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->Communicator->GatherV(sendBuffer, recvBuffer,
                                       sendLength, recvLengths,
                                       offsets, destProcessId);
  }
  //@}

  int GatherV(vtkDataArray *sendBuffer, vtkDataArray *recvBuffer,
              vtkIdType *recvLengths, vtkIdType *offsets, int destProcessId) {
    return this->Communicator->GatherV(sendBuffer, recvBuffer,
                                       recvLengths, offsets,
                                       destProcessId);
  }
  int GatherV(vtkDataArray *sendBuffer, vtkDataArray *recvBuffer,
              vtkIdTypeArray* recvLengths,
              vtkIdTypeArray* offsets,
              int destProcessId)
  {
    return this->Communicator->GatherV(sendBuffer, recvBuffer,
                                       recvLengths, offsets, destProcessId);
  }


  //@{
  /**
   * This special form of GatherV will automatically determine \c recvLengths
   * and \c offsets to tightly pack the data in the \c recvBuffer in process
   * order.  It will also resize \c recvBuffer in order to accommodate the
   * incoming data (unlike the other GatherV variants).
   */
  int GatherV(vtkDataArray *sendBuffer, vtkDataArray *recvBuffer,
              int destProcessId) {
    return this->Communicator->GatherV(sendBuffer, recvBuffer, destProcessId);
  }
  int GatherV(vtkDataObject* sendData, vtkSmartPointer<vtkDataObject>* recvData,
              int destProcessId)
  {
    return this->Communicator->GatherV(sendData, recvData, destProcessId);
  }
  //@}

  //@{
  /**
   * Scatter takes an array in the process with id \c srcProcessId and
   * distributes it.  Each process (including the source) receives a portion of
   * the send buffer.  Process 0 receives the first \c length values, process 1
   * receives the second \c length values, and so on.  Scatter is the inverse
   * operation of Gather.
   */
  int Scatter(const int *sendBuffer, int *recvBuffer,
              vtkIdType length, int srcProcessId) {
    return this->Communicator->Scatter(sendBuffer, recvBuffer, length,
                                       srcProcessId);
  }
  int Scatter(const unsigned int *sendBuffer, unsigned int *recvBuffer,
              vtkIdType length, int srcProcessId) {
    return this->Communicator->Scatter(sendBuffer, recvBuffer, length,
                                       srcProcessId);
  }
  int Scatter(const short *sendBuffer, short *recvBuffer,
              vtkIdType length, int srcProcessId) {
    return this->Communicator->Scatter(sendBuffer, recvBuffer, length,
                                       srcProcessId);
  }
  int Scatter(const unsigned short *sendBuffer, unsigned short *recvBuffer,
              vtkIdType length, int srcProcessId) {
    return this->Communicator->Scatter(sendBuffer, recvBuffer, length,
                                       srcProcessId);
  }
  int Scatter(const long *sendBuffer, long *recvBuffer,
              vtkIdType length, int srcProcessId) {
    return this->Communicator->Scatter(sendBuffer, recvBuffer, length,
                                       srcProcessId);
  }
  int Scatter(const unsigned long *sendBuffer, unsigned long *recvBuffer,
              vtkIdType length, int srcProcessId) {
    return this->Communicator->Scatter(sendBuffer, recvBuffer, length,
                                       srcProcessId);
  }
  int Scatter(const unsigned char *sendBuffer, unsigned char *recvBuffer,
              vtkIdType length, int srcProcessId) {
    return this->Communicator->Scatter(sendBuffer, recvBuffer, length,
                                       srcProcessId);
  }
  int Scatter(const char *sendBuffer, char *recvBuffer,
              vtkIdType length, int srcProcessId) {
    return this->Communicator->Scatter(sendBuffer, recvBuffer, length,
                                       srcProcessId);
  }
  int Scatter(const signed char *sendBuffer, signed char *recvBuffer,
              vtkIdType length, int srcProcessId) {
    return this->Communicator->Scatter(sendBuffer, recvBuffer, length,
                                       srcProcessId);
  }
  int Scatter(const float *sendBuffer, float *recvBuffer,
              vtkIdType length, int srcProcessId) {
    return this->Communicator->Scatter(sendBuffer, recvBuffer, length,
                                       srcProcessId);
  }
  int Scatter(const double *sendBuffer, double *recvBuffer,
              vtkIdType length, int srcProcessId) {
    return this->Communicator->Scatter(sendBuffer, recvBuffer, length,
                                       srcProcessId);
  }
  int Scatter(const long long *sendBuffer, long long *recvBuffer,
              vtkIdType length, int srcProcessId) {
    return this->Communicator->Scatter(sendBuffer, recvBuffer, length,
                                       srcProcessId);
  }
  int Scatter(const unsigned long long *sendBuffer, unsigned long long *recvBuffer,
              vtkIdType length, int srcProcessId) {
    return this->Communicator->Scatter(sendBuffer, recvBuffer, length,
                                       srcProcessId);
  }
  int Scatter(vtkDataArray *sendBuffer, vtkDataArray *recvBuffer,
              int srcProcessId) {
    return this->Communicator->Scatter(sendBuffer, recvBuffer, srcProcessId);
  }
  //@}

  //@{
  /**
   * ScatterV is the vector variant of Scatter.  It extends the functionality of
   * Scatter by allowing a varying count of data to each process.
   * ScatterV takes an array in the process with id \c srcProcessId and
   * distributes it.  Each process (including the source) receives a portion of
   * the send buffer defined by the \c sendLengths and \c offsets arrays.
   */
  int ScatterV(const int *sendBuffer, int *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->Communicator->ScatterV(sendBuffer, recvBuffer,
                                        sendLengths, offsets, recvLength,
                                        srcProcessId);
  }
  int ScatterV(const unsigned int *sendBuffer, unsigned int *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->Communicator->ScatterV(sendBuffer, recvBuffer,
                                        sendLengths, offsets, recvLength,
                                        srcProcessId);
  }
  int ScatterV(const short *sendBuffer, short *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->Communicator->ScatterV(sendBuffer, recvBuffer,
                                        sendLengths, offsets, recvLength,
                                        srcProcessId);
  }
  int ScatterV(const unsigned short *sendBuffer, unsigned short *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->Communicator->ScatterV(sendBuffer, recvBuffer,
                                        sendLengths, offsets, recvLength,
                                        srcProcessId);
  }
  int ScatterV(const long *sendBuffer, long *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->Communicator->ScatterV(sendBuffer, recvBuffer,
                                        sendLengths, offsets, recvLength,
                                        srcProcessId);
  }
  int ScatterV(const unsigned long *sendBuffer, unsigned long *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->Communicator->ScatterV(sendBuffer, recvBuffer,
                                        sendLengths, offsets, recvLength,
                                        srcProcessId);
  }
  int ScatterV(const unsigned char *sendBuffer, unsigned char *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->Communicator->ScatterV(sendBuffer, recvBuffer,
                                        sendLengths, offsets, recvLength,
                                        srcProcessId);
  }
  int ScatterV(const char *sendBuffer, char *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->Communicator->ScatterV(sendBuffer, recvBuffer,
                                        sendLengths, offsets, recvLength,
                                        srcProcessId);
  }
  int ScatterV(const signed char *sendBuffer, signed char *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->Communicator->ScatterV(sendBuffer, recvBuffer,
                                        sendLengths, offsets, recvLength,
                                        srcProcessId);
  }
  int ScatterV(const float *sendBuffer, float *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->Communicator->ScatterV(sendBuffer, recvBuffer,
                                        sendLengths, offsets, recvLength,
                                        srcProcessId);
  }
  int ScatterV(const double *sendBuffer, double *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->Communicator->ScatterV(sendBuffer, recvBuffer,
                                        sendLengths, offsets, recvLength,
                                        srcProcessId);
  }
  int ScatterV(const long long *sendBuffer, long long *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->Communicator->ScatterV(sendBuffer, recvBuffer,
                                        sendLengths, offsets, recvLength,
                                        srcProcessId);
  }
  int ScatterV(const unsigned long long *sendBuffer, unsigned long long *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->Communicator->ScatterV(sendBuffer, recvBuffer,
                                        sendLengths, offsets, recvLength,
                                        srcProcessId);
  }
  //@}

  //@{
  /**
   * Same as gather except that the result ends up on all processes.
   */
  int AllGather(const int *sendBuffer, int *recvBuffer, vtkIdType length) {
    return this->Communicator->AllGather(sendBuffer, recvBuffer, length);
  }
  int AllGather(const unsigned int *sendBuffer, unsigned int *recvBuffer, vtkIdType length) {
    return this->Communicator->AllGather(sendBuffer, recvBuffer, length);
  }
  int AllGather(const short *sendBuffer, short *recvBuffer, vtkIdType length) {
    return this->Communicator->AllGather(sendBuffer, recvBuffer, length);
  }
  int AllGather(const unsigned short *sendBuffer, unsigned short *recvBuffer, vtkIdType length) {
    return this->Communicator->AllGather(sendBuffer, recvBuffer, length);
  }
  int AllGather(const long *sendBuffer, long *recvBuffer, vtkIdType length) {
    return this->Communicator->AllGather(sendBuffer, recvBuffer, length);
  }
  int AllGather(const unsigned long *sendBuffer,
                unsigned long *recvBuffer, vtkIdType length) {
    return this->Communicator->AllGather(sendBuffer, recvBuffer, length);
  }
  int AllGather(const unsigned char *sendBuffer,
                unsigned char *recvBuffer, vtkIdType length) {
    return this->Communicator->AllGather(sendBuffer, recvBuffer, length);
  }
  int AllGather(const char *sendBuffer, char *recvBuffer, vtkIdType length) {
    return this->Communicator->AllGather(sendBuffer, recvBuffer, length);
  }
  int AllGather(const signed char *sendBuffer, signed char *recvBuffer, vtkIdType length) {
    return this->Communicator->AllGather(sendBuffer, recvBuffer, length);
  }
  int AllGather(const float *sendBuffer, float *recvBuffer, vtkIdType length) {
    return this->Communicator->AllGather(sendBuffer, recvBuffer, length);
  }
  int AllGather(const double *sendBuffer,
                double *recvBuffer, vtkIdType length) {
    return this->Communicator->AllGather(sendBuffer, recvBuffer, length);
  }
  int AllGather(const long long *sendBuffer, long long *recvBuffer, vtkIdType length) {
    return this->Communicator->AllGather(sendBuffer, recvBuffer, length);
  }
  int AllGather(const unsigned long long *sendBuffer, unsigned long long *recvBuffer, vtkIdType length) {
    return this->Communicator->AllGather(sendBuffer, recvBuffer, length);
  }
  int AllGather(vtkDataArray *sendBuffer, vtkDataArray *recvBuffer) {
    return this->Communicator->AllGather(sendBuffer, recvBuffer);
  }
  //@}

  //@{
  /**
   * Same as GatherV except that the result is placed in all processes.
   */
  int AllGatherV(const int* sendBuffer, int* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->Communicator->AllGatherV(sendBuffer, recvBuffer,
                                          sendLength, recvLengths,
                                          offsets);
  }
  int AllGatherV(const unsigned int* sendBuffer, unsigned int* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->Communicator->AllGatherV(sendBuffer, recvBuffer,
                                          sendLength, recvLengths,
                                          offsets);
  }
  int AllGatherV(const short* sendBuffer, short* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->Communicator->AllGatherV(sendBuffer, recvBuffer,
                                          sendLength, recvLengths,
                                          offsets);
  }
  int AllGatherV(const unsigned short* sendBuffer, unsigned short* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->Communicator->AllGatherV(sendBuffer, recvBuffer,
                                          sendLength, recvLengths,
                                          offsets);
  }
  int AllGatherV(const long* sendBuffer, long* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->Communicator->AllGatherV(sendBuffer, recvBuffer,
                                          sendLength, recvLengths,
                                          offsets);
  }
  int AllGatherV(const unsigned long* sendBuffer, unsigned long* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->Communicator->AllGatherV(sendBuffer, recvBuffer,
                                          sendLength, recvLengths,
                                          offsets);
  }
  int AllGatherV(const unsigned char* sendBuffer, unsigned char* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->Communicator->AllGatherV(sendBuffer, recvBuffer,
                                          sendLength, recvLengths,
                                          offsets);
  }
  int AllGatherV(const char* sendBuffer, char* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->Communicator->AllGatherV(sendBuffer, recvBuffer,
                                          sendLength, recvLengths,
                                          offsets);
  }
  int AllGatherV(const signed char* sendBuffer, signed char* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->Communicator->AllGatherV(sendBuffer, recvBuffer,
                                          sendLength, recvLengths,
                                          offsets);
  }
  int AllGatherV(const float* sendBuffer, float* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->Communicator->AllGatherV(sendBuffer, recvBuffer,
                                          sendLength, recvLengths,
                                          offsets);
  }
  int AllGatherV(const double* sendBuffer, double* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->Communicator->AllGatherV(sendBuffer, recvBuffer,
                                          sendLength, recvLengths,
                                          offsets);
  }
  int AllGatherV(const long long* sendBuffer, long long* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->Communicator->AllGatherV(sendBuffer, recvBuffer,
                                          sendLength, recvLengths,
                                          offsets);
  }
  int AllGatherV(const unsigned long long* sendBuffer, unsigned long long* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->Communicator->AllGatherV(sendBuffer, recvBuffer,
                                          sendLength, recvLengths,
                                          offsets);
  }
  int AllGatherV(vtkDataArray *sendBuffer, vtkDataArray *recvBuffer,
                 vtkIdType *recvLengths, vtkIdType *offsets) {
    return this->Communicator->AllGatherV(sendBuffer, recvBuffer,
                                          recvLengths, offsets);
  }
  //@}

  /**
   * This special form of AllGatherV will automatically determine \c recvLengths
   * and \c offsets to tightly pack the data in the \c recvBuffer in process
   * order.  It will also resize \c recvBuffer in order to accommodate the
   * incoming data (unlike the other GatherV variants).
   */
  int AllGatherV(vtkDataArray *sendBuffer, vtkDataArray *recvBuffer) {
    return this->Communicator->AllGatherV(sendBuffer, recvBuffer);
  }

  //@{
  /**
   * Reduce an array to the given destination process.  This version of Reduce
   * takes an identifier defined in the
   * vtkCommunicator::StandardOperations enum to define the operation.
   */
  int Reduce(const int *sendBuffer, int *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(const unsigned int *sendBuffer, unsigned int *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(const short *sendBuffer, short *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(const unsigned short *sendBuffer, unsigned short *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(const long *sendBuffer, long *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(const unsigned long *sendBuffer, unsigned long *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(const unsigned char *sendBuffer, unsigned char *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(const char *sendBuffer, char *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(const signed char *sendBuffer, signed char *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(const float *sendBuffer, float *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(const double *sendBuffer, double *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(const long long *sendBuffer, long long *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(const unsigned long long *sendBuffer, unsigned long long *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(vtkDataArray *sendBuffer, vtkDataArray *recvBuffer,
             int operation, int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer,
                                      operation, destProcessId);
  }
  //@}

  //@{
  /**
   * Reduce an array to the given destination process.  This version of Reduce
   * takes a custom operation as a subclass of vtkCommunicator::Operation.
   */
  int Reduce(const int *sendBuffer, int *recvBuffer,
             vtkIdType length, vtkCommunicator::Operation *operation,
             int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(const unsigned int *sendBuffer, unsigned int *recvBuffer,
             vtkIdType length, vtkCommunicator::Operation *operation,
             int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(const short *sendBuffer, short *recvBuffer,
             vtkIdType length, vtkCommunicator::Operation *operation,
             int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(const unsigned short *sendBuffer, unsigned short *recvBuffer,
             vtkIdType length, vtkCommunicator::Operation *operation,
             int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(const long *sendBuffer, long *recvBuffer,
             vtkIdType length, vtkCommunicator::Operation *operation,
             int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(const unsigned long *sendBuffer, unsigned long *recvBuffer,
             vtkIdType length, vtkCommunicator::Operation *operation,
             int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(const unsigned char *sendBuffer, unsigned char *recvBuffer,
             vtkIdType length, vtkCommunicator::Operation *operation,
             int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(const char *sendBuffer, char *recvBuffer,
             vtkIdType length, vtkCommunicator::Operation *operation,
             int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(const signed char *sendBuffer, signed char *recvBuffer,
             vtkIdType length, vtkCommunicator::Operation *operation,
             int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(const float *sendBuffer, float *recvBuffer,
             vtkIdType length, vtkCommunicator::Operation *operation,
             int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(const double *sendBuffer, double *recvBuffer,
             vtkIdType length, vtkCommunicator::Operation *operation,
             int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(const long long *sendBuffer, long long *recvBuffer,
             vtkIdType length, vtkCommunicator::Operation *operation,
             int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(const unsigned long long *sendBuffer, unsigned long long *recvBuffer,
             vtkIdType length, vtkCommunicator::Operation *operation,
             int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer, length,
                                      operation, destProcessId);
  }
  int Reduce(vtkDataArray *sendBuffer, vtkDataArray *recvBuffer,
             vtkCommunicator::Operation *operation, int destProcessId) {
    return this->Communicator->Reduce(sendBuffer, recvBuffer,
                                      operation, destProcessId);
  }
  //@}

  //@{
  /**
   * Same as Reduce except that the result is placed in all of the processes.
   */
  int AllReduce(const int *sendBuffer, int *recvBuffer,
                vtkIdType length, int operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(const unsigned int *sendBuffer, unsigned int *recvBuffer,
                vtkIdType length, int operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(const short *sendBuffer, short *recvBuffer,
                vtkIdType length, int operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(const unsigned short *sendBuffer, unsigned short *recvBuffer,
                vtkIdType length, int operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(const long *sendBuffer, long *recvBuffer,
                vtkIdType length, int operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(const unsigned long *sendBuffer, unsigned long *recvBuffer,
                vtkIdType length, int operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(const unsigned char *sendBuffer, unsigned char *recvBuffer,
                vtkIdType length, int operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(const char *sendBuffer, char *recvBuffer,
                vtkIdType length, int operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(const signed char *sendBuffer, signed char *recvBuffer,
                vtkIdType length, int operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(const float *sendBuffer, float *recvBuffer,
                vtkIdType length, int operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(const double *sendBuffer, double *recvBuffer,
                vtkIdType length, int operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(const long long *sendBuffer, long long *recvBuffer,
                vtkIdType length, int operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(const unsigned long long *sendBuffer, unsigned long long *recvBuffer,
                vtkIdType length, int operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(vtkDataArray *sendBuffer, vtkDataArray *recvBuffer,
                int operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, operation);
  }
  //@}

  int AllReduce(const int *sendBuffer, int *recvBuffer,
                vtkIdType length, vtkCommunicator::Operation *operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(const unsigned int *sendBuffer, unsigned int *recvBuffer,
                vtkIdType length, vtkCommunicator::Operation *operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(const short *sendBuffer, short *recvBuffer,
                vtkIdType length, vtkCommunicator::Operation *operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(const unsigned short *sendBuffer, unsigned short *recvBuffer,
                vtkIdType length, vtkCommunicator::Operation *operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(const long *sendBuffer, long *recvBuffer,
                vtkIdType length, vtkCommunicator::Operation *operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(const unsigned long *sendBuffer, unsigned long *recvBuffer,
                vtkIdType length, vtkCommunicator::Operation *operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(const unsigned char *sendBuffer, unsigned char *recvBuffer,
                vtkIdType length, vtkCommunicator::Operation *operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(const char *sendBuffer, char *recvBuffer,
                vtkIdType length, vtkCommunicator::Operation *operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(const signed char *sendBuffer, signed char *recvBuffer,
                vtkIdType length, vtkCommunicator::Operation *operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(const float *sendBuffer, float *recvBuffer,
                vtkIdType length, vtkCommunicator::Operation *operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(const double *sendBuffer, double *recvBuffer,
                vtkIdType length, vtkCommunicator::Operation *operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(const long long *sendBuffer, long long *recvBuffer,
                vtkIdType length, vtkCommunicator::Operation *operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(const unsigned long long *sendBuffer, unsigned long long *recvBuffer,
                vtkIdType length, vtkCommunicator::Operation *operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, length,
                                         operation);
  }
  int AllReduce(vtkDataArray *sendBuffer, vtkDataArray *recvBuffer,
                vtkCommunicator::Operation *operation) {
    return this->Communicator->AllReduce(sendBuffer, recvBuffer, operation);
  }

// Internally implemented RMI to break the process loop.

protected:
  vtkMultiProcessController();
  ~vtkMultiProcessController() VTK_OVERRIDE;

  /**
   * Implementation for TriggerRMI() provides subclasses an opportunity to
   * modify the behaviour eg. MPIController provides ability to use SSend
   * instead of Send.
   */
  virtual void TriggerRMIInternal(int remoteProcessId,
    void* arg, int argLength, int rmiTag, bool propagate);

  vtkProcessFunctionType      SingleMethod;
  void                       *SingleData;

  void GetMultipleMethod(int index, vtkProcessFunctionType &func, void *&data);

  // This is a flag that can be used by the ports to break
  // their update loop. (same as ProcessRMIs)
  int BreakFlag;

  void ProcessRMI(int remoteProcessId, void *arg, int argLength, int rmiTag);

  // This method implements "GetGlobalController".
  // It needs to be virtual and static.
  virtual vtkMultiProcessController *GetLocalController();


  // This flag can force deep copies during send.
  int ForceDeepCopy;

  // This flag can be used to indicate that an MPI Broadcast will be used
  // when calling TriggerRMIOnAllChildren(), instead of the binary tree
  // propagation of the data to the satellite ranks from rank 0.
  bool BroadcastTriggerRMI;

  vtkOutputWindow* OutputWindow;

  // Note that since the communicators can be created differently
  // depending on the type of controller, the subclasses are
  // responsible of deleting them.
  vtkCommunicator* Communicator;

  // Communicator which is a copy of the current user
  // level communicator except the context; i.e. even if the tags
  // are the same, the RMI messages will not interfere with user
  // level messages.
  // Note that since the communicators can be created differently
  // depending on the type of controller, the subclasses are
  // responsible of deleting them.
  vtkCommunicator* RMICommunicator;

private:
  vtkMultiProcessController(const vtkMultiProcessController&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMultiProcessController&) VTK_DELETE_FUNCTION;

  unsigned long RMICount;

  class vtkInternal;
  vtkInternal *Internal;

};


inline int vtkMultiProcessController::Send(vtkDataObject *data,
                                           int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Send(data, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Send(vtkDataArray *data,
                                           int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Send(data, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Send(const int* data, vtkIdType length,
                                           int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Send(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Send(const short* data, vtkIdType length,
                                           int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Send(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Send(const unsigned short* data, vtkIdType length,
                                           int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Send(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Send(const unsigned int* data, vtkIdType length,
                                           int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Send(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Send(const unsigned long* data,
                                           vtkIdType length,
                                           int remoteProcessId,
                                           int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Send(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Send(const long* data,
                                           vtkIdType length,
                                           int remoteProcessId,
                                           int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Send(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Send(const signed char* data, vtkIdType length,
                                           int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Send(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Send(const char* data, vtkIdType length,
                                           int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Send(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Send(const unsigned char* data,
                                           vtkIdType length,
                                           int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Send(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Send(const float* data, vtkIdType length,
                                           int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Send(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Send(const double* data, vtkIdType length,
                                           int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Send(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Send(const long long* data,
                                           vtkIdType length,
                                           int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Send(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Send(const unsigned long long* data,
                                           vtkIdType length,
                                           int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Send(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Send(const vtkMultiProcessStream& stream,
  int remoteId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Send(stream, remoteId, tag);
  }
  return 0;
}

inline int vtkMultiProcessController::Receive(vtkDataObject* data,
                                              int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Receive(data, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline vtkDataObject* vtkMultiProcessController::ReceiveDataObject(
  int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->ReceiveDataObject(remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Receive(vtkDataArray* data,
                                              int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Receive(data, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Receive(int* data, vtkIdType length,
                                              int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Receive(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Receive(unsigned int* data, vtkIdType length,
                                              int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Receive(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Receive(short* data, vtkIdType length,
                                              int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Receive(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Receive(unsigned short* data, vtkIdType length,
                                              int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Receive(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Receive(long* data, vtkIdType length,
                                              int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Receive(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}


inline int vtkMultiProcessController::Receive(unsigned long* data,
                                              vtkIdType length,
                                              int remoteProcessId,
                                              int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Receive(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Receive(char* data, vtkIdType length,
                                              int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Receive(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Receive(unsigned char* data,
                                              vtkIdType length,
                                              int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Receive(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Receive(signed char* data, vtkIdType length,
                                              int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Receive(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}


inline int vtkMultiProcessController::Receive(float* data, vtkIdType length,
                                              int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Receive(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Receive(double* data, vtkIdType length,
                                              int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Receive(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Receive(long long* data, vtkIdType length,
                                              int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Receive(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Receive(unsigned long long* data, vtkIdType length,
                                              int remoteProcessId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Receive(data, length, remoteProcessId, tag);
  }
  else
  {
    return 0;
  }
}

inline int vtkMultiProcessController::Receive(vtkMultiProcessStream& stream,
  int remoteId, int tag)
{
  if (this->Communicator)
  {
    return this->Communicator->Receive(stream, remoteId, tag);
  }
  return 0;
}

inline void vtkMultiProcessController::Barrier()
{
  if (this->Communicator)
  {
    this->Communicator->Barrier();
  }
}

inline vtkIdType vtkMultiProcessController::GetCount()
{
  if (this->Communicator)
  {
    return this->Communicator->GetCount();
  }
  return 0;
}

#endif
