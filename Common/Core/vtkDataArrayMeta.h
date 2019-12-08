/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArrayMeta.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkDataArrayMeta_h
#define vtkDataArrayMeta_h

#include "vtkAssume.h"
#include "vtkConfigure.h"
#include "vtkDataArray.h"
#include "vtkMeta.h"
#include "vtkSetGet.h"
#include "vtkType.h"

#include <type_traits>
#include <utility>

/**
 * @file vtkDataArrayMeta.h
 * This file contains a variety of metaprogramming constructs for working
 * with vtkDataArrays.
 */

// When enabled, extra debugging checks are enabled for the iterators.
// Specifically:
// - Specializations are disabled (All code uses the generic implementation).
// - Additional assertions are inserted to ensure correct runtime usage.
// - Performance-related annotations (e.g. force inlining) are disabled.
#if defined(VTK_DEBUG_RANGE_ITERATORS)
#define VTK_ITER_ASSERT(x, msg) assert((x) && msg)
#else
#define VTK_ITER_ASSERT(x, msg)
#endif

#if defined(VTK_ALWAYS_OPTIMIZE_ARRAY_ITERATORS) && !defined(VTK_DEBUG_RANGE_ITERATORS)
#define VTK_ITER_INLINE VTK_ALWAYS_INLINE
#define VTK_ITER_ASSUME VTK_ASSUME_NO_ASSERT
#define VTK_ITER_OPTIMIZE_START VTK_ALWAYS_OPTIMIZE_START
#define VTK_ITER_OPTIMIZE_END VTK_ALWAYS_OPTIMIZE_START
#else
#define VTK_ITER_INLINE inline
#define VTK_ITER_ASSUME VTK_ASSUME
#define VTK_ITER_OPTIMIZE_START
#define VTK_ITER_OPTIMIZE_END
#endif

VTK_ITER_OPTIMIZE_START

// For IsAOSDataArray:
template <typename ValueType>
class vtkAOSDataArrayTemplate;

namespace vtk
{

// Typedef for data array indices:
using ComponentIdType = int;
using TupleIdType = vtkIdType;
using ValueIdType = vtkIdType;

namespace detail
{

//------------------------------------------------------------------------------
// Used by ranges/iterators when tuple size is unknown at compile time
static constexpr ComponentIdType DynamicTupleSize = 0;

//------------------------------------------------------------------------------
// Detect data array value types
template <typename T>
struct IsVtkDataArray : std::is_base_of<vtkDataArray, T>
{
};

template <typename T>
using EnableIfVtkDataArray = typename std::enable_if<IsVtkDataArray<T>::value>::type;

//------------------------------------------------------------------------------
// If a value is a valid tuple size
template <ComponentIdType Size>
struct IsValidTupleSize : std::integral_constant<bool, (Size > 0 || Size == DynamicTupleSize)>
{
};

template <ComponentIdType TupleSize>
using EnableIfValidTupleSize = typename std::enable_if<IsValidTupleSize<TupleSize>::value>::type;

//------------------------------------------------------------------------------
// If a value is a non-dynamic tuple size
template <ComponentIdType Size>
struct IsStaticTupleSize : std::integral_constant<bool, (Size > 0)>
{
};

template <ComponentIdType TupleSize>
using EnableIfStaticTupleSize = typename std::enable_if<IsStaticTupleSize<TupleSize>::value>::type;

//------------------------------------------------------------------------------
// If two values are valid non-dynamic tuple sizes:
template <ComponentIdType S1, ComponentIdType S2>
struct AreStaticTupleSizes
  : std::integral_constant<bool, (IsStaticTupleSize<S1>::value && IsStaticTupleSize<S2>::value)>
{
};

template <ComponentIdType S1, ComponentIdType S2, typename T = void>
using EnableIfStaticTupleSizes =
  typename std::enable_if<AreStaticTupleSizes<S1, S2>::value, T>::type;

//------------------------------------------------------------------------------
// If either of the tuple sizes is not statically defined
template <ComponentIdType S1, ComponentIdType S2>
struct IsEitherTupleSizeDynamic
  : std::integral_constant<bool, (!IsStaticTupleSize<S1>::value || !IsStaticTupleSize<S2>::value)>
{
};

template <ComponentIdType S1, ComponentIdType S2, typename T = void>
using EnableIfEitherTupleSizeIsDynamic =
  typename std::enable_if<IsEitherTupleSizeDynamic<S1, S2>::value, T>::type;

//------------------------------------------------------------------------------
// Helper that switches between a storageless integral constant for known
// sizes, and a runtime variable for variable sizes.
template <ComponentIdType TupleSize>
struct GenericTupleSize : public std::integral_constant<ComponentIdType, TupleSize>
{
  static_assert(IsValidTupleSize<TupleSize>::value, "Invalid tuple size.");

private:
  using Superclass = std::integral_constant<ComponentIdType, TupleSize>;

public:
  // Need to construct from array for specialization.
  using Superclass::Superclass;
  VTK_ITER_INLINE GenericTupleSize() noexcept = default;
  VTK_ITER_INLINE GenericTupleSize(vtkDataArray*) noexcept {}
};

// Specialize for dynamic types, mimicking integral_constant API:
template <>
struct GenericTupleSize<DynamicTupleSize>
{
  using value_type = ComponentIdType;

  VTK_ITER_INLINE GenericTupleSize() noexcept : value(0) {}
  VTK_ITER_INLINE explicit GenericTupleSize(vtkDataArray* array)
    : value(array->GetNumberOfComponents())
  {
  }

  VTK_ITER_INLINE operator value_type() const noexcept { return value; }
  VTK_ITER_INLINE value_type operator()() const noexcept { return value; }

  ComponentIdType value;
};

template <typename ArrayType>
struct GetAPITypeImpl
{
  using APIType = typename ArrayType::ValueType;
};
template <>
struct GetAPITypeImpl<vtkDataArray>
{
  using APIType = double;
};

} // end namespace detail

//------------------------------------------------------------------------------
// Typedef for double if vtkDataArray, or the array's ValueType for subclasses.
template <typename ArrayType, typename = detail::EnableIfVtkDataArray<ArrayType> >
using GetAPIType = typename detail::GetAPITypeImpl<ArrayType>::APIType;

//------------------------------------------------------------------------------
namespace detail
{

template <typename ArrayType>
struct IsAOSDataArrayImpl
{
  using APIType = GetAPIType<ArrayType>;
  static constexpr bool value = std::is_base_of<vtkAOSDataArrayTemplate<APIType>, ArrayType>::value;
};

} // end namespace detail

//------------------------------------------------------------------------------
// True if ArrayType inherits some specialization of vtkAOSDataArrayTemplate
template <typename ArrayType>
using IsAOSDataArray = std::integral_constant<bool, detail::IsAOSDataArrayImpl<ArrayType>::value>;

} // end namespace vtk

VTK_ITER_OPTIMIZE_END

#endif // vtkDataArrayMeta_h

// VTK-HeaderTest-Exclude: vtkDataArrayMeta.h
