/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCommunicator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCommunicator - Used to send/receive messages in a multiprocess environment.
// .SECTION Description
// This is an abstact class which contains functionality for sending
// and receiving inter-process messages. It contains methods for marshaling
// an object into a string (currently used by the MPI communicator but
// not the shared memory communicator).

// .SECTION Caveats
// Communication between systems with different vtkIdTypes is not
// supported. All machines have to have the same vtkIdType.

// .SECTION see also
// vtkMPICommunicator

#ifndef vtkCommunicator_h
#define vtkCommunicator_h

#include "vtkParallelCoreModule.h" // For export macro
#include "vtkObject.h"
#include "vtkSmartPointer.h"

class vtkBoundingBox;
class vtkCharArray;
class vtkDataArray;
class vtkDataObject;
class vtkDataSet;
class vtkIdTypeArray;
class vtkImageData;
class vtkMultiBlockDataSet;
class vtkMultiProcessStream;

class VTKPARALLELCORE_EXPORT vtkCommunicator : public vtkObject
{

public:

  vtkTypeMacro(vtkCommunicator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the number of processes you will be using.  This defaults
  // to the maximum number available.  If you set this to a value
  // higher than the default, you will get an error.
  virtual void SetNumberOfProcesses(int num);
  vtkGetMacro(NumberOfProcesses, int);

  // Description:
  // Tells you which process [0, NumProcess) you are in.
  vtkGetMacro(LocalProcessId, int);

//BTX

  enum Tags
  {
    BROADCAST_TAG       = 10,
    GATHER_TAG          = 11,
    GATHERV_TAG         = 12,
    SCATTER_TAG         = 13,
    SCATTERV_TAG        = 14,
    REDUCE_TAG          = 15,
    BARRIER_TAG         = 16
  };

  enum StandardOperations
  {
    MAX_OP,
    MIN_OP,
    SUM_OP,
    PRODUCT_OP,
    LOGICAL_AND_OP,
    BITWISE_AND_OP,
    LOGICAL_OR_OP,
    BITWISE_OR_OP,
    LOGICAL_XOR_OP,
    BITWISE_XOR_OP
  };

  // Description:
  // A custom operation to use in a reduce command.  Subclass this object to
  // provide your own operations.
  class Operation
  {
  public:
    // Description:
    // Subclasses must overload this method, which performs the actual
    // operations.  The methods should first do a reintepret cast of the arrays
    // to the type suggestsed by \c datatype (which will be one of the VTK type
    // identifiers like VTK_INT, etc.).  Both arrays are considered top be
    // length entries.  The method should perform the operation A*B (where * is
    // a placeholder for whatever operation is actually performed) and store the
    // result in B.  The operation is assumed to be associative.  Commutativity
    // is specified by the Commutative method.
    virtual void Function(const void *A, void *B, vtkIdType length,
                          int datatype) = 0;

    // Description:
    // Subclasses override this method to specify whether their operation
    // is commutative.  It should return 1 if commutative or 0 if not.
    virtual int Commutative() = 0;

    virtual ~Operation() {}
  };

//ETX

  // Description:
  // This method sends a data object to a destination.
  // Tag eliminates ambiguity
  // and is used to match sends to receives.
  int Send(vtkDataObject* data, int remoteHandle, int tag);

  // Description:
  // This method sends a data array to a destination.
  // Tag eliminates ambiguity
  // and is used to match sends to receives.
  int Send(vtkDataArray* data, int remoteHandle, int tag);

  // Description:
  // Subclasses have to supply this method to send various arrays of data.
  // The \c type arg is one of the VTK type constants recognized by the
  // vtkTemplateMacro (VTK_FLOAT, VTK_INT, etc.).  \c length is measured
  // in number of values (as opposed to number of bytes).
  virtual int SendVoidArray(const void *data, vtkIdType length, int type,
                            int remoteHandle, int tag) = 0;

  // Description:
  // Convenience methods for sending data arrays.
  int Send(const int* data, vtkIdType length, int remoteHandle, int tag) {
    return this->SendVoidArray(data, length, VTK_INT, remoteHandle, tag);
  }
  int Send(const unsigned int* data, vtkIdType length, int remoteHandle, int tag) {
    return this->SendVoidArray(data, length, VTK_INT, remoteHandle, tag);
  }
  int Send(const short* data, vtkIdType length, int remoteHandle, int tag) {
    return this->SendVoidArray(data, length, VTK_SHORT, remoteHandle, tag);
  }
  int Send(const unsigned short* data, vtkIdType length, int remoteHandle, int tag) {
    return this->SendVoidArray(data, length, VTK_UNSIGNED_SHORT, remoteHandle, tag);
  }
  int Send(const long* data, vtkIdType length,
           int remoteHandle, int tag) {
    return this->SendVoidArray(data, length,VTK_LONG,remoteHandle,tag);
  }
  int Send(const unsigned long* data, vtkIdType length,
           int remoteHandle, int tag) {
    return this->SendVoidArray(data, length,VTK_UNSIGNED_LONG,remoteHandle,tag);
  }
  int Send(const unsigned char* data, vtkIdType length,
           int remoteHandle, int tag) {
    return this->SendVoidArray(data, length,VTK_UNSIGNED_CHAR,remoteHandle,tag);
  }
  int Send(const char* data, vtkIdType length, int remoteHandle, int tag) {
    return this->SendVoidArray(data, length, VTK_CHAR, remoteHandle, tag);
  }
  int Send(const signed char* data, vtkIdType length, int remoteHandle, int tag) {
    return this->SendVoidArray(data, length, VTK_SIGNED_CHAR, remoteHandle, tag);
  }
  int Send(const float* data, vtkIdType length, int remoteHandle, int tag) {
    return this->SendVoidArray(data, length, VTK_FLOAT, remoteHandle, tag);
  }
  int Send(const double* data, vtkIdType length, int remoteHandle, int tag) {
    return this->SendVoidArray(data, length, VTK_DOUBLE, remoteHandle, tag);
  }
#ifdef VTK_USE_64BIT_IDS
  int Send(const vtkIdType* data, vtkIdType length, int remoteHandle, int tag) {
    return this->SendVoidArray(data, length, VTK_ID_TYPE, remoteHandle, tag);
  }
#elif defined(VTK_TYPE_USE_LONG_LONG)
  int Send(const long long* data, vtkIdType length, int remoteHandle, int tag) {
    return this->SendVoidArray(data, length, VTK_LONG_LONG, remoteHandle, tag);
  }
#endif
#ifdef VTK_TYPE_USE_LONG_LONG
  int Send(const unsigned long long* data, vtkIdType length, int remoteHandle, int tag) {
    return this->SendVoidArray(data, length, VTK_UNSIGNED_LONG_LONG, remoteHandle, tag);
  }
#endif
//BTX
  int Send(const vtkMultiProcessStream& stream, int remoteId, int tag);
//ETX


  // Description:
  // This method receives a data object from a corresponding send. It blocks
  // until the receive is finished.
  int Receive(vtkDataObject* data, int remoteHandle, int tag);

  // Description:
  // The caller does not have to know the data type before this call is made.
  // It returns the newly created object.
  vtkDataObject *ReceiveDataObject(int remoteHandle, int tag);

  // Description:
  // This method receives a data array from a corresponding send. It blocks
  // until the receive is finished.
  int Receive(vtkDataArray* data, int remoteHandle, int tag);

  // Description:
  // Subclasses have to supply this method to receive various arrays of data.
  // The \c type arg is one of the VTK type constants recognized by the
  // vtkTemplateMacro (VTK_FLOAT, VTK_INT, etc.).  \c maxlength is measured
  // in number of values (as opposed to number of bytes) and is the maxmum
  // length of the data to receive.  If the maxlength is less than the length
  // of the message sent by the sender, an error will be flagged. Once a
  // message is received, use the GetCount() method to determine the actual
  // size of the data received.
  virtual int ReceiveVoidArray(void *data, vtkIdType maxlength, int type,
                               int remoteHandle, int tag) = 0;

  // Description:
  // Convenience methods for receiving data arrays.
  int Receive(int* data, vtkIdType maxlength, int remoteHandle, int tag) {
    return this->ReceiveVoidArray(data, maxlength, VTK_INT, remoteHandle, tag);
  }
  int Receive(unsigned int* data, vtkIdType maxlength, int remoteHandle, int tag) {
    return this->ReceiveVoidArray(data, maxlength, VTK_INT, remoteHandle, tag);
  }
  int Receive(short* data, vtkIdType maxlength, int remoteHandle, int tag) {
    return this->ReceiveVoidArray(data, maxlength, VTK_SHORT, remoteHandle, tag);
  }
  int Receive(unsigned short* data, vtkIdType maxlength, int remoteHandle, int tag) {
    return this->ReceiveVoidArray(data, maxlength, VTK_UNSIGNED_SHORT, remoteHandle, tag);
  }
  int Receive(long* data, vtkIdType maxlength, int remoteHandle, int tag) {
    return this->ReceiveVoidArray(data, maxlength, VTK_LONG, remoteHandle, tag);
  }
  int Receive(unsigned long* data, vtkIdType maxlength, int remoteHandle, int tag){
    return this->ReceiveVoidArray(data, maxlength, VTK_UNSIGNED_LONG, remoteHandle,
                                  tag);
  }
  int Receive(unsigned char* data, vtkIdType maxlength, int remoteHandle, int tag){
    return this->ReceiveVoidArray(data, maxlength, VTK_UNSIGNED_CHAR, remoteHandle,
                                  tag);
  }
  int Receive(char* data, vtkIdType maxlength, int remoteHandle, int tag) {
    return this->ReceiveVoidArray(data, maxlength, VTK_CHAR, remoteHandle, tag);
  }
  int Receive(signed char* data, vtkIdType maxlength, int remoteHandle, int tag) {
    return this->ReceiveVoidArray(data, maxlength, VTK_SIGNED_CHAR, remoteHandle, tag);
  }
  int Receive(float* data, vtkIdType maxlength, int remoteHandle, int tag) {
    return this->ReceiveVoidArray(data, maxlength, VTK_FLOAT, remoteHandle, tag);
  }
  int Receive(double* data, vtkIdType maxlength, int remoteHandle, int tag) {
    return this->ReceiveVoidArray(data, maxlength, VTK_DOUBLE, remoteHandle, tag);
  }
#ifdef VTK_USE_64BIT_IDS
  int Receive(vtkIdType* data, vtkIdType maxlength, int remoteHandle, int tag) {
    return this->ReceiveVoidArray(data, maxlength, VTK_ID_TYPE, remoteHandle, tag);
  }
#elif defined(VTK_TYPE_USE_LONG_LONG)
  int Receive(long long* data, vtkIdType maxlength, int remoteHandle, int tag) {
    return this->ReceiveVoidArray(data, maxlength, VTK_LONG_LONG, remoteHandle, tag);
  }
#endif
#ifdef VTK_TYPE_USE_LONG_LONG
  int Receive(unsigned long long* data, vtkIdType maxlength, int remoteHandle, int tag) {
    return this->ReceiveVoidArray(data, maxlength, VTK_UNSIGNED_LONG_LONG, remoteHandle, tag);
  }
#endif
//BTX
  int Receive(vtkMultiProcessStream& stream, int remoteId, int tag);
//ETX

  // Description:
  // Returns the number of words received by the most recent Receive().
  // Note that this is not the number of bytes received, but the number of items
  // of the data-type received by the most recent Receive() eg. if
  // Receive(int*,..) was used, then this returns the number of ints received;
  // if Receive(double*,..) was used, then this returns the number of doubles
  // received etc. The return value is valid only after a successful Receive().
  vtkGetMacro(Count, vtkIdType);

  //---------------------- Collective Operations ----------------------

  // Description:
  // Will block the processes until all other processes reach the Barrier
  // function.
  virtual void Barrier();

  // Description:
  // Broadcast sends the array in the process with id \c srcProcessId to all of
  // the other processes.  All processes must call these method with the same
  // arguments in order for it to complete.
  int Broadcast(int *data, vtkIdType length, int srcProcessId) {
    return this->BroadcastVoidArray(data, length, VTK_INT, srcProcessId);
  }
  int Broadcast(unsigned int *data, vtkIdType length, int srcProcessId) {
    return this->BroadcastVoidArray(data, length, VTK_UNSIGNED_INT, srcProcessId);
  }
  int Broadcast(short *data, vtkIdType length, int srcProcessId) {
    return this->BroadcastVoidArray(data, length, VTK_SHORT, srcProcessId);
  }
  int Broadcast(unsigned short *data, vtkIdType length, int srcProcessId) {
    return this->BroadcastVoidArray(data, length, VTK_UNSIGNED_SHORT, srcProcessId);
  }
  int Broadcast(long *data, vtkIdType length, int srcProcessId) {
    return this->BroadcastVoidArray(data, length, VTK_LONG, srcProcessId);
  }
  int Broadcast(unsigned long *data, vtkIdType length, int srcProcessId) {
    return this->BroadcastVoidArray(data,length,VTK_UNSIGNED_LONG,srcProcessId);
  }
  int Broadcast(unsigned char *data, vtkIdType length, int srcProcessId) {
    return this->BroadcastVoidArray(data,length,VTK_UNSIGNED_CHAR,srcProcessId);
  }
  int Broadcast(char *data, vtkIdType length, int srcProcessId) {
    return this->BroadcastVoidArray(data, length, VTK_CHAR, srcProcessId);
  }
  int Broadcast(signed char *data, vtkIdType length, int srcProcessId) {
    return this->BroadcastVoidArray(data, length, VTK_SIGNED_CHAR, srcProcessId);
  }
  int Broadcast(float *data, vtkIdType length, int srcProcessId) {
    return this->BroadcastVoidArray(data, length, VTK_FLOAT, srcProcessId);
  }
  int Broadcast(double *data, vtkIdType length, int srcProcessId) {
    return this->BroadcastVoidArray(data, length, VTK_DOUBLE, srcProcessId);
  }
#ifdef VTK_USE_64BIT_IDS
  int Broadcast(vtkIdType *data, vtkIdType length, int srcProcessId) {
    return this->BroadcastVoidArray(data, length, VTK_ID_TYPE, srcProcessId);
  }
#elif defined(VTK_TYPE_USE_LONG_LONG)
  int Broadcast(long long *data, vtkIdType length, int srcProcessId) {
    return this->BroadcastVoidArray(data, length, VTK_LONG_LONG, srcProcessId);
  }
#endif
#ifdef VTK_TYPE_USE_LONG_LONG
  int Broadcast(unsigned long long *data, vtkIdType length, int srcProcessId) {
    return this->BroadcastVoidArray(data, length, VTK_UNSIGNED_LONG_LONG, srcProcessId);
  }
#endif
  int Broadcast(vtkDataObject *data, int srcProcessId);
  int Broadcast(vtkDataArray *data, int srcProcessId);
//BTX
  int Broadcast(vtkMultiProcessStream& stream, int srcProcessId);
//ETX

  // Description:
  // Gather collects arrays in the process with id \c destProcessId.  Each
  // process (including the destination) sends the contents of its send buffer
  // to the destination process.  The destination process receives the
  // messages and stores them in rank order.  The \c length argument
  // (which must be the same on all processes) is the length of the
  // sendBuffers.  The \c recvBuffer (on te destination process) must be of
  // length length*numProcesses.  Gather is the inverse operation of Scatter.
  int Gather(const int *sendBuffer, int *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_INT, destProcessId);
  }
  int Gather(const unsigned int *sendBuffer, unsigned int *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_UNSIGNED_INT, destProcessId);
  }
  int Gather(const short *sendBuffer, short *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_SHORT, destProcessId);
  }
  int Gather(const unsigned short *sendBuffer, unsigned short *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_UNSIGNED_SHORT, destProcessId);
  }
  int Gather(const long *sendBuffer, long *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_LONG, destProcessId);
  }
  int Gather(const unsigned long *sendBuffer, unsigned long *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_UNSIGNED_LONG, destProcessId);
  }
  int Gather(const unsigned char *sendBuffer, unsigned char *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_UNSIGNED_CHAR, destProcessId);
  }
  int Gather(const char *sendBuffer, char *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_CHAR, destProcessId);
  }
  int Gather(const signed char *sendBuffer, signed char *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_SIGNED_CHAR, destProcessId);
  }
  int Gather(const float *sendBuffer, float *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_FLOAT, destProcessId);
  }
  int Gather(const double *sendBuffer, double *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_DOUBLE, destProcessId);
  }
