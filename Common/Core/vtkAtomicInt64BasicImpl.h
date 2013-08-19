/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAtomicInt64BasicImpl.h

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

vtkTypeInt64 vtkAtomicInt64Increment(vtkTypeInt64* value,
                                     vtkSimpleCriticalSection* cs)
{
  (void)cs;

#if defined(_WIN32)
  return InterlockedIncrement64(value);

#elif defined(__APPLE__)
  return OSAtomicIncrement64(value);

// GCC, CLANG, etc
#elif defined(VTK_HAVE_SYNC_BUILTINS)
  return __sync_add_and_fetch(value, 1);

// General case
#else
  vtkTypeInt64 val;
  cs->Lock();
  val = ++(*value);
  cs->Unlock();
  return val;

#endif

}

vtkTypeInt64 vtkAtomicInt64Add(vtkTypeInt64* value,
                               vtkTypeInt64 val,
                               vtkSimpleCriticalSection* cs)
{
  (void)cs;

#if defined(_WIN32)
  return InterlockedAdd64(value, val);

#elif defined(__APPLE__)
  return OSAtomicAdd64(val, value);

// GCC, CLANG, etc
#elif defined(VTK_HAVE_SYNC_BUILTINS)
  return __sync_add_and_fetch(value, val);

#else
  vtkTypeInt64 val2;
  cs->Lock();
  val2 = (*value += val);
  cs->Unlock();
  return val2;

#endif

}

vtkTypeInt64 vtkAtomicInt64Decrement(vtkTypeInt64* value,
                                     vtkSimpleCriticalSection* cs)
{
  (void)cs;

#if defined(_WIN32)
  return InterlockedDecrement64(value);

#elif defined(__APPLE__)
  return OSAtomicDecrement64(value);

// GCC, CLANG, etc
#elif defined(VTK_HAVE_SYNC_BUILTINS)
  return __sync_sub_and_fetch(value, 1);

#else
  vtkTypeInt64 val;
  cs->Lock();
  val = --(*value);
  cs->Unlock();
  return val;

#endif
}

#ifdef _WIN32
# define __align64 __declspec(align(64))
#else
# define __align64
#endif

struct vtkAtomicInt64Internal
{
  // Explicitely aligning Value on Windows is probably not necessary
  // since the compiler should automatically do it. Just being extra
  // cautious since the InterlockedXXX() functions require alignment.
  __align64 vtkTypeInt64 Value;
  vtkSimpleCriticalSection* AtomicInt64CritSec;

  vtkAtomicInt64Internal()
    {
#if defined(_WIN32) || defined(__APPLE__) || defined(VTK_HAVE_SYNC_BUILTINS)
      this->AtomicInt64CritSec = 0;
#else
      this->AtomicInt64CritSec = new vtkSimpleCriticalSection;
#endif
    }

  ~vtkAtomicInt64Internal()
    {
      delete this->AtomicInt64CritSec;
    }
};
// VTK-HeaderTest-Exclude: vtkAtomicInt64BasicImpl.h
