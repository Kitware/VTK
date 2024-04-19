// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file vtkDataArrayRange.h
 * STL-compatible iterable ranges that provide access to vtkDataArray elements.
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

/**
 * @file vtkDataArrayRange.h
 *
 * The vtkDataArrayRange.h header provides utilities to convert vtkDataArrays
 * into "range" objects that behave like STL ranges. There are two types of
 * ranges: TupleRange and ValueRange.
 *
 * See Testing/Cxx/ExampleDataArrayRangeAPI.cxx for an illustrative example of
 * how these ranges and their associated iterators and references are used.
 *
 * These ranges unify the different memory layouts supported by VTK and provide
 * a consistent interface to processing them with high efficiency. Whether a
 * range is constructed from a vtkDataArray, vtkFloatArray, or even
 * vtkScaledSOADataArrayTemplate, the same range-based algorithm implementation
 * can be used to provide the best performance possible using the input array's
 * API.
 *
 * Constructing a range using a derived subclass of vtkDataArray (such as
 * vtkFloatArray) will always give better performance than a range constructed
 * from a vtkDataArray pointer, since the vtkDataArray API requires virtual
 * calls and type conversion. Using a more derived type generally allows the
 * compiler to optimize out any function calls and emit assembly that directly
 * operates on the array's raw memory buffer(s). See vtkArrayDispatch for
 * utilities to convert an unknown vtkDataArray into a more derived type.
 * Testing/Cxx/ExampleDataArrayRangeDispatch.cxx demonstrates how ranges may
 * be used with the dispatcher system.
 *
 * # TupleRanges
 *
 * A TupleRange traverses a vtkDataArray tuple-by-tuple, providing iterators
 * and reference objects that refer to conceptual tuples. The tuple references
 * themselves may be iterated upon to access individual components.
 *
 * TupleRanges are created via the function vtk::DataArrayTupleRange. See
 * that function's documentation for more information about creating
 * TupleRanges.
 *
 * # ValueRanges
 *
 * A ValueRange will traverse a vtkDataArray in "value index" order, e.g. as
 * if walking a pointer into an AOS layout array:
 *
 * ```
 * Array:    {X, X, X}, {X, X, X}, {X, X, X}, ...
 * TupleIdx:  0  0  0    1  1  1    2  2  2
 * CompIdx:   0  1  2    0  1  2    0  1  2
 * ValueIdx:  0  1  2    3  4  5    6  7  8
 * ```
 *
 * ValueRanges are created via the function vtk::DataArrayValueRange. See that
 * function's documentation for more information about creating ValueRanges.
 */

VTK_ITER_OPTIMIZE_START

namespace vtk
{
namespace detail
{
VTK_ABI_NAMESPACE_BEGIN

// Internal detail: This utility is not directly needed by users of
// DataArrayRange.
//
// These classes are used to detect when specializations exist for a given
// array type. They are necessary because given:
//
// template <typename ArrayType> class SomeTemplateClass;
// template <typename T> class SomeTemplateClass<vtkAOSDataArrayTemplate<T>>;
//
// SomeTemplateClass<vtkFloatArray> will pick the generic version, as ArrayType
// is a better match than vtkAOSDataArrayTemplate<T>. This class works around
// that by using Declare[Tuple|Value]RangeSpecialization functions that map an
// input ArrayTypePtr and tuple size to a specific version of the appropriate
// Range.
template <typename ArrayTypePtr, ComponentIdType TupleSize>
struct SelectTupleRange
{
private:
  // Allow this to work with vtkNew, vtkSmartPointer, etc.
  using ArrayType = typename detail::StripPointers<ArrayTypePtr>::type;

  static_assert(detail::IsValidTupleSize<TupleSize>::value, "Invalid tuple size.");
  static_assert(detail::IsVtkDataArray<ArrayType>::value, "Invalid array type.");

public:
  using type =
    typename std::decay<decltype(vtk::detail::DeclareTupleRangeSpecialization<ArrayType, TupleSize>(
      std::declval<ArrayType*>()))>::type;
};

template <typename ArrayTypePtr, ComponentIdType TupleSize,
  typename ForceValueTypeForVtkDataArray = double>
struct SelectValueRange
{
private:
  // Allow this to work with vtkNew, vtkSmartPointer, etc.
  using ArrayType = typename detail::StripPointers<ArrayTypePtr>::type;

