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

#include "vtkMutexLock.h"

//BTX
#if defined(VTK_USE_PTHREADS) || defined(VTK_HP_PTHREADS)
#include <pthread.h> // Need POSIX thread implementation of mutex (even win32 provides mutexes)
typedef pthread_cond_t vtkConditionType;
#endif

#ifdef VTK_USE_WIN32_THREADS
typedef struct
{
  // Number of threads waiting on condition.
  int WaitingThreadCount;
  //int waiters_count_;

  // Lock for WaitingThreadCount
  CRITICAL_SECTION WaitingThreadCountLock;
  //CRITICAL_SECTION waiters_count_lock_;

  // Semaphore to block threads waiting for the condition to change.
  HANDLE Semaphore;
  //HANDLE sema_;

  // An event used to wake up thread(s) waiting on the semaphore
  // when pthread_cond_signal or pthread_cond_broadcast is called.
  HANDLE DoneWaiting;
  //HANDLE waiters_done_;

  // Was pthread_cond_broadcast called?
  size_t WasBroadcast;
  //size_t was_broadcast_;
} pthread_cond_t;

typedef pthread_cond_t vtkConditionType;
#endif

#ifndef VTK_USE_PTHREADS
#ifndef VTK_HP_PTHREADS
#ifndef VTK_USE_WIN32_THREADS
typedef int vtkConditionType;
#endif
#endif
#endif

// Condition variable that is not a vtkObject.
class VTK_COMMON_EXPORT vtkSimpleConditionVariable
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
  void Wait( vtkSimpleMutexLock& mutex );

protected:
  vtkConditionType   ConditionVariable;
};

//ETX

class VTK_COMMON_EXPORT vtkConditionVariable : public vtkObject
{
public:
  static vtkConditionVariable* New();
  vtkTypeRevisionMacro(vtkConditionVariable,vtkObject);
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
  void Wait( vtkMutexLock* mutex );

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

inline void vtkConditionVariable::Wait( vtkMutexLock* lock )
{
  this->SimpleConditionVariable.Wait( lock->SimpleMutexLock );
}
//ETX

#endif // __vtkConditionVariable_h
