/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSharedMemoryCommunicator.h
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
// .NAME vtkSharedMemoryCommunicator - Provides communication using shared memory.

// .SECTION Description
// This class is used together with vtkThreadedController for communication
// between threads. Once initialized, it creates one communicator
// per thread. The messages to be sent are copied to the message list
// of the appropriate communicator by the sending thread and then
// read by the receiving thread. Mutexes are used to ensure safe
// access to the data structures. By default, when an object is sent,
// it is copied with DeepCopy. This behavior can be changed by un-setting
// ForceDeepCopy.

// .SECTION See Also
// vtkCommunicator vtkThreadedController

#ifndef __vtkSharedMemoryCommunicator_h
#define __vtkSharedMemoryCommunicator_h

#include "vtkCommunicator.h"
#include "vtkMultiProcessController.h"
#include "vtkCriticalSection.h"

class vtkThreadedController;
class vtkSharedMemoryCommunicatorMessage;

class VTK_PARALLEL_EXPORT vtkSharedMemoryCommunicator : public vtkCommunicator
{

public:

  vtkTypeRevisionMacro( vtkSharedMemoryCommunicator,vtkObject);
  
  // Description:
  // Creates an empty communicator.
  static vtkSharedMemoryCommunicator* New();

  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method sends data to another process.  Tag eliminates ambiguity
  // when multiple sends or receives exist in the same process.
  virtual int Send(int* data, int length, int remoteThreadId, int tag);
  virtual int Send(unsigned long* data, int length, int remoteThreadId, 
                   int tag);
  virtual int Send(char* data, int length, int remoteThreadId, int tag);
  virtual int Send(unsigned char* data, int length, int remoteThreadId, int tag);
  virtual int Send(float* data, int length, int remoteThreadId, int tag);
  virtual int Send(double* data, int length, int remoteThreadId, int tag);
#ifdef VTK_USE_64BIT_IDS
  virtual int Send(vtkIdType* data, int length, int remoteThreadId, int tag);
#endif
  virtual int Send(vtkDataObject* data, int remoteThreadId, int tag);
  virtual int Send(vtkDataArray* data, int remoteThreadId, int tag);

  // Description:
  // This method receives data from a corresponding send. It blocks
  // until the receive is finished.  It calls methods in "data"
  // to communicate the sending data.
  virtual int Receive(int* data, int length, int remoteThreadId, 
                      int tag);
  virtual int Receive(unsigned long* data, int length, 
                      int remoteThreadId, int tag);
  virtual int Receive(char* data, int length, int remoteThreadId, 
                      int tag);
  virtual int Receive(unsigned char* data, int length, int remoteThreadId, 
                      int tag);
  virtual int Receive(float* data, int length, int remoteThreadId, 
                      int tag);
  virtual int Receive(double* data, int length, int remoteThreadId, 
                      int tag);
#ifdef VTK_USE_64BIT_IDS
  virtual int Receive(vtkIdType* data, int length, int remoteThreadId, 
                      int tag);
#endif
  virtual int Receive(vtkDataObject *data, int remoteThreadId, int tag);
  virtual int Receive(vtkDataArray *data, int remoteThreadId, int tag);

//BTX

  friend class vtkThreadedController;

//ETX

protected:

  int NumberOfThreads;
  int Initialized;
  void Initialize(int nThreads, int forceDeepCopy);

  int LocalThreadId;
  int WaitingForId;

  int ForceDeepCopy;

  // It is not enough to block on the messages, we have to mutex 
  // the whole send interaction.  I was trying to avoid a central 
  // mutex (oh well).
  vtkSimpleCriticalSection* MessageListLock;


  // Each thread has its own communicator.
  vtkSharedMemoryCommunicator** Communicators;

  vtkSharedMemoryCommunicator* Parent;
  
  // Double linked list.
  vtkSharedMemoryCommunicatorMessage *MessageListStart;
  vtkSharedMemoryCommunicatorMessage *MessageListEnd;

  vtkSharedMemoryCommunicator();
  ~vtkSharedMemoryCommunicator();

  // The generic send and receive methods.
  int Send(vtkDataObject* object, void *data, int dataLength, 
           int remoteThreadId, int tag);
  int Receive(vtkDataObject* object, void *data, int dataLength, 
              int remoteThreadId, int tag);

  int Send(vtkDataArray* object, int dataLength, 
           int remoteThreadId, int tag);
  int Receive(vtkDataArray* object, int dataLength, 
              int remoteThreadId, int tag);

  vtkSharedMemoryCommunicatorMessage* NewMessage(vtkDataObject* object,
                                                 void* data, 
                                                 int dataLength);
  vtkSharedMemoryCommunicatorMessage* NewMessage(vtkDataArray* object,
                                                 void* data, 
                                                 int dataLength);
  void DeleteMessage(vtkSharedMemoryCommunicatorMessage *message);
  void AddMessage(vtkSharedMemoryCommunicatorMessage *message);
  vtkSharedMemoryCommunicatorMessage* FindMessage(int sendId, int tag);

#ifdef _WIN32
  // Event signaling the arrival of a new message.
  // Windows implementation only.
  HANDLE MessageSignal;
#else
  // This mutex is normally locked.  It is used to block the execution 
  // of the receiving process when the send has not been called yet.
  vtkSimpleCriticalSection* Gate;
#endif

  void SignalNewMessage(vtkSharedMemoryCommunicator* receiveCommunicator)
    {
#ifdef _WIN32
    SetEvent( receiveCommunicator->MessageSignal );
#else
    receiveCommunicator->Gate->Unlock();
#endif
    }

  void WaitForNewMessage()
    {
#ifdef _WIN32
      WaitForSingleObject( this->MessageSignal, INFINITE );
#else
      this->Gate->Lock();
#endif
    }

private:
  vtkSharedMemoryCommunicator(const vtkSharedMemoryCommunicator&);  // Not implemented.
  void operator=(const vtkSharedMemoryCommunicator&);  // Not implemented.
};

#endif //  __vtkSharedMemoryCommunicator_h
