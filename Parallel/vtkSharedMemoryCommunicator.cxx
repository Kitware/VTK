/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSharedMemoryCommunicator.cxx
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

#include "vtkSharedMemoryCommunicator.h"
#include "vtkDataObject.h"
#include "vtkObjectFactory.h"

class vtkSharedMemoryCommunicatorMessage
{
public:
  vtkDataObject *Object;
  vtkDataArray  *Array;
  void          *Data;
  int            DataLength;
  int            Tag;
  int            SendId;
  vtkSharedMemoryCommunicatorMessage* Next;
  vtkSharedMemoryCommunicatorMessage* Previous;
};

vtkSharedMemoryCommunicator* vtkSharedMemoryCommunicator::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkSharedMemoryCommunicator");
  if(ret)
    {
    return (vtkSharedMemoryCommunicator*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkSharedMemoryCommunicator;
}


void vtkSharedMemoryCommunicator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkCommunicator::PrintSelf(os,indent);
  os << indent << "Number of threads: " << this->NumberOfThreads << endl;
  os << indent << "Initialized: " << (this->Initialized ? "(yes)" : "(no)")
     << endl;
  os << indent << "Local thread id: " << this->LocalThreadId << endl;
  os << indent << "Waiting for id: " << this->WaitingForId << endl;
  os << indent << "Force deep copy: " << (this->ForceDeepCopy ? "(yes)" : "(no)")
     << endl;
  os << indent << "Message list lock" << this->MessageListLock << endl;
  os << indent << "Communicators: " << this->Communicators << endl;
  os << indent << "Parent: " << this->Parent << endl;
  os << indent << "Message list start: " << this->MessageListStart << endl;
  os << indent << "Message list end: " << this->MessageListEnd << endl;
  
}

vtkSharedMemoryCommunicator::vtkSharedMemoryCommunicator()
{
  this->NumberOfThreads = 0;
  this->Initialized = 0;
  this->Communicators = 0;
  this->Parent = 0;

  this->LocalThreadId = 0;
  this->WaitingForId = vtkMultiProcessController::INVALID_SOURCE;
  this->ForceDeepCopy = 1;

  this->MessageListLock = new vtkSimpleCriticalSection;
  
#ifdef _WIN32
  this->MessageSignal = CreateEvent(0, FALSE, FALSE, 0);
#else
  this->Gate = new vtkSimpleCriticalSection;
  this->Gate->Lock();
#endif

  this->MessageListStart = 0;
  this->MessageListEnd = 0;
}

vtkSharedMemoryCommunicator::~vtkSharedMemoryCommunicator()
{
  // Note the communicators are not deleted because ThreadedControllers
  // delete them when they are destroyed
  delete this->MessageListLock;
#ifdef _WIN32
  CloseHandle(this->MessageSignal);
#else
  delete this->Gate;
#endif
}

void vtkSharedMemoryCommunicator::Initialize(int nThreads, int forceDeepCopy)
{
  // Can only be initialized once.
  if (this->Initialized)
    {
    return;
    }
  
  // This should never happen. The controller should not call
  // Initialize() if 
  // nThreads >= vtkMultiProcessController::MAX_PROCESSES
  if (nThreads >= vtkMultiProcessController::MAX_PROCESSES)
    {
    return;
    }

  this->NumberOfThreads = nThreads;
  this->Communicators = new vtkSharedMemoryCommunicator*[nThreads];
  this->Communicators[0] = this;
  this->Parent = this;
  this->ForceDeepCopy = forceDeepCopy;

  // Create nThreads communicators. One for each thread.
  for (int i = 1; i < this->NumberOfThreads; ++i)
    {
    this->Communicators[i] = vtkSharedMemoryCommunicator::New();
    this->Communicators[i]->Initialize(1, forceDeepCopy);
    this->Communicators[i]->LocalThreadId = i;
    this->Communicators[i]->Parent = this;
    }

  this->Initialized = 1;
  this->Modified();
}  


int vtkSharedMemoryCommunicator::Send(vtkDataObject* object, 
				      void *data, int dataLength,
				      int remoteThreadId, int tag)
{
  vtkSharedMemoryCommunicatorMessage* message;
  vtkSharedMemoryCommunicator* receiveCommunicator;
  receiveCommunicator = this->Parent->Communicators[remoteThreadId];

  // >>>>>>>>>> Lock >>>>>>>>>>
  receiveCommunicator->MessageListLock->Lock();
  // Create and copy the message.
  message = receiveCommunicator->NewMessage(object, data, dataLength);
  message->SendId = this->LocalThreadId;
  message->Tag = tag;
  
  receiveCommunicator->AddMessage(message);

  // Check to see if the other process is blocked waiting for this message.
  if (receiveCommunicator->WaitingForId == this->LocalThreadId ||
      receiveCommunicator->WaitingForId == 
      vtkMultiProcessController::ANY_SOURCE)
    {
    
    // Do this here before the MessageList is unlocked (avoids a race condition).
    receiveCommunicator->WaitingForId = 
      vtkMultiProcessController::INVALID_SOURCE;
    // Tell the receiving thread that there is a new message.
    this->SignalNewMessage(receiveCommunicator);
    }

  receiveCommunicator->MessageListLock->Unlock();
  // <<<<<<<<< Unlock <<<<<<<<<<

  return 1;
}

