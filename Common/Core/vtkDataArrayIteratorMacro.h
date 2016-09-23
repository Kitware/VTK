/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArrayIteratorMacro.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDataArrayIteratorMacro
 * @brief   A macro for obtaining iterators to
 * vtkDataArray data when the array implementation and type are unknown.
 *
 *
 *
 * NOTE: This macro is deprecated and should not be used any longer. Use
 * vtkArrayDispatch and the vtkGenericDataArray API instead of
 * vtkDataArrayIteratorMacro and vtkTypedDataArrayIterator.
 *
 * See vtkTemplateMacro.
 * This macro is similar, but defines several additional typedefs and variables
 * for safely iterating through data in a vtkAbstractArray container:
 *  - vtkDAValueType is typedef'd to the array's element value type.
 *  - vtkDAContainerType is typedef'd to the most derived class of
 *    vtkAbstractArray for which a suitable iterator has been found.
 *  - vtkDAIteratorType is typedef'd to the most suitable iterator type found.
 *  - vtkDABegin is an object of vtkDAIteratorType that points to the first
 *    component of the first tuple in the array.
 *  - vtkDAEnd is an object of vtkDAIteratorType that points to the element
 *    *after* the last component of the last tuple in the array.
 * The primary advantage to using this macro is that arrays with non-standard
 * memory layouts will be safely handled, and dangerous calls to GetVoidPointer
 * are avoided.
 * For arrays with > 1 component, the iterator will proceed through all
 * components of a tuple before moving on to the next tuple.
 * This matches the memory layout of the standard vtkDataArray subclasses such
 * as vtkFloatArray.
 *
 * For the standard vtkDataArray implementations (which are subclasses of
 * vtkAOSDataArrayTemplate), the iterators will simply be pointers to the raw
 * memory of the array.
 * This allows very fast iteration when possible, and permits compiler
 * optimizations in the standard template library to occur (such as reducing
 * std::copy to memmove).
 *
 * For arrays that are subclasses of vtkTypedDataArray, a
 * vtkTypedDataArrayIterator is used.
 * Such iterators safely traverse the array using API calls and have
 * pointer-like semantics, but add about a 35% performance overhead compared
 * with iterating over the raw memory (measured by summing a vtkFloatArray
 * containing 10M values on GCC 4.8.1 with -O3 optimization using both iterator
 * types -- see TestDataArrayIterators).
 *
 * For arrays that are not subclasses of vtkTypedDataArray, there is no reliably
 * safe way to iterate over the array elements.
 * In such cases, this macro performs the legacy behavior of casting
 * vtkAbstractArray::GetVoidPointer(...) to vtkDAValueType* to create the
 * iterators.
 *
 * To use this macro, create a templated worker function:
 *
 * template <class Iterator>
 * void myFunc(Iterator begin, Iterator end, ...) {...}
 *
 * and then call the vtkDataArrayIteratorMacro inside of a switch statement
 * using the above objects and typedefs as needed:
 *
 * vtkAbstractArray *someArray = ...;
 * switch (someArray->GetDataType())
 *   {
 *   vtkDataArrayIteratorMacro(someArray, myFunc(vtkDABegin, vtkDAEnd, ...));
 *   }
 *
 * @sa
 * vtkArrayDispatch vtkGenericDataArray
 * vtkTemplateMacro vtkTypedDataArrayIterator
*/

#ifndef vtkDataArrayIteratorMacro_h
#define vtkDataArrayIteratorMacro_h

#include "vtkAOSDataArrayTemplate.h" // For classes referred to in the macro
#include "vtkSetGet.h" // For vtkTemplateMacro
#include "vtkTypedDataArray.h" // For classes referred to in the macro

// Silence 'unused typedef' warnings on GCC.
// use of the typedef in question depends on the macro
// argument _call and thus should not be removed.
#if defined(__GNUC__)
#define _vtkDAIMUnused __attribute__ ((unused))
#else
#define _vtkDAIMUnused
#endif

#define vtkDataArrayIteratorMacro(_array, _call)                           \
  vtkTemplateMacro(                                                        \
    vtkAbstractArray *_aa(_array);                                         \
    if (vtkAOSDataArrayTemplate<VTK_TT> *_dat =                            \
        vtkAOSDataArrayTemplate<VTK_TT>::FastDownCast(_aa))                \
    {                                                                    \
      typedef VTK_TT vtkDAValueType;                                       \
      typedef vtkAOSDataArrayTemplate<vtkDAValueType> vtkDAContainerType;     \
      typedef vtkDAContainerType::Iterator vtkDAIteratorType;              \
      vtkDAIteratorType vtkDABegin(_dat->Begin());                         \
      vtkDAIteratorType vtkDAEnd(_dat->End());                             \
      (void)vtkDABegin; /* Prevent warnings when unused */                 \
      (void)vtkDAEnd;                                                      \
      _call;                                                               \
    }                                                                    \
    else if (vtkTypedDataArray<VTK_TT> *_tda =                             \
             vtkTypedDataArray<VTK_TT>::FastDownCast(_aa))                 \
    {                                                                    \
      typedef VTK_TT vtkDAValueType;                                       \
      typedef vtkTypedDataArray<vtkDAValueType> vtkDAContainerType;        \
      typedef vtkDAContainerType::Iterator vtkDAIteratorType;              \
      vtkDAIteratorType vtkDABegin(_tda->Begin());                         \
      vtkDAIteratorType vtkDAEnd(_tda->End());                             \
      (void)vtkDABegin;                                                    \
      (void)vtkDAEnd;                                                      \
      _call;                                                               \
    }                                                                    \
    else                                                                   \
    {                                                                    \
      /* This is not ideal, as no explicit iterator has been declared.     \
       * Cast the void pointer and hope for the best! */                   \
      typedef VTK_TT vtkDAValueType;                                       \
      typedef vtkAbstractArray vtkDAContainerType _vtkDAIMUnused;          \
      typedef vtkDAValueType* vtkDAIteratorType;                           \
      vtkDAIteratorType vtkDABegin =                                       \
        static_cast<vtkDAIteratorType>(_aa->GetVoidPointer(0));            \
      vtkDAIteratorType vtkDAEnd = vtkDABegin + _aa->GetMaxId() + 1;       \
      (void)vtkDABegin;                                                    \
      (void)vtkDAEnd;                                                      \
      _call;                                                               \
    }                                                                    \
    )

#endif //vtkDataArrayIteratorMacro_h

// VTK-HeaderTest-Exclude: vtkDataArrayIteratorMacro.h
