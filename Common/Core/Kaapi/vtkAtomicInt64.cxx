/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAtomicInt64.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAtomicInt64.h"

#include <stdint.h>
#include <kaapi_atomic.h>

struct vtkAtomicInt64Internal
{
  kaapi_atomic64_t Value;
};

vtkAtomicInt64::~vtkAtomicInt64()
{
  delete this->Internal;
  this->Internal = 0;
}

void vtkAtomicInt64::Initialize(const vtkTypeInt64 val)
{
  this->Internal = new vtkAtomicInt64Internal;
  this->Set(val);
}

void vtkAtomicInt64::Set(vtkTypeInt64 value)
{
  this->Internal->Value._counter = value;
}

vtkTypeInt64 vtkAtomicInt64::Get() const
{
  return this->Internal->Value._counter;
}

vtkTypeInt64 vtkAtomicInt64::Increment()
{
  return KAAPI_ATOMIC_INCR64(&this->Internal->Value);
}

vtkTypeInt64 vtkAtomicInt64::Add(vtkTypeInt64 val)
{
  return KAAPI_ATOMIC_ADD64(&this->Internal->Value, val);
}

vtkTypeInt64 vtkAtomicInt64::Decrement()
{
  return KAAPI_ATOMIC_DECR64(&this->Internal->Value);
}
