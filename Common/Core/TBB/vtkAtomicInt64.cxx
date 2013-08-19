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

#include <tbb/atomic.h>

struct vtkAtomicInt64Internal
{
  tbb::atomic<vtkTypeInt64> AtomicInt64;
};

void vtkAtomicInt64::Initialize(const vtkTypeInt64 val)
{
  this->Internal = new vtkAtomicInt64Internal;
  this->Set(val);
}

vtkAtomicInt64::~vtkAtomicInt64()
{
  delete this->Internal;
  this->Internal = 0;
}

void vtkAtomicInt64::Set(vtkTypeInt64 value)
{
  this->Internal->AtomicInt64 = value;
}

vtkTypeInt64 vtkAtomicInt64::Get() const
{
  return this->Internal->AtomicInt64;
}

vtkTypeInt64 vtkAtomicInt64::Increment()
{
  return ++this->Internal->AtomicInt64;
}

vtkTypeInt64 vtkAtomicInt64::Add(vtkTypeInt64 val)
{
  return (this->Internal->AtomicInt64 += val);
}

vtkTypeInt64 vtkAtomicInt64::Decrement()
{
  return --this->Internal->AtomicInt64;
}
