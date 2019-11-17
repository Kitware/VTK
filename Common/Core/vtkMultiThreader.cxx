/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiThreader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiThreader.h"

#include "vtkObjectFactory.h"
#include "vtkWindows.h"

vtkStandardNewMacro(vtkMultiThreader);

// Need to define "vtkExternCThreadFunctionType" to avoid warning on some
// platforms about passing function pointer to an argument expecting an
// extern "C" function.  Placing the typedef of the function pointer type
// inside an extern "C" block solves this problem.
#if defined(VTK_USE_PTHREADS)
#include <pthread.h>
extern "C"
{
  typedef void* (*vtkExternCThreadFunctionType)(void*);
}
#else
typedef vtkThreadFunctionType vtkExternCThreadFunctionType;
#endif

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <sys/types.h>
#endif

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

#ifdef VTK_USE_PTHREADS
    // Default the number of threads to be the number of available
    // processors if we are using pthreads()
#ifdef _SC_NPROCESSORS_ONLN
    num = sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(_SC_NPROC_ONLN)
    num = sysconf(_SC_NPROC_ONLN);
#endif
#endif

#ifdef __APPLE__
    // Determine the number of CPU cores.
    // hw.logicalcpu takes into account cores/CPUs that are
    // disabled because of power management.
    size_t dataLen = sizeof(int); // 'num' is an 'int'
    int result = sysctlbyname("hw.logicalcpu", &num, &dataLen, nullptr, 0);
    if (result == -1)
    {
      num = 1;
    }
#endif

#ifdef _WIN32
    {
      SYSTEM_INFO sysInfo;
      GetSystemInfo(&sysInfo);
      num = sysInfo.dwNumberOfProcessors;
    }
#endif

#ifndef VTK_USE_WIN32_THREADS
#ifndef VTK_USE_PTHREADS
    // If we are not multithreading, the number of threads should
    // always be 1
    num = 1;
#endif
#endif

    // Lets limit the number of threads to VTK_MAX_THREADS
    if (num > VTK_MAX_THREADS)
    {
      num = VTK_MAX_THREADS;
    }

    vtkMultiThreaderGlobalDefaultNumberOfThreads = num;
  }

  return vtkMultiThreaderGlobalDefaultNumberOfThreads;
}

// Constructor. Default all the methods to nullptr. Since the
// ThreadInfoArray is static, the ThreadIDs can be initialized here
// and will not change.
vtkMultiThreader::vtkMultiThreader()
{
  for (int i = 0; i < VTK_MAX_THREADS; i++)
  {
    this->ThreadInfoArray[i].ThreadID = i;
    this->ThreadInfoArray[i].ActiveFlag = nullptr;
    this->ThreadInfoArray[i].ActiveFlagLock = nullptr;
    this->MultipleMethod[i] = nullptr;
    this->SpawnedThreadActiveFlag[i] = 0;
    this->SpawnedThreadActiveFlagLock[i] = nullptr;
    this->SpawnedThreadInfoArray[i].ThreadID = i;
  }

  this->SingleMethod = nullptr;
  this->NumberOfThreads = vtkMultiThreader::GetGlobalDefaultNumberOfThreads();
}

vtkMultiThreader::~vtkMultiThreader()
{
  for (int i = 0; i < VTK_MAX_THREADS; i++)
  {
    delete this->ThreadInfoArray[i].ActiveFlagLock;
    delete this->SpawnedThreadActiveFlagLock[i];
  }
}

//----------------------------------------------------------------------------
int vtkMultiThreader::GetNumberOfThreads()
{
  int num = this->NumberOfThreads;
  if (vtkMultiThreaderGlobalMaximumNumberOfThreads > 0 &&
    num > vtkMultiThreaderGlobalMaximumNumberOfThreads)
  {
    num = vtkMultiThreaderGlobalMaximumNumberOfThreads;
  }
  return num;
}