#ifdef VTK_USE_64BIT_IDS
  int Gather(const vtkIdType *sendBuffer, vtkIdType *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_ID_TYPE, destProcessId);
  }
#elif defined(VTK_TYPE_USE_LONG_LONG)
  int Gather(const long long *sendBuffer, long long *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_LONG_LONG, destProcessId);
  }
#endif
#ifdef VTK_TYPE_USE_LONG_LONG
  int Gather(const unsigned long long *sendBuffer, unsigned long long *recvBuffer,
             vtkIdType length, int destProcessId) {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_UNSIGNED_LONG_LONG, destProcessId);
  }
#endif
  int Gather(vtkDataArray *sendBuffer, vtkDataArray *recvBuffer,
             int destProcessId);

  // Description:
  // GatherV is the vector variant of Gather.  It extends the functionality of
  // Gather by allowing a varying count of data from each process.
  // GatherV collects arrays in the process with id \c destProcessId.  Each
  // process (including the destination) sends the contents of its send buffer
  // to the destination process.  The destination process receives the
  // messages and stores them in rank order.  The \c sendLength argument
  // defines how much the local process sends to \c destProcessId and
  // \c recvLengths is an array containing the amount \c destProcessId
  // receives from each process, in rank order.
  int GatherV(const int* sendBuffer, int* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->GatherVVoidArray(sendBuffer, recvBuffer,
                                  sendLength, recvLengths,
                                  offsets, VTK_INT, destProcessId);
  }
  int GatherV(const unsigned int* sendBuffer, unsigned int* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->GatherVVoidArray(sendBuffer, recvBuffer,
                                  sendLength, recvLengths,
                                  offsets, VTK_UNSIGNED_INT, destProcessId);
  }
  int GatherV(const short* sendBuffer, short* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->GatherVVoidArray(sendBuffer, recvBuffer,
                                  sendLength, recvLengths,
                                  offsets, VTK_SHORT, destProcessId);
  }
  int GatherV(const unsigned short* sendBuffer, unsigned short* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->GatherVVoidArray(sendBuffer, recvBuffer,
                                  sendLength, recvLengths,
                                  offsets, VTK_UNSIGNED_SHORT, destProcessId);
  }
  int GatherV(const long* sendBuffer, long* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->GatherVVoidArray(sendBuffer, recvBuffer,
                                  sendLength, recvLengths,
                                  offsets, VTK_LONG, destProcessId);
  }
  int GatherV(const unsigned long* sendBuffer, unsigned long* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->GatherVVoidArray(sendBuffer, recvBuffer,
                                  sendLength, recvLengths,
                                  offsets, VTK_UNSIGNED_LONG, destProcessId);
  }
  int GatherV(const unsigned char* sendBuffer, unsigned char* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->GatherVVoidArray(sendBuffer, recvBuffer,
                                  sendLength, recvLengths,
                                  offsets, VTK_UNSIGNED_CHAR, destProcessId);
  }
  int GatherV(const char* sendBuffer, char* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->GatherVVoidArray(sendBuffer, recvBuffer,
                                  sendLength, recvLengths,
                                  offsets, VTK_CHAR, destProcessId);
  }
  int GatherV(const signed char* sendBuffer, signed char* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->GatherVVoidArray(sendBuffer, recvBuffer,
                                  sendLength, recvLengths,
                                  offsets, VTK_SIGNED_CHAR, destProcessId);
  }
  int GatherV(const float* sendBuffer, float* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->GatherVVoidArray(sendBuffer, recvBuffer,
                                  sendLength, recvLengths,
                                  offsets, VTK_FLOAT, destProcessId);
  }
  int GatherV(const double* sendBuffer, double* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->GatherVVoidArray(sendBuffer, recvBuffer,
                                  sendLength, recvLengths,
                                  offsets, VTK_DOUBLE, destProcessId);
  }