  static_assert(detail::IsValidTupleSize<TupleSize>::value, "Invalid tuple size.");
  static_assert(detail::IsVtkDataArray<ArrayType>::value, "Invalid array type.");

public:
  using type =
    typename std::remove_reference<decltype(vtk::detail::DeclareValueRangeSpecialization<ArrayType,
      TupleSize, ForceValueTypeForVtkDataArray>(std::declval<ArrayType*>()))>::type;
};

VTK_ABI_NAMESPACE_END
} // end namespace detail

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
 *   using T = vtk::GetAPIType<ArrayType>;
 *
 *   for (const auto tuple : vtk::DataArrayTupleRange(array))
 *   {
 *     double mag = 0.;
 *     for (const T comp : tuple)
 *     {
 *       mag += static_cast<double>(comp) * static_cast<double>(comp);
 *     }
 *     mag = std::sqrt(mag);
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
 * range/iterator/reference classes. This slows things down significantly, but
 * is useful for diagnosing problems.
 *
 * In some situations, developers may want to build in Debug mode while still
 * maintaining decent performance for data-heavy computations. For these
 * usecases, an additional CMake option `VTK_ALWAYS_OPTIMIZE_ARRAY_ITERATORS`
 * may be enabled to force optimization of code using these iterators. This
 * option will force inlining and enable -O3 (or equivalent) optimization level
 * for iterator code when compiling on platforms that support these features.
 * This option has no effect when `VTK_DEBUG_RANGE_ITERATORS` is enabled.
 *
 * @warning Use caution when using `auto` to hold values or references obtained
 * from iterators, as they may not behave as expected. This is a deficiency in
 * C++ that affects all proxy iterators (such as those from `vector<bool>`)
 * that use a reference object instead of an actual C++ reference type. When in
 * doubt, use `std::iterator_traits` (along with decltype) or the typedefs
 * listed below to determine the proper value/reference type to use. The
 * examples below show how these may be used.
 *
 *
 * To mitigate this, the following types are defined on the range object:
 * - `Range::TupleIteratorType`: Iterator that visits tuples.
 * - `Range::ConstTupleIteratorType`: Const iterator that visits tuples.
 * - `Range::TupleReferenceType`: Mutable tuple proxy reference.
 * - `Range::ConstTupleReferenceType`: Const tuple proxy reference.
 * - `Range::ComponentIteratorType`: Iterator that visits components in a tuple.
 * - `Range::ConstComponentIteratorType`: Const iterator that visits tuple components.
 * - `Range::ComponentReferenceType`: Reference proxy to a single tuple component.
 * - `Range::ConstComponentReferenceType`: Const reference proxy to a single tuple component.
 * - `Range::ComponentType`: `ValueType` of components.
 *
 * These can be accessed via the range objects, e.g.:
 *
 * ```
 * auto range = vtk::DataArrayTupleRange(array);
 *
 * using TupleRef = typename decltype(range)::TupleReferenceType;
 * using ComponentRef = typename decltype(range)::ComponentReferenceType;
 *
 * for (TupleRef tuple : range)
 * {
 *   for (ComponentRef comp : tuple)
 *   {
 *     comp = comp - 1; // Array is modified.
 *   }
 * }
 *
 * using ConstTupleRef = typename decltype(range)::ConstTupleReferenceType;
 * using ComponentType = typename decltype(range)::ComponentType;
 *
 * for (ConstTupleRef tuple : range)
 * {
 *   for (ComponentType comp : tuple)
 *   {
 *     comp = comp - 1; // Array is not modified.
 *   }
 * }
 * ```
 * @todo Just like the `DataArrayValueRange`, the tuple range can also accept a forced value type
 * for generic vtkDataArray.
 */
VTK_ABI_NAMESPACE_BEGIN
template <ComponentIdType TupleSize = detail::DynamicTupleSize,
  typename ArrayTypePtr = vtkDataArray*>
VTK_ITER_INLINE auto DataArrayTupleRange(const ArrayTypePtr& array, TupleIdType start = -1,
  TupleIdType end = -1) -> typename detail::SelectTupleRange<ArrayTypePtr, TupleSize>::type
{
  // Lookup specializations:
  using RangeType = typename detail::SelectTupleRange<ArrayTypePtr, TupleSize>::type;

  assert(array);

  return RangeType(array, start < 0 ? 0 : start, end < 0 ? array->GetNumberOfTuples() : end);
}

/**
 * @brief Generate an stl and for-range compatible range of flat AOS iterators
 * from a vtkDataArray.
 *
 * This function returns a ValueRange object that is compatible with C++11
 * for-range syntax. The array is traversed as if calling
 * vtkGenericDataArray::GetValue with consecutive, increasing indices. As an
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
 *   for (const T val : vtk::DataArrayValueRange(array))
 *   {
 *     sum += val;
 *   }
 *   return sum;
 * }
 * ```
 *
 * These ranges may also be used with STL algorithms:
 *
 * ```
 * template <typename ArrayType>
 * auto ComputeSum(ArrayType *array) -> vtk::GetAPIType<ArrayType>
 * {
 *   const auto range = vtk::DataArrayValueRange(array);
 *   return std::accumulate(range.begin(), range.end(), 0);
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
 * range/iterator/reference classes. This slows things down significantly, but
 * is useful for diagnosing problems.
 *
 * In some situations, developers may want to build in Debug mode while still
 * maintaining decent performance for data-heavy computations. For these
 * usecases, an additional CMake option `VTK_ALWAYS_OPTIMIZE_ARRAY_ITERATORS`
 * may be enabled to force optimization of code using these iterators. This
 * option will force inlining and enable -O3 (or equivalent) optimization level
 * for iterator code when compiling on platforms that support these features.
 * This option has no effect when `VTK_DEBUG_RANGE_ITERATORS` is enabled.
 *
 * @warning Use caution when using `auto` to hold values or references obtained
 * from iterators, as they may not behave as expected. This is a deficiency in
 * C++ that affects all proxy iterators (such as those from `vector<bool>`)
 * that use a reference object instead of an actual C++ reference type. When in
 * doubt, use `std::iterator_traits` (along with decltype) or the typedefs
 * listed below to determine the proper value/reference type to use. The
 * examples below show how these may be used.
 *
 * To mitigate this, the following types are defined on the range object:
 * - `Range::IteratorType`: Iterator that visits values in AOS order.
 * - `Range::ConstIteratorType`: Const iterator that visits values in AOS order.
 * - `Range::ReferenceType`: Mutable value proxy reference.
 * - `Range::ConstReferenceType`: Const value proxy reference.
 * - `Range::ValueType`: `ValueType` of array's API.
 *
 * These can be accessed via the range objects, e.g.:
 *
 * ```
 * auto range = vtk::DataArrayValueRange(array);
 *
 * using RefType = typename decltype(range)::ReferenceType;
 * for (RefType ref : range)
 * { // `ref` is a reference (or reference proxy) to the data held by the array.
 *   ref -= 1; // Array is modified.
 * }
 *
 * using ValueType = typename decltype(range)::ValueType;
 * for (ValueType value : range)
 * { // implicitly converts from a reference (or proxy) to a local lvalue `value`
 *   value -= 1; // Array is not modified.
 * }
 * ```
 */
template <ComponentIdType TupleSize = detail::DynamicTupleSize,
  typename ForceValueTypeForVtkDataArray = double, typename ArrayTypePtr = vtkDataArray*>
VTK_ITER_INLINE auto DataArrayValueRange(
  const ArrayTypePtr& array, ValueIdType start = -1, ValueIdType end = -1) ->
  typename detail::SelectValueRange<ArrayTypePtr, TupleSize, ForceValueTypeForVtkDataArray>::type
{
  using RangeType =
    typename detail::SelectValueRange<ArrayTypePtr, TupleSize, ForceValueTypeForVtkDataArray>::type;

  assert(array);

  return RangeType(array, start < 0 ? 0 : start, end < 0 ? array->GetNumberOfValues() : end);
}

VTK_ABI_NAMESPACE_END
} // end namespace vtk

VTK_ITER_OPTIMIZE_END

#endif // vtkDataArrayRange_h

// VTK-HeaderTest-Exclude: vtkDataArrayRange.h
