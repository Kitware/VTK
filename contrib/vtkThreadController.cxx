/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkThreadController.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkThreadController.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkThreadController* vtkThreadController::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkThreadController");
  if(ret)
    {
    return (vtkThreadController*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkThreadController;
}






// Since sends and receives block until the transation is complete,
// each thread can have at most one receive or send pending.
class vtkThreadControllerProcessInfo
{
public:
  vtkThreadControllerProcessInfo() 
    { this->BlockLock = vtkMutexLock::New();
      this->InfoLock = vtkMutexLock::New();
      this->Data = NULL;
      this->DataLength = 0;
      this->Tag = 0;
      this->SendFlag = 0;
      this->RemoteId = VTK_MP_CONTROLLER_INVALID_SOURCE; };
  ~vtkThreadControllerProcessInfo() 
    { this->BlockLock->Delete();
      this->BlockLock = NULL; 
      this->InfoLock->Delete();
      this->InfoLock = NULL; };

  // Mechanism for blocking this thread during a receive or send call.
  vtkMutexLock        *BlockLock;
  // To avoid changing this info in one thread while reading an another.
  vtkMutexLock        *InfoLock;
  void                *Data;
  int                 DataLength;
  int                 Tag;
  int                 RemoteId;
  // This flag saves whether this info is for a send or a receive.
  int                 SendFlag;
};


//----------------------------------------------------------------------------
vtkThreadController::vtkThreadController()
{
  int idx;
  
  this->MultiThreader = vtkMultiThreader::New();
  this->MultipleMethodFlag = 0;
  
  for (idx = 0; idx < VTK_MP_CONTROLLER_MAX_PROCESSES; ++idx)
    {
    this->Processes[idx] = NULL;
    }  
}

//----------------------------------------------------------------------------
vtkThreadController::~vtkThreadController()
{
  this->MultiThreader->Delete();
  this->MultiThreader = NULL;
}

//----------------------------------------------------------------------------
void vtkThreadController::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMultiProcessController::PrintSelf(os,indent);
  os << indent << "MultiThreader:\n";
  this->MultiThreader->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
void vtkThreadController::Initialize(int argc, char *argv[])
{
  this->Modified();
  
  this->NumberOfProcesses = this->MultiThreader->GetNumberOfThreads();
}
  

//----------------------------------------------------------------------------
// Called before threads are spawned to create the thread info.
void vtkThreadController::CreateThreadInfoObjects()
{
  int idx;

  for (idx = 0; idx < this->NumberOfProcesses; ++idx)
    {
    this->Processes[idx] = new vtkThreadControllerProcessInfo;
    // By default these blocking mutexes are locked.
    // I am assuming it is ok for one thread to lock, and another to unlock.
    this->Processes[idx]->BlockLock->Lock();
    }
}


//----------------------------------------------------------------------------
// Called before threads are spawned to create the thread info.
void vtkThreadController::DeleteThreadInfoObjects()
{
  int idx;

  for (idx = 0; idx < this->NumberOfProcesses; ++idx)
    {
    // By default these blocking mutexes are locked.
    // I am assuming it is ok for one thread to lock, and another to unlock.
    this->Processes[idx]->BlockLock->Unlock();
    delete this->Processes[idx];
    this->Processes[idx] = NULL;
    }
}


  
  



//----------------------------------------------------------------------------
VTK_THREAD_RETURN_TYPE vtkThreadControllerStart( void *arg )
{
  ThreadInfoStruct *info = (ThreadInfoStruct*)(arg);
  int threadIdx = info->ThreadID;
  vtkThreadController *self = (vtkThreadController*)(info->UserData);

  self->Start(threadIdx);
  return VTK_THREAD_RETURN_VALUE;
}
void vtkThreadController::Start(int threadIdx)
{
  
  // Store threadId in a table.
#ifdef VTK_USE_PTHREADS  
  this->ThreadIds[threadIdx] = pthread_self();
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
void vtkThreadController::SingleMethodExecute()
{
  this->CreateThreadInfoObjects();
  this->MultipleMethodFlag = 0;
  this->MultiThreader->SetSingleMethod(vtkThreadControllerStart, (void*)this);
  this->MultiThreader->SetNumberOfThreads(this->NumberOfProcesses);
  this->MultiThreader->SingleMethodExecute();
  this->DeleteThreadInfoObjects();
}
//----------------------------------------------------------------------------
// Execute the methods set as the MultipleMethods.
void vtkThreadController::MultipleMethodExecute()
{
  this->CreateThreadInfoObjects();
  this->MultipleMethodFlag = 1;
  this->MultiThreader->SetSingleMethod(vtkThreadControllerStart, (void*)this);
  this->MultiThreader->SetNumberOfThreads(this->NumberOfProcesses);
  this->MultiThreader->SingleMethodExecute();
  this->DeleteThreadInfoObjects();
}


//----------------------------------------------------------------------------
// I may need a mutex lock here. Although my own pid must have already been
// set,  once before me could be in a funky state.
int vtkThreadController::GetLocalProcessId()
{
  int idx;
  
#ifdef VTK_USE_PTHREADS  
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
#else
  vtkErrorMacro("ThreadController only works with pthreads for now");
  return -1;
#endif
  
}


//----------------------------------------------------------------------------
int vtkThreadController::Send(void *data, int length, int remoteProcessId, 
			      int tag)
{
  int myIdx = this->GetLocalProcessId();
  vtkThreadControllerProcessInfo *snd;
  vtkThreadControllerProcessInfo *rcv = this->Processes[remoteProcessId];

  // Look at pending receives to find any matches.
  rcv->InfoLock->Lock();
  if ((rcv->RemoteId == myIdx || rcv->RemoteId == VTK_MP_CONTROLLER_ANY_SOURCE)
      && rcv->SendFlag == 0 && rcv->Tag == tag)
    {
    // We have a match. A receive is already waiting.
    if (length != rcv->DataLength)
      {
      vtkWarningMacro("tag: " << tag << ", Sending length " << length 
		      << " does not match receive length " 
		      << rcv->DataLength);
      if (length > rcv->DataLength)
	{
	length = rcv->DataLength;
	}
      }
    
    // Copy the message.
    if (length > 0)
      {
      memcpy(rcv->Data, data, length);
      }
    // set "ProcessInfo" back to default values
    rcv->Data = NULL;
    rcv->DataLength = 0;
    rcv->SendFlag = 0;
    rcv->Tag = 0;
    rcv->RemoteId = VTK_MP_CONTROLLER_INVALID_SOURCE;
    
    // we are done writing/reading the rcv
    rcv->InfoLock->Unlock();
    // Release the receive block, so it can return.
    rcv->BlockLock->Unlock();
    return 1;
    }
  
  // Matching receive has not been initiated.
  // Put message information in "ProcessInfo"
  snd = this->Processes[myIdx];
  snd->InfoLock->Lock();
  snd->SendFlag = 1;
  snd->Data = data;
  snd->DataLength = length;
  snd->Tag = tag;
  snd->RemoteId = remoteProcessId;
  snd->InfoLock->Unlock();
  
  // Wait until this data is consumed.
  // I am assuming it is ok for one thread to lock, and another to unlock.
  snd->BlockLock->Lock();
  // Transaction has been completed by the receive.
  
  return 1;
}


//----------------------------------------------------------------------------
int vtkThreadController::Receive(void *data, int length, int remoteProcessId, 
				 int tag)
{
  int myIdx;
  int start, end, i;
  vtkThreadControllerProcessInfo *snd, *rcv;
  
  myIdx = this->GetLocalProcessId();

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
    snd->InfoLock->Lock();
    if (snd->RemoteId == myIdx && snd->SendFlag == 1 && snd->Tag == tag)
      {
      // We have a match. A send is already waiting.
      if (length != snd->DataLength)
	{
	vtkWarningMacro("tag: " << tag << ", Sending length "
	   << snd->DataLength << " does not match receive length " << length);
	if (length > snd->DataLength)
	  {
	  length = snd->DataLength;
	  }
	}
    
      // Copy the message.
      if (length > 0)
	{
	memcpy(data, snd->Data, length);
	}
      // set "ProcessInfo" back to default values
      snd->Data = NULL;
      snd->DataLength = 0;
      snd->SendFlag = 0;
      snd->Tag = 0;
      snd->RemoteId = VTK_MP_CONTROLLER_INVALID_SOURCE;
      
      // we are done writing/reading the rcv
      snd->InfoLock->Unlock();
      // Release the send block, so it can return.
      snd->BlockLock->Unlock();
      return 1;
      }
    }
  
  // Matching send has not been initiated.
  // Put message information in "ProcessInfo"
  rcv = this->Processes[myIdx];
  rcv->InfoLock->Lock();
  rcv->SendFlag = 0;
  rcv->Data = data;
  rcv->DataLength = length;
  rcv->Tag = tag;
  rcv->RemoteId = remoteProcessId;
  rcv->InfoLock->Unlock();
  
  // Wait until this data is consumed.
  // I am assuming it is ok for one thread to lock, and another to unlock.
  rcv->BlockLock->Lock();
  // Transaction has been completed by the send.
  
  return 1;
}



//----------------------------------------------------------------------------
int vtkThreadController::Send(int *data, int length, int remoteProcessId, 
			      int tag)
{
  length = length * sizeof(int);
  return this->Send((void*)data, length, remoteProcessId, tag);
}

//----------------------------------------------------------------------------
int vtkThreadController::Send(unsigned long *data, int length, 
			      int remoteProcessId, int tag)
{
  length = length * sizeof(unsigned long);
  return this->Send((void*)data, length, remoteProcessId, tag);
}

//----------------------------------------------------------------------------
int vtkThreadController::Send(char *data, int length, int remoteProcessId, 
			      int tag)
{
  length = length * sizeof(char);
  return this->Send((void*)data, length, remoteProcessId, tag);
}

//----------------------------------------------------------------------------
int vtkThreadController::Send(float *data, int length, int remoteProcessId, 
			      int tag)
{
  length = length * sizeof(float);
  return this->Send((void*)data, length, remoteProcessId, tag);
}




//----------------------------------------------------------------------------
int vtkThreadController::Receive(int *data, int length, int remoteProcessId, 
				 int tag)
{
  length = length * sizeof(int);
  return this->Receive((void*)data, length, remoteProcessId, tag);
}

//----------------------------------------------------------------------------
int vtkThreadController::Receive(unsigned long *data, int length, 
				 int remoteProcessId, int tag)
{
  length = length * sizeof(unsigned long);
  return this->Receive((void*)data, length, remoteProcessId, tag);
}

//----------------------------------------------------------------------------
int vtkThreadController::Receive(char *data, int length, int remoteProcessId, 
				 int tag)
{
  length = length * sizeof(char);
  return this->Receive((void*)data, length, remoteProcessId, tag);
}

//----------------------------------------------------------------------------
int vtkThreadController::Receive(float *data, int length, int remoteProcessId, 
				 int tag)
{
  length = length * sizeof(float);
  return this->Receive((void*)data, length, remoteProcessId, tag);
}

