/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCriticalSection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCriticalSection.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkCriticalSection);

// New for the SimpleCriticalSection
vtkSimpleCriticalSection *vtkSimpleCriticalSection::New()
{
  return new vtkSimpleCriticalSection;
}

void vtkSimpleCriticalSection::Init()
{
#ifdef VTK_USE_SPROC
  init_lock( &this->CritSec );
#endif

#ifdef VTK_USE_WIN32_THREADS
  //this->MutexLock = CreateMutex( NULL, FALSE, NULL );
  InitializeCriticalSection(&this->CritSec);
#endif

#ifdef VTK_USE_PTHREADS
#ifdef VTK_HP_PTHREADS
  pthread_mutex_init(&(this->CritSec), pthread_mutexattr_default);
#else
  pthread_mutex_init(&(this->CritSec), NULL);
#endif
#endif
}


// Destruct the vtkMutexVariable
vtkSimpleCriticalSection::~vtkSimpleCriticalSection()
{
#ifdef VTK_USE_WIN32_THREADS
  //CloseHandle(this->MutexLock);
  DeleteCriticalSection(&this->CritSec);
#endif

#ifdef VTK_USE_PTHREADS
  pthread_mutex_destroy( &this->CritSec);
#endif
}

// Lock the vtkCriticalSection
void vtkSimpleCriticalSection::Lock()
{
#ifdef VTK_USE_SPROC
  spin_lock( &this->CritSec );
#endif

#ifdef VTK_USE_WIN32_THREADS
  //WaitForSingleObject( this->MutexLock, INFINITE );
  EnterCriticalSection(&this->CritSec);
#endif

#ifdef VTK_USE_PTHREADS
  pthread_mutex_lock( &this->CritSec);
#endif
}

// Unlock the vtkCriticalSection
void vtkSimpleCriticalSection::Unlock()
{
#ifdef VTK_USE_SPROC
  release_lock( &this->CritSec );
#endif

#ifdef VTK_USE_WIN32_THREADS
  //ReleaseMutex( this->MutexLock );
  LeaveCriticalSection(&this->CritSec);
#endif

#ifdef VTK_USE_PTHREADS
  pthread_mutex_unlock( &this->CritSec);
#endif
}

void vtkCriticalSection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

