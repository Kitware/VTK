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
class vtkImageData;

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






// Since sends and receives block until the transation is complete,
// each thread can have at most one receive or send pending.
class vtkThreadedControllerProcessInfo
{
public:
  vtkThreadedControllerProcessInfo() 
    { this->BlockLock = vtkMutexLock::New();
      this->InfoLock = vtkMutexLock::New();
      this->Object = NULL;
      this->Data = NULL;
      this->DataLength = 0;
      this->Tag = 0;
      this->SendFlag = 0;
      this->RemoteId = VTK_MP_CONTROLLER_INVALID_SOURCE; };
  ~vtkThreadedControllerProcessInfo() 
    { this->BlockLock->Delete();
      this->BlockLock = NULL; 
      this->InfoLock->Delete();
      this->InfoLock = NULL; };

  // Mechanism for blocking this thread during a receive or send call.
  vtkMutexLock        *BlockLock;
  // To avoid changing this info in one thread while reading an another.
  vtkMutexLock        *InfoLock;
  // Messages can be sent by reference
  vtkObject           *Object;
  // Or by marshalling data
  void                *Data;
  int                 DataLength;
  // Tag and process id are the only things that differentiat messages.
  int                 Tag;
  int                 RemoteId;
  // This flag saves whether this info is for a send or a receive.
  int                 SendFlag;
};


//----------------------------------------------------------------------------
vtkThreadedController::vtkThreadedController()
{
  int idx;

  // This may no longer be neede now that superclass sets 
  // GlobalDefaultNumberOfThreads.
  vtkMultiThreader::SetGlobalMaximumNumberOfThreads(0);
  
  this->MultiThreader = vtkMultiThreader::New();
  this->MultipleMethodFlag = 0;
  
  for (idx = 0; idx < VTK_MP_CONTROLLER_MAX_PROCESSES; ++idx)
    {
    this->Processes[idx] = NULL;
    }  
  
  // Here for debugging intermitent problems
  this->LogFile = NULL;
  //this->LogFile = fopen("ThreadedController.log", "w");
  
  this->MessageLock = vtkMutexLock::New();
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
  this->MessageLock->Delete();
}

//----------------------------------------------------------------------------
void vtkThreadedController::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMultiProcessController::PrintSelf(os,indent);
  os << indent << "MultiThreader:\n";
  this->MultiThreader->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
void vtkThreadedController::Initialize(int vtkNotUsed(argc), char *argv[])
{
  this->Modified();
  
  argv = argv;
  this->NumberOfProcesses = this->MultiThreader->GetNumberOfThreads();
}
  

//----------------------------------------------------------------------------
// Called before threads are spawned to create the thread info.
void vtkThreadedController::CreateThreadInfoObjects()
{
  int idx;

  for (idx = 0; idx < this->NumberOfProcesses; ++idx)
    {
    this->Processes[idx] = new vtkThreadedControllerProcessInfo;
    // By default these blocking mutexes are locked.
    // I am assuming it is ok for one thread to lock, and another to unlock.
    if (this->LogFile)
      {
      fprintf(this->LogFile, "Block%d: Lock (contruction)\n", idx);
      fflush(this->LogFile);
      }
    this->Processes[idx]->BlockLock->Lock();
    }
}


//----------------------------------------------------------------------------
// Called before threads are spawned to create the thread info.
void vtkThreadedController::DeleteThreadInfoObjects()
{
  int idx;

  for (idx = 0; idx < this->NumberOfProcesses; ++idx)
    {
    // By default these blocking mutexes are locked.
    // I am assuming it is ok for one thread to lock, and another to unlock.
    if (this->LogFile)
      {
      fprintf(this->LogFile, "Block%d: Unlock (destruction)\n", idx);
      fflush(this->LogFile);
      }
    this->Processes[idx]->BlockLock->Unlock();
    delete this->Processes[idx];
    this->Processes[idx] = NULL;
    }
}


  
  



