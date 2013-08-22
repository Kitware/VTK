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
  this->Set(val);
}

void vtkAtomicInt64::Set(vtkTypeInt64 value)
{
#if defined(_OPENMP) && _OPENMP >= 201107
#pragma omp atomic write
  this->Internal->Value = value;
#else
  this->Internal->Value = value;
#endif
}

vtkTypeInt64 vtkAtomicInt64::Get() const
{
#if defined(_OPENMP) && _OPENMP >= 201107
  vtkTypeInt64 val;
#pragma omp flush
#pragma omp atomic read
  val = this->Internal->Value;
  return val;
#else
  return this->Internal->Value;
#endif
}

vtkTypeInt64 vtkAtomicInt64::Increment()
{
#if defined(_OPENMP) && _OPENMP >= 201107
  vtkTypeInt64 val;
#pragma omp atomic capture
  val = ++this->Internal->Value;
  return val;

#else
  return vtkAtomicInt64Increment(
    &this->Internal->Value,
    this->Internal->AtomicInt64CritSec);

#endif
}

vtkTypeInt64 vtkAtomicInt64::Add(vtkTypeInt64 val)
{
#if defined(_OPENMP) && _OPENMP >= 201107
  vtkTypeInt64 val2;
#pragma omp atomic capture
  { this->Internal->Value += val; val2 = this->Internal->Value; }
  return val2;

#else
  return vtkAtomicInt64Add(
    &this->Internal->Value,
    val,
    this->Internal->AtomicInt64CritSec);

#endif

}

vtkTypeInt64 vtkAtomicInt64::Decrement()
{
#if defined(_OPENMP) && _OPENMP >= 201107
  vtkTypeInt64 val;
#pragma omp atomic capture
  val = --this->Internal->Value;
  return val;

#else
  return vtkAtomicInt64Decrement(
    &this->Internal->Value,
    this->Internal->AtomicInt64CritSec);

#endif
}
