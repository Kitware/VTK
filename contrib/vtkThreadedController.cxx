/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkThreadedController.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkThreadedController.h"
#include "vtkObjectFactory.h"

#include "vtkDataSet.h"
#include "vtkImageData.h"

#ifdef VTK_USE_SPROC
#include <sys/prctl.h>
#endif


//----------------------------------------------------------------------------
vtkThreadedController* vtkThreadedController::New()
{
  // First try to create the object from the vtkObjectactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkThreadedController");
  if(ret)
    {
    return (vtkThreadedController*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkThreadedController;
}

class vtkThreadedControllerMessage
{
public:
  vtkDataObject *Object;
  void          *Data;
  int            DataLength;
  int            Tag;
  int            SendId;
  vtkThreadedControllerMessage *Next;
  vtkThreadedControllerMessage *Previous;
};


//----------------------------------------------------------------------------
vtkThreadedController::vtkThreadedController()
{
  // This may no longer be neede now that superclass sets 
  // GlobalDefaultNumberOfThreads.
  vtkMultiThreader::SetGlobalMaximumNumberOfThreads(0);
  
  this->LocalProcessId = 0;
  this->WaitingForId = VTK_MP_CONTROLLER_INVALID_SOURCE;

  this->MultiThreader = vtkMultiThreader::New();
  this->MultipleMethodFlag = 0;
    
  // Here for debugging intermitent problems
  this->LogFile = NULL;
  //this->LogFile = fopen("ThreadedController.log", "w");
  
  this->MessageListLock = vtkMutexLock::New();
  this->MessageListStart = NULL;
  this->MessageListEnd = NULL;

  this->Gate = vtkMutexLock::New();
  this->Gate->Lock();
}

//----------------------------------------------------------------------------
vtkThreadedController::~vtkThreadedController()
{
  this->MultiThreader->Delete();
  this->MultiThreader = NULL;
  if (this->LogFile)
    {
    fclose(this->LogFile);
    }
  this->MessageListLock->Delete();
}

//----------------------------------------------------------------------------
void vtkThreadedController::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMultiProcessController::PrintSelf(os,indent);
  os << indent << "MultiThreader:\n";
  this->MultiThreader->PrintSelf(os, indent.GetNextIndent());
  os << indent << "LocalProcessId: " << this->LocalProcessId << endl;
}

//----------------------------------------------------------------------------
void vtkThreadedController::Initialize(int vtkNotUsed(argc), char *argv[])
{
  this->Modified();
  
  argv = argv;
  this->NumberOfProcesses = this->MultiThreader->GetNumberOfThreads();
}
  

//----------------------------------------------------------------------------
// Called before threads are spawned to create the "process objecs".
void vtkThreadedController::CreateProcessControllers()
{
  int i, j;

  // Create the controllers.
  // The original controller will be assigned thread 0.
  this->Controllers[0] = this;
  this->LocalProcessId = 0;
  for (i = 1; i < this->NumberOfProcesses; ++i)
    {
    this->Controllers[i] = vtkThreadedController::New();
    this->Controllers[i]->LocalProcessId = i;
    this->Controllers[i]->NumberOfProcesses = this->NumberOfProcesses;
    }

  // Copy the array of controllers into each controller.
  for (i = 1; i < this->NumberOfProcesses; ++i)
    {
    for (j = 0; j < this->NumberOfProcesses; ++j)
      {
      this->Controllers[i]->Controllers[j] = this->Controllers[j];
      }
    }
}




  
  



//----------------------------------------------------------------------------
VTK_THREAD_RETURN_TYPE vtkThreadedControllerStart( void *arg )
{
  ThreadInfoStruct *info = (ThreadInfoStruct*)(arg);
  int threadId = info->ThreadID;
  vtkThreadedController *controller0 =(vtkThreadedController*)(info->UserData);

  controller0->Start(threadId);
  return VTK_THREAD_RETURN_VALUE;
}

//----------------------------------------------------------------------------
// We are going to try something new.  We will pass the local controller
// as the argument.
void vtkThreadedController::Start(int threadId)
{
  vtkThreadedController *localController = this->Controllers[threadId];

    // Store threadId in a table.
#ifdef VTK_USE_PTHREADS  
  this->ThreadIds[threadId] = pthread_self();
#endif
#ifdef VTK_USE_SPROC
  this->ThreadIds[threadId] = PRDA->sys_prda.prda_sys.t_pid;
#endif
  
  if (this->MultipleMethodFlag)
    {
    if (this->MultipleMethod[threadId])
      {
      (this->MultipleMethod[threadId])(localController, 
				       this->MultipleData[threadId]);
      }
    else
      {
      vtkErrorMacro("MultipleMethod " << threadId << " not set");
      }
    }
  else
    {
    if (this->SingleMethod)
      {
      (this->SingleMethod)(localController, this->SingleData);
      }
    else
      {
      vtkErrorMacro("SingleMethod not set");
      } 
    }
}

//----------------------------------------------------------------------------
// Execute the method set as the SingleMethod on NumberOfThreads threads.
void vtkThreadedController::SingleMethodExecute()
{
  this->CreateProcessControllers();
  this->MultipleMethodFlag = 0;
  this->MultiThreader->SetSingleMethod(vtkThreadedControllerStart, 
				       (void*)this);
  this->MultiThreader->SetNumberOfThreads(this->NumberOfProcesses);

  // GLOBAL_CONTROLLER will be from thread0 always.
  // GetLocalController will translate to the local controller.
  this->SetGlobalController(this);
  
  this->MultiThreader->SingleMethodExecute();
}
//----------------------------------------------------------------------------
// Execute the methods set as the MultipleMethods.
void vtkThreadedController::MultipleMethodExecute()
{
  this->CreateProcessControllers();
  this->MultipleMethodFlag = 1;

  this->MultiThreader->SetSingleMethod(vtkThreadedControllerStart, 
				       (void*)this);
  this->MultiThreader->SetNumberOfThreads(this->NumberOfProcesses);

  // GLOBAL_CONTROLLER will be from thread0 always.
  // GetLocalController will translate to the local controller.
  this->SetGlobalController(this);

  this->MultiThreader->SingleMethodExecute();
}

  


//----------------------------------------------------------------------------
int vtkThreadedController::Send(vtkDataObject *object, 
                                 void *data, int dataLength,
                                 int receiveId, int tag)
{
  vtkThreadedControllerMessage *message;
  vtkThreadedController *receiveController;
  receiveController = this->Controllers[receiveId];

  // >>>>>>>>>> Lock >>>>>>>>>>
  receiveController->MessageListLock->Lock();
  // Create and copy the message.
  message = receiveController->NewMessage(object, data, dataLength);
  message->SendId = this->LocalProcessId;
  message->Tag = tag;
  
  //cerr << this->LocalProcessId << ": Send to " << receiveId << " : message = " << message 
  //     << ", object = " << message->Object << ", data = " << message->Data 
  //     << ", tag = " << message->Tag << ", sendId = " << message->SendId << endl;
  
  receiveController->AddMessage(message);

  // Check to see if the other process is blocked waiting for this message.
  if (receiveController->WaitingForId == this->LocalProcessId ||
      receiveController->WaitingForId == VTK_MP_CONTROLLER_ANY_SOURCE)
    {
    //cerr << this->LocalProcessId << ": receive process " << receiveId << " is already waiting\n";
    
    // Do this here before the MessageList is unlocked (avoids a race condition).
    receiveController->WaitingForId = VTK_MP_CONTROLLER_INVALID_SOURCE;
    receiveController->Gate->Unlock();
    }

  receiveController->MessageListLock->Unlock();
  // <<<<<<<<< Unlock <<<<<<<<<<

  return 1;
}



//----------------------------------------------------------------------------
int vtkThreadedController::Receive(vtkDataObject *object, 
                                   void *data, int dataLength,
                                   int remoteId, int tag)
{
  vtkThreadedControllerMessage *message;

  // >>>>>>>>>> Lock >>>>>>>>>>
  this->MessageListLock->Lock();
  
  // Look for the message (has it arrived before me?).
  message = this->FindMessage(remoteId, tag);
  while (message == NULL)
    {
    //cerr << this->LocalProcessId << ": message not sent yet.  tag = " << tag << endl;
    
    this->WaitingForId = remoteId;
    // Temporarily unlock the mutex until we receive the message.
    this->MessageListLock->Unlock();
    // Block until the message arrives.
    this->Gate->Lock();
    // Now lock the mutex again.  The message should be here.
    this->MessageListLock->Lock();
    message = this->FindMessage(remoteId, tag);
    if (message == NULL)
      {
      vtkErrorMacro("I passed through the gate, but there is no message.");
      }
    }

  //cerr << this->LocalProcessId << ": receive from " << remoteId << " : message = " << message 
  //     << ", object = " << message->Object << ", data = " << message->Data 
  //     << ", tag = " << message->Tag << ", sendId = " << message->SendId << endl;
  
  // Copy the message to the reveive data/object.
  if (object && message->Object)
    {
    // The object was already copied into the message.
    // We can shallow copy here even if deep copy was set.
    object->ShallowCopy(message->Object);
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

//----------------------------------------------------------------------------
// This method assumes that the message list mutex is handled externally.
vtkThreadedControllerMessage *vtkThreadedController::FindMessage(int sendId, 
                                                                 int tag)
{
  vtkThreadedControllerMessage *message;
  
  message = this->MessageListStart;
  while (message != NULL)
    {
    if ((sendId == VTK_MP_CONTROLLER_ANY_SOURCE || message->SendId == sendId) &&
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


//----------------------------------------------------------------------------
// The new and delete methods could reuse messages and maybe memory to avoid
// allocating and deleting memory each send.
vtkThreadedControllerMessage *vtkThreadedController::NewMessage(
                          vtkDataObject *object, void *data, int dataLength)
{
  vtkThreadedControllerMessage *message = new vtkThreadedControllerMessage;

  message->Next = message->Previous = NULL;
  message->Tag = 0;
  message->Object = NULL;
  message->Data = NULL;
  message->DataLength = 0;

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



//----------------------------------------------------------------------------
void vtkThreadedController::DeleteMessage(vtkThreadedControllerMessage *message)
{
  if (message->Object)
    {
    message->Object->Delete();
    message->Object = NULL;
    }

  if (message->Data)
    {
    delete [] (unsigned char*)message->Data;
    message->Data = NULL;
    message->DataLength = 0;
    }

  delete message;
}

//----------------------------------------------------------------------------
// Add the message to the end.
void vtkThreadedController::AddMessage(vtkThreadedControllerMessage *message)
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
int vtkThreadedController::Send(int *data, int length, int remoteProcessId, 
				int tag)
{
  length = length * sizeof(int);
  return this->Send(NULL, (void*)data, length, remoteProcessId, tag);
}

//----------------------------------------------------------------------------
int vtkThreadedController::Send(unsigned long *data, int length, 
				int remoteProcessId, int tag)
{
  length = length * sizeof(unsigned long);
  return this->Send(NULL, (void*)data, length, remoteProcessId, tag);
}

//----------------------------------------------------------------------------
int vtkThreadedController::Send(char *data, int length, int remoteProcessId, 
				int tag)
{
  length = length * sizeof(char);
  return this->Send(NULL, (void*)data, length, remoteProcessId, tag);
}

//----------------------------------------------------------------------------
int vtkThreadedController::Send(float *data, int length, int remoteProcessId, 
				int tag)
{
  length = length * sizeof(float);
  return this->Send(NULL, (void*)data, length, remoteProcessId, tag);
}




//----------------------------------------------------------------------------
int vtkThreadedController::Receive(int *data, int length, int remoteProcessId, 
				   int tag)
{
  length = length * sizeof(int);
  return this->Receive(NULL, (void*)data, length, remoteProcessId, tag);
}

//----------------------------------------------------------------------------
int vtkThreadedController::Receive(unsigned long *data, int length, 
				   int remoteProcessId, int tag)
{
  length = length * sizeof(unsigned long);
  return this->Receive(NULL, (void*)data, length, remoteProcessId, tag);
}

//----------------------------------------------------------------------------
int vtkThreadedController::Receive(char *data, int length, 
				   int remoteProcessId, int tag)
{
  length = length * sizeof(char);
  return this->Receive(NULL, (void*)data, length, remoteProcessId, tag);
}

//----------------------------------------------------------------------------
int vtkThreadedController::Receive(float *data, int length, 
				   int remoteProcessId, int tag)
{
  length = length * sizeof(float);
  return this->Receive(NULL, (void*)data, length, remoteProcessId, tag);
}



//----------------------------------------------------------------------------
int vtkThreadedController::Send(vtkDataObject *data, int remoteProcessId, 
				int tag)
{ 
  return this->Send(data, NULL, 0, remoteProcessId, tag);
}

//----------------------------------------------------------------------------
int vtkThreadedController::Receive(vtkDataObject *data, 
				   int remoteProcessId, int tag)
{
  return this->Receive(data, NULL, 0, remoteProcessId, tag);
}




//----------------------------------------------------------------------------
// Does not work for windows yet.
vtkMultiProcessController *vtkThreadedController::GetLocalController()
{
#ifdef VTK_USE_PTHREADS  
  int idx;
  pthread_t pid = pthread_self();
  for (idx = 0; idx < this->NumberOfProcesses; ++idx)
    {
    if (pthread_equal(pid, this->ThreadIds[idx]))
      {
      return this->Controllers[idx];
      }
    }
  
  vtkErrorMacro("Could Not Find my process id.");
  return NULL;
#elif defined VTK_USE_SPROC
  int idx;
  pid_t pid = PRDA->sys_prda.prda_sys.t_pid;
  for (idx = 0; idx < this->NumberOfProcesses; ++idx)
    {
    if (pid == this->ThreadIds[idx])
      {
      return this->Controllers[idx];
      }
    }
  
  vtkErrorMacro("Could Not Find my process id.");
  return NULL;
#else

  vtkErrorMacro("ThreadedController only works with pthreads or sproc");
  return NULL;
  
#endif  
}







