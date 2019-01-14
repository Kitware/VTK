/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAtomic.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAtomic.h"

namespace detail
{

vtkTypeInt64 AtomicOps<8>::AddAndFetch(vtkTypeInt64 *ref, vtkTypeInt64 val)
{
  vtkTypeInt64 result;
# pragma omp atomic capture
  {
    (*ref) += val;
    result = *ref;
  }
# pragma omp flush
  return result;
}

vtkTypeInt64 AtomicOps<8>::SubAndFetch(vtkTypeInt64 *ref, vtkTypeInt64 val)
{
  vtkTypeInt64 result;
# pragma omp atomic capture
  {
    (*ref) -= val;
    result = *ref;
  }
# pragma omp flush
  return result;
}

vtkTypeInt64 AtomicOps<8>::PreIncrement(vtkTypeInt64 *ref)
{
  vtkTypeInt64 result;
# pragma omp atomic capture
  result = ++(*ref);
# pragma omp flush
  return result;
}

vtkTypeInt64 AtomicOps<8>::PreDecrement(vtkTypeInt64 *ref)
{
  vtkTypeInt64 result;
# pragma omp atomic capture
  result = --(*ref);
# pragma omp flush
  return result;
}

vtkTypeInt64 AtomicOps<8>::PostIncrement(vtkTypeInt64 *ref)
{
  vtkTypeInt64 result;
# pragma omp atomic capture
  result = (*ref)++;
# pragma omp flush
  return result;
}

vtkTypeInt64 AtomicOps<8>::PostDecrement(vtkTypeInt64 *ref)
{
  vtkTypeInt64 result;
# pragma omp atomic capture
  result = (*ref)--;
# pragma omp flush
  return result;
}

vtkTypeInt64 AtomicOps<8>::Load(const vtkTypeInt64 *ref)
{
  vtkTypeInt64 result;
# pragma omp flush
# pragma omp atomic read
  result = *ref;
  return result;
}

void AtomicOps<8>::Store(vtkTypeInt64 *ref, vtkTypeInt64 val)
{
# pragma omp atomic write
  *ref = val;
# pragma omp flush
}


vtkTypeInt32 AtomicOps<4>::AddAndFetch(vtkTypeInt32 *ref, vtkTypeInt32 val)
{
  vtkTypeInt32 result;
# pragma omp atomic capture
  {
    (*ref) += val;
    result = *ref;
  }
# pragma omp flush
  return result;
}

vtkTypeInt32 AtomicOps<4>::SubAndFetch(vtkTypeInt32 *ref, vtkTypeInt32 val)
{
  vtkTypeInt32 result;
# pragma omp atomic capture
  {
    (*ref) -= val;
    result = *ref;
  }
# pragma omp flush
  return result;
}

vtkTypeInt32 AtomicOps<4>::PreIncrement(vtkTypeInt32 *ref)
{
  vtkTypeInt32 result;
# pragma omp atomic capture
  result = ++(*ref);
# pragma omp flush
  return result;
}

vtkTypeInt32 AtomicOps<4>::PreDecrement(vtkTypeInt32 *ref)
{
  vtkTypeInt32 result;
# pragma omp atomic capture
  result = --(*ref);
# pragma omp flush
  return result;
}

vtkTypeInt32 AtomicOps<4>::PostIncrement(vtkTypeInt32 *ref)
{
  vtkTypeInt32 result;
# pragma omp atomic capture
  result = (*ref)++;
# pragma omp flush
  return result;
}

vtkTypeInt32 AtomicOps<4>::PostDecrement(vtkTypeInt32 *ref)
{
  vtkTypeInt32 result;
# pragma omp atomic capture
  result = (*ref)--;
# pragma omp flush
  return result;
}

vtkTypeInt32 AtomicOps<4>::Load(const vtkTypeInt32 *ref)
{
  vtkTypeInt32 result;
# pragma omp flush
# pragma omp atomic read
  result = *ref;
  return result;
}

void AtomicOps<4>::Store(vtkTypeInt32 *ref, vtkTypeInt32 val)
{
# pragma omp atomic write
  *ref = val;
# pragma omp flush
}

}