#ifdef VTK_USE_64BIT_IDS
  int GatherV(const vtkIdType* sendBuffer, vtkIdType* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->GatherVVoidArray(sendBuffer, recvBuffer,
                                  sendLength, recvLengths,
                                  offsets, VTK_ID_TYPE, destProcessId);
  }
#elif defined(VTK_TYPE_USE_LONG_LONG)
  int GatherV(const long long* sendBuffer, long long* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->GatherVVoidArray(sendBuffer, recvBuffer,
                                  sendLength, recvLengths,
                                  offsets, VTK_LONG_LONG, destProcessId);
  }
#endif
#ifdef VTK_TYPE_USE_LONG_LONG
  int GatherV(const unsigned long long* sendBuffer, unsigned long long* recvBuffer,
              vtkIdType sendLength, vtkIdType* recvLengths, vtkIdType* offsets,
              int destProcessId) {
    return this->GatherVVoidArray(sendBuffer, recvBuffer,
                                  sendLength, recvLengths,
                                  offsets, VTK_UNSIGNED_LONG_LONG, destProcessId);
  }
#endif
  // Description:
  // For the first GatherV variant, \c recvLenghts and \c offsets known on
  // \c destProcessId and are passed in as parameters
  // For the second GatherV variant, \c recvLenghts and \c offsets are not known
  // on \c destProcessId.  The \c recvLenghts is set using a gather operation
  // and \c offsets is computed from \c recvLenghts. recvLengths has
  // \c NumberOfProcesses elements and \offsets has NumberOfProcesses + 1 elements.
  // The third variant is the same as the second variant but it does not expose
  // \c recvLength and \c offsets
  int GatherV(vtkDataArray *sendBuffer, vtkDataArray *recvBuffer,
              vtkIdType *recvLengths, vtkIdType *offsets,
              int destProcessId);
  int GatherV(vtkDataArray *sendBuffer, vtkDataArray *recvBuffer,
              vtkIdTypeArray* recvLengths,
              vtkIdTypeArray* offsets,
              int destProcessId);
  int GatherV(vtkDataArray *sendBuffer, vtkDataArray *recvBuffer,
              int destProcessId);
  // Description:
  // Collects data objects in the process with id \c
  // destProcessId.  Each process (including the destination) marshals
  // and then sends the data object to the destination process.  The
  // destination process unmarshals and then stores the data objects
  // in rank order. The \c recvData (on the destination process) must
  // be of length numProcesses.
  int GatherV(vtkDataObject* sendData, vtkSmartPointer<vtkDataObject>* recvData,
              int destProcessId);

  // Description:
  // Scatter takes an array in the process with id \c srcProcessId and
  // distributes it.  Each process (including the source) receives a portion of
  // the send buffer.  Process 0 receives the first \c length values, process 1
  // receives the second \c length values, and so on.  Scatter is the inverse
  // operation of Gather.
  int Scatter(const int *sendBuffer, int *recvBuffer,
             vtkIdType length, int srcProcessId) {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length,
                                  VTK_INT, srcProcessId);
  }
  int Scatter(const unsigned int *sendBuffer, unsigned int *recvBuffer,
             vtkIdType length, int srcProcessId) {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length,
                                  VTK_UNSIGNED_INT, srcProcessId);
  }
  int Scatter(const short *sendBuffer, short *recvBuffer,
             vtkIdType length, int srcProcessId) {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length,
                                  VTK_SHORT, srcProcessId);
  }
  int Scatter(const unsigned short *sendBuffer, unsigned short *recvBuffer,
             vtkIdType length, int srcProcessId) {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length,
                                  VTK_UNSIGNED_SHORT, srcProcessId);
  }
  int Scatter(const long *sendBuffer, long *recvBuffer,
             vtkIdType length, int srcProcessId) {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length,
                                  VTK_INT, srcProcessId);
  }
  int Scatter(const unsigned long *sendBuffer, unsigned long *recvBuffer,
             vtkIdType length, int srcProcessId) {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length,
                                  VTK_UNSIGNED_LONG, srcProcessId);
  }
  int Scatter(const unsigned char *sendBuffer, unsigned char *recvBuffer,
             vtkIdType length, int srcProcessId) {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length,
                                  VTK_UNSIGNED_CHAR, srcProcessId);
  }
  int Scatter(const char *sendBuffer, char *recvBuffer,
             vtkIdType length, int srcProcessId) {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length,
                                  VTK_CHAR, srcProcessId);
  }
  int Scatter(const signed char *sendBuffer, signed char *recvBuffer,
             vtkIdType length, int srcProcessId) {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length,
                                  VTK_SIGNED_CHAR, srcProcessId);
  }
  int Scatter(const float *sendBuffer, float *recvBuffer,
             vtkIdType length, int srcProcessId) {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length,
                                  VTK_FLOAT, srcProcessId);
  }
  int Scatter(const double *sendBuffer, double *recvBuffer,
             vtkIdType length, int srcProcessId) {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length,
                                  VTK_DOUBLE, srcProcessId);
  }
