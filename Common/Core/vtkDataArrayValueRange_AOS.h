// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Specialization of value ranges and iterators for vtkAOSDataArrayTemplate.
 */

#ifndef vtkDataArrayValueRange_AOS_h
#define vtkDataArrayValueRange_AOS_h

#include "vtkAOSDataArrayTemplate.h"
#include "vtkDataArrayMeta.h"
#include "vtkDataArrayValueRange_Generic.h"
#include "vtkDebugRangeIterators.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <type_traits>

// Disable this specialization when iterator debugging is requested:
#ifndef VTK_DEBUG_RANGE_ITERATORS

VTK_ITER_OPTIMIZE_START

namespace vtk
{
namespace detail
{
VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// ValueRange
// For vtkAOSDataArrayTemplate, the `ForceValueTypeForVtkDataArray` template parameter is not used
// at all.
template <typename ValueTypeT, ComponentIdType TupleSize, typename ForceValueTypeForVtkDataArray>
struct ValueRange<vtkAOSDataArrayTemplate<ValueTypeT>, TupleSize, ForceValueTypeForVtkDataArray>
{
private:
  static_assert(IsValidTupleSize<TupleSize>::value, "Invalid tuple size.");

  using IdStorageType = IdStorage<TupleSize>;
  using NumCompsType = GenericTupleSize<TupleSize>;

public:
  using ArrayType = vtkAOSDataArrayTemplate<ValueTypeT>;
  using ValueType = ValueTypeT;

  using IteratorType = ValueType*;
  using ConstIteratorType = ValueType const*;
  using ReferenceType = ValueType&;
  using ConstReferenceType = ValueType const&;

  // May be DynamicTupleSize, or the actual tuple size.
  constexpr static ComponentIdType TupleSizeTag = TupleSize;

  // STL-compat
  using value_type = ValueType;
  using size_type = ValueIdType;
  using iterator = IteratorType;
  using const_iterator = ConstIteratorType;
  using reference = ReferenceType;
  using const_reference = ConstReferenceType;

  VTK_ITER_INLINE
  ValueRange() noexcept = default;

  VTK_ITER_INLINE
  ValueRange(ArrayType* arr, ValueIdType beginValue, ValueIdType endValue) noexcept
    : Array(arr)
    , NumComps(arr)
    , BeginValue(beginValue)
    , EndValue(endValue)
  {
    assert(this->Array);
    assert(beginValue >= 0 && beginValue <= endValue);
    assert(endValue >= 0 && endValue <= this->Array->GetNumberOfValues());
  }

  VTK_ITER_INLINE
  ValueRange GetSubRange(ValueIdType beginValue = 0, ValueIdType endValue = -1) const noexcept
  {
    const ValueIdType realBegin =
      std::distance(this->Array->GetPointer(0), this->Array->GetPointer(this->BeginValue)) +
      beginValue;
    const ValueIdType realEnd = endValue >= 0
      ? std::distance(this->Array->GetPointer(0), this->Array->GetPointer(this->BeginValue)) +
        endValue
      : std::distance(this->Array->GetPointer(0), this->Array->GetPointer(this->EndValue));

    return ValueRange<ArrayType, TupleSize, ForceValueTypeForVtkDataArray>{ this->Array, realBegin,
      realEnd };
  }

  VTK_ITER_INLINE
  ArrayType* GetArray() const noexcept { return this->Array; }

  VTK_ITER_INLINE
  ComponentIdType GetTupleSize() const noexcept { return this->NumComps.value; }

  VTK_ITER_INLINE
  ValueIdType GetBeginValueId() const noexcept
  {
    return static_cast<ValueIdType>(
      this->Array->GetPointer(this->BeginValue) - this->Array->GetPointer(0));
  }

  VTK_ITER_INLINE
  ValueIdType GetEndValueId() const noexcept
  {
    return static_cast<ValueIdType>(
      this->Array->GetPointer(this->EndValue) - this->Array->GetPointer(0));
  }

  VTK_ITER_INLINE
  size_type size() const noexcept
  {
    return static_cast<size_type>(
      this->Array->GetPointer(this->EndValue) - this->Array->GetPointer(this->BeginValue));
  }

  VTK_ITER_INLINE
  iterator begin() noexcept { return this->Array->GetPointer(this->BeginValue); }
  VTK_ITER_INLINE
  iterator end() noexcept { return this->Array->GetPointer(this->EndValue); }

  VTK_ITER_INLINE
  const_iterator begin() const noexcept { return this->Array->GetPointer(this->BeginValue); }
  VTK_ITER_INLINE
  const_iterator end() const noexcept { return this->Array->GetPointer(this->EndValue); }

  VTK_ITER_INLINE
  const_iterator cbegin() const noexcept { return this->Array->GetPointer(this->BeginValue); }
  VTK_ITER_INLINE
  const_iterator cend() const noexcept { return this->Array->GetPointer(this->EndValue); }

  VTK_ITER_INLINE
  reference operator[](size_type i) noexcept
  {
    return this->Array->Buffer->GetBuffer()[this->BeginValue + i];
  }
  VTK_ITER_INLINE
  const_reference operator[](size_type i) const noexcept
  {
    return this->Array->Buffer->GetBuffer()[this->BeginValue + i];
  }

  // Danger! pointer is non-const!
  value_type* data() noexcept { return this->Array->Buffer->GetBuffer(); }

  value_type* data() const noexcept { return this->Array->Buffer->GetBuffer(); }

private:
  mutable ArrayType* Array{ nullptr };
  NumCompsType NumComps{};
  ValueIdType BeginValue{ 0 };
  ValueIdType EndValue{ 0 };
};

// Unimplemented, only used inside decltype in SelectValueRange:
template <typename ArrayType, ComponentIdType TupleSize, typename ForceValueTypeForVtkDataArray,
  // Convenience:
  typename ValueType = typename ArrayType::ValueType,
  typename AOSArrayType = vtkAOSDataArrayTemplate<ValueType>,
  // SFINAE to select AOS arrays:
  typename = typename std::enable_if<IsAOSDataArray<ArrayType>::value>::type>
ValueRange<AOSArrayType, TupleSize, ForceValueTypeForVtkDataArray> DeclareValueRangeSpecialization(
  ArrayType*);

VTK_ABI_NAMESPACE_END
}
} // end namespace vtk::detail

VTK_ITER_OPTIMIZE_END

#endif // VTK_DEBUG_RANGE_ITERATORS
#endif // vtkDataArrayValueRange_AOS_h

// VTK-HeaderTest-Exclude: vtkDataArrayValueRange_AOS.h