int vtkSharedMemoryCommunicator::Send(vtkDataArray* object, 
				      int dataLength,
				      int remoteThreadId, int tag)
{
  vtkSharedMemoryCommunicatorMessage* message;
  vtkSharedMemoryCommunicator* receiveCommunicator;
  receiveCommunicator = this->Parent->Communicators[remoteThreadId];

  // >>>>>>>>>> Lock >>>>>>>>>>
  receiveCommunicator->MessageListLock->Lock();
  // Create and copy the message.
  message = receiveCommunicator->NewMessage(object, NULL, dataLength);
  message->SendId = this->LocalThreadId;
  message->Tag = tag;
  
  receiveCommunicator->AddMessage(message);

  // Check to see if the other process is blocked waiting for this message.
  if (receiveCommunicator->WaitingForId == this->LocalThreadId ||
      receiveCommunicator->WaitingForId == 
      vtkMultiProcessController::ANY_SOURCE)
    {
    
    // Do this here before the MessageList is unlocked (avoids a race condition).
    receiveCommunicator->WaitingForId = 
      vtkMultiProcessController::INVALID_SOURCE;
    // Tell the receiving thread that there is a new message.
    this->SignalNewMessage(receiveCommunicator);
    }

  receiveCommunicator->MessageListLock->Unlock();
  // <<<<<<<<< Unlock <<<<<<<<<<

  return 1;
}


int vtkSharedMemoryCommunicator::Receive(vtkDataObject* object, 
					 void *data, int dataLength,
					 int remoteThreadId, int tag)
{
  vtkSharedMemoryCommunicatorMessage* message;

  // >>>>>>>>>> Lock >>>>>>>>>>
  this->MessageListLock->Lock();
  
  // Look for the message (has it arrived before me?).
  message = this->FindMessage(remoteThreadId, tag);
  while (message == NULL)
    {
    
    this->WaitingForId = remoteThreadId;
    // Temporarily unlock the mutex until we receive the message.
    this->MessageListLock->Unlock();
    // Block until the message arrives.
    this->WaitForNewMessage();
    // Now lock the mutex again.  The message should be here.
    this->MessageListLock->Lock();
    message = this->FindMessage(remoteThreadId, tag);
    if (message == NULL)
      {
      vtkErrorMacro("I passed through the gate, but there is no message.");
      }
    }

  // Copy the message to the receive data/object.
  if (object && message->Object)
    {
    // The object was already copied into the message.
    // We can shallow copy here even if deep copy was set.
    ((vtkDataObject *)object)->ShallowCopy(message->Object);
    }
  if (data != NULL && message->Data != NULL && dataLength > 0)
    {
    if (dataLength != message->DataLength)
      {
      vtkErrorMacro("Receive message length does not match send.");
      }
    memcpy(data, message->Data, dataLength);
    }


  // Delete the message.
  this->DeleteMessage(message);

  this->MessageListLock->Unlock();
  // <<<<<<<<< Unlock <<<<<<<<<

  return 1;
}

int vtkSharedMemoryCommunicator::Receive(vtkDataArray* object, 
					 int dataLength,
					 int remoteThreadId, int tag)
{
  vtkSharedMemoryCommunicatorMessage* message;

  // >>>>>>>>>> Lock >>>>>>>>>>
  this->MessageListLock->Lock();
  
  // Look for the message (has it arrived before me?).
  message = this->FindMessage(remoteThreadId, tag);
  while (message == NULL)
    {
    
    this->WaitingForId = remoteThreadId;
    // Temporarily unlock the mutex until we receive the message.
    this->MessageListLock->Unlock();
    // Block until the message arrives.
    this->WaitForNewMessage();
    // Now lock the mutex again.  The message should be here.
    this->MessageListLock->Lock();
    message = this->FindMessage(remoteThreadId, tag);
    if (message == NULL)
      {
      vtkErrorMacro("I passed through the gate, but there is no message.");
      }
    }

  // Copy the message to the receive data/object.
  if (object && message->Array)
    {
    // The array was already copied into the message.
    // We can shallow copy here even if deep copy was set.
    ((vtkDataArray *)object)->DeepCopy(message->Array);
    }


  // Delete the message.
  this->DeleteMessage(message);

  this->MessageListLock->Unlock();
  // <<<<<<<<< Unlock <<<<<<<<<

  return 1;
}

