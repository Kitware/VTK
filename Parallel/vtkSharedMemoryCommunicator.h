/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSharedMemoryCommunicator.h
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

  vtkTypeMacro( vtkSharedMemoryCommunicator,vtkObject);
  
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
  virtual int Send(vtkIdType* data, int length, int remoteThreadId, int tag);
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
  virtual int Receive(vtkIdType* data, int length, int remoteThreadId, 
		      int tag);
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
  vtkSharedMemoryCommunicator(const vtkSharedMemoryCommunicator&);
  void operator=(const vtkSharedMemoryCommunicator&);

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

};

#endif //  __vtkSharedMemoryCommunicator_h
