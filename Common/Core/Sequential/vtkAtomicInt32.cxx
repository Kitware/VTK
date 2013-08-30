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

#include "vtkAtomicInt32BasicImpl.h"

vtkAtomicInt32::~vtkAtomicInt32()
{
  delete this->Internal;
  this->Internal = 0;
}

void vtkAtomicInt32::Initialize(const int val)
{
  this->Internal = new vtkAtomicInt32Internal;
  this->Internal->Value = val;
}

void vtkAtomicInt32::Set(int value)
{
  vtkAtomicInt32Set(&this->Internal->Value,
                    value,
                    this->Internal->AtomicInt32CritSec);
}

int vtkAtomicInt32::Get() const
{
  return vtkAtomicInt32Get(&this->Internal->Value,
                           this->Internal->AtomicInt32CritSec);
}

int vtkAtomicInt32::Increment()
{
  return vtkAtomicInt32Increment(
    &this->Internal->Value,
    this->Internal->AtomicInt32CritSec);

}

int vtkAtomicInt32::Add(int val)
{
  return vtkAtomicInt32Add(
    &this->Internal->Value,
    val,
    this->Internal->AtomicInt32CritSec);
}

int vtkAtomicInt32::Decrement()
{
  return vtkAtomicInt32Decrement(
    &this->Internal->Value,
    this->Internal->AtomicInt32CritSec);
}