#ifdef VTK_USE_64BIT_IDS
  int Scatter(const vtkIdType *sendBuffer, vtkIdType *recvBuffer,
             vtkIdType length, int srcProcessId) {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length,
                                  VTK_ID_TYPE, srcProcessId);
  }
#elif defined(VTK_TYPE_USE_LONG_LONG)
  int Scatter(const long long *sendBuffer, long long *recvBuffer,
             vtkIdType length, int srcProcessId) {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length,
                                  VTK_LONG_LONG, srcProcessId);
  }
#endif
#ifdef VTK_TYPE_USE_LONG_LONG
  int Scatter(const unsigned long long *sendBuffer, unsigned long long *recvBuffer,
             vtkIdType length, int srcProcessId) {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length,
                                  VTK_UNSIGNED_LONG_LONG, srcProcessId);
  }
#endif
  int Scatter(vtkDataArray *sendBuffer, vtkDataArray *recvBuffer,
             int srcProcessId);

  // Description:
  // ScatterV is the vector variant of Scatter.  It extends the functionality of
  // Scatter by allowing a varying count of data to each process.
  // ScatterV takes an array in the process with id \c srcProcessId and
  // distributes it.  Each process (including the source) receives a portion of
  // the send buffer defined by the \c sendLengths and \c offsets arrays.
  int ScatterV(const int *sendBuffer, int *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->ScatterVVoidArray(sendBuffer, recvBuffer,
                                   sendLengths, offsets, recvLength,
                                   VTK_INT, srcProcessId);
  }
  int ScatterV(const unsigned int *sendBuffer, unsigned int *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->ScatterVVoidArray(sendBuffer, recvBuffer,
                                   sendLengths, offsets, recvLength,
                                   VTK_UNSIGNED_INT, srcProcessId);
  }
  int ScatterV(const short *sendBuffer, short *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->ScatterVVoidArray(sendBuffer, recvBuffer,
                                   sendLengths, offsets, recvLength,
                                   VTK_SHORT, srcProcessId);
  }
  int ScatterV(const unsigned short *sendBuffer, unsigned short *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->ScatterVVoidArray(sendBuffer, recvBuffer,
                                   sendLengths, offsets, recvLength,
                                   VTK_UNSIGNED_SHORT, srcProcessId);
  }
  int ScatterV(const long *sendBuffer, long *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->ScatterVVoidArray(sendBuffer, recvBuffer,
                                   sendLengths, offsets, recvLength,
                                   VTK_LONG, srcProcessId);
  }
  int ScatterV(const unsigned long *sendBuffer, unsigned long *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->ScatterVVoidArray(sendBuffer, recvBuffer,
                                   sendLengths, offsets, recvLength,
                                   VTK_UNSIGNED_LONG, srcProcessId);
  }
  int ScatterV(const unsigned char *sendBuffer, unsigned char *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->ScatterVVoidArray(sendBuffer, recvBuffer,
                                   sendLengths, offsets, recvLength,
                                   VTK_UNSIGNED_CHAR, srcProcessId);
  }
  int ScatterV(const char *sendBuffer, char *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->ScatterVVoidArray(sendBuffer, recvBuffer,
                                   sendLengths, offsets, recvLength,
                                   VTK_CHAR, srcProcessId);
  }
  int ScatterV(const signed char *sendBuffer, signed char *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->ScatterVVoidArray(sendBuffer, recvBuffer,
                                   sendLengths, offsets, recvLength,
                                   VTK_SIGNED_CHAR, srcProcessId);
  }
  int ScatterV(const float *sendBuffer, float *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->ScatterVVoidArray(sendBuffer, recvBuffer,
                                   sendLengths, offsets, recvLength,
                                   VTK_FLOAT, srcProcessId);
  }
  int ScatterV(const double *sendBuffer, double *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->ScatterVVoidArray(sendBuffer, recvBuffer,
                                   sendLengths, offsets, recvLength,
                                   VTK_DOUBLE, srcProcessId);
  }
