/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayDispatch.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkArrayDispatch
 * @brief   vtkDataArray code generator/dispatcher.
 *
 * vtkArrayDispatch implements a mechanism for generating optimized code for
 * multiple subclasses of vtkDataArray at once. Using a TypeList based
 * approach (see vtkTypeList), a templated worker implementation is generated
 * for a restricted or unrestricted set of vtkDataArray subclasses.
 *
 * A more detailed description of this class and related tools can be found
 * \ref VTK-7-1-ArrayDispatch "here".
 *
 * The primary goals of this class are to simplify multi-array dispatch
 * implementations, and provide tools to lower compilation time and binary
 * size (i.e. avoiding 'template explosions').
 *
 * vtkArrayDispatch is also intended to replace code that currently relies on
 * the encapsulation-breaking vtkDataArray::GetVoidPointer method. Not all
 * subclasses of vtkDataArray use the memory layout assumed by GetVoidPointer;
 * calling this method on, e.g. a vtkSOADataArrayTemplate will trigger a deep
 * copy of the array data into an AOS buffer. This is very inefficient and
 * should be avoided.
 *
 * The vtkDataArrayAccessor wrapper is worth mentioning here, as it allows
 * vtkArrayDispatch workers to operate on selected concrete subclasses for
 * 'fast paths', yet fallback to using the slower vtkDataArray API for uncommon
 * array types. This helps mitigate the "template explosion" issues that can
 * result from instantiating a large worker functions for many array types.
 *
 * These dispatchers extend the basic functionality of vtkTemplateMacro with
 * the following features:
 * - Multiarray dispatch: A single call can dispatch up to 3 arrays at once.
 * - Array restriction: The set of valid arrays for a particular dispatch
 *   can be restricted. For example, if only vtkDoubleArray or vtkFloatArray
 *   will be passed into the call, the dispatcher will only generate code paths
 *   that use those arrays.
 * - ValueType restriction: If both SoA and AoS arrays need to be supported,
 *   but only certain ValueTypes are expected, the dispatcher can restrict
 *   itself to only use arrays that match this critera.
 * - Application-wide array restrictions: If a VTK application uses only a few
 *   arraytype / valuetype combinations, certain dispatchers will eliminate
 *   paths using unsupported arrays at compile time.
 *
 * The basic Dispatch implementation will generate code paths for all arrays
 * in the application-wide array list, and operates on a single array.
 *
 * Dispatchers that start with Dispatch2 operate on 2 arrays simultaneously,
 * while those that begin with Dispatch3 operate on 3 arrays.
 *
 * To reduce compile time and binary size, the following dispatchers can be
 * used to restrict the set of arrays that will be used. There are versions of
 * these that operate on 1, 2, or 3 arrays:
 *
 * - DispatchByArray:
 *   Accepts an explicit TypeList of arrays to consider.
 *   These dispatchers do NOT respect the application-wide array restrictions.
 *   Example usecase: A filter that creates either vtkFloatArray or
 *   vtkDoubleArray depending on configuration can use this to restrict itself
 *   to only these specific types.
 *   Note that these should not be used for operating on filter inputs, instead
 *   use DispatchByValueType, which also considers variations in vtkDataArray
 *   subclasses and respects the application-wide array restrictions.
 *
 * - DispatchByValueType:
 *   Accepts an explicit TypeList of ValueTypes to consider.
 *   These dispatchers respect the application-wide array restrictions.
 *   Example usecase: An image filter that operates on an input array that must
 *   have either a float or unsigned char ValueType.
 *
 * - DispatchNByArrayWithSameValueType:
 *   Multiarray dispatcher that accepts an explicit TypeList of arrays for
 *   consideration. Generated code paths are further restricted to enforce that
 *   all dispatched arrays will have the same ValueType.
 *   Example usecase: A filter that creates and operates on multiple arrays at
 *   the same time, where the arrays are guaranteed to have the same ValueType.
 *   Note that these should not be used for operating on filter inputs, instead
 *   use DispatchNBySameValueType, which also considers variations in
 *   vtkDataArray subclasses and respects the application-wide array
 *   restrictions.
 *
 * - DispatchNBySameValueType:
 *   Multiarray dispatcher that accepts an explicit TypeList of ValueTypes to
 *   consider. Generated code paths are further restricted to enforce that
 *   all dispatched arrays will have the same ValueType.
 *   Example usecase: A filter that creates a modified version of an input
 *   array using NewInstance(). Both arrays may be passed into the dispatcher
 *   using a worker function that produces the output from the input.
 *
 * Execution:
 * There are three components to a dispatch: The dispatcher, the worker, and
 * the array(s). They are combined like so:
 *
 * @code
 * bool result = Dispatcher<...>::Execute(array, worker);
 * @endcode
 *
 * The dispatcher can also be instantiated into an object, e.g.:
 *
 * @code
 * vtkArrayDispatch::SomeDispatcher<...> myDispatcher;
 * MyWorker worker;
 * bool result = myDispatcher.Execute(array, worker);
 * @endcode
 *
 * Return value:
 * The Execute method of the dispatcher will return true if a code path matching
 * the array arguments is found, or false if the arrays are not supported. If
 * false is returned, the arrays will not be modified, and the worker will not
 * be executed.
 *
 * Workers:
 * The dispatch requires a Worker functor that performs the work.
 * For single array, the functor must be callable with the array object as an
 * argument. For 2-array dispatch, the arguments must be (array1, array2).
 * For 3-array dispatch, the arguments must be (array1, array2, array3).
 * Workers are passed by reference, so stateful functors are permitted if
 * additional input/output data is needed.
 *
 * A simple worker implementation for triple dispatch:
 * @code
 * struct MyWorker
 * {
 *   template <typename Array1T, typename Array2T, typename Array3T>
 *   void operator()(Array1T *array1, Array2T *array2, Array3T *array3)
 *   {
 *     // Do work using vtkGenericDataArray API...
 *   }
 * };
 * @endcode
 *
 * Note that optimized implementations (e.g. for AoS arrays vs SoA arrays) can
 * be supported by providing overloads of operator() that have more restrictive
 * template parameters.
 *
 * Examples:
 * See TestArrayDispatchers.cxx for examples of each dispatch type.
 *
 * @sa
 * vtkDataArrayAccessor