// Set the user defined method that will be run on NumberOfThreads threads
// when SingleMethodExecute is called.
void vtkMultiThreader::SetSingleMethod(vtkThreadFunctionType f, void* data)
{
  this->SingleMethod = f;
  this->SingleData = data;
}

// Set one of the user defined methods that will be run on NumberOfThreads
// threads when MultipleMethodExecute is called. This method should be
// called with index = 0, 1, ..,  NumberOfThreads-1 to set up all the
// required user defined methods
void vtkMultiThreader::SetMultipleMethod(int index, vtkThreadFunctionType f, void* data)
{
  // You can only set the method for 0 through NumberOfThreads-1
  if (index >= this->NumberOfThreads)
  {
    vtkErrorMacro(<< "Can't set method " << index << " with a thread count of "
                  << this->NumberOfThreads);
  }
  else
  {
    this->MultipleMethod[index] = f;
    this->MultipleData[index] = data;
  }
}

// Execute the method set as the SingleMethod on NumberOfThreads threads.
void vtkMultiThreader::SingleMethodExecute()
{
  int thread_loop = 0;

#ifdef VTK_USE_WIN32_THREADS
  DWORD threadId;
  HANDLE process_id[VTK_MAX_THREADS] = {};
#endif

#ifdef VTK_USE_PTHREADS
  pthread_t process_id[VTK_MAX_THREADS] = {};
#endif

  if (!this->SingleMethod)
  {
    vtkErrorMacro(<< "No single method set!");
    return;
  }

  // obey the global maximum number of threads limit
  if (vtkMultiThreaderGlobalMaximumNumberOfThreads &&
    this->NumberOfThreads > vtkMultiThreaderGlobalMaximumNumberOfThreads)
  {
    this->NumberOfThreads = vtkMultiThreaderGlobalMaximumNumberOfThreads;
  }

#ifdef VTK_USE_WIN32_THREADS
  // Using CreateThread on Windows
  //
  // We want to use CreateThread to start this->NumberOfThreads - 1
  // additional threads which will be used to call this->SingleMethod().
  // The parent thread will also call this routine.  When it is done,
  // it will wait for all the children to finish.
  //
  // First, start up the this->NumberOfThreads-1 processes.  Keep track
  // of their process ids for use later in the waitid call
  for (thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++)
  {
    this->ThreadInfoArray[thread_loop].UserData = this->SingleData;
    this->ThreadInfoArray[thread_loop].NumberOfThreads = this->NumberOfThreads;
    process_id[thread_loop] = CreateThread(
      nullptr, 0, this->SingleMethod, ((void*)(&this->ThreadInfoArray[thread_loop])), 0, &threadId);
    if (process_id[thread_loop] == nullptr)
    {
      vtkErrorMacro("Error in thread creation !!!");
    }
  }

  // Now, the parent thread calls this->SingleMethod() itself
  this->ThreadInfoArray[0].UserData = this->SingleData;
  this->ThreadInfoArray[0].NumberOfThreads = this->NumberOfThreads;
  this->SingleMethod((void*)(&this->ThreadInfoArray[0]));

  // The parent thread has finished this->SingleMethod() - so now it
  // waits for each of the other processes to exit
  for (thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++)
  {
    WaitForSingleObject(process_id[thread_loop], INFINITE);
  }

  // close the threads
  for (thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++)
  {
    CloseHandle(process_id[thread_loop]);
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

  pthread_attr_init(&attr);
#if !defined(__CYGWIN__)
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
#endif

  for (thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++)
  {
    this->ThreadInfoArray[thread_loop].UserData = this->SingleData;
    this->ThreadInfoArray[thread_loop].NumberOfThreads = this->NumberOfThreads;

    int threadError = pthread_create(&(process_id[thread_loop]), &attr,
      reinterpret_cast<vtkExternCThreadFunctionType>(this->SingleMethod),
      ((void*)(&this->ThreadInfoArray[thread_loop])));
    if (threadError != 0)
    {
      vtkErrorMacro(<< "Unable to create a thread.  pthread_create() returned " << threadError);
    }
  }

  // Now, the parent thread calls this->SingleMethod() itself
  this->ThreadInfoArray[0].UserData = this->SingleData;
  this->ThreadInfoArray[0].NumberOfThreads = this->NumberOfThreads;
  this->SingleMethod((void*)(&this->ThreadInfoArray[0]));

  // The parent thread has finished this->SingleMethod() - so now it
  // waits for each of the other processes to exit
  for (thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++)
  {
    pthread_join(process_id[thread_loop], nullptr);
  }
#endif

#ifndef VTK_USE_WIN32_THREADS
#ifndef VTK_USE_PTHREADS
  // There is no multi threading, so there is only one thread.
  this->ThreadInfoArray[0].UserData = this->SingleData;
  this->ThreadInfoArray[0].NumberOfThreads = this->NumberOfThreads;
  this->SingleMethod((void*)(&this->ThreadInfoArray[0]));
#endif
#endif
}

void vtkMultiThreader::MultipleMethodExecute()
{
  int thread_loop;

#ifdef VTK_USE_WIN32_THREADS
  DWORD threadId;
  HANDLE process_id[VTK_MAX_THREADS] = {};
#endif

#ifdef VTK_USE_PTHREADS
  pthread_t process_id[VTK_MAX_THREADS] = {};
#endif

  // obey the global maximum number of threads limit
  if (vtkMultiThreaderGlobalMaximumNumberOfThreads &&
    this->NumberOfThreads > vtkMultiThreaderGlobalMaximumNumberOfThreads)
  {
    this->NumberOfThreads = vtkMultiThreaderGlobalMaximumNumberOfThreads;
  }

  for (thread_loop = 0; thread_loop < this->NumberOfThreads; thread_loop++)
  {
    if (this->MultipleMethod[thread_loop] == (vtkThreadFunctionType)nullptr)
    {
      vtkErrorMacro(<< "No multiple method set for: " << thread_loop);
      return;
    }
  }

#ifdef VTK_USE_WIN32_THREADS
  // Using CreateThread on Windows
  //
  // We want to use CreateThread to start this->NumberOfThreads - 1
  // additional threads which will be used to call the NumberOfThreads-1
  // methods defined in this->MultipleMethods[](). The parent thread
  // will call this->MultipleMethods[NumberOfThreads-1]().  When it is done,
  // it will wait for all the children to finish.
  //
  // First, start up the this->NumberOfThreads-1 processes.  Keep track
  // of their process ids for use later in the waitid call
  for (thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++)
  {
    this->ThreadInfoArray[thread_loop].UserData = this->MultipleData[thread_loop];
    this->ThreadInfoArray[thread_loop].NumberOfThreads = this->NumberOfThreads;
    process_id[thread_loop] = CreateThread(nullptr, 0, this->MultipleMethod[thread_loop],
      ((void*)(&this->ThreadInfoArray[thread_loop])), 0, &threadId);
    if (process_id[thread_loop] == nullptr)
    {
      vtkErrorMacro("Error in thread creation !!!");
    }
  }

  // Now, the parent thread calls the last method itself
  this->ThreadInfoArray[0].UserData = this->MultipleData[0];
  this->ThreadInfoArray[0].NumberOfThreads = this->NumberOfThreads;
  (this->MultipleMethod[0])((void*)(&this->ThreadInfoArray[0]));

  // The parent thread has finished its method - so now it
  // waits for each of the other threads to exit
  for (thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++)
  {
    WaitForSingleObject(process_id[thread_loop], INFINITE);
  }

  // close the threads
  for (thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++)
  {
    CloseHandle(process_id[thread_loop]);
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

  pthread_attr_init(&attr);
#ifndef __CYGWIN__
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
#endif

  for (thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++)
  {
    this->ThreadInfoArray[thread_loop].UserData = this->MultipleData[thread_loop];
    this->ThreadInfoArray[thread_loop].NumberOfThreads = this->NumberOfThreads;
    pthread_create(&(process_id[thread_loop]), &attr,
      reinterpret_cast<vtkExternCThreadFunctionType>(this->MultipleMethod[thread_loop]),
      ((void*)(&this->ThreadInfoArray[thread_loop])));
  }

  // Now, the parent thread calls the last method itself
  this->ThreadInfoArray[0].UserData = this->MultipleData[0];
  this->ThreadInfoArray[0].NumberOfThreads = this->NumberOfThreads;
  (this->MultipleMethod[0])((void*)(&this->ThreadInfoArray[0]));

  // The parent thread has finished its method - so now it
  // waits for each of the other processes to exit
  for (thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++)
  {
    pthread_join(process_id[thread_loop], nullptr);
  }
#endif

#ifndef VTK_USE_WIN32_THREADS
#ifndef VTK_USE_PTHREADS
  // There is no multi threading, so there is only one thread.
  this->ThreadInfoArray[0].UserData = this->MultipleData[0];
  this->ThreadInfoArray[0].NumberOfThreads = this->NumberOfThreads;
  (this->MultipleMethod[0])((void*)(&this->ThreadInfoArray[0]));
#endif
#endif
}

int vtkMultiThreader::SpawnThread(vtkThreadFunctionType f, void* userdata)
{
  int id;

  for (id = 0; id < VTK_MAX_THREADS; id++)
  {
    if (this->SpawnedThreadActiveFlagLock[id] == nullptr)
    {
      this->SpawnedThreadActiveFlagLock[id] = new std::mutex;
    }
    std::lock_guard<std::mutex>(*this->SpawnedThreadActiveFlagLock[id]);
    if (this->SpawnedThreadActiveFlag[id] == 0)
    {
      // We've got a usable thread id, so grab it
      this->SpawnedThreadActiveFlag[id] = 1;
      break;
    }
  }

  if (id >= VTK_MAX_THREADS)
  {
    vtkErrorMacro(<< "You have too many active threads!");
    return -1;
  }

  this->SpawnedThreadInfoArray[id].UserData = userdata;
  this->SpawnedThreadInfoArray[id].NumberOfThreads = 1;
  this->SpawnedThreadInfoArray[id].ActiveFlag = &this->SpawnedThreadActiveFlag[id];
  this->SpawnedThreadInfoArray[id].ActiveFlagLock = this->SpawnedThreadActiveFlagLock[id];

#ifdef VTK_USE_WIN32_THREADS
  // Using CreateThread on Windows
  //
  DWORD threadId;
  this->SpawnedThreadProcessID[id] =
    CreateThread(nullptr, 0, f, ((void*)(&this->SpawnedThreadInfoArray[id])), 0, &threadId);
  if (this->SpawnedThreadProcessID[id] == nullptr)
  {
    vtkErrorMacro("Error in thread creation !!!");
  }
#endif

#ifdef VTK_USE_PTHREADS
  // Using POSIX threads
  //
  pthread_attr_t attr;
  pthread_attr_init(&attr);
#ifndef __CYGWIN__
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
#endif

  pthread_create(&(this->SpawnedThreadProcessID[id]), &attr,
    reinterpret_cast<vtkExternCThreadFunctionType>(f),
    ((void*)(&this->SpawnedThreadInfoArray[id])));

#endif

#ifndef VTK_USE_WIN32_THREADS
#ifndef VTK_USE_PTHREADS
  // There is no multi threading, so there is only one thread.
  // This won't work - so give an error message.
  vtkErrorMacro(<< "Cannot spawn thread in a single threaded environment!");
  delete this->SpawnedThreadActiveFlagLock[id];
  id = -1;
#endif
#endif

  return id;
}

void vtkMultiThreader::TerminateThread(int threadID)
{
  // check if the threadID argument is in range
  if (threadID >= VTK_MAX_THREADS)
  {
    vtkErrorMacro("ThreadID is out of range. Must be less that " << VTK_MAX_THREADS);
    return;
  }

  // If we don't have a lock, then this thread is definitely not active
  if (!this->SpawnedThreadActiveFlag[threadID])
  {
    return;
  }

  // If we do have a lock, use it and find out the status of the active flag
  int val = 0;
  {
    std::lock_guard<std::mutex>(*this->SpawnedThreadActiveFlagLock[threadID]);
    val = this->SpawnedThreadActiveFlag[threadID];
  }

  // If the active flag is 0, return since this thread is not active
  if (val == 0)
  {
    return;
  }

  // OK - now we know we have an active thread - set the active flag to 0
  // to indicate to the thread that it should terminate itself
  {
    std::lock_guard<std::mutex>(*this->SpawnedThreadActiveFlagLock[threadID]);
    this->SpawnedThreadActiveFlag[threadID] = 0;
  }

#ifdef VTK_USE_WIN32_THREADS
  WaitForSingleObject(this->SpawnedThreadProcessID[threadID], INFINITE);
  CloseHandle(this->SpawnedThreadProcessID[threadID]);
#endif

#ifdef VTK_USE_PTHREADS
  pthread_join(this->SpawnedThreadProcessID[threadID], nullptr);
#endif

#ifndef VTK_USE_WIN32_THREADS
#ifndef VTK_USE_PTHREADS
  // There is no multi threading, so there is only one thread.
  // This won't work - so give an error message.
  vtkErrorMacro(<< "Cannot terminate thread in single threaded environment!");
#endif
#endif

  delete this->SpawnedThreadActiveFlagLock[threadID];
  this->SpawnedThreadActiveFlagLock[threadID] = nullptr;
}

//----------------------------------------------------------------------------
vtkMultiThreaderIDType vtkMultiThreader::GetCurrentThreadID()
{
#if defined(VTK_USE_PTHREADS)
  return pthread_self();
#elif defined(VTK_USE_WIN32_THREADS)
  return GetCurrentThreadId();
#else
  // No threading implementation.  Assume all callers are in the same
  // thread.
  return 0;
#endif
}

vtkTypeBool vtkMultiThreader::IsThreadActive(int threadID)
{
  // check if the threadID argument is in range
  if (threadID >= VTK_MAX_THREADS)
  {
    vtkErrorMacro("ThreadID is out of range. Must be less that " << VTK_MAX_THREADS);
    return 0;
  }

  // If we don't have a lock, then this thread is not active
  if (this->SpawnedThreadActiveFlagLock[threadID] == nullptr)
  {
    return 0;
  }

  // We have a lock - use it to get the active flag value
  int val = 0;
  {
    std::lock_guard<std::mutex>(*this->SpawnedThreadActiveFlagLock[threadID]);
    val = this->SpawnedThreadActiveFlag[threadID];
  }

  // now return that value
  return val;
}

//----------------------------------------------------------------------------
vtkTypeBool vtkMultiThreader::ThreadsEqual(vtkMultiThreaderIDType t1, vtkMultiThreaderIDType t2)
{
#if defined(VTK_USE_PTHREADS)
  return pthread_equal(t1, t2) != 0;
#elif defined(VTK_USE_WIN32_THREADS)
  return t1 == t2;
#else
  // No threading implementation.  Assume all callers are in the same
  // thread.
  return 1;
#endif
}

// Print method for the multithreader
void vtkMultiThreader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Thread Count: " << this->NumberOfThreads << "\n";
  os << indent
     << "Global Maximum Number Of Threads: " << vtkMultiThreaderGlobalMaximumNumberOfThreads
     << endl;
  os << "Thread system used: "
     <<
#ifdef VTK_USE_PTHREADS
    "PTHREADS"
#elif defined VTK_USE_WIN32_THREADS
    "WIN32 Threads"
#else
    "NO THREADS SUPPORT"
#endif
     << endl;
}
