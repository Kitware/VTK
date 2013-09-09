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

#include "vtkConfigure.h"
#include "vtkWindows.h"

#if defined(__APPLE__)
  #include <libkern/OSAtomic.h>
#endif

#if defined(_WIN32) && (VTK_SIZEOF_VOID_P == 8)
# define VTK_HAS_ATOMIC64
#elif defined(__APPLE__)
# if !defined(__ppc__)
#  define VTK_HAS_ATOMIC64
# endif
#elif defined(VTK_HAVE_SYNC_BUILTINS)
# define VTK_HAS_ATOMIC64
#endif

void vtkAtomicInt64Set(vtkTypeInt64* value,
                       vtkTypeInt64 newval,
                       vtkSimpleCriticalSection* cs)
{
  (void)cs;

#if defined(VTK_HAS_ATOMIC64)

# if defined(_WIN32)
  InterlockedExchange64(value, newval);

# elif defined(__APPLE__)
  OSAtomicCompareAndSwap64Barrier(*value, newval, value);

// GCC, CLANG, etc
# elif defined(VTK_HAVE_SYNC_BUILTINS)
  __sync_val_compare_and_swap(value, *value, newval);

# endif

// General case
#else
  cs->Lock();
  *value = newval;
  cs->Unlock();

#endif
}

int vtkAtomicInt64Get(vtkTypeInt64* value, vtkSimpleCriticalSection* cs)
{
  (void)cs;

#if defined(VTK_HAS_ATOMIC64)

# if defined(_WIN32)
  vtkTypeInt64 retval;
  InterlockedExchange64(&retval, *value);
  return retval;

# elif defined(__APPLE__)
  vtkTypeInt64 retval = 0;
  OSAtomicCompareAndSwap64Barrier(retval, *value, &retval);
  return retval;

// GCC, CLANG, etc
# elif defined(VTK_HAVE_SYNC_BUILTINS)
  vtkTypeInt64 retval = 0;
  __sync_val_compare_and_swap(&retval, retval, *value);
  return retval;

# endif

#else
  vtkTypeInt64 val;
  cs->Lock();
  val = *value;
  cs->Unlock();
  return val;

#endif
}

vtkTypeInt64 vtkAtomicInt64Increment(vtkTypeInt64* value,
                                     vtkSimpleCriticalSection* cs)
{
  (void)cs;

#if defined(VTK_HAS_ATOMIC64)

# if defined(_WIN32)
  return InterlockedIncrement64(value);

# elif defined(__APPLE__)
  return OSAtomicIncrement64Barrier(value);

// GCC, CLANG, etc
# elif defined(VTK_HAVE_SYNC_BUILTINS)
  return __sync_add_and_fetch(value, 1);

# endif

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

#if defined(VTK_HAS_ATOMIC64)

# if defined(_WIN32)
#  if defined(VTK_HAS_INTERLOCKEDADD)
  return InterlockedAdd64(value, val);
#  else
  return InterlockedExchangeAdd64(value, val) + val;
#  endif

# elif defined(__APPLE__)
  return OSAtomicAdd64Barrier(val, value);

// GCC, CLANG, etc
# elif defined(VTK_HAVE_SYNC_BUILTINS)
  return __sync_add_and_fetch(value, val);

# endif

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

#if defined(VTK_HAS_ATOMIC64)

# if defined(_WIN32)
  return InterlockedDecrement64(value);

# elif defined(__APPLE__)
  return OSAtomicDecrement64Barrier(value);

// GCC, CLANG, etc
# elif defined(VTK_HAVE_SYNC_BUILTINS)
  return __sync_sub_and_fetch(value, 1);

# endif

#else
  vtkTypeInt64 val;
  cs->Lock();
  val = --(*value);
  cs->Unlock();
  return val;

#endif
}

#if defined (_WIN32) && !defined(__MINGW32__)
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
#if defined(VTK_HAS_ATOMIC64)
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
