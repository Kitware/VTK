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

#include "vtkAtomicInt32.h"

#include <tbb/atomic.h>

struct vtkAtomicInt32Internal
{
  tbb::atomic<int> AtomicInt;
};

vtkAtomicInt32::~vtkAtomicInt32()
{
  delete this->Internal;
  this->Internal = 0;
}

void vtkAtomicInt32::Initialize(const int val)
{
  this->Internal = new vtkAtomicInt32Internal;
  this->Set(val);
}

void vtkAtomicInt32::Set(int value)
{
  this->Internal->AtomicInt = value;
}

int vtkAtomicInt32::Get() const
{
  return this->Internal->AtomicInt;
}

int vtkAtomicInt32::Increment()
{
 return ++this->Internal->AtomicInt;
}

int vtkAtomicInt32::Add(int val)
{
  return (this->Internal->AtomicInt += val);
}

int vtkAtomicInt32::Decrement()
{
  return --this->Internal->AtomicInt;
}