vtkSharedMemoryCommunicatorMessage 
*vtkSharedMemoryCommunicator::NewMessage(vtkDataObject* object, 
					 void* data, int dataLength)
{
  vtkSharedMemoryCommunicatorMessage *message = 
    new vtkSharedMemoryCommunicatorMessage;

  message->Next = message->Previous = NULL;
  message->Tag = 0;
  message->Object = NULL;
  message->Data = NULL;
  message->DataLength = 0;
  message->Array = 0;

  if (object)
    {
    message->Object = object->MakeObject();
    if (this->ForceDeepCopy)
      {
      message->Object->DeepCopy(object);
      }
    else
      {
      message->Object->ShallowCopy(object);
      }
    }
  if (data && dataLength > 0)
    {
    message->Data = (void *)(new unsigned char[dataLength]);
    message->DataLength = dataLength;
    memcpy(message->Data, data, dataLength);
    }

  return message;
}

vtkSharedMemoryCommunicatorMessage 
*vtkSharedMemoryCommunicator::NewMessage(vtkDataArray* object, 
					 void* data, int dataLength)
{
  vtkSharedMemoryCommunicatorMessage *message = 
    new vtkSharedMemoryCommunicatorMessage;

  message->Next = message->Previous = NULL;
  message->Tag = 0;
  message->Object = NULL;
  message->Array = NULL;
  message->Data = NULL;
  message->DataLength = 0;

  if (object)
    {
    message->Array = object->MakeObject();
    if (this->ForceDeepCopy)
      {
      message->Array->DeepCopy(object);
      }
    else
      {
      message->Array->DeepCopy(object);
      }
    }
  if (data && dataLength > 0)
    {
    message->Data = (void *)(new unsigned char[dataLength]);
    message->DataLength = dataLength;
    memcpy(message->Data, data, dataLength);
    }

  return message;
}

//----------------------------------------------------------------------------
// This method assumes that the message list mutex is handled externally.
vtkSharedMemoryCommunicatorMessage *vtkSharedMemoryCommunicator::FindMessage(
  int sendId, int tag)
{
  vtkSharedMemoryCommunicatorMessage *message;
  
  message = this->MessageListStart;
  while (message != NULL)
    {
    if ((sendId == vtkMultiProcessController::ANY_SOURCE 
	 || message->SendId == sendId) &&
         message->Tag == tag)
      { // We have found a message that matches.
      // Remove the message from the list.

      if (message->Next)
        {
        message->Next->Previous = message->Previous;
        }
      if (message->Previous)
        {
        message->Previous->Next = message->Next;
        }
      // Special Case: first in the list.
      if (message == this->MessageListStart)
        {
        this->MessageListStart = message->Next;
        }
      // Special Case: last in list.
      if (message == this->MessageListEnd)
        {
        this->MessageListEnd = message->Previous;
        }
      
      // Return the message.
      message->Next = message->Previous = NULL;

      return message;
      }
    message = message->Next;
    }
  return NULL;
}

void vtkSharedMemoryCommunicator::DeleteMessage(
  vtkSharedMemoryCommunicatorMessage *message)
{
  if (message->Object)
    {
    message->Object->Delete();
    message->Object = NULL;
    }

  if (message->Array)
    {
    message->Array->Delete();
    message->Array = NULL;
    }

  if (message->Data)
    {
    delete [] (unsigned char*)message->Data;
    message->Data = NULL;
    message->DataLength = 0;
    }

  delete message;
}

void vtkSharedMemoryCommunicator::AddMessage(
  vtkSharedMemoryCommunicatorMessage *message)
{
  // Special case: Empty list.
  if (this->MessageListEnd == NULL)
    {
    // sanity check
    if (this->MessageListStart)
      {
      vtkErrorMacro("List inconsistancy");
      }
    this->MessageListEnd = this->MessageListStart = message;
    message->Next = message->Previous = NULL;
    return;
    }
  
  message->Next = NULL;
  message->Previous = this->MessageListEnd;
  this->MessageListEnd->Next = message;
  this->MessageListEnd = message;
}

//----------------------------------------------------------------------------
int vtkSharedMemoryCommunicator::Send(int* data, int length, 
				      int remoteThreadId, int tag)
{
  length = length * sizeof(int);
  return this->Send(NULL, (void*)data, length, remoteThreadId, tag);
}

