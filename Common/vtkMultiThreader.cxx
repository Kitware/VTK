/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiThreader.cxx
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
#include "vtkMultiThreader.h"
#include "vtkObjectFactory.h"



// These are the includes necessary for multithreaded rendering on an SGI
// using the sproc() call
#ifdef VTK_USE_SPROC
#include <sys/resource.h>
#include <sys/prctl.h>
#include <wait.h>
#include <errno.h>
#endif

#ifdef VTK_USE_PTHREADS
#include <pthread.h>
#endif


//----------------------------------------------------------------------------
vtkMultiThreader* vtkMultiThreader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMultiThreader");
  if(ret)
    {
    return (vtkMultiThreader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMultiThreader;
}



// Initialize static member that controls global maximum number of threads
static int vtkMultiThreaderGlobalMaximumNumberOfThreads = 0;

void vtkMultiThreader::SetGlobalMaximumNumberOfThreads(int val)
{
  if (val == vtkMultiThreaderGlobalMaximumNumberOfThreads)
    {
    return;
    }
  vtkMultiThreaderGlobalMaximumNumberOfThreads = val;
}

int vtkMultiThreader::GetGlobalMaximumNumberOfThreads()
{
  return vtkMultiThreaderGlobalMaximumNumberOfThreads;
}

// 0 => Not initialized.
static int vtkMultiThreaderGlobalDefaultNumberOfThreads = 0;

void vtkMultiThreader::SetGlobalDefaultNumberOfThreads(int val)
{
  if (val == vtkMultiThreaderGlobalDefaultNumberOfThreads)
    {
    return;
    }
  vtkMultiThreaderGlobalDefaultNumberOfThreads = val;
}

int vtkMultiThreader::GetGlobalDefaultNumberOfThreads()
{
  if (vtkMultiThreaderGlobalDefaultNumberOfThreads == 0)
    {
    int num = 1; // default is 1
#ifdef VTK_USE_SPROC
    // Default the number of threads to be the number of available
    // processors if we are using sproc()
    num = prctl( PR_MAXPPROCS );
#endif

#ifdef VTK_USE_PTHREADS
    // Default the number of threads to be the number of available
    // processors if we are using pthreads()
#ifdef _SC_NPROCESSORS_ONLN
    num = sysconf( _SC_NPROCESSORS_ONLN );
#elif defined(_SC_NPROC_ONLN)
    num = sysconf( _SC_NPROC_ONLN );
#endif
#endif

#ifdef _WIN32
    {
      SYSTEM_INFO sysInfo;
      GetSystemInfo(&sysInfo);
      num = sysInfo.dwNumberOfProcessors;
    }
#endif

#ifndef VTK_USE_WIN32_THREADS
#ifndef VTK_USE_SPROC
#ifndef VTK_USE_PTHREADS
    // If we are not multithreading, the number of threads should
    // always be 1
    num = 1;
#endif  
#endif  
#endif
  
    // Lets limit the number of threads to 8
    if (num > 8)
      {
      num = 8;
      }

    vtkMultiThreaderGlobalDefaultNumberOfThreads = num;
    }


  return vtkMultiThreaderGlobalDefaultNumberOfThreads;
}

// Constructor. Default all the methods to NULL. Since the
// ThreadInfoArray is static, the ThreadIDs can be initialized here
// and will not change.
vtkMultiThreader::vtkMultiThreader()
{
  int i;

  for ( i = 0; i < VTK_MAX_THREADS; i++ )
    {
    this->ThreadInfoArray[i].ThreadID           = i;
    this->ThreadInfoArray[i].ActiveFlag         = NULL;
    this->ThreadInfoArray[i].ActiveFlagLock     = NULL;
    this->MultipleMethod[i]                     = NULL;
    this->SpawnedThreadActiveFlag[i]            = 0;
    this->SpawnedThreadActiveFlagLock[i]        = NULL;
    this->SpawnedThreadInfoArray[i].ThreadID    = i;
    }

  this->SingleMethod = NULL;
  this->NumberOfThreads = 
    vtkMultiThreader::GetGlobalDefaultNumberOfThreads();

}

// Destructor. Nothing allocated so nothing needs to be done here.
vtkMultiThreader::~vtkMultiThreader()
{
}

// Set the user defined method that will be run on NumberOfThreads threads
// when SingleMethodExecute is called.
void vtkMultiThreader::SetSingleMethod( vtkThreadFunctionType f, 
					void *data )
{ 
  this->SingleMethod = f;
  this->SingleData   = data;
}

// Set one of the user defined methods that will be run on NumberOfThreads
// threads when MultipleMethodExecute is called. This method should be
// called with index = 0, 1, ..,  NumberOfThreads-1 to set up all the
// required user defined methods
void vtkMultiThreader::SetMultipleMethod( int index, 
					  vtkThreadFunctionType f, void *data )
{ 
  // You can only set the method for 0 through NumberOfThreads-1
  if ( index >= this->NumberOfThreads ) {
    vtkErrorMacro( << "Can't set method " << index << 
    " with a thread count of " << this->NumberOfThreads );
    }
  else
    {
    this->MultipleMethod[index] = f;
    this->MultipleData[index]   = data;
    }
}

// Execute the method set as the SingleMethod on NumberOfThreads threads.
void vtkMultiThreader::SingleMethodExecute()
{
  int                thread_loop = 0;

#ifdef VTK_USE_WIN32_THREADS
  DWORD              threadId;
  HANDLE             process_id[VTK_MAX_THREADS];
#endif

#ifdef VTK_USE_SPROC
  siginfo_t          info_ptr;
  int                process_id[VTK_MAX_THREADS];
#endif

#ifdef VTK_USE_PTHREADS
  pthread_t          process_id[VTK_MAX_THREADS];
#endif

  if ( !this->SingleMethod )
    {
    vtkErrorMacro( << "No single method set!" );
    return;
    }

  // obey the global maximum number of threads limit
  if (vtkMultiThreaderGlobalMaximumNumberOfThreads &&
      this->NumberOfThreads > vtkMultiThreaderGlobalMaximumNumberOfThreads)
    {
    this->NumberOfThreads = vtkMultiThreaderGlobalMaximumNumberOfThreads;
    }
  
    
  // We are using sproc (on SGIs), pthreads(on Suns), or a single thread
  // (the default)  

#ifdef VTK_USE_WIN32_THREADS
  // Using CreateThread on a PC
  //
  // We want to use CreateThread to start this->NumberOfThreads - 1 
  // additional threads which will be used to call this->SingleMethod().
  // The parent thread will also call this routine.  When it is done,
  // it will wait for all the children to finish. 
  //
  // First, start up the this->NumberOfThreads-1 processes.  Keep track
  // of their process ids for use later in the waitid call
  for (thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++ )
    {
    this->ThreadInfoArray[thread_loop].UserData    = this->SingleData;
    this->ThreadInfoArray[thread_loop].NumberOfThreads = this->NumberOfThreads;
    process_id[thread_loop] = 
      CreateThread(NULL, 0, this->SingleMethod, 
	     ((void *)(&this->ThreadInfoArray[thread_loop])), 0, &threadId);
    if (process_id == NULL)
      {
      vtkErrorMacro("Error in thread creation !!!");
      } 
    }
  
  // Now, the parent thread calls this->SingleMethod() itself
  this->ThreadInfoArray[0].UserData = this->SingleData;
  this->ThreadInfoArray[0].NumberOfThreads = this->NumberOfThreads;
  this->SingleMethod((void *)(&this->ThreadInfoArray[0]));

  // The parent thread has finished this->SingleMethod() - so now it
  // waits for each of the other processes to exit
  for ( thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++ )
    {
    WaitForSingleObject(process_id[thread_loop], INFINITE);
    }

  // close the threads
  for ( thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++ )
    {
    CloseHandle(process_id[thread_loop]);
    }
#endif

#ifdef VTK_USE_SPROC
  // Using sproc() on an SGI
  //
  // We want to use sproc to start this->NumberOfThreads - 1 additional
  // threads which will be used to call this->SingleMethod(). The
  // parent thread will also call this routine.  When it is done,
  // it will wait for all the children to finish. 
  //
  // First, start up the this->NumberOfThreads-1 processes.  Keep track
  // of their process ids for use later in the waitid call

  for ( thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++ )
    {
    this->ThreadInfoArray[thread_loop].UserData    = this->SingleData;
    this->ThreadInfoArray[thread_loop].NumberOfThreads = this->NumberOfThreads;
    process_id[thread_loop] = 
      sproc( this->SingleMethod, PR_SADDR, 
	     ( (void *)(&this->ThreadInfoArray[thread_loop]) ) );
    if ( process_id[thread_loop] == -1)
      {
      vtkErrorMacro("sproc call failed. Code: " << errno << endl);
      }
    }
  
  // Now, the parent thread calls this->SingleMethod() itself
  this->ThreadInfoArray[0].UserData = this->SingleData;
  this->ThreadInfoArray[0].NumberOfThreads = this->NumberOfThreads;
  this->SingleMethod((void *)(&this->ThreadInfoArray[0]) );

  // The parent thread has finished this->SingleMethod() - so now it
  // waits for each of the other processes to exit
  for ( thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++ )
    {
    waitid( P_PID, (id_t) process_id[thread_loop], &info_ptr, WEXITED );
    }
#endif

#ifdef VTK_USE_PTHREADS
  // Using POSIX threads
  //
  // We want to use pthread_create to start this->NumberOfThreads-1 additional
  // threads which will be used to call this->SingleMethod(). The
  // parent thread will also call this routine.  When it is done,
  // it will wait for all the children to finish. 
  //
  // First, start up the this->NumberOfThreads-1 processes.  Keep track
  // of their process ids for use later in the pthread_join call

  pthread_attr_t attr;

#ifdef VTK_HP_PTHREADS
  pthread_attr_create( &attr );
#else  
  pthread_attr_init(&attr);
#if !defined(__CYGWIN__)
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
#endif
#endif
  
  for ( thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++ )
    {
    this->ThreadInfoArray[thread_loop].UserData    = this->SingleData;
    this->ThreadInfoArray[thread_loop].NumberOfThreads = this->NumberOfThreads;

#ifdef VTK_HP_PTHREADS
    pthread_create( &(process_id[thread_loop]),
		    attr, this->SingleMethod,  
		    ( (void *)(&this->ThreadInfoArray[thread_loop]) ) );
#else
    int                threadError;
    threadError =
      pthread_create( &(process_id[thread_loop]), &attr, this->SingleMethod,  
		      ( (void *)(&this->ThreadInfoArray[thread_loop]) ) );
    if (threadError != 0)
      {
      vtkErrorMacro(<< "Unable to create a thread.  pthread_create() returned "
                    << threadError);
      }
#endif
    }
  
  // Now, the parent thread calls this->SingleMethod() itself
  this->ThreadInfoArray[0].UserData = this->SingleData;
  this->ThreadInfoArray[0].NumberOfThreads = this->NumberOfThreads;
  this->SingleMethod((void *)(&this->ThreadInfoArray[0]) );

  // The parent thread has finished this->SingleMethod() - so now it
  // waits for each of the other processes to exit
  for ( thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++ )
    {
    pthread_join( process_id[thread_loop], NULL );
    }
#endif

#ifndef VTK_USE_WIN32_THREADS
#ifndef VTK_USE_SPROC
#ifndef VTK_USE_PTHREADS
  // There is no multi threading, so there is only one thread.
  this->ThreadInfoArray[0].UserData    = this->SingleData;
  this->ThreadInfoArray[0].NumberOfThreads = this->NumberOfThreads;
  this->SingleMethod( (void *)(&this->ThreadInfoArray[0]) );
#endif
#endif
#endif
}

void vtkMultiThreader::MultipleMethodExecute()
{
  int                thread_loop;

#ifdef VTK_USE_WIN32_THREADS
  DWORD              threadId;
  HANDLE             process_id[VTK_MAX_THREADS];
#endif

#ifdef VTK_USE_SPROC
  siginfo_t          info_ptr;
  int                process_id[VTK_MAX_THREADS];
#endif

#ifdef VTK_USE_PTHREADS
  pthread_t          process_id[VTK_MAX_THREADS];
#endif


  // obey the global maximum number of threads limit
  if (vtkMultiThreaderGlobalMaximumNumberOfThreads &&
      this->NumberOfThreads > vtkMultiThreaderGlobalMaximumNumberOfThreads)
    {
    this->NumberOfThreads = vtkMultiThreaderGlobalMaximumNumberOfThreads;
    }

  for ( thread_loop = 0; thread_loop < this->NumberOfThreads; thread_loop++ )
    {
    if ( this->MultipleMethod[thread_loop] == (vtkThreadFunctionType)NULL)
      {
      vtkErrorMacro( << "No multiple method set for: " << thread_loop );
      return;
      }
    }

  // We are using sproc (on SGIs), pthreads(on Suns), CreateThread
  // on a PC or a single thread (the default)  

#ifdef VTK_USE_WIN32_THREADS
  // Using CreateThread on a PC
  //
  // We want to use CreateThread to start this->NumberOfThreads - 1 
  // additional threads which will be used to call the NumberOfThreads-1
  // methods defined in this->MultipleMethods[](). The parent thread
  // will call this->MultipleMethods[NumberOfThreads-1]().  When it is done,
  // it will wait for all the children to finish. 
  //
  // First, start up the this->NumberOfThreads-1 processes.  Keep track
  // of their process ids for use later in the waitid call
  for ( thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++ )
    {
    this->ThreadInfoArray[thread_loop].UserData = 
      this->MultipleData[thread_loop];
    this->ThreadInfoArray[thread_loop].NumberOfThreads = this->NumberOfThreads;
    process_id[thread_loop] = 
      CreateThread(NULL, 0, this->MultipleMethod[thread_loop], 
	     ((void *)(&this->ThreadInfoArray[thread_loop])), 0, &threadId);
    if (process_id == NULL)
      {
      vtkErrorMacro("Error in thread creation !!!");
      } 
    }
  
  // Now, the parent thread calls the last method itself
  this->ThreadInfoArray[0].UserData = this->MultipleData[0];
  this->ThreadInfoArray[0].NumberOfThreads = this->NumberOfThreads;
  (this->MultipleMethod[0])((void *)(&this->ThreadInfoArray[0]) );

  // The parent thread has finished its method - so now it
  // waits for each of the other processes (created with sproc) to
  // exit
  for ( thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++ )
    {
    WaitForSingleObject(process_id[thread_loop], INFINITE);
    }

  // close the threads
  for ( thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++ )
    {
    CloseHandle(process_id[thread_loop]);
    }
#endif

#ifdef VTK_USE_SPROC
  // Using sproc() on an SGI
  //
  // We want to use sproc to start this->NumberOfThreads - 1 additional
  // threads which will be used to call the NumberOfThreads-1 methods
  // defined in this->MultipleMethods[](). The parent thread
  // will call this->MultipleMethods[NumberOfThreads-1]().  When it is done,
  // it will wait for all the children to finish. 
  //
  // First, start up the this->NumberOfThreads-1 processes.  Keep track
  // of their process ids for use later in the waitid call
  for ( thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++ )
    {
    this->ThreadInfoArray[thread_loop].UserData = 
      this->MultipleData[thread_loop];
    this->ThreadInfoArray[thread_loop].NumberOfThreads = this->NumberOfThreads;
    process_id[thread_loop] = 
      sproc( this->MultipleMethod[thread_loop], PR_SADDR, 
	     ( (void *)(&this->ThreadInfoArray[thread_loop]) ) );
    }
  
  // Now, the parent thread calls the last method itself
  this->ThreadInfoArray[0].UserData = this->MultipleData[0];
  this->ThreadInfoArray[0].NumberOfThreads = this->NumberOfThreads;
  (this->MultipleMethod[0])((void *)(&this->ThreadInfoArray[0]) );

  // The parent thread has finished its method - so now it
  // waits for each of the other processes (created with sproc) to
  // exit
  for ( thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++ )
    {
    waitid( P_PID, (id_t) process_id[thread_loop], &info_ptr, WEXITED );
    }
#endif

#ifdef VTK_USE_PTHREADS
  // Using POSIX threads
  //
  // We want to use pthread_create to start this->NumberOfThreads - 1 
  // additional
  // threads which will be used to call the NumberOfThreads-1 methods
  // defined in this->MultipleMethods[](). The parent thread
  // will call this->MultipleMethods[NumberOfThreads-1]().  When it is done,
  // it will wait for all the children to finish. 
  //
  // First, start up the this->NumberOfThreads-1 processes.  Keep track
  // of their process ids for use later in the pthread_join call

  pthread_attr_t attr;

#ifdef VTK_HP_PTHREADS
  pthread_attr_create( &attr );
#else  
  pthread_attr_init(&attr);
#ifndef __CYGWIN__
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
#endif
#endif

  for ( thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++ )
    {
    this->ThreadInfoArray[thread_loop].UserData = 
      this->MultipleData[thread_loop];
    this->ThreadInfoArray[thread_loop].NumberOfThreads = this->NumberOfThreads;
#ifdef VTK_HP_PTHREADS
    pthread_create( &(process_id[thread_loop]),
		    attr, this->MultipleMethod[thread_loop],  
		    ( (void *)(&this->ThreadInfoArray[thread_loop]) ) );
#else
    pthread_create( &(process_id[thread_loop]),
		    &attr, this->MultipleMethod[thread_loop],  
		    ( (void *)(&this->ThreadInfoArray[thread_loop]) ) );
#endif
    }
  
  // Now, the parent thread calls the last method itself
  this->ThreadInfoArray[0].UserData = this->MultipleData[0];
  this->ThreadInfoArray[0].NumberOfThreads = this->NumberOfThreads;
  (this->MultipleMethod[0])((void *)(&this->ThreadInfoArray[0]) );

  // The parent thread has finished its method - so now it
  // waits for each of the other processes to exit
  for ( thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++ )
    {
    pthread_join( process_id[thread_loop], NULL );
    }
#endif

#ifndef VTK_USE_WIN32_THREADS
#ifndef VTK_USE_SPROC
#ifndef VTK_USE_PTHREADS
  // There is no multi threading, so there is only one thread.
  this->ThreadInfoArray[0].UserData    = this->MultipleData[0];
  this->ThreadInfoArray[0].NumberOfThreads = this->NumberOfThreads;
  (this->MultipleMethod[0])( (void *)(&this->ThreadInfoArray[0]) );
#endif
#endif
#endif
}

int vtkMultiThreader::SpawnThread( vtkThreadFunctionType f, void *UserData )
{
  int id;

  // avoid a warning
  vtkThreadFunctionType tf;
  tf = f; tf= tf;
  
#ifdef VTK_USE_WIN32_THREADS
  DWORD              threadId;
#endif

  id = 0;

  while ( id < VTK_MAX_THREADS )
    {
    if ( this->SpawnedThreadActiveFlagLock[id] == NULL )
      {
      this->SpawnedThreadActiveFlagLock[id] = vtkMutexLock::New();
      }
    this->SpawnedThreadActiveFlagLock[id]->Lock();
    if (this->SpawnedThreadActiveFlag[id] == 0)
      {
      // We've got a useable thread id, so grab it
      this->SpawnedThreadActiveFlag[id] = 1;
      this->SpawnedThreadActiveFlagLock[id]->Unlock();
      break;
      }
    this->SpawnedThreadActiveFlagLock[id]->Unlock();
      
    id++;
    }

  if ( id >= VTK_MAX_THREADS )
    {
    vtkErrorMacro( << "You have too many active threads!" );
    return -1;
    }


  this->SpawnedThreadInfoArray[id].UserData        = UserData;
  this->SpawnedThreadInfoArray[id].NumberOfThreads = 1;
  this->SpawnedThreadInfoArray[id].ActiveFlag = 
    &this->SpawnedThreadActiveFlag[id];
  this->SpawnedThreadInfoArray[id].ActiveFlagLock = 
    this->SpawnedThreadActiveFlagLock[id];

  // We are using sproc (on SGIs), pthreads(on Suns or HPs), 
  // CreateThread (on win32), or generating an error  

#ifdef VTK_USE_WIN32_THREADS
  // Using CreateThread on a PC
  //
  this->SpawnedThreadProcessID[id] = 
      CreateThread(NULL, 0, f, 
	     ((void *)(&this->SpawnedThreadInfoArray[id])), 0, &threadId);
  if (this->SpawnedThreadProcessID[id] == NULL)
    {
    vtkErrorMacro("Error in thread creation !!!");
    } 
#endif

#ifdef VTK_USE_SPROC
  // Using sproc() on an SGI
  //
  this->SpawnedThreadProcessID[id] = 
    sproc( f, PR_SADDR, ( (void *)(&this->SpawnedThreadInfoArray[id]) ) );

#endif

#ifdef VTK_USE_PTHREADS
  // Using POSIX threads
  //
  pthread_attr_t attr;

#ifdef VTK_HP_PTHREADS
  pthread_attr_create( &attr );
#else  
  pthread_attr_init(&attr);
#ifndef __CYGWIN__
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
#endif
#endif
  
#ifdef VTK_HP_PTHREADS
  pthread_create( &(this->SpawnedThreadProcessID[id]),
		  attr, f,  
		  ( (void *)(&this->SpawnedThreadInfoArray[id]) ) );
#else
  pthread_create( &(this->SpawnedThreadProcessID[id]),
		  &attr, f,  
		  ( (void *)(&this->SpawnedThreadInfoArray[id]) ) );
#endif

#endif

#ifndef VTK_USE_WIN32_THREADS
#ifndef VTK_USE_SPROC
#ifndef VTK_USE_PTHREADS
  // There is no multi threading, so there is only one thread.
  // This won't work - so give an error message.
  vtkErrorMacro( << "Cannot spawn thread in a single threaded environment!" );
  this->SpawnedThreadActiveFlagLock[id]->Delete();
  id = -1;
#endif
#endif
#endif

  return id;
}

void vtkMultiThreader::TerminateThread( int ThreadID )
{

  if ( !this->SpawnedThreadActiveFlag[ThreadID] ) {
    return;
  }

  this->SpawnedThreadActiveFlagLock[ThreadID]->Lock();
  this->SpawnedThreadActiveFlag[ThreadID] = 0;
  this->SpawnedThreadActiveFlagLock[ThreadID]->Unlock();

#ifdef VTK_USE_WIN32_THREADS
  WaitForSingleObject(this->SpawnedThreadProcessID[ThreadID], INFINITE);
  CloseHandle(this->SpawnedThreadProcessID[ThreadID]);
#endif

#ifdef VTK_USE_SPROC
  siginfo_t info_ptr;

  waitid( P_PID, (id_t) this->SpawnedThreadProcessID[ThreadID], 
	  &info_ptr, WEXITED );
#endif

#ifdef VTK_USE_PTHREADS
  pthread_join( this->SpawnedThreadProcessID[ThreadID], NULL );
#endif

#ifndef VTK_USE_WIN32_THREADS
#ifndef VTK_USE_SPROC
#ifndef VTK_USE_PTHREADS
  // There is no multi threading, so there is only one thread.
  // This won't work - so give an error message.
  vtkErrorMacro(<< "Cannot terminate thread in single threaded environment!");
#endif
#endif
#endif

  this->SpawnedThreadActiveFlagLock[ThreadID]->Delete();
  this->SpawnedThreadActiveFlagLock[ThreadID] = NULL;

}

// Print method for the multithreader
void vtkMultiThreader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os,indent); 

  os << indent << "Thread Count: " << this->NumberOfThreads << "\n";
  os << indent << "Global Maximum Number Of Threads: " << 
    vtkMultiThreaderGlobalMaximumNumberOfThreads << endl;
  os << "Thread system used: " <<
#ifdef VTK_USE_PTHREADS  
   "PTHREADS"
#elif defined VTK_USE_SPROC
    "SPROC"
#elif defined VTK_USE_WIN32_THREADS
    "WIN32 Threads"
#elif defined VTK_HP_PTHREADS
    "HP PThreads"
#else
    "NO THREADS SUPPORT"
#endif
     << endl;
}
