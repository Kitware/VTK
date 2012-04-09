#include "vtkConditionVariable.h"

#include "vtkObjectFactory.h"

#include <errno.h>

vtkStandardNewMacro(vtkConditionVariable);

#ifndef EPERM
#  define EPERM 1
#endif
#ifndef ENOMEM
#  define ENOMEM 12
#endif
#ifndef EBUSY
#  define EBUSY 16
#endif
#ifndef EINVAL
#  define EINVAL 22
#endif
#ifndef EAGAIN
#  define EAGAIN 35
#endif

#if ! defined(VTK_USE_PTHREADS) && ! defined(VTK_HP_PTHREADS) && ! defined(VTK_USE_WIN32_THREADS)
// Why is this encapsulated in a namespace?  Because you can get errors if
// these symbols (particularly the typedef) are already defined.  We run
// into this problem on a system that has pthread headers but no libraries
// (which can happen when, for example, cross compiling).  By using the
// namespace, we will at worst get a warning.
namespace {

typedef int pthread_condattr_t;

int pthread_cond_init( vtkConditionType* cv, const pthread_condattr_t* )
{
  *cv = 0;
  return 0;
}

int pthread_cond_destroy( vtkConditionType* cv )
{
  if ( *cv )
    return EBUSY;
  return 0;
}

int pthread_cond_signal( vtkConditionType* cv )
{
  *cv = 1;
  return 0;
}

int pthread_cond_broadcast( vtkConditionType* cv )
{
  *cv = 1;
  return 0;
}

int pthread_cond_wait( vtkConditionType* cv, vtkMutexType* lock )
{
#ifdef VTK_USE_SPROC
  release_lock( lock );
#else // VTK_USE_SPROC
  *lock = 0;
#endif // VTK_USE_SPROC
  while ( ! *cv );
#ifdef VTK_USE_SPROC
  spin_lock( lock );
#else // VTK_USE_SPROC
  *lock = 1;
#endif // VTK_USE_SPROC
  return 0;
}

}
#endif // ! defined(VTK_USE_PTHREADS) && ! defined(VTK_HP_PTHREADS) && ! defined(VTK_USE_WIN32_THREADS)

#ifdef VTK_USE_WIN32_THREADS
typedef int pthread_condattr_t;

#  if 1
int pthread_cond_init( pthread_cond_t* cv, const pthread_condattr_t* )
{
  cv->WaitingThreadCount = 0;
  cv->WasBroadcast = 0;
  cv->Semaphore = CreateSemaphore(
    NULL,       // no security
    0,          // initially 0
    0x7fffffff, // max count
    NULL );     // unnamed
  InitializeCriticalSection( &cv->WaitingThreadCountCritSec );
  cv->DoneWaiting = CreateEvent(
    NULL,   // no security
    FALSE,  // auto-reset
    FALSE,  // non-signaled initially
    NULL ); // unnamed

  return 0;
}

int pthread_cond_wait( pthread_cond_t* cv, vtkMutexType* externalMutex )
{
  // Avoid race conditions.
  EnterCriticalSection( &cv->WaitingThreadCountCritSec );
  ++ cv->WaitingThreadCount;
  LeaveCriticalSection( &cv->WaitingThreadCountCritSec );

  // This call atomically releases the mutex and waits on the
  // semaphore until <pthread_cond_signal> or <pthread_cond_broadcast>
  // are called by another thread.
  SignalObjectAndWait( *externalMutex, cv->Semaphore, INFINITE, FALSE );

  // Reacquire lock to avoid race conditions.
  EnterCriticalSection( &cv->WaitingThreadCountCritSec );

  // We're no longer waiting...
  -- cv->WaitingThreadCount;

  // Check to see if we're the last waiter after <pthread_cond_broadcast>.
  int last_waiter = cv->WasBroadcast && cv->WaitingThreadCount == 0;

  LeaveCriticalSection( &cv->WaitingThreadCountCritSec );

  // If we're the last waiter thread during this particular broadcast
  // then let all the other threads proceed.
  if ( last_waiter )
    {
    // This call atomically signals the <DoneWaiting> event and waits until
    // it can acquire the <externalMutex>.  This is required to ensure fairness.
    SignalObjectAndWait( cv->DoneWaiting, *externalMutex, INFINITE, FALSE );
    }
  else
    {
    // Always regain the external mutex since that's the guarantee we
    // give to our callers.
    WaitForSingleObject( *externalMutex, INFINITE );
    }
  return 0;
}