#ifdef VTK_USE_64BIT_IDS
  int ScatterV(const vtkIdType *sendBuffer, vtkIdType *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->ScatterVVoidArray(sendBuffer, recvBuffer,
                                   sendLengths, offsets, recvLength,
                                   VTK_ID_TYPE, srcProcessId);
  }
#elif defined(VTK_TYPE_USE_LONG_LONG)
  int ScatterV(const long long *sendBuffer, long long *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->ScatterVVoidArray(sendBuffer, recvBuffer,
                                   sendLengths, offsets, recvLength,
                                   VTK_LONG_LONG, srcProcessId);
  }
#endif
#ifdef VTK_TYPE_USE_LONG_LONG
  int ScatterV(const unsigned long long *sendBuffer, unsigned long long *recvBuffer,
               vtkIdType *sendLengths, vtkIdType *offsets,
               vtkIdType recvLength, int srcProcessId) {
    return this->ScatterVVoidArray(sendBuffer, recvBuffer,
                                   sendLengths, offsets, recvLength,
                                   VTK_UNSIGNED_LONG_LONG, srcProcessId);
  }
#endif

  // Description:
  // Same as gather except that the result ends up on all processes.
  int AllGather(const int *sendBuffer, int *recvBuffer, vtkIdType length) {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length, VTK_INT);
  }
  int AllGather(const unsigned int *sendBuffer, unsigned int *recvBuffer,
                vtkIdType length) {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_UNSIGNED_INT);
  }
  int AllGather(const short *sendBuffer, short *recvBuffer, vtkIdType length) {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length, VTK_SHORT);
  }
  int AllGather(const unsigned short *sendBuffer, unsigned short *recvBuffer,
                vtkIdType length) {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_UNSIGNED_SHORT);
  }
  int AllGather(const long *sendBuffer, long *recvBuffer, vtkIdType length) {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length, VTK_LONG);
  }
  int AllGather(const unsigned long *sendBuffer,
                unsigned long *recvBuffer, vtkIdType length) {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_UNSIGNED_LONG);
  }
  int AllGather(const unsigned char *sendBuffer,
                unsigned char *recvBuffer, vtkIdType length) {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_UNSIGNED_CHAR);
  }
  int AllGather(const char *sendBuffer, char *recvBuffer, vtkIdType length) {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length, VTK_CHAR);
  }
  int AllGather(const signed char *sendBuffer, signed char *recvBuffer,
                vtkIdType length) {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_SIGNED_CHAR);
  }
  int AllGather(const float *sendBuffer, float *recvBuffer, vtkIdType length) {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length, VTK_FLOAT);
  }
  int AllGather(const double *sendBuffer,
                double *recvBuffer, vtkIdType length) {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length, VTK_DOUBLE);
  }
