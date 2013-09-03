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

#include "vtkAtomicInt64BasicImpl.h"

vtkAtomicInt64::~vtkAtomicInt64()
{
  delete this->Internal;
  this->Internal = 0;
}

void vtkAtomicInt64::Initialize(const vtkTypeInt64 val)
{
  this->Internal = new vtkAtomicInt64Internal;
  this->Internal->Value = val;
}

void vtkAtomicInt64::Set(vtkTypeInt64 value)
{
  vtkAtomicInt64Set(&this->Internal->Value,
                    value,
                    this->Internal->AtomicInt64CritSec);
}

vtkTypeInt64 vtkAtomicInt64::Get() const
{
  return vtkAtomicInt64Get(&this->Internal->Value,
                           this->Internal->AtomicInt64CritSec);
}

vtkTypeInt64 vtkAtomicInt64::Increment()
{
  return vtkAtomicInt64Increment(
    &this->Internal->Value,
    this->Internal->AtomicInt64CritSec);
}

vtkTypeInt64 vtkAtomicInt64::Add(vtkTypeInt64 val)
{
  return vtkAtomicInt64Add(
    &this->Internal->Value,
    val,
    this->Internal->AtomicInt64CritSec);
}

vtkTypeInt64 vtkAtomicInt64::Decrement()
{
  return vtkAtomicInt64Decrement(
    &this->Internal->Value,
    this->Internal->AtomicInt64CritSec);
}