int pthread_cond_signal( pthread_cond_t* cv )
{
  EnterCriticalSection( &cv->WaitingThreadCountCritSec );
  int have_waiters = cv->WaitingThreadCount > 0;
  LeaveCriticalSection( &cv->WaitingThreadCountCritSec );

  // If there aren't any waiters, then this is a no-op.
  if ( have_waiters )
    {
    ReleaseSemaphore( cv->Semaphore, 1, 0 );
    }
  return 0;
}

int pthread_cond_broadcast( pthread_cond_t* cv )
{
  // This is needed to ensure that <WaitingThreadCount> and <WasBroadcast> are
  // consistent relative to each other.
  EnterCriticalSection( &cv->WaitingThreadCountCritSec );
  int have_waiters = 0;

  if ( cv->WaitingThreadCount > 0 )
    {
    // We are broadcasting, even if there is just one waiter...
    // Record that we are broadcasting, which helps optimize
    // pthread_cond_wait for the non-broadcast case.
    cv->WasBroadcast = 1;
    have_waiters = 1;
    }

  if (have_waiters)
    {
    // Wake up all the waiters atomically.
    ReleaseSemaphore( cv->Semaphore, cv->WaitingThreadCount, 0 );
    LeaveCriticalSection( &cv->WaitingThreadCountCritSec );

    // Wait for all the awakened threads to acquire the counting semaphore.
    WaitForSingleObject( cv->DoneWaiting, INFINITE );
    // This assignment is okay, even without the <WaitingThreadCountCritSec> held
    // because no other waiter threads can wake up to access it.
    cv->WasBroadcast = 0;
    }
  else
    {
    LeaveCriticalSection( &cv->WaitingThreadCountCritSec );
    }

  return 0;
}

int pthread_cond_destroy( pthread_cond_t* cv )
{
  DeleteCriticalSection( &cv->WaitingThreadCountCritSec );
  CloseHandle( cv->Semaphore );
  //CloseHandle( cv->Event );
  if ( cv->WaitingThreadCount > 0 && ! cv->DoneWaiting )
    {
    return EBUSY;
    }
  return 0;
}
#  else // 0
int pthread_cond_init( pthread_cond_t* cv, const pthread_condattr_t * )
{
  if ( ! cv )
    {
    return EINVAL;
    }

  cv->WaitingThreadCount = 0;
  cv->NotifyCount = 0;
  cv->ReleaseCount = 0;

  // Create a manual-reset event.
  cv->Event = CreateEvent(
    NULL,   // no security
    TRUE,   // manual-reset
    FALSE,  // non-signaled initially
    NULL ); // unnamed

  InitializeCriticalSection( &cv->WaitingThreadCountCritSec );

  return 0;
}

int pthread_cond_wait( pthread_cond_t* cv, vtkMutexType* externalMutex )
{
  // Avoid race conditions.
  EnterCriticalSection( &cv->WaitingThreadCountCritSec );

  // Increment count of waiters.
  ++ cv->WaitingThreadCount;

  // Store the notification we should respond to.
  int tmpNotify = cv->NotifyCount;

  LeaveCriticalSection( &cv->WaitingThreadCountCritSec );
  ReleaseMutex( *externalMutex );

  while ( 1 )
    {
    // Wait until the event is signaled.
    WaitForSingleObject( cv->Event, INFINITE );

    EnterCriticalSection( &cv->WaitingThreadCountCritSec );
    // Exit the loop when cv->Event is signaled, the
    // release count indicates more threads need to receive
    // the signal/broadcast, and the signal occurred after
    // we started waiting.
    int waitDone =
      ( cv->ReleaseCount > 0 ) &&
      ( cv->NotifyCount != tmpNotify );
    LeaveCriticalSection( &cv->WaitingThreadCountCritSec );

    if ( waitDone )
      break;
    }

  WaitForSingleObject( *externalMutex, INFINITE );
  EnterCriticalSection( &cv->WaitingThreadCountCritSec );
  -- cv->WaitingThreadCount;
  -- cv->ReleaseCount;
  int lastWaiter = ( cv->ReleaseCount == 0 );
  LeaveCriticalSection( &cv->WaitingThreadCountCritSec );

  // If we're the last waiter to be notified, reset the manual event.
  if ( lastWaiter )
    ResetEvent( cv->Event );

  return 0;
}