#ifdef VTK_USE_64BIT_IDS
  int AllGather(const vtkIdType *sendBuffer, vtkIdType *recvBuffer,
                vtkIdType length) {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_ID_TYPE);
  }
#elif defined(VTK_TYPE_USE_LONG_LONG)
  int AllGather(const long long *sendBuffer, long long *recvBuffer,
                vtkIdType length) {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_LONG_LONG);
  }
#endif
#ifdef VTK_TYPE_USE_LONG_LONG
  int AllGather(const unsigned long long *sendBuffer,
                unsigned long long *recvBuffer, vtkIdType length) {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_UNSIGNED_LONG_LONG);
  }
#endif
  int AllGather(vtkDataArray *sendBuffer, vtkDataArray *recvBuffer);

  // Description:
  // Same as GatherV except that the result is placed in all processes.
  int AllGatherV(const int* sendBuffer, int* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->AllGatherVVoidArray(sendBuffer, recvBuffer,
                                     sendLength, recvLengths,
                                     offsets, VTK_INT);
  }
  int AllGatherV(const unsigned int* sendBuffer, unsigned int* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->AllGatherVVoidArray(sendBuffer, recvBuffer,
                                     sendLength, recvLengths,
                                     offsets, VTK_UNSIGNED_INT);
  }
  int AllGatherV(const short* sendBuffer, short* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->AllGatherVVoidArray(sendBuffer, recvBuffer,
                                     sendLength, recvLengths,
                                     offsets, VTK_SHORT);
  }
  int AllGatherV(const unsigned short* sendBuffer, unsigned short* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->AllGatherVVoidArray(sendBuffer, recvBuffer,
                                     sendLength, recvLengths,
                                     offsets, VTK_UNSIGNED_SHORT);
  }
  int AllGatherV(const long* sendBuffer, long* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->AllGatherVVoidArray(sendBuffer, recvBuffer,
                                     sendLength, recvLengths,
                                     offsets, VTK_LONG);
  }
  int AllGatherV(const unsigned long* sendBuffer, unsigned long* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->AllGatherVVoidArray(sendBuffer, recvBuffer,
                                     sendLength, recvLengths,
                                     offsets, VTK_UNSIGNED_LONG);
  }
  int AllGatherV(const unsigned char* sendBuffer, unsigned char* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->AllGatherVVoidArray(sendBuffer, recvBuffer,
                                     sendLength, recvLengths,
                                     offsets, VTK_UNSIGNED_CHAR);
  }
  int AllGatherV(const char* sendBuffer, char* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->AllGatherVVoidArray(sendBuffer, recvBuffer,
                                     sendLength, recvLengths,
                                     offsets, VTK_CHAR);
  }
  int AllGatherV(const signed char* sendBuffer, signed char* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->AllGatherVVoidArray(sendBuffer, recvBuffer,
                                     sendLength, recvLengths,
                                     offsets, VTK_UNSIGNED_CHAR);
  }
  int AllGatherV(const float* sendBuffer, float* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->AllGatherVVoidArray(sendBuffer, recvBuffer,
                                     sendLength, recvLengths,
                                     offsets, VTK_FLOAT);
  }
  int AllGatherV(const double* sendBuffer, double* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->AllGatherVVoidArray(sendBuffer, recvBuffer,
                                     sendLength, recvLengths,
                                     offsets, VTK_DOUBLE);
  }
#ifdef VTK_USE_64BIT_IDS
  int AllGatherV(const vtkIdType* sendBuffer, vtkIdType* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->AllGatherVVoidArray(sendBuffer, recvBuffer,
                                     sendLength, recvLengths,
                                     offsets, VTK_ID_TYPE);
  }
#elif defined(VTK_TYPE_USE_LONG_LONG)
  int AllGatherV(const long long* sendBuffer, long long* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->AllGatherVVoidArray(sendBuffer, recvBuffer,
                                     sendLength, recvLengths,
                                     offsets, VTK_LONG_LONG);
  }
#endif
#ifdef VTK_TYPE_USE_LONG_LONG
  int AllGatherV(const unsigned long long* sendBuffer, unsigned long long* recvBuffer,
                 vtkIdType sendLength, vtkIdType* recvLengths,
                 vtkIdType* offsets) {
    return this->AllGatherVVoidArray(sendBuffer, recvBuffer,
                                     sendLength, recvLengths,
                                     offsets, VTK_UNSIGNED_LONG_LONG);
  }
#endif
  int AllGatherV(vtkDataArray *sendBuffer, vtkDataArray *recvBuffer,
                 vtkIdType *recvLengths, vtkIdType *offsets);
  int AllGatherV(vtkDataArray *sendBuffer, vtkDataArray *recvBuffer);

  // Description:
  // Reduce an array to the given destination process.  This version of Reduce
  // takes an identifier defined in the
  // vtkCommunicator::StandardOperations enum to define the operation.
  int Reduce(const int *sendBuffer, int *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_INT, operation, destProcessId);
  }
  int Reduce(const unsigned int *sendBuffer, unsigned int *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_UNSIGNED_INT, operation, destProcessId);
  }
  int Reduce(const short *sendBuffer, short *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_SHORT, operation, destProcessId);
  }
  int Reduce(const unsigned short *sendBuffer, unsigned short *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_UNSIGNED_SHORT, operation, destProcessId);
  }
  int Reduce(const long *sendBuffer, long *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_LONG, operation, destProcessId);
  }
  int Reduce(const unsigned long *sendBuffer, unsigned long *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_UNSIGNED_LONG, operation, destProcessId);
  }
  int Reduce(const unsigned char *sendBuffer, unsigned char *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_UNSIGNED_CHAR, operation, destProcessId);
  }
  int Reduce(const char *sendBuffer, char *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_CHAR, operation, destProcessId);
  }
  int Reduce(const signed char *sendBuffer, signed char *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_SIGNED_CHAR, operation, destProcessId);
  }
  int Reduce(const float *sendBuffer, float *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_FLOAT, operation, destProcessId);
  }
  int Reduce(const double *sendBuffer, double *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_DOUBLE, operation, destProcessId);
  }
