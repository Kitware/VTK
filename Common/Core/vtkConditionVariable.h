/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConditionVariable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkConditionVariable - mutual exclusion locking class
// .SECTION Description
// vtkConditionVariable allows the locking of variables which are accessed
// through different threads.  This header file also defines
// vtkSimpleConditionVariable which is not a subclass of vtkObject.
//
// The win32 implementation is based on notes provided by
// Douglas C. Schmidt and Irfan Pyarali,
// Department of Computer Science,
// Washington University, St. Louis, Missouri.
// http://www.cs.wustl.edu/~schmidt/win32-cv-1.html

#ifndef __vtkConditionVariable_h
#define __vtkConditionVariable_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

#include "vtkMutexLock.h" // Need for friend access to vtkSimpleMutexLock

//BTX
#if defined(VTK_USE_PTHREADS) || defined(VTK_HP_PTHREADS)
#  include <pthread.h> // Need POSIX thread implementation of mutex (even win32 provides mutexes)
typedef pthread_cond_t vtkConditionType;
#endif


// Typically a top level windows application sets _WIN32_WINNT. If it is not set we set it to
// 0x0501 (Windows XP)
#ifdef VTK_USE_WIN32_THREADS
#  ifndef _WIN32_WINNT
#    define _WIN32_WINNT 0x0501 // 0x0501 means target Windows XP or later
#  endif
#  include "vtkWindows.h" // Needed for win32 CRITICAL_SECTION, HANDLE, etc.
#endif

#ifdef VTK_USE_WIN32_THREADS
#if 1
typedef struct
{
  // Number of threads waiting on condition.
  int WaitingThreadCount;

  // Lock for WaitingThreadCount
  CRITICAL_SECTION WaitingThreadCountCritSec;

  // Semaphore to block threads waiting for the condition to change.
  vtkWindowsHANDLE Semaphore;

  // An event used to wake up thread(s) waiting on the semaphore
  // when pthread_cond_signal or pthread_cond_broadcast is called.
  vtkWindowsHANDLE DoneWaiting;

  // Was pthread_cond_broadcast called?
  size_t WasBroadcast;
} pthread_cond_t;

typedef pthread_cond_t vtkConditionType;
#  else // 0
typedef struct
{
  // Number of threads waiting on condition.
  int WaitingThreadCount;

  // Lock for WaitingThreadCount
  CRITICAL_SECTION WaitingThreadCountCritSec;

  // Number of threads to release when pthread_cond_broadcast()
  // or pthread_cond_signal() is called.
  int ReleaseCount;

  // Used to prevent one thread from decrementing ReleaseCount all
  // by itself instead of letting others respond.
  int NotifyCount;

  // A manual-reset event that's used to block and release waiting threads.
  vtkWindowsHANDLE Event;
} pthread_cond_t;

typedef pthread_cond_t vtkConditionType;
#  endif // 0
#endif // VTK_USE_WIN32_THREADS

#ifndef VTK_USE_PTHREADS
#ifndef VTK_HP_PTHREADS
#ifndef VTK_USE_WIN32_THREADS
typedef int vtkConditionType;
#endif
#endif
#endif

// Condition variable that is not a vtkObject.
class VTKCOMMONCORE_EXPORT vtkSimpleConditionVariable
{
public:
  vtkSimpleConditionVariable();
  ~vtkSimpleConditionVariable();

  static vtkSimpleConditionVariable* New();

  void Delete() { delete this; }

  // Description:
  // Wake one thread waiting for the condition to change.
  void Signal();

  // Description:
  // Wake all threads waiting for the condition to change.
  void Broadcast();

  // Description:
  // Wait for the condition to change.
  // Upon entry, the mutex must be locked and the lock held by the calling thread.
  // Upon exit, the mutex will be locked and held by the calling thread.
  // Between entry and exit, the mutex will be unlocked and may be held by other threads.
  //
  // @param mutex The mutex that should be locked on entry and will be locked on exit (but not in between)
  // @retval Normally, this function returns 0. Should a thread be interrupted by a signal, a non-zero value may be returned.
  int Wait( vtkSimpleMutexLock& mutex );

protected:
  vtkConditionType   ConditionVariable;

private:
  vtkSimpleConditionVariable(const vtkSimpleConditionVariable& other); // no copy constructor
  vtkSimpleConditionVariable& operator=(const vtkSimpleConditionVariable& rhs); // no copy assignment
};

//ETX

class VTKCOMMONCORE_EXPORT vtkConditionVariable : public vtkObject
{
public:
  static vtkConditionVariable* New();
  vtkTypeMacro(vtkConditionVariable,vtkObject);
  void PrintSelf( ostream& os, vtkIndent indent );

  // Description:
  // Wake one thread waiting for the condition to change.
  void Signal();

  // Description:
  // Wake all threads waiting for the condition to change.
  void Broadcast();

  // Description:
  // Wait for the condition to change.
  // Upon entry, the mutex must be locked and the lock held by the calling thread.
  // Upon exit, the mutex will be locked and held by the calling thread.
  // Between entry and exit, the mutex will be unlocked and may be held by other threads.
  //
  // @param mutex The mutex that should be locked on entry and will be locked on exit (but not in between)
  // @retval Normally, this function returns 0. Should a thread be interrupted by a signal, a non-zero value may be returned.
  int Wait( vtkMutexLock* mutex );

protected:
  vtkConditionVariable() { }

  //BTX
  vtkSimpleConditionVariable SimpleConditionVariable;
  //ETX

private:
  vtkConditionVariable( const vtkConditionVariable& ); // Not implemented.
  void operator = ( const vtkConditionVariable& ); // Not implemented.
};

//BTX
inline void vtkConditionVariable::Signal()
{
  this->SimpleConditionVariable.Signal();
}

inline void vtkConditionVariable::Broadcast()
{
  this->SimpleConditionVariable.Broadcast();
}

inline int vtkConditionVariable::Wait( vtkMutexLock* lock )
{
  return this->SimpleConditionVariable.Wait( lock->SimpleMutexLock );
}
//ETX

#endif // __vtkConditionVariable_h
