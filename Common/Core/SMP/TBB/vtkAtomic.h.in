 /*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAtomic.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAtomic -
// .SECTION Description

#ifndef vtkAtomic_h
#define vtkAtomic_h

#include "vtkAtomicTypeConcepts.h"

#ifdef _MSC_VER
#  pragma push_macro("__TBB_NO_IMPLICIT_LINKAGE")
#  define __TBB_NO_IMPLICIT_LINKAGE 1
#endif

#include <tbb/atomic.h>

#ifdef _MSC_VER
#  pragma pop_macro("__TBB_NO_IMPLICIT_LINKAGE")
#endif

#include <cstddef>


template <typename T> class vtkAtomic : private vtk::atomic::detail::IntegralType<T>
{
public:
  vtkAtomic()
  {
    this->Atomic = 0;
  }

  vtkAtomic(T val)
  {
    this->Atomic = val;
  }

  vtkAtomic(const vtkAtomic<T> &atomic)
  {
    this->Atomic = atomic.Atomic;
  }

  T operator++()
  {
    return ++this->Atomic;
  }

  T operator++(int)
  {
    return this->Atomic++;
  }

  T operator--()
  {
    return --this->Atomic;
  }

  T operator--(int)
  {
    return this->Atomic--;
  }

  T operator+=(T val)
  {
    return this->Atomic += val;
  }

  T operator-=(T val)
  {
    return this->Atomic -= val;
  }

  operator T() const
  {
    return this->Atomic;
  }

  T operator=(T val)
  {
    this->Atomic = val;
    return val;
  }

  vtkAtomic<T>& operator=(const vtkAtomic<T> &atomic)
  {
    this->Atomic = atomic.Atomic;
    return *this;
  }

  T load() const
  {
    return this->Atomic;
  }

  void store(T val)
  {
    this->Atomic = val;
  }

private:
  tbb::atomic<T> Atomic;
};


template <typename T> class vtkAtomic<T*>
{
public:
  vtkAtomic()
  {
    this->Atomic = 0;
  }

  vtkAtomic(T* val)
  {
    this->Atomic = val;
  }

  vtkAtomic(const vtkAtomic<T*> &atomic)
  {
    this->Atomic = atomic.Atomic;
  }

  T* operator++()
  {
    return ++this->Atomic;
  }

  T* operator++(int)
  {
    return this->Atomic++;
  }

  T* operator--()
  {
    return --this->Atomic;
  }

  T* operator--(int)
  {
    return this->Atomic--;
  }

  T* operator+=(std::ptrdiff_t val)
  {
    return this->Atomic += val;
  }

  T* operator-=(std::ptrdiff_t val)
  {
    return this->Atomic -= val;
  }

  operator T*() const
  {
    return this->Atomic;
  }

  T* operator=(T* val)
  {
    this->Atomic = val;
    return val;
  }

  vtkAtomic<T*>& operator=(const vtkAtomic<T*> &atomic)
  {
    this->Atomic = atomic.Atomic;
    return *this;
  }

  T* load() const
  {
    return this->Atomic;
  }

  void store(T* val)
  {
    this->Atomic = val;
  }

private:
  tbb::atomic<T*> Atomic;
};


template <> class vtkAtomic<void*>
{
public:
  vtkAtomic()
  {
    this->Atomic = 0;
  }

  vtkAtomic(void* val)
  {
    this->Atomic = val;
  }

  vtkAtomic(const vtkAtomic<void*> &atomic)
  {
    this->Atomic = atomic.Atomic;
  }

  operator void*() const
  {
    return this->Atomic;
  }

  void* operator=(void* val)
  {
    this->Atomic = val;
    return val;
  }

  vtkAtomic<void*>& operator=(const vtkAtomic<void*> &atomic)
  {
    this->Atomic = atomic.Atomic;
    return *this;
  }

  void* load() const
  {
    return this->Atomic;
  }

  void store(void* val)
  {
    this->Atomic = val;
  }

private:
  tbb::atomic<void*> Atomic;
};

#endif
// VTK-HeaderTest-Exclude: vtkAtomic.h