#ifdef VTK_USE_64BIT_IDS
  int Reduce(const vtkIdType *sendBuffer, vtkIdType *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_ID_TYPE, operation, destProcessId);
  }
#elif defined(VTK_TYPE_USE_LONG_LONG)
  int Reduce(const long long *sendBuffer, long long *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_LONG_LONG, operation, destProcessId);
  }
#endif
#ifdef VTK_TYPE_USE_LONG_LONG
  int Reduce(const unsigned long long *sendBuffer, unsigned long long *recvBuffer,
             vtkIdType length, int operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_UNSIGNED_LONG_LONG, operation, destProcessId);
  }
#endif
  int Reduce(vtkDataArray *sendBuffer, vtkDataArray *recvBuffer,
             int operation, int destProcessId);

  // Description:
  // Reduce an array to the given destination process.  This version of Reduce
  // takes a custom operation as a subclass of vtkCommunicator::Operation.
  int Reduce(const int *sendBuffer, int *recvBuffer,
             vtkIdType length, Operation *operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_INT, operation, destProcessId);
  }
  int Reduce(const unsigned int *sendBuffer, unsigned int *recvBuffer,
             vtkIdType length, Operation *operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_UNSIGNED_INT, operation, destProcessId);
  }
  int Reduce(const short *sendBuffer, short *recvBuffer,
             vtkIdType length, Operation *operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_SHORT, operation, destProcessId);
  }
  int Reduce(const unsigned short *sendBuffer, unsigned short *recvBuffer,
             vtkIdType length, Operation *operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_UNSIGNED_SHORT, operation, destProcessId);
  }
  int Reduce(const long *sendBuffer, long *recvBuffer,
             vtkIdType length, Operation *operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_LONG, operation, destProcessId);
  }
  int Reduce(const unsigned long *sendBuffer, unsigned long *recvBuffer,
             vtkIdType length, Operation *operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_UNSIGNED_LONG, operation, destProcessId);
  }
  int Reduce(const unsigned char *sendBuffer, unsigned char *recvBuffer,
             vtkIdType length, Operation *operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_UNSIGNED_CHAR, operation, destProcessId);
  }
  int Reduce(const char *sendBuffer, char *recvBuffer,
             vtkIdType length, Operation *operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_CHAR, operation, destProcessId);
  }
  int Reduce(const signed char *sendBuffer, signed char *recvBuffer,
             vtkIdType length, Operation *operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_SIGNED_CHAR, operation, destProcessId);
  }
  int Reduce(const float *sendBuffer, float *recvBuffer,
             vtkIdType length, Operation *operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_FLOAT, operation, destProcessId);
  }
  int Reduce(const double *sendBuffer, double *recvBuffer,
             vtkIdType length, Operation *operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_DOUBLE, operation, destProcessId);
  }
#ifdef VTK_USE_64BIT_IDS
  int Reduce(const vtkIdType *sendBuffer, vtkIdType *recvBuffer,
             vtkIdType length, Operation *operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_ID_TYPE, operation, destProcessId);
  }
#elif defined(VTK_TYPE_USE_LONG_LONG)
  int Reduce(const long long *sendBuffer, long long *recvBuffer,
             vtkIdType length, Operation *operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_LONG_LONG, operation, destProcessId);
  }
#endif
#ifdef VTK_TYPE_USE_LONG_LONG
  int Reduce(const unsigned long long *sendBuffer, unsigned long long *recvBuffer,
             vtkIdType length, Operation *operation, int destProcessId) {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length,
                                 VTK_UNSIGNED_LONG_LONG, operation, destProcessId);
  }
#endif
  int Reduce(vtkDataArray *sendBuffer, vtkDataArray *recvBuffer,
             Operation *operation, int destProcessId);

  // Description:
  // Same as Reduce except that the result is placed in all of the processes.
  int AllReduce(const int *sendBuffer, int *recvBuffer,
                vtkIdType length, int operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_INT, operation);
  }
  int AllReduce(const unsigned int *sendBuffer, unsigned int *recvBuffer,
                vtkIdType length, int operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_UNSIGNED_INT, operation);
  }
  int AllReduce(const short *sendBuffer, short *recvBuffer,
                vtkIdType length, int operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_SHORT, operation);
  }
  int AllReduce(const unsigned short *sendBuffer, unsigned short *recvBuffer,
                vtkIdType length, int operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_UNSIGNED_SHORT, operation);
  }
  int AllReduce(const long *sendBuffer, long *recvBuffer,
                vtkIdType length, int operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_LONG, operation);
  }
  int AllReduce(const unsigned long *sendBuffer, unsigned long *recvBuffer,
                vtkIdType length, int operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_UNSIGNED_LONG, operation);
  }
  int AllReduce(const unsigned char *sendBuffer, unsigned char *recvBuffer,
                vtkIdType length, int operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_UNSIGNED_CHAR, operation);
  }
  int AllReduce(const char *sendBuffer, char *recvBuffer,
                vtkIdType length, int operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_CHAR, operation);
  }
  int AllReduce(const signed char *sendBuffer, signed char *recvBuffer,
                vtkIdType length, int operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_SIGNED_CHAR, operation);
  }
  int AllReduce(const float *sendBuffer, float *recvBuffer,
                vtkIdType length, int operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_FLOAT, operation);
  }
  int AllReduce(const double *sendBuffer, double *recvBuffer,
                vtkIdType length, int operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_DOUBLE, operation);
  }
#ifdef VTK_USE_64BIT_IDS
  int AllReduce(const vtkIdType *sendBuffer, vtkIdType *recvBuffer,
                vtkIdType length, int operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_ID_TYPE, operation);
  }
#elif defined(VTK_TYPE_USE_LONG_LONG)
  int AllReduce(const long long *sendBuffer, long long *recvBuffer,
                vtkIdType length, int operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_LONG_LONG, operation);
  }