*/

#ifndef vtkArrayDispatch_h
#define vtkArrayDispatch_h

#include "vtkArrayDispatchArrayList.h"
#include "vtkConfigure.h"
#include "vtkType.h"
#include "vtkTypeList.h"

namespace vtkArrayDispatch {

/**
 * A TypeList containing all real ValueTypes.
 */
typedef vtkTypeList_Create_2(double, float) Reals;

/**
 * A Typelist containing all integral ValueTypes.
 */
typedef vtkTypeList::Unique<
  vtkTypeList_Create_12(char, int, long, long long, short, signed char,
                        unsigned char, unsigned int, unsigned long,
                        unsigned long long, unsigned short, vtkIdType)
  >::Result Integrals;

/**
 * A Typelist containing all standard VTK array ValueTypes.
 */
typedef vtkTypeList::Append<Reals, Integrals>::Result AllTypes;

//------------------------------------------------------------------------------
/**
 * Dispatch a single array against all array types in the application-wide
 * vtkArrayDispatch::Arrays list.
 * The entry point is:
 * bool Dispatch::Execute(vtkDataArray *array, Worker &worker).
 */
struct Dispatch;

//------------------------------------------------------------------------------
/**
 * Dispatch a single array against all array types mentioned in the ArrayList
 * template parameter.
 * The entry point is:
 * bool DispatchByArray<...>::Execute(vtkDataArray *array, Worker &worker).
 */
template <typename ArrayList>
struct DispatchByArray;

//------------------------------------------------------------------------------
/**
 * Dispatch a single array against all array types in the application-wide
 * vtkArrayDispatch::Arrays list with the added restriction that the array
 * must have a type that appears the ValueTypeList TypeList.
 * The entry point is:
 * bool DispatchByValueType<...>::Execute(vtkDataArray *array, Worker &worker).
 */
template <typename ValueTypeList>
struct DispatchByValueType;

//------------------------------------------------------------------------------
/**
 * Dispatch two arrays using all array types in the application-wide
 * vtkArrayDispatch::Arrays list.
 * The entry point is:
 * bool Dispatch2::Execute(vtkDataArray *array, vtkDataArray *array2,
 * Worker &worker).
 */
struct Dispatch2;

//------------------------------------------------------------------------------
/**
 * Dispatch two arrays, restricting the valid code paths to use only arrays that
 * have the same ValueType.
 * All application-wide arrays in vtkArrayDispatch::Arrays are used.
 * The entry point is:
 * bool Dispatch2SameValueType::Execute(
 * vtkDataArray *a1, vtkDataArray *a2, Worker &worker).
 */
struct Dispatch2SameValueType;

//------------------------------------------------------------------------------
/**
 * Dispatch two arrays with the restriction that the type of the first array is
 * in the ArrayList1 TypeList, and the second is in ArrayList2.
 * If all application-wide arrays are desired, use vtkArrayDispatch::Arrays for
 * the first two template parameters.
 * The entry point is:
 * bool Dispatch2ByArray<...>::Execute(vtkDataArray *a1, vtkDataArray *a2,
 * Worker &worker).
 */
template <
    typename ArrayList1,
    typename ArrayList2
    >
struct Dispatch2ByArray;

//------------------------------------------------------------------------------
/**
 * Dispatch two arrays, restricting the valid code paths to use
 * ValueType-filtered versions of the application-wide vtkArrayDispatch::Arrays
 * TypeList. The first array's ValueType must be in the ValueTypeList1 TypeList,
 * and the second's must be in ValueTypeList2.
 * If all types are to be considered, use vtkArrayDispatch::AllTypes for the
 * first two template parameters.
 * The entry point is:
 * bool Dispatch2ByValueType<...>::Execute(vtkDataArray *a1, vtkDataArray *a2,
 * Worker &worker).
 */
template <
    typename ValueTypeList1,
    typename ValueTypeList2
    >
struct Dispatch2ByValueType;

//------------------------------------------------------------------------------
/**
 * Dispatch two arrays, restricting the valid code paths to use only array types
 * specified in the ArrayList TypeList, additionally enforcing that all arrays
 * must have the same ValueType.
 * If all application-wide arrays are desired, use vtkArrayDispatch::Arrays for
 * the first two template parameters.
 * The entry point is:
 * bool Dispatch2ByArrayWithSameValueType<...>::Execute(
 * vtkDataArray *a1, vtkDataArray *a2, Worker &worker).
 */
template <
    typename ArrayList1,
    typename ArrayList2
    >
struct Dispatch2ByArrayWithSameValueType;

//------------------------------------------------------------------------------
/**
 * Dispatch two arrays, restricting the valid code paths to use only array types
 * found in application-wide vtkArrayDispatch::Arrays TypeList that have a
 * ValueType contained in the ValueTypeList TypeList. This dispatcher also
 * enforces that all arrays have the same ValueType.
 * If all types are to be considered, use vtkArrayDispatch::AllTypes for the
 * first two template parameters.
 * The entry point is:
 * bool Dispatch2BySameValueType<...>::Execute(
 * vtkDataArray *a1, vtkDataArray *a2, Worker &worker).
 */
template <typename ValueTypeList>
struct Dispatch2BySameValueType;

//------------------------------------------------------------------------------
/**
 * Dispatch three arrays using all array types in the application-wide
 * vtkArrayDispatch::Arrays list.
 * The entry point is:
 * bool Dispatch3::Execute(vtkDataArray *array1, vtkDataArray *array2,
 * vtkDataArray *array3, Worker &worker).
 */
struct Dispatch3;

//------------------------------------------------------------------------------
/**
 * Dispatch three arrays, restricting the valid code paths to use only arrays
 * that have the same ValueType.
 * All application-wide arrays in vtkArrayDispatch::Arrays are used.
 * The entry point is:
 * bool Dispatch3SameValueType::Execute(
 * vtkDataArray *a1, vtkDataArray *a2, vtkDataArray *a3, Worker &worker).
 */
struct Dispatch3SameValueType;

//------------------------------------------------------------------------------
/**
 * Dispatch three arrays with the restriction that the type of the first array
 * is in the ArrayList1 TypeList, the second is in ArrayList2, and the third
 * is in ArrayList3.
 * If all application-wide arrays are desired, use vtkArrayDispatch::Arrays for
 * the first three template parameters.
 * The entry point is:
 * bool Dispatch3ByArray::Execute<...>(vtkDataArray *a1, vtkDataArray *a2,
 * vtkDataArray *a3, Worker &worker).
 */
template <
    typename ArrayList1,
    typename ArrayList2,
    typename ArrayList3
    >
struct Dispatch3ByArray;

//------------------------------------------------------------------------------
/**
 * Dispatch three arrays, restricting the valid code paths to use
 * ValueType-filtered versions of the application-wide vtkArrayDispatch::Arrays
 * TypeList. The first array's ValueType must be in the ValueTypeList1 TypeList,
 * the second's must be in ValueTypeList2, and the third's must be in
 * ValueTypeList3.
 * If all types are to be considered, use vtkArrayDispatch::AllTypes for the
 * first three template parameters.
 * The entry point is:
 * bool Dispatch3ByValueType<...>::Execute(vtkDataArray *a1, vtkDataArray *a2,
 * vtkDataArray *a3, Worker &worker).
 */
template <
    typename ValueTypeList1,
    typename ValueTypeList2,
    typename ValueTypeList3
    >
struct Dispatch3ByValueType;

//------------------------------------------------------------------------------
/**
 * Dispatch three arrays, restricting the valid code paths to use only array
 * types specified in the ArrayList TypeList, additionally enforcing that all
 * arrays must have the same ValueType.
 * If all application-wide arrays are desired, use vtkArrayDispatch::Arrays for
 * the first three template parameters.
 * The entry point is:
 * bool Dispatch3ByArrayWithSameValueType<...>::Execute(
 * vtkDataArray *a1, vtkDataArray *a2, vtkDataArray *a3, Worker &worker).
 */
template <
    typename ArrayList1,
    typename ArrayList2,
    typename ArrayList3
    >
struct Dispatch3ByArrayWithSameValueType;

//------------------------------------------------------------------------------
/**
 * Dispatch three arrays, restricting the valid code paths to use only array
 * types found in application-wide vtkArrayDispatch::Arrays TypeList that have a
 * ValueType contained in the ValueTypeList TypeList. This dispatcher also
 * enforces that all arrays have the same ValueType.
 * If all types are to be considered, use vtkArrayDispatch::AllTypes for the
 * first three template parameters.
 * The entry point is:
 * bool Dispatch3BySameValueType<...>::Execute(
 * vtkDataArray *a1, vtkDataArray *a2, vtkDataArray *a3, Worker &worker).
 */
template <typename ValueTypeList>
struct Dispatch3BySameValueType;

//------------------------------------------------------------------------------
/**
 * Filter the ArrayList to contain only arrays with ArrayType::ValueType that
 * exist in ValueList. The result TypeList is stored in Result.
 */
template <typename ArrayList, typename ValueList>
struct FilterArraysByValueType;

} // end namespace vtkArrayDispatch

#include "vtkArrayDispatch.txx"

#endif // vtkArrayDispatch_h
// VTK-HeaderTest-Exclude: vtkArrayDispatch.h
