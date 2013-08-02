/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAtomicInt32BasicImpl.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCriticalSection.h"

#include "vtkWindows.h"

#if defined(__APPLE__)
  #include <libkern/OSAtomic.h>
#endif

int vtkAtomicInt32Increment(int* value, vtkSimpleCriticalSection* cs)
{
  (void)cs;

#if defined(WIN32) || defined(_WIN32)
  return InterlockedIncrement((long*)value);

#elif defined(__APPLE__)
  return OSAtomicIncrement32(value);

// GCC, CLANG, etc
#elif defined(VTK_HAVE_SYNC_BUILTINS)
  return __sync_add_and_fetch(value, 1);

// General case
#else
  int val;
  cs->Lock();
  val = ++(*value);
  cs->Unlock();
  return val;

#endif

}

int vtkAtomicInt32Add(int* value, int val, vtkSimpleCriticalSection* cs)
{
  (void)cs;

#if defined(WIN32) || defined(_WIN32)
  return InterlockedAdd((long*)value, val);

#elif defined(__APPLE__)
  return OSAtomicAdd32(val, value);

// GCC, CLANG, etc
#elif defined(VTK_HAVE_SYNC_BUILTINS)
  return __sync_add_and_fetch(value, val);

#else
  int val2;
  cs->Lock();
  val2 = (*value += val);
  cs->Unlock();
  return val2;

#endif

}

int vtkAtomicInt32Decrement(int* value, vtkSimpleCriticalSection* cs)
{
  (void)cs;

#if defined(WIN32) || defined(_WIN32)
  return InterlockedDecrement((long*)value);

#elif defined(__APPLE__)
  return OSAtomicDecrement32(value);

// GCC, CLANG, etc
#elif defined(VTK_HAVE_SYNC_BUILTINS)
  return __sync_sub_and_fetch(value, 1);

#else
  int val;
  cs->Lock();
  val = --(*value);
  cs->Unlock();
  return val;

#endif
}

struct vtkAtomicInt32Internal
{
  int Value;

  vtkSimpleCriticalSection* AtomicInt32CritSec;

  vtkAtomicInt32Internal()
    {
#if defined(WIN32) || defined(_WIN32) || defined(__APPLE__) || defined(VTK_HAVE_SYNC_BUILTINS)
      this->AtomicInt32CritSec = 0;
#else
      this->AtomicInt32CritSec = new vtkSimpleCriticalSection;
#endif
    }

  ~vtkAtomicInt32Internal()
    {
      delete this->AtomicInt32CritSec;
    }
};
// VTK-HeaderTest-Exclude: vtkAtomicInt32BasicImpl.h