int pthread_cond_signal( pthread_cond_t* cv )
{
  EnterCriticalSection( &cv->WaitingThreadCountCritSec );
  if ( cv->WaitingThreadCount > cv->ReleaseCount )
    {
    SetEvent( cv->Event ); // Signal the manual-reset event.
    ++ cv->ReleaseCount;
    ++ cv->NotifyCount;
    }
  LeaveCriticalSection( &cv->WaitingThreadCountCritSec );
  return 0;
}

int pthread_cond_broadcast( pthread_cond_t* cv )
{
  EnterCriticalSection( &cv->WaitingThreadCountCritSec );
  if ( cv->WaitingThreadCount > 0 )
    {
    SetEvent( cv->Event );
    // Release all the threads in this generation.
    cv->ReleaseCount = cv->WaitingThreadCount;
    ++ cv->NotifyCount;
    }
  LeaveCriticalSection( &cv->WaitingThreadCountCritSec );
  return 0;
}

int pthread_cond_destroy( pthread_cond_t* cv )
{
  if ( cv->WaitingThreadCount > 0 )
    {
    return EBUSY;
    }
  CloseHandle( cv->Event );
  DeleteCriticalSection( &cv->WaitingThreadCountCritSec );
  return 0;
}
#  endif // 0
#endif // VTK_USE_WIN32_THREADS

vtkSimpleConditionVariable::vtkSimpleConditionVariable()
{
  int result = pthread_cond_init( &this->ConditionVariable, 0 );
  switch ( result )
    {
  case EINVAL:
      {
      vtkGenericWarningMacro( "Invalid condition variable attributes." );
      }
    break;
  case ENOMEM:
      {
      vtkGenericWarningMacro( "Not enough memory to create a condition variable." );
      }
    break;
  case EAGAIN:
      {
      vtkGenericWarningMacro( "Temporarily not enough memory to create a condition variable." );
      }
    break;
    }
}

vtkSimpleConditionVariable::~vtkSimpleConditionVariable()
{
  int result = pthread_cond_destroy( &this->ConditionVariable );
  switch ( result )
    {
  case EINVAL:
      {
      vtkGenericWarningMacro( "Could not destroy condition variable (invalid value)" );
      }
    break;
  case EBUSY:
      {
      vtkGenericWarningMacro( "Could not destroy condition variable (locked by another thread)" );
      }
    break;
    }
}

void vtkSimpleConditionVariable::Signal()
{
  pthread_cond_signal( &this->ConditionVariable );
}

void vtkSimpleConditionVariable::Broadcast()
{
  pthread_cond_broadcast( &this->ConditionVariable );
}

int vtkSimpleConditionVariable::Wait( vtkSimpleMutexLock& lock )
{
  return pthread_cond_wait( &this->ConditionVariable, &lock.MutexLock );
}

void vtkConditionVariable::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "SimpleConditionVariable: " << &this->SimpleConditionVariable << "\n";
  os << indent << "ThreadingModel: "
#ifdef VTK_USE_PTHREADS
    << "pthreads "
#endif
#ifdef VTK_HP_PTHREADS
    << "HP pthreads "
#endif
#ifdef VTK_USE_WIN32_THREADS
    << "win32 threads "
#endif
    << "\n";
}