#endif
#ifdef VTK_TYPE_USE_LONG_LONG
  int AllReduce(const unsigned long long *sendBuffer, unsigned long long *recvBuffer,
                vtkIdType length, int operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_UNSIGNED_LONG_LONG, operation);
  }
#endif
  int AllReduce(vtkDataArray *sendBuffer, vtkDataArray *recvBuffer,
                int operation);
  int AllReduce(const int *sendBuffer, int *recvBuffer,
                vtkIdType length, Operation *operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_INT, operation);
  }
  int AllReduce(const unsigned int *sendBuffer, unsigned int *recvBuffer,
                vtkIdType length, Operation *operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_UNSIGNED_INT, operation);
  }
  int AllReduce(const short *sendBuffer, short *recvBuffer,
                vtkIdType length, Operation *operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_SHORT, operation);
  }
  int AllReduce(const unsigned short *sendBuffer, unsigned short *recvBuffer,
                vtkIdType length, Operation *operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_UNSIGNED_SHORT, operation);
  }
  int AllReduce(const long *sendBuffer, long *recvBuffer,
                vtkIdType length, Operation *operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_LONG, operation);
  }
  int AllReduce(const unsigned long *sendBuffer, unsigned long *recvBuffer,
                vtkIdType length, Operation *operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_UNSIGNED_LONG, operation);
  }
  int AllReduce(const unsigned char *sendBuffer, unsigned char *recvBuffer,
                vtkIdType length, Operation *operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_UNSIGNED_CHAR, operation);
  }
  int AllReduce(const char *sendBuffer, char *recvBuffer,
                vtkIdType length, Operation *operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_CHAR, operation);
  }
  int AllReduce(const signed char *sendBuffer, signed char *recvBuffer,
                vtkIdType length, Operation *operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_SIGNED_CHAR, operation);
  }
  int AllReduce(const float *sendBuffer, float *recvBuffer,
                vtkIdType length, Operation *operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_FLOAT, operation);
  }
  int AllReduce(const double *sendBuffer, double *recvBuffer,
                vtkIdType length, Operation *operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_DOUBLE, operation);
  }
#ifdef VTK_USE_64BIT_IDS
  int AllReduce(const vtkIdType *sendBuffer, vtkIdType *recvBuffer,
                vtkIdType length, Operation *operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_ID_TYPE, operation);
  }
#elif defined(VTK_TYPE_USE_LONG_LONG)
  int AllReduce(const long long *sendBuffer, long long *recvBuffer,
                vtkIdType length, Operation *operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_LONG_LONG, operation);
  }
#endif
#ifdef VTK_TYPE_USE_LONG_LONG
  int AllReduce(const unsigned long long *sendBuffer, unsigned long long *recvBuffer,
                vtkIdType length, Operation *operation) {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length,
                                    VTK_UNSIGNED_LONG_LONG, operation);
  }
#endif
  int AllReduce(vtkDataArray *sendBuffer, vtkDataArray *recvBuffer,
                Operation *operation);

  // Description:
  // Subclasses should reimplement these if they have a more efficient
  // implementation.
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

  static void SetUseCopy(int useCopy);

//BTX
  // Description:
  // Determine the global bounds for a set of processes.  BBox is
  // initially set (outside of the call to the local bounds of the process
  // and will be modified to be the global bounds - this default implementation
  // views the processors as a heap tree with the root being processId = 0
  // If either rightHasBounds or leftHasBounds is not 0 then the
  // corresponding int will be set to 1 if the right/left processor has
  // bounds else it will be set to 0
  // The last three arguments are the tags to be used when performing
  // the operation
  virtual int ComputeGlobalBounds(int processorId, int numProcesses,
                                  vtkBoundingBox *bounds,
                                  int *rightHasBounds = 0,
                                  int *leftHasBounds = 0,
                                  int hasBoundsTag = 288402,
                                  int localBoundsTag = 288403,
                                  int globalBoundsTag = 288404);
//ETX

  // Description:
  // Some helper functions when dealing with heap tree - based
  // algorthims - we don't need a function for getting the right
  // processor since it is 1 + theLeftProcessor
  static int GetParentProcessor(int pid);
  static int GetLeftChildProcessor(int pid);

  // Description:
  // Convert a data object into a string that can be transmitted and vice versa.
  // Returns 1 for success and 0 for failure.
  // WARNING: This will only work for types that have a vtkDataWriter class.
  static int MarshalDataObject(vtkDataObject *object, vtkCharArray *buffer);
  static int UnMarshalDataObject(vtkCharArray *buffer, vtkDataObject *object);

protected:

  int WriteDataArray(vtkDataArray *object);
  int ReadDataArray(vtkDataArray *object);

  vtkCommunicator();
  ~vtkCommunicator();

  // Internal methods called by Send/Receive(vtkDataObject *... ) above.
  int SendElementalDataObject(vtkDataObject* data, int remoteHandle, int tag);
  // Description:
  // GatherV collects arrays in the process with id \c destProcessId.
  // Each process (including the destination) sends its sendArray to
  // the destination process.  The destination process receives the
  // arrays and stores them in rank order in recvArrays.  The \c recvArays is an
  // array containing  \c NumberOfProcesses elements. The \c recvArray allocates
  // and manages memory for \c recvArrays.
  int GatherV(vtkDataArray *sendArray, vtkDataArray* recvArray,
              vtkSmartPointer<vtkDataArray>* recvArrays, int destProcessId);
  int GatherVElementalDataObject(vtkDataObject* sendData,
                                 vtkSmartPointer<vtkDataObject>* receiveData,
                                 int destProcessId);

  int ReceiveDataObject(vtkDataObject* data,
                        int remoteHandle, int tag, int type=-1);
  int ReceiveElementalDataObject(vtkDataObject* data,
                                 int remoteHandle, int tag);
  int ReceiveMultiBlockDataSet(
    vtkMultiBlockDataSet* data, int remoteHandle, int tag);

  int MaximumNumberOfProcesses;
  int NumberOfProcesses;

  int LocalProcessId;

  static int UseCopy;

  vtkIdType Count;

private:
  vtkCommunicator(const vtkCommunicator&);  // Not implemented.
  void operator=(const vtkCommunicator&);  // Not implemented.
};

#endif // vtkCommunicator_h
// VTK-HeaderTest-Exclude: vtkCommunicator.h
