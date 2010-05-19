/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreadMessager.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkThreadMessager.h"

#include "vtkObjectFactory.h"

#ifdef VTK_USE_WIN32_THREADS
# include "vtkWindows.h"
#endif

vtkStandardNewMacro(vtkThreadMessager);

vtkThreadMessager::vtkThreadMessager()
{
#ifdef VTK_USE_WIN32_THREADS
  this->WSignal = CreateEvent(0, FALSE, FALSE, 0);
#elif defined(VTK_USE_PTHREADS) || defined(VTK_HP_PTHREADS)
  pthread_cond_init(&this->PSignal, 0);
  pthread_mutex_init(&this->Mutex, 0);
  pthread_mutex_lock(&this->Mutex);
#endif
}

vtkThreadMessager::~vtkThreadMessager()
{
#ifdef VTK_USE_WIN32_THREADS
  CloseHandle(this->WSignal);
#elif defined(VTK_USE_PTHREADS) || defined(VTK_HP_PTHREADS)
  pthread_mutex_unlock(&this->Mutex);
  pthread_mutex_destroy(&this->Mutex);
  pthread_cond_destroy(&this->PSignal);
#endif
}

void vtkThreadMessager::WaitForMessage()
{
#ifdef VTK_USE_WIN32_THREADS
  WaitForSingleObject( this->WSignal, INFINITE );
#elif defined(VTK_USE_PTHREADS) || defined(VTK_HP_PTHREADS)
  pthread_cond_wait(&this->PSignal, &this->Mutex);
#endif
}

//----------------------------------------------------------------------------
void vtkThreadMessager::SendWakeMessage()
{
#ifdef VTK_USE_WIN32_THREADS
  SetEvent( this->WSignal );
#elif defined(VTK_USE_PTHREADS) || defined(VTK_HP_PTHREADS)
  pthread_cond_broadcast(&this->PSignal);
#endif
}

void vtkThreadMessager::EnableWaitForReceiver()
{
#if defined(VTK_USE_PTHREADS) || defined(VTK_HP_PTHREADS)
  pthread_mutex_lock(&this->Mutex);
#endif
}

void vtkThreadMessager::WaitForReceiver()
{
#if defined(VTK_USE_PTHREADS) || defined(VTK_HP_PTHREADS)
  pthread_mutex_lock(&this->Mutex);
#endif
}

void vtkThreadMessager::DisableWaitForReceiver()
{
#if defined(VTK_USE_PTHREADS) || defined(VTK_HP_PTHREADS)
  pthread_mutex_unlock(&this->Mutex);
#endif
}

void vtkThreadMessager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent); 
}

//----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
# ifdef VTK_WORKAROUND_WINDOWS_MANGLE
#  undef SendMessage
void vtkThreadMessager::SendMessageA()
{
  VTK_LEGACY_REPLACED_BODY(vtkThreadMessager::SendMessage, "VTK 5.0",
                           vtkThreadMessager::SendWakeMessage);
  this->SendWakeMessage();
}
void vtkThreadMessager::SendMessageW()
{
  VTK_LEGACY_REPLACED_BODY(vtkThreadMessager::SendMessage, "VTK 5.0",
                           vtkThreadMessager::SendWakeMessage);
  this->SendWakeMessage();
}
# endif
void vtkThreadMessager::SendMessage()
{
  VTK_LEGACY_REPLACED_BODY(vtkThreadMessager::SendMessage, "VTK 5.0",
                           vtkThreadMessager::SendWakeMessage);
  this->SendWakeMessage();
}
#endif
