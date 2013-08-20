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

#include "vtkConfigure.h"
#include "vtkWindows.h"

#if defined(__APPLE__)
  #include <libkern/OSAtomic.h>
#endif

int vtkAtomicInt32Increment(int* value, vtkSimpleCriticalSection* cs)
{
  (void)cs;

#if defined(_WIN32) && defined(VTK_HAS_INTERLOCKEDADD)
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

#if defined(_WIN32) && defined(VTK_HAS_INTERLOCKEDADD)
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

#if defined(_WIN32) && defined(VTK_HAS_INTERLOCKEDADD)
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

#ifdef _WIN32
# define __align32 __declspec(align(32))
#else
# define __align32
#endif

struct vtkAtomicInt32Internal
{
  // Explicitely aligning Value on Windows is probably not necessary
  // since the compiler should automatically do it. Just being extra
  // cautious since the InterlockedXXX() functions require alignment.
  __align32 int Value;

  vtkSimpleCriticalSection* AtomicInt32CritSec;

  vtkAtomicInt32Internal()
    {
#if (defined(_WIN32) && defined(VTK_HAS_INTERLOCKEDADD)) || defined(__APPLE__) || defined(VTK_HAVE_SYNC_BUILTINS)
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
