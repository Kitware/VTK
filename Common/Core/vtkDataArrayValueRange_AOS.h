/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArrayValueRange_AOS.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * Specialization of value ranges and iterators for vtkAOSDataArrayTemplate.
 */

#ifndef vtkDataArrayValueRange_AOS_h
#define vtkDataArrayValueRange_AOS_h

#include "vtkAOSDataArrayTemplate.h"
#include "vtkDataArrayMeta.h"
#include "vtkDataArrayValueRange_Generic.h"

#include <cassert>
#include <algorithm>
#include <iterator>
#include <type_traits>

#ifndef __VTK_WRAP__

// Disable this specialization when iterator debugging is requested:
#ifndef VTK_DEBUG_RANGE_ITERATORS

VTK_ITER_OPTIMIZE_START

namespace vtk
{

namespace detail
{

//------------------------------------------------------------------------------
// ValueRange
template <typename ValueTypeT,
          ComponentIdType TupleSize>
struct ValueRange<vtkAOSDataArrayTemplate<ValueTypeT>, TupleSize>
{
  using ArrayType = vtkAOSDataArrayTemplate<ValueTypeT>;
  using ComponentType = ValueTypeT;
  using ValueType = ValueTypeT;

private:
  static_assert(IsValidTupleSize<TupleSize>::value, "Invalid tuple size.");

  using APIType = GetAPIType<ArrayType>;
  using IdStorageType = IdStorage<TupleSize>;
  using NumCompsType = GenericTupleSize<TupleSize>;

public:

  // May be DynamicTupleSize, or the actual tuple size.
  constexpr static ComponentIdType TupleSizeTag = TupleSize;

  using value_type = ValueType;
  using size_type = ValueIdType;
  using iterator = ValueType*;
  using const_iterator = const ValueType*;

  VTK_ITER_INLINE
  ValueRange(ArrayType *arr,
             ValueIdType beginValue,
             ValueIdType endValue) noexcept
    : Array(arr)
    , NumComps(arr)
    , Begin(arr->GetPointer(beginValue))
    , End(arr->GetPointer(endValue))
  {
    assert(this->Array);
    assert(beginValue >= 0 && beginValue <= endValue);
    assert(endValue >= 0 && endValue <= this->Array->GetNumberOfValues());
  }

  VTK_ITER_INLINE
  ArrayType* GetArray() const noexcept { return this->Array; }

  VTK_ITER_INLINE
  ComponentIdType GetTupleSize() const noexcept { return this->NumComps.value; }

  VTK_ITER_INLINE
  ValueIdType GetBeginValueId() const noexcept
  {
    return static_cast<ValueIdType>(this->Begin - this->Array->GetPointer(0));
  }

  VTK_ITER_INLINE
  ValueIdType GetEndValueId() const noexcept
  {
    return static_cast<ValueIdType>(this->End - this->Array->GetPointer(0));
  }

  VTK_ITER_INLINE
  size_type size() const noexcept
  {
    return static_cast<size_type>(this->End - this->Begin);
  }

  VTK_ITER_INLINE
  iterator begin() noexcept { return this->Begin; }
  VTK_ITER_INLINE
  iterator end() noexcept { return this->End; }

  VTK_ITER_INLINE
  const_iterator begin() const noexcept { return this->Begin; }
  VTK_ITER_INLINE
  const_iterator end() const noexcept { return this->End; }

  VTK_ITER_INLINE
  const_iterator cbegin() const noexcept { return this->Begin; }
  VTK_ITER_INLINE
  const_iterator cend() const noexcept { return this->End; }

private:
  mutable vtkSmartPointer<ArrayType> Array;
  NumCompsType NumComps;
  ValueType* Begin;
  ValueType* End;
};

}
} // end namespace vtk::detail

VTK_ITER_OPTIMIZE_END

#endif // VTK_DEBUG_RANGE_ITERATORS
#endif // __VTK_WRAP__
#endif // vtkDataArrayValueRange_AOS_h

// VTK-HeaderTest-Exclude: vtkDataArrayValueRange_AOS.h
