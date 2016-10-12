/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArrayAccessor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkDataArrayAccessor
 * @brief   Efficient templated access to vtkDataArray.
 *
 *
 * vtkDataArrayAccessor provides access to data stored in a vtkDataArray. It
 * is intended to be used in conjunction with vtkArrayDispatcher.
 *
 * A more detailed description of this class and related tools can be found
 * \ref VTK-7-1-ArrayDispatch "here".
 *
 * The goal of this helper template is to allow developers to write a single
 * templated worker function that will generates code to use the efficient typed
 * APIs provided by vtkGenericDataArray when the array type is known, but
 * fallback to the slower vtkDataArray virtual double API if needed.
 *
 * This can be used to reduce template-explosion issues by restricting the
 * vtkArrayDispatch call to only dispatch a few common cases/array types, and
 * route all other arrays through a slower implementation using vtkDataArray's
 * API. With vtkDataArrayAccessor, a single templated worker function can be
 * used to generate both.
 *
 * Note that while this interface provides both component-wise and
 * tuple access, the tuple methods are discouraged as they are
 * significantly slower as they copy data into a temporary array, and
 * prevent many advanced compiler optimizations that are possible when
 * using the component accessors. In other words, prefer the methods
 * that operate on a single component instead of an entire tuple when
 * performance matters.
 *
 * A standard usage pattern of this class would be:
 *
 * @code
 * // vtkArrayDispatch worker struct:
 * struct Worker
 * {
 *   // Templated worker function:
 *   template <typename ArrayT>
 *   void operator()(ArrayT *array)
 *   {
 *     // The accessor:
 *     vtkDataArrayAccessor<ArrayT> accessor(array);
 *     // The data type used by ArrayT's API, use this for
 *     // temporary/intermediate results:
 *     typedef typename vtkDataArrayAccessor<ArrayT>::APIType APIType;
 *
 *     // Do work using accessor to set/get values....
 *   }
 * };
 *
 * // Usage:
 * Worker worker;
 * vtkDataArray *array = ...;
 * if (!vtkArrayDispatch::Dispatch::Execute(array, worker))
 *   {
 *   // Dispatch failed: unknown array type. Fallback to vtkDataArray API:
 *   worker(array);
 *   }
 * @endcode
 *
 * We define Worker::operator() as the templated worker function, restricting
 * all data accesses to go through the 'accessor' object (methods like
 * GetNumberOfTuples, GetNumberOfComponents, etc should be called on the array
 * itself).
 *
 * This worker is passed into an array dispatcher, which tests 'array' to see
 * if it can figure out the array subclass. If it does, Worker is instantiated
 * with ArrayT set to the array's subclass, resulting in efficient code. If
 * Dispatch::Execute returns false (meaning the array type is not recognized),
 * the worker is executed using the vtkDataArray pointer. While slower, this
 * ensures that less-common cases will still be handled -- all from one worker
 * function template.
 *
 * .SEE ALSO
 * vtkArrayDispatch
*/

#include "vtkDataArray.h"
#include "vtkGenericDataArray.h"

#ifndef vtkDataArrayAccessor_h
#define vtkDataArrayAccessor_h

// Generic form for all (non-bit) vtkDataArray subclasses.
template <typename ArrayT>
struct vtkDataArrayAccessor
{
  typedef ArrayT ArrayType;
  typedef typename ArrayType::ValueType APIType;

  ArrayType *Array;

  vtkDataArrayAccessor(ArrayType *array) : Array(array) {}

  inline APIType Get(vtkIdType tupleIdx, int compIdx) const
  {
    return this->Array->GetTypedComponent(tupleIdx, compIdx);
  }

  inline void Set(vtkIdType tupleIdx, int compIdx, APIType val) const
  {
    this->Array->SetTypedComponent(tupleIdx, compIdx, val);
  }

  inline void Insert(vtkIdType tupleIdx, int compIdx, APIType val) const
  {
    this->Array->InsertTypedComponent(tupleIdx, compIdx, val);
  }

  inline void Get(vtkIdType tupleIdx, APIType *tuple) const
  {
    this->Array->GetTypedTuple(tupleIdx, tuple);
  }

  inline void Set(vtkIdType tupleIdx, const APIType *tuple) const
  {
    this->Array->SetTypedTuple(tupleIdx, tuple);
  }

  inline void Insert(vtkIdType tupleIdx, const APIType *tuple) const
  {
    this->Array->InsertTypedTuple(tupleIdx, tuple);
  }
};

// Specialization for vtkDataArray.
template <>
struct vtkDataArrayAccessor<vtkDataArray>
{
  typedef vtkDataArray ArrayType;
  typedef double APIType;

  ArrayType *Array;

  vtkDataArrayAccessor(ArrayType *array) : Array(array) {}

  inline APIType Get(vtkIdType tupleIdx, int compIdx) const
  {
    return this->Array->GetComponent(tupleIdx, compIdx);
  }

  inline void Set(vtkIdType tupleIdx, int compIdx, APIType val) const
  {
    this->Array->SetComponent(tupleIdx, compIdx, val);
  }

  inline void Insert(vtkIdType tupleIdx, int compIdx, APIType val) const
  {
    this->Array->InsertComponent(tupleIdx, compIdx, val);
  }

  inline void Get(vtkIdType tupleIdx, APIType *tuple) const
  {
    this->Array->GetTuple(tupleIdx, tuple);
  }

  inline void Set(vtkIdType tupleIdx, const APIType *tuple) const
  {
    this->Array->SetTuple(tupleIdx, tuple);
  }

  inline void Insert(vtkIdType tupleIdx, const APIType *tuple) const
  {
    this->Array->InsertTuple(tupleIdx, tuple);
  }
};

#endif // vtkDataArrayAccessor_h
// VTK-HeaderTest-Exclude: vtkDataArrayAccessor.h