//----------------------------------------------------------------------------
int vtkSharedMemoryCommunicator::Send(unsigned long* data, int length, 
				      int remoteThreadId, int tag)
{
  length = length * sizeof(unsigned long);
  return this->Send(NULL, (void*)data, length, remoteThreadId, tag);
}

//----------------------------------------------------------------------------
int vtkSharedMemoryCommunicator::Send(char* data, int length, 
				      int remoteThreadId, int tag)
{
  length = length * sizeof(char);
  return this->Send(NULL, (void*)data, length, remoteThreadId, tag);
}

//----------------------------------------------------------------------------
int vtkSharedMemoryCommunicator::Send(unsigned char* data, int length, 
				      int remoteThreadId, int tag)
{
  length = length * sizeof(unsigned char);
  return this->Send(NULL, (void*)data, length, remoteThreadId, tag);
}

//----------------------------------------------------------------------------
int vtkSharedMemoryCommunicator::Send(float* data, int length, 
				      int remoteThreadId, int tag)
{
  length = length * sizeof(float);
  return this->Send(NULL, (void*)data, length, remoteThreadId, tag);
}

//----------------------------------------------------------------------------
int vtkSharedMemoryCommunicator::Send(double* data, int length, 
				      int remoteThreadId, int tag)
{
  length = length * sizeof(double);
  return this->Send(NULL, (void*)data, length, remoteThreadId, tag);
}

//----------------------------------------------------------------------------
int vtkSharedMemoryCommunicator::Send(vtkIdType* data, int length, 
				      int remoteThreadId, int tag)
{
  length = length * sizeof(vtkIdType);
  return this->Send(NULL, (void*)data, length, remoteThreadId, tag);
}

//----------------------------------------------------------------------------
int vtkSharedMemoryCommunicator::Receive(int* data, int length, 
					 int remoteThreadId, int tag)
{
  length = length * sizeof(int);
  return this->Receive(NULL, (void*)data, length, remoteThreadId, tag);
}

//----------------------------------------------------------------------------
int vtkSharedMemoryCommunicator::Receive(unsigned long* data, 
					 int length, int remoteThreadId, 
					 int tag)
{
  length = length * sizeof(unsigned long);
  return this->Receive(NULL, (void*)data, length, remoteThreadId, tag);
}

//----------------------------------------------------------------------------
int vtkSharedMemoryCommunicator::Receive(char* data, int length, 
					 int remoteThreadId, int tag)
{
  length = length * sizeof(char);
  return this->Receive(NULL, (void*)data, length, remoteThreadId, tag);
}

//----------------------------------------------------------------------------
int vtkSharedMemoryCommunicator::Receive(unsigned char* data, int length, 
					 int remoteThreadId, int tag)
{
  length = length * sizeof(unsigned char);
  return this->Receive(NULL, (void*)data, length, remoteThreadId, tag);
}

//----------------------------------------------------------------------------
int vtkSharedMemoryCommunicator::Receive(float* data, int length, 
					 int remoteThreadId, int tag)
{
  length = length * sizeof(float);
  return this->Receive(NULL, (void*)data, length, remoteThreadId, tag);
}

//----------------------------------------------------------------------------
int vtkSharedMemoryCommunicator::Receive(double* data, int length, 
					 int remoteThreadId, int tag)
{
  length = length * sizeof(double);
  return this->Receive(NULL, (void*)data, length, remoteThreadId, tag);
}

//----------------------------------------------------------------------------
int vtkSharedMemoryCommunicator::Receive(vtkIdType* data, int length, 
					 int remoteThreadId, int tag)
{
  length = length * sizeof(vtkIdType);
  return this->Receive(NULL, (void*)data, length, remoteThreadId, tag);
}

//----------------------------------------------------------------------------
int vtkSharedMemoryCommunicator::Send(vtkDataObject* data, 
				      int remoteThreadId, int tag)
{ 
  return this->Send(data, NULL, 0, remoteThreadId, tag);
}

//----------------------------------------------------------------------------
int vtkSharedMemoryCommunicator::Receive(vtkDataObject* data, 
					 int remoteThreadId, int tag)
{
  return this->Receive(data, NULL, 0, remoteThreadId, tag);
}

//----------------------------------------------------------------------------
int vtkSharedMemoryCommunicator::Send(vtkDataArray* data, 
				      int remoteThreadId, int tag)
{ 
  return this->Send(data, 0, remoteThreadId, tag);
}

//----------------------------------------------------------------------------
int vtkSharedMemoryCommunicator::Receive(vtkDataArray* data, 
					 int remoteThreadId, int tag)
{
  return this->Receive(data, 0, remoteThreadId, tag);
}