//----------------------------------------------------------------------------
VTK_THREAD_RETURN_TYPE vtkThreadedControllerStart( void *arg )
{
  ThreadInfoStruct *info = (ThreadInfoStruct*)(arg);
  int threadIdx = info->ThreadID;
  vtkThreadedController *self = (vtkThreadedController*)(info->UserData);

  self->Start(threadIdx);
  return VTK_THREAD_RETURN_VALUE;
}
void vtkThreadedController::Start(int threadIdx)
{
  
  // Store threadId in a table.
#ifdef VTK_USE_PTHREADS  
  this->ThreadIds[threadIdx] = pthread_self();
#endif
#ifdef VTK_USE_SPROC
  this->ThreadIds[threadIdx] = PRDA->sys_prda.prda_sys.t_pid;
#endif

  if (this->MultipleMethodFlag)
    {
    if (this->MultipleMethod[threadIdx])
      {
      (this->MultipleMethod[threadIdx])
	    ((void *)(this->MultipleData[threadIdx]));
      }
    else
      {
      vtkErrorMacro("MultipleMethod " << threadIdx << " not set");
      }
    }
  else
    {
    if (this->SingleMethod)
      {
      (this->SingleMethod)( this->SingleData );
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
  this->CreateThreadInfoObjects();
  this->MultipleMethodFlag = 0;
  this->MultiThreader->SetSingleMethod(vtkThreadedControllerStart, 
				       (void*)this);
  this->MultiThreader->SetNumberOfThreads(this->NumberOfProcesses);

  this->MultiThreader->SingleMethodExecute();
  this->DeleteThreadInfoObjects();
}
//----------------------------------------------------------------------------
// Execute the methods set as the MultipleMethods.
void vtkThreadedController::MultipleMethodExecute()
{
  this->CreateThreadInfoObjects();
  this->MultipleMethodFlag = 1;

  this->MultiThreader->SetSingleMethod(vtkThreadedControllerStart, 
				       (void*)this);
  this->MultiThreader->SetNumberOfThreads(this->NumberOfProcesses);

  this->MultiThreader->SingleMethodExecute();
  this->DeleteThreadInfoObjects();
}


//----------------------------------------------------------------------------
// I may need a mutex lock here. Although my own pid must have already been
// set,  once before me could be in a funky state.
int vtkThreadedController::GetLocalProcessId()
{
#ifdef VTK_USE_PTHREADS  
  int idx;
  pthread_t pid = pthread_self();
  for (idx = 0; idx < this->NumberOfProcesses; ++idx)
    {
    if (pthread_equal(pid, this->ThreadIds[idx]))
      {
      return idx;
      }
    }
  
  vtkErrorMacro("Could Not Find my process id.");
  return -1;
#elif defined VTK_USE_SPROC
  int idx;
  pid_t pid = PRDA->sys_prda.prda_sys.t_pid;
  for (idx = 0; idx < this->NumberOfProcesses; ++idx)
    {
    if (pid == this->ThreadIds[idx])
      {
      return idx;
      }
    }
  
  vtkErrorMacro("Could Not Find my process id.");
  return -1;
#else

  vtkErrorMacro("ThreadedController only works with pthreads or sproc");
  return -1;
  
#endif  
}

  
//----------------------------------------------------------------------------
// Handles message in object or in data string.
int vtkThreadedController::Send(vtkObject *object, void *data, int length, 
				int remoteProcessId, int tag)
{
  int myIdx = this->GetLocalProcessId();
  vtkThreadedControllerProcessInfo *snd;
  vtkThreadedControllerProcessInfo *rcv;

    if (this->LogFile)
      {
      fprintf(this->LogFile, "%d: Send: object = %d, data = %d, length = %d, remoteId = %d, tag = %d\n",
	      myIdx, object, data, length, remoteProcessId, tag);
      fflush(this->LogFile);
      }
    
  // Avoid a send and recv starting at the same time.
    if (this->LogFile)
      {
      fprintf(this->LogFile, "%d: Message: Lock (send %d->%d) T: %d\n",
	      myIdx, myIdx, remoteProcessId, tag);
      fflush(this->LogFile);
      }
    this->MessageLock->Lock();  
  
  // Try to get a lock on the recv.
  // Receive always gets blocked first to avoid deadlock
  rcv = this->Processes[remoteProcessId];
  if (this->LogFile)
    {
    fprintf(this->LogFile, "%d:     Info%d: Lock (Send %d->%d) T: %d\n", 
	    myIdx, remoteProcessId, myIdx, remoteProcessId, tag);
    fflush(this->LogFile);
    }
  rcv->InfoLock->Lock();
  
  // Look at pending receives to find any matches.
  if ((rcv->RemoteId == myIdx || rcv->RemoteId == VTK_MP_CONTROLLER_ANY_SOURCE)
      && rcv->SendFlag == 0 && rcv->Tag == tag && rcv->DataLength == length
      && !(rcv->Object) == !object)
    {
    // We have a match. A receive is already waiting.
    if (this->LogFile)
      {
      fprintf(this->LogFile, "%d:              send %d->%d, T: %d,  Match! \n",
	      myIdx, myIdx, remoteProcessId, tag);
      fflush(this->LogFile);    
      }
    // Copy the message.
    if (length > 0 && rcv->Data && data)
      {
      memcpy(rcv->Data, data, length);
      }
    if (rcv->Object && object)
      {
      this->CopyObject(object, rcv->Object);
      }
    
    // set "ProcessInfo" back to default values
    rcv->Object = NULL;
    rcv->Data = NULL;
    rcv->DataLength = 0;
    rcv->SendFlag = 0;
    rcv->Tag = 0;
    rcv->RemoteId = VTK_MP_CONTROLLER_INVALID_SOURCE;
    // Release the receive block, so it can return.
    if (this->LogFile)
      {
      fprintf(this->LogFile, "%d: Block%d: Unlock (send %d->%d) T: %d\n", 
	      myIdx, remoteProcessId, myIdx, remoteProcessId, tag);
      fflush(this->LogFile);
      }
    rcv->BlockLock->Unlock();
    
    // we are done writing/reading the rcv
    if (this->LogFile)
      {
      fprintf(this->LogFile, "%d:     Info%d: Unlock (Send %d->%d) T: %d\n", 
	      myIdx, remoteProcessId, myIdx, remoteProcessId, tag);
      fflush(this->LogFile);
      }
    rcv->InfoLock->Unlock();
    if (this->LogFile)
      {
      fprintf(this->LogFile, "%d: Message: Unlock (send %d->%d) T: %d\n",
	      myIdx, myIdx, remoteProcessId, tag);
      fflush(this->LogFile);
      }
    this->MessageLock->Unlock();
    
    return 1;
    }
  if (this->LogFile)
    {
    fprintf(this->LogFile, "%d:            send %d->%d, T: %d,  first=>wait\n",
	    myIdx, myIdx, remoteProcessId, tag);
    fflush(this->LogFile);
  
    fprintf(this->LogFile, "%d:     Info%d: Unlock (Send %d->%d) T: %d\n", 
	    myIdx, remoteProcessId, myIdx, remoteProcessId, tag);
    fflush(this->LogFile);
    }
  
  rcv->InfoLock->Unlock();
  
  // Matching receive has not been initiated.
  // Put message information in "ProcessInfo"
  // Block others before we write our message 
  snd = this->Processes[myIdx];
  if (this->LogFile)
    {
    fprintf(this->LogFile, "%d:     Info%d: Lock (Send %d->%d) T: %d\n", 
	    myIdx, myIdx, myIdx, remoteProcessId, tag);
    fflush(this->LogFile);
    }
  snd->InfoLock->Lock();
  snd->SendFlag = 1;
  snd->Object = object;
  snd->Data = data;
  snd->DataLength = length;
  snd->Tag = tag;
  snd->RemoteId = remoteProcessId;
  if (this->LogFile)
    {
    fprintf(this->LogFile, "%d:     Info%d: Unlock (Send %d->%d) T: %d\n", 
	    myIdx, myIdx, myIdx, remoteProcessId, tag);
    fflush(this->LogFile);
    }
  snd->InfoLock->Unlock();
  
  // Message in in the Que. Go ahead and start a receive.
  if (this->LogFile)
    {
    fprintf(this->LogFile, "%d: Message: Unlock (send %d->%d) T: %d\n",
	    myIdx, myIdx, remoteProcessId, tag);
    fflush(this->LogFile);
    }
  this->MessageLock->Unlock();
  
  // Wait until this data is consumed.
  // I am assuming it is ok for one thread to lock, and another to unlock.
  if (this->LogFile)
    {
    fprintf(this->LogFile, "%d: Block%d: Lock (send waiting for a recv) T: %d\n",
	    myIdx, myIdx, tag);
    fflush(this->LogFile);
    }
  snd->BlockLock->Lock();
  // Transaction has been completed by the receive.
  
  return 1;
}


//----------------------------------------------------------------------------
// Handles message in object or in data string.
int vtkThreadedController::Receive(vtkObject *object, void *data, int length, 
				   int remoteProcessId, int tag)
{
  int myIdx;
  int start, end, i;
  vtkThreadedControllerProcessInfo *snd, *rcv;
  
  myIdx = this->GetLocalProcessId();

  if (this->LogFile)
    {
    fprintf(this->LogFile, "%d: Recv: object = %d, data = %d, length = %d, remoteId = %d, tag = %d\n",
	    myIdx, object, data, length, remoteProcessId, tag);
    fflush(this->LogFile);

    // Avoid a send and recv starting at the same time.
    fprintf(this->LogFile, "%d: Message: Lock (recv %d->%d) T: %d\n",
	    myIdx, remoteProcessId, myIdx, tag);
    fflush(this->LogFile);
    }
  
  this->MessageLock->Lock();
  
  // Look at pending sends to find any matches.
  // A bit of a hack to handle AnySource.
  if (remoteProcessId == VTK_MP_CONTROLLER_ANY_SOURCE)
    {
    start = 0;
    end = this->NumberOfProcesses-1;
    }
  else
    {
    start = end = remoteProcessId;
    }
  for (i = start; i <= end; ++i)
    {
    snd = this->Processes[i];
    if (this->LogFile)
      {
      fprintf(this->LogFile, "%d:     Info%d: Lock (recv %d->%d) T: %d\n", 
	      myIdx, i, remoteProcessId, myIdx, tag);
      fflush(this->LogFile);
      }
    snd->InfoLock->Lock();
    if (snd->RemoteId == myIdx && snd->SendFlag == 1 && snd->Tag == tag &&
        snd->DataLength == length && !(snd->Object) == !object)
      {
      // We have a match. A send is already waiting.
      if (this->LogFile)
	{
	fprintf(this->LogFile, "%d:            recv %d->%d, T: %d,  Match! \n",
		myIdx, remoteProcessId, myIdx, tag);
	fflush(this->LogFile);    
	}
      
   
      // Copy the message.
      if (length > 0 && snd->Data && data)
	{
	memcpy(data, snd->Data, length);
	}
      // Copy the object
      if (object && snd->Object)
	{
	this->CopyObject(snd->Object, object);
	}
      
      // set "ProcessInfo" back to default values
      snd->Object = NULL;
      snd->Data = NULL;
      snd->DataLength = 0;
      snd->SendFlag = 0;
      snd->Tag = 0;
      snd->RemoteId = VTK_MP_CONTROLLER_INVALID_SOURCE;
      // Release the send block, so it can return.
      if (this->LogFile)
	{
	fprintf(this->LogFile, "%d: Block%d: Unlock (recv %d->%d) T: %d\n", 
		myIdx, i, remoteProcessId, myIdx, tag);
	fflush(this->LogFile); 
	}
      snd->BlockLock->Unlock();      
      
      // Free up any locks we have
      if (this->LogFile)
	{
	fprintf(this->LogFile, "%d:     Info%d: Unlock (recv %d->%d) T: %d\n", 
		myIdx, i, remoteProcessId, myIdx, tag);
	fflush(this->LogFile);
	}
      snd->InfoLock->Unlock();
      if (this->LogFile)
	{
	fprintf(this->LogFile, "%d: Message: Unlock (recv %d->%d) T: %d\n",
		myIdx, remoteProcessId, myIdx, tag);
	fflush(this->LogFile);
	}
      this->MessageLock->Unlock();
      
      return 1;
      }
    if (this->LogFile)
      {
      fprintf(this->LogFile, "%d:     Info%d: Unlock (recv %d->%d) T: %d\n", 
	      myIdx, i, remoteProcessId, myIdx, tag);
      fflush(this->LogFile);
      }
    snd->InfoLock->Unlock();
    }
  if (this->LogFile)
    {
    fprintf(this->LogFile, "%d:            recv %d->%d, T: %d,  first=>wait\n",
	    myIdx, remoteProcessId, myIdx, tag);
    fflush(this->LogFile);    
    }
  
  // Matching send has not been initiated.
  // Put message information in "ProcessInfo"
  rcv = this->Processes[myIdx];
  if (this->LogFile)
    {
    fprintf(this->LogFile, "%d:     Info%d: Lock (recv %d->%d) T: %d\n", 
	    myIdx, myIdx, remoteProcessId, myIdx, tag);
    fflush(this->LogFile);
    }
  rcv->InfoLock->Lock();
  rcv->SendFlag = 0;
  rcv->Object = object;
  rcv->Data = data;
  rcv->DataLength = length;
  rcv->Tag = tag;
  rcv->RemoteId = remoteProcessId;
  if (this->LogFile)
    {
    fprintf(this->LogFile, "%d:     Info%d: Unlock (recv %d->%d) T: %d\n", 
	    myIdx, myIdx, remoteProcessId, myIdx, tag);
    fflush(this->LogFile);
    }
  rcv->InfoLock->Unlock();
  
  
  // Message request is on the queue.  Go ahead and start a send.
  if (this->LogFile)
    {
    fprintf(this->LogFile, "%d: Message: Unlock (recv %d->%d) T: %d\n",
	    myIdx, remoteProcessId, myIdx, tag);
    fflush(this->LogFile);
    }
  this->MessageLock->Unlock();
  
  // Wait for the send (wait until this data is consumed).
  // I am assuming it is ok for one thread to lock, and another to unlock.
  if (this->LogFile)
    {
    fprintf(this->LogFile, "%d: Block%d: Lock (recv waiting for a send) T: %d\n",
	    myIdx, myIdx, tag);
    fflush(this->LogFile);
    }
  rcv->BlockLock->Lock();
  // Transaction has been completed by the send.
  
  return 1;
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
int vtkThreadedController::Send(vtkObject *data, int remoteProcessId, 
				int tag)
{
  if (this->ForceDeepCopy)
    {
    return this->vtkMultiProcessController::Send(data, remoteProcessId, tag);
    }
  
  if (strcmp(data->GetClassName(), "vtkPolyData") == 0  ||
      strcmp(data->GetClassName(), "vtkUnstructuredGrid") == 0  ||
      strcmp(data->GetClassName(), "vtkStructuredGrid") == 0  ||
      strcmp(data->GetClassName(), "vtkStructuredPoints") == 0  ||
      strcmp(data->GetClassName(), "vtkRectilinearGrid") == 0)
    {
    return this->Send(data, NULL, 0, remoteProcessId, tag);
    }
  if (strcmp(data->GetClassName(), "vtkImageData") == 0)
    {
    return this->Send(data, NULL, 0, remoteProcessId, tag);
    }
  
  // By default, just use the normal marshaling from the superclass. 
  return this->vtkMultiProcessController::Send(data, remoteProcessId, tag);
}

//----------------------------------------------------------------------------
int vtkThreadedController::Receive(vtkObject *data, 
				   int remoteProcessId, int tag)
{
  // If we want to disable copy by reference.
  if (this->ForceDeepCopy)
    {
    return this->vtkMultiProcessController::Receive(data,remoteProcessId,tag);
    }
  
  if (strcmp(data->GetClassName(), "vtkPolyData") == 0  ||
      strcmp(data->GetClassName(), "vtkUnstructuredGrid") == 0  ||
      strcmp(data->GetClassName(), "vtkStructuredGrid") == 0  ||
      strcmp(data->GetClassName(), "vtkStructuredPoints") == 0  ||
      strcmp(data->GetClassName(), "vtkRectilinearGrid") == 0)
    {
    return this->Receive(data, NULL, 0, remoteProcessId, tag);
    }
  if (strcmp(data->GetClassName(), "vtkImageData") == 0)
    {
    return this->Receive(data, NULL, 0, remoteProcessId, tag);
    }

  // By default, just use the normal marshaling from the superclass. 
  return this->vtkMultiProcessController::Receive(data, remoteProcessId, tag);
}


//----------------------------------------------------------------------------
void vtkThreadedController::CopyObject(vtkObject *src, vtkObject *dest)
{
  if (strcmp(src->GetClassName(), dest->GetClassName()) != 0)
    {
    vtkErrorMacro("Object are not the same type. Cannot copy");
    return;
    }
  
  if (strcmp(src->GetClassName(), "vtkPolyData") == 0  ||
      strcmp(src->GetClassName(), "vtkUnstructuredGrid") == 0  ||
      strcmp(src->GetClassName(), "vtkStructuredGrid") == 0  ||
      strcmp(src->GetClassName(), "vtkStructuredPoints") == 0  ||
      strcmp(src->GetClassName(), "vtkRectilinearGrid") == 0)
    {
    this->CopyDataSet((vtkDataSet*)src, (vtkDataSet*)dest);
    return;
    }
  if (strcmp(src->GetClassName(), "vtkImageData") == 0)
    {
    this->CopyImageData((vtkImageData*)src, (vtkImageData*)dest);
    return;
    }
  
  vtkErrorMacro("Missing case for shallow copy");
}











