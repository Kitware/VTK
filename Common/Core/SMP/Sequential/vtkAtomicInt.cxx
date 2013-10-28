/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAtomicInt.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAtomicInt.h"

#if !defined(VTK_HAS_ATOMIC64) || !defined(VTK_HAS_ATOMIC32)
# include "vtkSimpleCriticalSection.h"
#elif defined(VTK_WINDOWS_ATOMIC)
# include "vtkWindows.h"
#endif

namespace detail
{

#if !defined(VTK_HAS_ATOMIC32)
vtkAtomicIntImpl<vtkTypeInt32>::vtkAtomicIntImpl()
{
  this->AtomicInt32CritSec = new vtkSimpleCriticalSection;
}

vtkAtomicIntImpl<vtkTypeInt32>::~vtkAtomicIntImpl()
{
  delete this->AtomicInt32CritSec;
}

vtkTypeInt32 vtkAtomicIntImpl<vtkTypeInt32>::operator++()
{
  if (!this->AtomicInt32CritSec)
    {
    return 0;
    }
  vtkTypeInt32 val;
  this->AtomicInt32CritSec->Lock();
  val = ++this->Value;
  this->AtomicInt32CritSec->Unlock();
  return val;
}

vtkTypeInt32 vtkAtomicIntImpl<vtkTypeInt32>::operator--()
{
  if (!this->AtomicInt32CritSec)
    {
    return 0;
    }
  vtkTypeInt32 val;
  this->AtomicInt32CritSec->Lock();
  val = --this->Value;
  this->AtomicInt32CritSec->Unlock();
  return val;
}

vtkTypeInt32 vtkAtomicIntImpl<vtkTypeInt32>::operator+=(vtkTypeInt32 val)
{
  if (!this->AtomicInt32CritSec)
    {
    return 0;
    }
  vtkTypeInt32 val2;
  this->AtomicInt32CritSec->Lock();
  val2 = (this->Value += val);
  this->AtomicInt32CritSec->Unlock();
  return val2;
}

vtkTypeInt32 vtkAtomicIntImpl<vtkTypeInt32>::load() const
{
  if (!this->AtomicInt32CritSec)
    {
    return 0;
    }
  vtkTypeInt32 val;
  this->AtomicInt32CritSec->Lock();
  val = this->Value;
  this->AtomicInt32CritSec->Unlock();
  return val;
}

void vtkAtomicIntImpl<vtkTypeInt32>::store(vtkTypeInt32 val)
{
  if (!this->AtomicInt32CritSec)
    {
    return;
    }
  this->AtomicInt32CritSec->Lock();
  this->Value = val;
  this->AtomicInt32CritSec->Unlock();
}
#elif defined (VTK_WINDOWS_ATOMIC)
vtkAtomicIntImpl<vtkTypeInt32>::vtkAtomicIntImpl()
{
}

vtkAtomicIntImpl<vtkTypeInt32>::~vtkAtomicIntImpl()
{
}

vtkTypeInt32 vtkAtomicIntImpl<vtkTypeInt32>::operator++()
{
  return InterlockedIncrement((long*)&this->Value);
}

vtkTypeInt32 vtkAtomicIntImpl<vtkTypeInt32>::operator--()
{
  return InterlockedDecrement((long*)&this->Value);
}

vtkTypeInt32 vtkAtomicIntImpl<vtkTypeInt32>::operator+=(vtkTypeInt32 val)
{
# if defined(VTK_HAS_INTERLOCKEDADD)
  return InterlockedAdd((long*)&this->Value, val);
# else
  return InterlockedExchangeAdd((long*)&this->Value, val) + val;
# endif
}

vtkTypeInt32 vtkAtomicIntImpl<vtkTypeInt32>::load() const
{
  long retval;
  InterlockedExchange(&retval, this->Value);
  return retval;
}

void vtkAtomicIntImpl<vtkTypeInt32>::store(vtkTypeInt32 val)
{
  InterlockedExchange((long*)&this->Value, val);
}
#endif // !defined(VTK_HAS_ATOMIC32)

#if !defined(VTK_HAS_ATOMIC64)
vtkAtomicIntImpl<vtkTypeInt64>::vtkAtomicIntImpl()
{
  this->AtomicInt64CritSec = new vtkSimpleCriticalSection;
}

vtkAtomicIntImpl<vtkTypeInt64>::~vtkAtomicIntImpl()
{
  delete this->AtomicInt64CritSec;
}

vtkTypeInt64 vtkAtomicIntImpl<vtkTypeInt64>::operator++()
{
  if (!this->AtomicInt64CritSec)
    {
    return 0;
    }
  vtkTypeInt64 val;
  this->AtomicInt64CritSec->Lock();
  val = ++this->Value;
  this->AtomicInt64CritSec->Unlock();
  return val;
}


vtkTypeInt64 vtkAtomicIntImpl<vtkTypeInt64>::operator--()
{
  if (!this->AtomicInt64CritSec)
    {
    return 0;
    }
  vtkTypeInt64 val;
  this->AtomicInt64CritSec->Lock();
  val = --this->Value;
  this->AtomicInt64CritSec->Unlock();
  return val;
}

vtkTypeInt64 vtkAtomicIntImpl<vtkTypeInt64>::operator+=(vtkTypeInt64 val)
{
  if (!this->AtomicInt64CritSec)
    {
    return 0;
    }
  vtkTypeInt64 val2;
  this->AtomicInt64CritSec->Lock();
  val2 = (this->Value += val);
  this->AtomicInt64CritSec->Unlock();
  return val2;
}

vtkTypeInt64 vtkAtomicIntImpl<vtkTypeInt64>::load() const
{
  if (!this->AtomicInt64CritSec)
    {
    return 0;
    }
  vtkTypeInt64 val;
  this->AtomicInt64CritSec->Lock();
  val = this->Value;
  this->AtomicInt64CritSec->Unlock();
  return val;
}

void vtkAtomicIntImpl<vtkTypeInt64>::store(vtkTypeInt64 val)
{
  if (!this->AtomicInt64CritSec)
    {
    return;
    }
  this->AtomicInt64CritSec->Lock();
  this->Value = val;
  this->AtomicInt64CritSec->Unlock();
}
#elif defined (VTK_WINDOWS_ATOMIC)
vtkAtomicIntImpl<vtkTypeInt64>::vtkAtomicIntImpl()
{
}

vtkAtomicIntImpl<vtkTypeInt64>::~vtkAtomicIntImpl()
{
}

vtkTypeInt64 vtkAtomicIntImpl<vtkTypeInt64>::operator++()
{
  return InterlockedIncrement64(&this->Value);
}


vtkTypeInt64 vtkAtomicIntImpl<vtkTypeInt64>::operator--()
{
  return InterlockedDecrement64(&this->Value);
}

vtkTypeInt64 vtkAtomicIntImpl<vtkTypeInt64>::operator+=(vtkTypeInt64 val)
{
# if defined(VTK_HAS_INTERLOCKEDADD)
  return InterlockedAdd64(&this->Value, val);
# else
  return InterlockedExchangeAdd64(&this->Value, val) + val;
# endif
}

vtkTypeInt64 vtkAtomicIntImpl<vtkTypeInt64>::load() const
{
  vtkTypeInt64 retval;
  InterlockedExchange64(&retval, this->Value);
  return retval;
}

void vtkAtomicIntImpl<vtkTypeInt64>::store(vtkTypeInt64 val)
{
  InterlockedExchange64(&this->Value, val);
}
#endif // !defined(VTK_HAS_ATOMIC64)
}
