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
  this->Set(val);
}

void vtkAtomicInt32::Set(int value)
{
  this->Internal->Value = value;
}

int vtkAtomicInt32::Get() const
{
  return this->Internal->Value;
}

int vtkAtomicInt32::Increment()
{
#if defined(_OPENMP) && _OPENMP >= 201107
  int val;
#pragma omp atomic capture
  val = ++this->Internal->Value;
  return val;

#else
  return vtkAtomicInt32Increment(
    &this->Internal->Value,
    this->Internal->AtomicInt32CritSec);

#endif
}

int vtkAtomicInt32::Add(int val)
{
#if defined(_OPENMP) && _OPENMP >= 201107
  int val2;
#pragma omp atomic capture
  { this->Internal->Value += val; val2 = this->Internal->Value; }
  return val2;

#else
  return vtkAtomicInt32Add(
    &this->Internal->Value,
    val,
    this->Internal->AtomicInt32CritSec);

#endif
}

int vtkAtomicInt32::Decrement()
{
#if defined(_OPENMP) && _OPENMP >= 201107
  int val;
#pragma omp atomic capture
  val = --this->Internal->Value;
  return val;

#else
  return vtkAtomicInt32Decrement(
    &this->Internal->Value,
    this->Internal->AtomicInt32CritSec);

#endif
}
