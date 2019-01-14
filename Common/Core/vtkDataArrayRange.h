/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArrayRange.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @file vtkDataArrayRange.h
 * Provides STL-compatible iterable ranges that provide access vtkDataArray
 * elements.
 *
 * @note Since the term 'range' is overloaded, it's worth pointing out that to
 * determine the value-range of an array's elements (an unrelated concept to
 * the Range objects defined here), see the vtkDataArray::GetRange and
 * vtkGenericDataArray::GetValueRange methods.
 */

#ifndef vtkDataArrayRange_h
#define vtkDataArrayRange_h

#include "vtkAOSDataArrayTemplate.h"
#include "vtkDataArray.h"
#include "vtkDataArrayAccessor.h"
#include "vtkDataArrayMeta.h"
#include "vtkDataArrayTupleRange_AOS.h"
#include "vtkDataArrayTupleRange_Generic.h"
#include "vtkDataArrayValueRange_AOS.h"
#include "vtkDataArrayValueRange_Generic.h"
#include "vtkMeta.h"
#include "vtkSmartPointer.h"

#include <cassert>
#include <iterator>
#include <type_traits>

VTK_ITER_OPTIMIZE_START

namespace vtk
{

/**
 * @brief Generate an stl and for-range compatible range of tuple iterators
 * from a vtkDataArray.
 *
 * This function returns a TupleRange object that is compatible with C++11
 * for-range syntax. As an example usage, consider a function that takes some
 * instance of vtkDataArray (or a subclass) and prints the magnitude of each
 * tuple:
 *
 * ```
 * template <typename ArrayType>
 * void PrintMagnitudes(ArrayType *array)
 * {
 *   for (const auto& tuple : vtk::DataArrayTupleRange(array))
 *   {
 *     double mag = 0.;
 *     for (const auto& comp : tuple)
 *     {
 *       mag += static_cast<double>(comp) * static_cast<double>(comp));
 *     }
 *     std::cerr << mag < "\n";
 *   }
 * }
 * ```
 *
 * Note that `ArrayType` is generic in the above function. When
 * `vtk::DataArrayTupleRange` is given a `vtkDataArray` pointer, the generated
 * code produces iterators and reference proxies that rely on the `vtkDataArray`
 * API. However, when a more derived `ArrayType` is passed in (for example,
 * `vtkFloatArray`), specialized implementations are used that generate highly
 * optimized code.
 *
 * Performance can be further improved when the number of components in the
 * array is known. By passing a compile-time-constant integer as a template
 * parameter, e.g. `vtk::DataArrayTupleRange<3>(array)`, specializations are
 * enabled that allow the compiler to perform additional optimizations.
 *
 * `vtk::DataArrayTupleRange` takes an additional two arguments that can be used
 * to restrict the range of tuples to [start, end).
 *
 * There is a compiler definition / CMake option called
 * `VTK_DEBUG_RANGE_ITERATORS` that enables checks for proper usage of the
 * range/iterator/reference classes. This slow things down significantly, but is
 * useful for diagnosing problems.
 *
 * In some situations, developers may want to build in Debug mode while still
 * maintaining decent performance for data-heavy computations. For these
 * usecases, and additional CMake option `VTK_ALWAYS_OPTIMIZE_ARRAY_ITERATORS`
 * may be enabled to force optimization of code using these iterators. This
 * option will force inlining and enable -O3 (or equivalent) optimization level
 * for iterator code when compiling on platforms that support these features.
 * This option has no effect when `VTK_DEBUG_RANGE_ITERATORS` is enabled.
 *
 * @note References obtained through iterators are invalidated when the iterator
 * is modified.
 *
 * @note `operator[]` is disabled for the iterators and tuple reference types,
 * as they cannot be implemented for vtkDataArray without dangling references.
 * These index operations may still be accessible when used with other array
 * classes, but their use is discouraged to ensure portability and any code
 * using them will not compile with VTK_DEBUG_RANGE_ITERATORS defined.
 */
template <ComponentIdType TupleSize = detail::DynamicTupleSize,
          typename ArrayType = vtkDataArray*>
VTK_ITER_INLINE
auto DataArrayTupleRange(const ArrayType& array,
                         TupleIdType start = -1,
                         TupleIdType end = -1)
    -> detail::TupleRange<typename detail::StripPointers<ArrayType>::type, TupleSize>
{
  // Allow this to work with vtkNew, vtkSmartPointer, etc.
  using RealArrayType = typename detail::StripPointers<ArrayType>::type;

  static_assert(detail::IsValidTupleSize<TupleSize>::value,
                "Invalid tuple size.");
  static_assert(detail::IsVtkDataArray<RealArrayType>::value,
                "Invalid array type.");

  assert(array);

  return detail::TupleRange<RealArrayType, TupleSize>(
        array,
        start < 0 ? 0 : start,
        end < 0 ? array->GetNumberOfTuples() : end);
}

/**
 * @brief Generate an stl and for-range compatible range of flat AOS iterators
 * from a vtkDataArray.
 *
 * This function returns a ValueRange object that is compatible with C++11
 * for-range syntax. The array is traverse as if calling
 * vtkGenericDataArray::GetValue with consecutive, increase indices. As an
 * example usage, consider a function that takes some instance of vtkDataArray
 * (or a subclass) and sums the values it contains:
 *
 * ```
 * template <typename ArrayType>
 * auto ComputeSum(ArrayType *array) -> vtk::GetAPIType<ArrayType>
 * {
 *   using T = vtk::GetAPIType<ArrayType>;
 *
 *   T sum = 0.;
 *   for (const auto& val : vtk::DataArrayValueType(array))
 *   {
 *     sum += val;
 *   }
 *   return sum;
 * }
 * ```
 *
 * Note that `ArrayType` is generic in the above function. When
 * `vtk::DataArrayValueRange` is given a `vtkDataArray` pointer, the generated
 * code produces iterators and reference proxies that rely on the `vtkDataArray`
 * API. However, when a more derived `ArrayType` is passed in (for example,
 * `vtkFloatArray`), specialized implementations are used that generate highly
 * optimized code.
 *
 * Performance can be further improved when the number of components in the
 * array is known. By passing a compile-time-constant integer as a template
 * parameter, e.g. `vtk::DataArrayValueRange<3>(array)`, specializations are
 * enabled that allow the compiler to perform additional optimizations.
 *
 * `vtk::DataArrayValueRange` takes an additional two arguments that can be used
 * to restrict the range of values to [start, end).
 *
 * There is a compiler definition / CMake option called
 * `VTK_DEBUG_RANGE_ITERATORS` that enables checks for proper usage of the
 * range/iterator/reference classes. This slow things down significantly, but is
 * useful for diagnosing problems.
 *
 * In some situations, developers may want to build in Debug mode while still
 * maintaining decent performance for data-heavy computations. For these
 * usecases, and additional CMake option `VTK_ALWAYS_OPTIMIZE_ARRAY_ITERATORS`
 * may be enabled to force optimization of code using these iterators. This
 * option will force inlining and enable -O3 (or equivalent) optimization level
 * for iterator code when compiling on platforms that support these features.
 * This option has no effect when `VTK_DEBUG_RANGE_ITERATORS` is enabled.
 *
 * @note References obtained through iterators are invalidated when the iterator
 * is modified.
 *
 * @note `operator[]` is disabled for the iterators and tuple reference types,
 * as they cannot be implemented for vtkDataArray without dangling references.
 * These index operations may still be accessible when used with other array
 * classes, but their use is discouraged to ensure portability and any code
 * using them will not compile with VTK_DEBUG_RANGE_ITERATORS defined.
 */
template <ComponentIdType TupleSize = detail::DynamicTupleSize,
          typename ArrayType = vtkDataArray*>
VTK_ITER_INLINE
auto DataArrayValueRange(const ArrayType& array,
                         ValueIdType start = -1,
                         ValueIdType end = -1)
-> detail::ValueRange<typename detail::StripPointers<ArrayType>::type, TupleSize>
{
  // Allow this to work with vtkNew, vtkSmartPointer, etc.
  using RealArrayType = typename detail::StripPointers<ArrayType>::type;

  static_assert(detail::IsValidTupleSize<TupleSize>::value,
                "Invalid tuple size.");
  static_assert(detail::IsVtkDataArray<RealArrayType>::value,
                "Invalid array type.");

  assert(array);

  return detail::ValueRange<RealArrayType, TupleSize>(
        array,
        start < 0 ? 0 : start,
        end < 0 ? array->GetNumberOfValues() : end);
}

} // end namespace vtk

VTK_ITER_OPTIMIZE_END

#endif // vtkDataArrayRange_h

// VTK-HeaderTest-Exclude: vtkDataArrayRange.h
