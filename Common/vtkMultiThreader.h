/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiThreader.h
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
// .NAME vtkMultiThreader - A class for performing multithreaded execution
// .SECTION Description
// vtkMultithreader is a class that provides support for multithreaded
// execution using sproc() on an SGI, or pthread_create on any platform
// supporting POSIX threads.  This class can be used to execute a single
// method on multiple threads, or to specify a method per thread. 

#ifndef __vtkMultiThreader_h
#define __vtkMultiThreader_h

#include "vtkObject.h"
#include "vtkMutexLock.h"

#ifdef VTK_USE_SPROC
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef VTK_USE_PTHREADS
#include <sys/types.h>
#include <unistd.h>
#endif

// If VTK_USE_SPROC is defined, then sproc() will be used to create
// multiple threads on an SGI. If VTK_USE_PTHREADS is defined, then
// pthread_create() will be used to create multiple threads (on
// a sun, for example)

// The maximum number of threads allowed
#ifdef VTK_USE_SPROC
#define VTK_MAX_THREADS              32
#endif

#ifdef VTK_USE_PTHREADS
#define VTK_MAX_THREADS              32
#endif


#ifdef VTK_USE_WIN32_THREADS
#define VTK_MAX_THREADS              8
#endif

#ifndef VTK_USE_WIN32_THREADS
#ifndef VTK_USE_SPROC
#ifndef VTK_USE_PTHREADS
#define VTK_MAX_THREADS              1
#endif
#endif
#endif

// If VTK_USE_PTHREADS is defined, then the multithreaded
// function is of type void *, and returns NULL
// Otherwise the type is void which is correct for WIN32
// and SPROC
//BTX
#ifdef VTK_USE_SPROC
typedef int vtkThreadProcessIDType;
#endif

#ifdef VTK_USE_PTHREADS
typedef void *(*vtkThreadFunctionType)(void *);
typedef pthread_t vtkThreadProcessIDType;
#define VTK_THREAD_RETURN_VALUE  NULL
#define VTK_THREAD_RETURN_TYPE   void *
#endif

#ifdef VTK_USE_WIN32_THREADS
typedef LPTHREAD_START_ROUTINE vtkThreadFunctionType;
typedef HANDLE vtkThreadProcessIDType;
#define VTK_THREAD_RETURN_VALUE 0
#define VTK_THREAD_RETURN_TYPE DWORD __stdcall
#endif

#if !defined(VTK_USE_PTHREADS) && !defined(VTK_USE_WIN32_THREADS)
typedef void (*vtkThreadFunctionType)(void *);
typedef int vtkThreadProcessIDType;
#define VTK_THREAD_RETURN_VALUE
#define VTK_THREAD_RETURN_TYPE void
#endif
//ETX

// Description:
// This is the structure that is passed to the thread that is
// created from the SingleMethodExecute, MultipleMethodExecute or
// the SpawnThread method. It is passed in as a void *, and it is
// up to the method to cast correctly and extract the information.
// The ThreadID is a number between 0 and NumberOfThreads-1 that indicates
// the id of this thread. The NumberOfThreads is this->NumberOfThreads for
// threads created from SingleMethodExecute or MultipleMethodExecute,
// and it is 1 for threads created from SpawnThread.
// The UserData is the (void *)arg passed into the SetSingleMethod,
// SetMultipleMethod, or SpawnThread method.

//BTX
struct ThreadInfoStruct
{
  int                 ThreadID;
  int                 NumberOfThreads;
  int                 *ActiveFlag;
  vtkMutexLock        *ActiveFlagLock;
  void                *UserData;
};
//ETX

class VTK_EXPORT vtkMultiThreader : public vtkObject 
{
public:
  static vtkMultiThreader *New();

  vtkTypeMacro(vtkMultiThreader,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the number of threads to create. It will be clamped to the range
  // 1 - VTK_MAX_THREADS, so the caller of this method should check that the
  // requested number of threads was accepted.
  vtkSetClampMacro( NumberOfThreads, int, 1, VTK_MAX_THREADS );
  vtkGetMacro( NumberOfThreads, int );

  // Description:
  // Set/Get the maximum number of threads to use when multithreading.
  // This limits and overrides any other settings for multithreading.
  // A value of zero indicates no limit.
  static void SetGlobalMaximumNumberOfThreads(int val);
  static int  GetGlobalMaximumNumberOfThreads();

  // Description:
  // Set/Get the value which is used to initialize the NumberOfThreads
  // in the constructor.  Initially this default is set to the number of 
  // processors or 8 (which ever is less).
  static void SetGlobalDefaultNumberOfThreads(int val);
  static int  GetGlobalDefaultNumberOfThreads();

  // These methods are excluded from Tcl wrapping 1) because the
  // wrapper gives up on them and 2) because they really shouldn't be
  // called from a script anyway.
  //BTX 
  
  // Description:
  // Execute the SingleMethod (as define by SetSingleMethod) using
  // this->NumberOfThreads threads.
  void SingleMethodExecute();

  // Description:
  // Execute the MultipleMethods (as define by calling SetMultipleMethod
  // for each of the required this->NumberOfThreads methods) using
  // this->NumberOfThreads threads.
  void MultipleMethodExecute();
  
  // Description:
  // Set the SingleMethod to f() and the UserData field of the
  // ThreadInfoStruct that is passed to it will be data.
  // This method (and all the methods passed to SetMultipleMethod)
  // must be of type vtkThreadFunctionType and must take a single argument of
  // type void *.
  void SetSingleMethod(vtkThreadFunctionType, void *data );
 
  // Description:
  // Set the MultipleMethod at the given index to f() and the UserData 
  // field of the ThreadInfoStruct that is passed to it will be data.
  void SetMultipleMethod( int index, vtkThreadFunctionType, void *data ); 

  // Description:
  // Create a new thread for the given function. Return a thread id
  // which is a number between 0 and VTK_MAX_THREADS - 1. This id should
  // be used to kill the thread at a later time.
  int SpawnThread( vtkThreadFunctionType, void *data );

  // Description:
  // Terminate the thread that was created with a SpawnThreadExecute()
  void TerminateThread( int thread_id );


protected:
  vtkMultiThreader();
  ~vtkMultiThreader();
  vtkMultiThreader(const vtkMultiThreader&);
  void operator=(const vtkMultiThreader&);

  // The number of threads to use
  int                        NumberOfThreads;

  // An array of thread info containing a thread id
  // (0, 1, 2, .. VTK_MAX_THREADS-1), the thread count, and a pointer
  // to void so that user data can be passed to each thread
  ThreadInfoStruct           ThreadInfoArray[VTK_MAX_THREADS];

  // The methods
  vtkThreadFunctionType      SingleMethod;
  vtkThreadFunctionType      MultipleMethod[VTK_MAX_THREADS];

  // Storage of MutexFunctions and ints used to control spawned 
  // threads and the spawned thread ids
  int                        SpawnedThreadActiveFlag[VTK_MAX_THREADS];
  vtkMutexLock               *SpawnedThreadActiveFlagLock[VTK_MAX_THREADS];
  vtkThreadProcessIDType     SpawnedThreadProcessID[VTK_MAX_THREADS];
  ThreadInfoStruct           SpawnedThreadInfoArray[VTK_MAX_THREADS];

//ETX

  // Internal storage of the data
  void                       *SingleData;
  void                       *MultipleData[VTK_MAX_THREADS];

};

#endif





