// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Specialization of tuple ranges and iterators for vtkAOSDataArrayTemplate.
 */

#ifndef vtkDataArrayTupleRange_AOS_h
#define vtkDataArrayTupleRange_AOS_h

#include "vtkAOSDataArrayTemplate.h"
#include "vtkDataArrayMeta.h"
#include "vtkDataArrayTupleRange_Generic.h"
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

// Forward decs for friends/args
template <typename ArrayType, ComponentIdType>
struct ConstTupleReference;
template <typename ArrayType, ComponentIdType>
struct TupleReference;
template <typename ArrayType, ComponentIdType>
struct ConstTupleIterator;
template <typename ArrayType, ComponentIdType>
struct TupleIterator;
template <typename ArrayType, ComponentIdType>
struct TupleRange;

//------------------------------------------------------------------------------
// Const tuple reference
template <typename ValueType, ComponentIdType TupleSize>
struct ConstTupleReference<vtkAOSDataArrayTemplate<ValueType>, TupleSize>
{
private:
  using ArrayType = vtkAOSDataArrayTemplate<ValueType>;
  using NumCompsType = GenericTupleSize<TupleSize>;
  using APIType = ValueType;

public:
  using size_type = ComponentIdType;
  using value_type = APIType;
  using const_reference = const ValueType&;
  using iterator = const ValueType*;
  using const_iterator = const ValueType*;

  VTK_ITER_INLINE
  ConstTupleReference() noexcept
    : Tuple{ nullptr }
  {
  }

  VTK_ITER_INLINE
  ConstTupleReference(const ValueType* tuple, NumCompsType numComps) noexcept
    : Tuple(tuple)
    , NumComps(numComps)
  {
  }

  VTK_ITER_INLINE
  ConstTupleReference(const TupleReference<ArrayType, TupleSize>& o) noexcept
    : Tuple{ o.Tuple }
    , NumComps{ o.NumComps }
  {
  }

  VTK_ITER_INLINE
  ConstTupleReference(const ConstTupleReference&) noexcept = default;
  VTK_ITER_INLINE
  ConstTupleReference(ConstTupleReference&&) noexcept = default;

  // Allow this type to masquerade as a pointer, so that tupleIiter->foo works.
  VTK_ITER_INLINE
  ConstTupleReference* operator->() noexcept { return this; }
  VTK_ITER_INLINE
  const ConstTupleReference* operator->() const noexcept { return this; }

  // Caller must ensure that there are size() elements in array.
  VTK_ITER_INLINE void GetTuple(volatile APIType* tuple) const noexcept
  {
    // Yes, the tuple argument is marked volatile. No, it's not a mistake.
    //
    // `volatile`'s intended usage per the standard is to disable optimizations
    // when accessing a variable. Without it, GCC 8 will optimize the following
    // loop to memcpy, but we're usually copying small tuples here, and the
    // call to memcpy is more expensive than just doing an inline copy. By
    // disabling the memcpy optimization, benchmarks are 60% faster when
    // iterating with the Get/SetTuple methods, and are comparable to other
    // methods of array access.
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    for (ComponentIdType i = 0; i < this->NumComps.value; ++i)
    {
      tuple[i] = this->Tuple[i];
    }
  }

  template <typename VT = ValueType>
  typename std::enable_if<!std::is_same<VT, double>::value>::type VTK_ITER_INLINE GetTuple(
    volatile double* tuple) const noexcept
  {
    // Yes, this variable argument is marked volatile. See the explanation in GetTuple.
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    for (ComponentIdType i = 0; i < this->NumComps.value; ++i)
    {
      tuple[i] = static_cast<double>(this->Tuple[i]);
    }
  }

  // skips some runtime checks when both sizes are fixed:
  template <typename OArrayType, ComponentIdType OSize>
  VTK_ITER_INLINE EnableIfStaticTupleSizes<TupleSize, OSize, bool> operator==(
    const TupleReference<OArrayType, OSize>& other) const noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert(
      (std::is_convertible<OAPIType, APIType>{}), "Incompatible types when assigning tuples.");

    // SFINAE guarantees that the tuple sizes are not dynamic in this overload:
    static_assert(TupleSize == OSize, "Cannot assign tuples with different sizes.");

    return std::equal(this->cbegin(), this->cend(), other.cbegin());
  }

  // Needs a runtime check:
  template <typename OArrayType, ComponentIdType OSize>
  VTK_ITER_INLINE EnableIfEitherTupleSizeIsDynamic<TupleSize, OSize, bool> operator==(
    const TupleReference<OArrayType, OSize>& other) const noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert(
      (std::is_convertible<OAPIType, APIType>{}), "Incompatible types when assigning tuples.");

    // Need to check the size at runtime :-(
    if (other.size() != this->NumComps.value)
    {
      return false;
    }

    return std::equal(this->cbegin(), this->cend(), other.cbegin());
  }

  // skips some runtime checks when both sizes are fixed:
  template <typename OArrayType, ComponentIdType OSize>
  VTK_ITER_INLINE EnableIfStaticTupleSizes<TupleSize, OSize, bool> operator==(
    const ConstTupleReference<OArrayType, OSize>& other) const noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert(
      (std::is_convertible<OAPIType, APIType>{}), "Incompatible types when assigning tuples.");

    // SFINAE guarantees that the tuple sizes are not dynamic in this overload:
    static_assert(TupleSize == OSize, "Cannot assign tuples with different sizes.");

    return std::equal(this->cbegin(), this->cend(), other.cbegin());
  }

  // Needs a runtime check:
  template <typename OArrayType, ComponentIdType OSize>
  VTK_ITER_INLINE EnableIfEitherTupleSizeIsDynamic<TupleSize, OSize, bool> operator==(
    const ConstTupleReference<OArrayType, OSize>& other) const noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert(
      (std::is_convertible<OAPIType, APIType>{}), "Incompatible types when assigning tuples.");

    // Need to check the size at runtime :-(
    if (other.size() != this->NumComps.value)
    {
      return false;
    }

    return std::equal(this->cbegin(), this->cend(), other.cbegin());
  }

  template <typename OArrayType, ComponentIdType OSize>
  VTK_ITER_INLINE bool operator!=(const TupleReference<OArrayType, OSize>& o) const noexcept
  {
    return !(*this == o);
  }

  template <typename OArray, ComponentIdType OSize>
  VTK_ITER_INLINE bool operator!=(const ConstTupleReference<OArray, OSize>& o) const noexcept
  {
    return !(*this == o);
  }

  VTK_ITER_INLINE
  const_reference operator[](size_type i) const noexcept { return this->Tuple[i]; }

  VTK_ITER_INLINE
  size_type size() const noexcept { return this->NumComps.value; }

  VTK_ITER_INLINE
  const_iterator begin() const noexcept { return const_iterator{ this->Tuple }; }

  VTK_ITER_INLINE
  const_iterator end() const noexcept
  {
    return const_iterator{ this->Tuple + this->NumComps.value };
  }

  VTK_ITER_INLINE
  const_iterator cbegin() const noexcept { return const_iterator{ this->Tuple }; }

  VTK_ITER_INLINE
  const_iterator cend() const noexcept
  {
    return const_iterator{ this->Tuple + this->NumComps.value };
  }

  friend struct ConstTupleIterator<ArrayType, TupleSize>;

protected:
  // Intentionally hidden:
  VTK_ITER_INLINE
  ConstTupleReference& operator=(const ConstTupleReference&) noexcept = default;

  const ValueType* Tuple;
  NumCompsType NumComps;
};

//------------------------------------------------------------------------------
// Tuple reference
template <typename ValueType, ComponentIdType TupleSize>
struct TupleReference<vtkAOSDataArrayTemplate<ValueType>, TupleSize>
{
private:
  using ArrayType = vtkAOSDataArrayTemplate<ValueType>;
  using NumCompsType = GenericTupleSize<TupleSize>;
  using APIType = ValueType;

public:
  using size_type = ComponentIdType;
  using value_type = APIType;
  using iterator = ValueType*;
  using const_iterator = const ValueType*;
  using reference = ValueType&;
  using const_reference = ValueType const&;

  VTK_ITER_INLINE
  TupleReference() noexcept
    : Tuple{ nullptr }
  {
  }

  VTK_ITER_INLINE
  TupleReference(ValueType* tuple, NumCompsType numComps) noexcept
    : Tuple(tuple)
    , NumComps(numComps)
  {
  }

  VTK_ITER_INLINE
  TupleReference(const TupleReference&) noexcept = default;
  VTK_ITER_INLINE
  TupleReference(TupleReference&&) noexcept = default;

  // Allow this type to masquerade as a pointer, so that tupleIiter->foo works.
  VTK_ITER_INLINE
  TupleReference* operator->() noexcept { return this; }
  VTK_ITER_INLINE
  const TupleReference* operator->() const noexcept { return this; }

  // Caller must ensure that there are size() elements in array.
  VTK_ITER_INLINE
  void GetTuple(volatile APIType* tuple) const noexcept
  {
    // Yes, the tuple argument is marked volatile. No, it's not a mistake.
    //
    // `volatile`'s intended usage per the standard is to disable optimizations
    // when accessing a variable. Without it, GCC 8 will optimize the following
    // loop to memcpy, but we're usually copying small tuples here, and the
    // call to memcpy is more expensive than just doing an inline copy. By
    // disabling the memcpy optimization, benchmarks are 60% faster when
    // iterating with the Get/SetTuple methods, and are comparable to other
    // methods of array access.
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    for (ComponentIdType i = 0; i < this->NumComps.value; ++i)
    {
      tuple[i] = this->Tuple[i];
    }
  }

  template <typename VT = ValueType>
  typename std::enable_if<!std::is_same<VT, double>::value>::type VTK_ITER_INLINE GetTuple(
    volatile double* tuple) const noexcept
  {
    // Yes, this variable argument is marked volatile. See the explanation in GetTuple.
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    for (ComponentIdType i = 0; i < this->NumComps.value; ++i)
    {
      tuple[i] = static_cast<double>(this->Tuple[i]);
    }
  }

  // Caller must ensure that there are size() elements in array.
  VTK_ITER_INLINE
  void SetTuple(const APIType* tuple) noexcept
  {
    volatile APIType* out = this->Tuple;
    // Yes, this variable argument is marked volatile. See the explanation in GetTuple.
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    for (ComponentIdType i = 0; i < this->NumComps.value; ++i)
    {
      out[i] = tuple[i];
    }
  }

  template <typename VT = ValueType>
  typename std::enable_if<!std::is_same<VT, double>::value>::type VTK_ITER_INLINE SetTuple(
    const double* tuple) noexcept
  {
    volatile APIType* out = this->Tuple;
    // Yes, this variable argument is marked volatile. See the explanation in GetTuple.
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    for (ComponentIdType i = 0; i < this->NumComps.value; ++i)
    {
      out[i] = static_cast<APIType>(tuple[i]);
    }
  }

  VTK_ITER_INLINE
  TupleReference& operator=(const TupleReference& other) noexcept
  {
    std::copy_n(other.cbegin(), this->NumComps.value, this->begin());
    return *this;
  }

  template <typename OArrayType, ComponentIdType OSize>
  VTK_ITER_INLINE EnableIfStaticTupleSizes<TupleSize, OSize, TupleReference&> operator=(
    const TupleReference<OArrayType, OSize>& other) noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert(
      (std::is_convertible<OAPIType, APIType>{}), "Incompatible types when assigning tuples.");

    // SFINAE guarantees that the tuple sizes are not dynamic in this overload:
    static_assert(TupleSize == OSize, "Cannot assign tuples with different sizes.");

    std::copy_n(other.cbegin(), OSize, this->begin());
    return *this;
  }

  template <typename OArrayType, ComponentIdType OSize>
  VTK_ITER_INLINE EnableIfEitherTupleSizeIsDynamic<TupleSize, OSize, TupleReference&> operator=(
    const TupleReference<OArrayType, OSize>& other) noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert(
      (std::is_convertible<OAPIType, APIType>{}), "Incompatible types when assigning tuples.");

    // Note that the sizes are not checked here. Enable
    // VTK_DEBUG_RANGE_ITERATORS to enable check.
    std::copy_n(other.cbegin(), this->NumComps.value, this->begin());
    return *this;
  }

  template <typename OArrayType, ComponentIdType OSize>
  VTK_ITER_INLINE EnableIfStaticTupleSizes<TupleSize, OSize, TupleReference&> operator=(
    const ConstTupleReference<OArrayType, OSize>& other) noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert(
      (std::is_convertible<OAPIType, APIType>{}), "Incompatible types when assigning tuples.");

    // SFINAE guarantees that the tuple sizes are not dynamic in this overload:
    static_assert(TupleSize == OSize, "Cannot assign tuples with different sizes.");

    std::copy_n(other.cbegin(), OSize, this->begin());
    return *this;
  }

  template <typename OArrayType, ComponentIdType OSize>
  VTK_ITER_INLINE EnableIfEitherTupleSizeIsDynamic<TupleSize, OSize, TupleReference&> operator=(
    const ConstTupleReference<OArrayType, OSize>& other) noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert(
      (std::is_convertible<OAPIType, APIType>{}), "Incompatible types when assigning tuples.");

    // Note that the sizes are not checked here. Enable
    // VTK_DEBUG_RANGE_ITERATORS to enable check.
    std::copy_n(other.cbegin(), this->NumComps.value, this->begin());
    return *this;
  }

  // skips some runtime checks when both sizes are fixed:
  template <typename OArrayType, ComponentIdType OSize>
  VTK_ITER_INLINE EnableIfStaticTupleSizes<TupleSize, OSize, bool> operator==(
    const TupleReference<OArrayType, OSize>& other) const noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert(
      (std::is_convertible<OAPIType, APIType>{}), "Incompatible types when assigning tuples.");

    // SFINAE guarantees that the tuple sizes are not dynamic in this overload:
    static_assert(TupleSize == OSize, "Cannot assign tuples with different sizes.");

    return std::equal(this->cbegin(), this->cend(), other.cbegin());
  }

  // Needs a runtime check:
  template <typename OArrayType, ComponentIdType OSize>
  VTK_ITER_INLINE EnableIfEitherTupleSizeIsDynamic<TupleSize, OSize, bool> operator==(
    const TupleReference<OArrayType, OSize>& other) const noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert(
      (std::is_convertible<OAPIType, APIType>{}), "Incompatible types when assigning tuples.");

    // Note that the sizes are not checked here. Enable
    // VTK_DEBUG_RANGE_ITERATORS to enable check.
    return std::equal(this->cbegin(), this->cend(), other.cbegin());
  }

  // skips some runtime checks when both sizes are fixed:
  template <typename OArrayType, ComponentIdType OSize>
  VTK_ITER_INLINE EnableIfStaticTupleSizes<TupleSize, OSize, bool> operator==(
    const ConstTupleReference<OArrayType, OSize>& other) const noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert(
      (std::is_convertible<OAPIType, APIType>{}), "Incompatible types when assigning tuples.");

    // SFINAE guarantees that the tuple sizes are not dynamic in this overload:
    static_assert(TupleSize == OSize, "Cannot assign tuples with different sizes.");

    return std::equal(this->cbegin(), this->cend(), other.cbegin());
  }

  // Needs a runtime check:
  template <typename OArrayType, ComponentIdType OSize>
  VTK_ITER_INLINE EnableIfEitherTupleSizeIsDynamic<TupleSize, OSize, bool> operator==(
    const ConstTupleReference<OArrayType, OSize>& other) const noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert(
      (std::is_convertible<OAPIType, APIType>{}), "Incompatible types when assigning tuples.");

    // Note that the sizes are not checked here. Enable
    // VTK_DEBUG_RANGE_ITERATORS to enable check.
    return std::equal(this->cbegin(), this->cend(), other.cbegin());
  }

  template <typename OArrayType, ComponentIdType OSize>
  VTK_ITER_INLINE bool operator!=(const TupleReference<OArrayType, OSize>& o) const noexcept
  {
    return !(*this == o);
  }

  template <typename OArray, ComponentIdType OSize>
  VTK_ITER_INLINE bool operator!=(const ConstTupleReference<OArray, OSize>& o) const noexcept
  {
    return !(*this == o);
  }

  // skips some runtime checks:
  template <typename OArrayType, ComponentIdType OSize>
  VTK_ITER_INLINE EnableIfStaticTupleSizes<TupleSize, OSize, void> swap(
    TupleReference<OArrayType, OSize> other) noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_same<OAPIType, APIType>{}), "Incompatible types when swapping tuples.");

    // SFINAE guarantees that the tuple sizes are not dynamic in this overload:
    static_assert(TupleSize == OSize, "Cannot swap tuples with different sizes.");

    std::swap_ranges(this->begin(), this->end(), other.begin());
  }

  // Needs a runtime check:
  template <typename OArrayType, ComponentIdType OSize>
  VTK_ITER_INLINE EnableIfEitherTupleSizeIsDynamic<TupleSize, OSize, void> swap(
    TupleReference<OArrayType, OSize> other) noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_same<OAPIType, APIType>{}), "Incompatible types when swapping tuples.");

    // Note that the sizes are not checked here. Enable
    // VTK_DEBUG_RANGE_ITERATORS to enable check.
    std::swap_ranges(this->begin(), this->end(), other.begin());
  }

  friend VTK_ITER_INLINE void swap(TupleReference a, TupleReference b) noexcept { a.swap(b); }

  template <typename OArray, ComponentIdType OSize>
  friend VTK_ITER_INLINE void swap(TupleReference a, TupleReference<OArray, OSize> b) noexcept
  {
    a.swap(b);
  }

  VTK_ITER_INLINE
  reference operator[](size_type i) noexcept { return this->Tuple[i]; }

  VTK_ITER_INLINE
  const_reference operator[](size_type i) const noexcept { return this->Tuple[i]; }

  VTK_ITER_INLINE
  void fill(const value_type& v) noexcept { std::fill(this->begin(), this->end(), v); }

  VTK_ITER_INLINE
  size_type size() const noexcept { return this->NumComps.value; }

  VTK_ITER_INLINE
  iterator begin() noexcept { return iterator{ this->Tuple }; }

  VTK_ITER_INLINE
  iterator end() noexcept { return iterator{ this->Tuple + this->NumComps.value }; }

  VTK_ITER_INLINE
  const_iterator begin() const noexcept { return const_iterator{ this->Tuple }; }

  VTK_ITER_INLINE
  const_iterator end() const noexcept
  {
    return const_iterator{ this->Tuple + this->NumComps.value };
  }

  VTK_ITER_INLINE
  const_iterator cbegin() const noexcept { return const_iterator{ this->Tuple }; }

  VTK_ITER_INLINE
  const_iterator cend() const noexcept
  {
    return const_iterator{ this->Tuple + this->NumComps.value };
  }

  friend struct ConstTupleReference<ArrayType, TupleSize>;
  friend struct TupleIterator<ArrayType, TupleSize>;

protected:
  VTK_ITER_INLINE
  void CopyReference(const TupleReference& o) noexcept
  {
    this->Tuple = o.Tuple;
    this->NumComps = o.NumComps;
  }

  ValueType* Tuple;
  NumCompsType NumComps;
};

//------------------------------------------------------------------------------
// Const tuple iterator
template <typename ValueType, ComponentIdType TupleSize>
struct ConstTupleIterator<vtkAOSDataArrayTemplate<ValueType>, TupleSize>
{
private:
  using ArrayType = vtkAOSDataArrayTemplate<ValueType>;
  using NumCompsType = GenericTupleSize<TupleSize>;

public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = ConstTupleReference<ArrayType, TupleSize>;
  using difference_type = TupleIdType;
  using pointer = ConstTupleReference<ArrayType, TupleSize>;
  using reference = ConstTupleReference<ArrayType, TupleSize>;

  VTK_ITER_INLINE
  ConstTupleIterator() noexcept = default;

  VTK_ITER_INLINE
  ConstTupleIterator(const ValueType* tuple, NumCompsType numComps) noexcept
    : Ref(tuple, numComps)
  {
  }

  VTK_ITER_INLINE
  ConstTupleIterator(const TupleIterator<ArrayType, TupleSize>& o) noexcept
    : Ref{ o.Ref }
  {
  }

  VTK_ITER_INLINE
  ConstTupleIterator(const ConstTupleIterator& o) noexcept = default;
  VTK_ITER_INLINE
  ConstTupleIterator& operator=(const ConstTupleIterator& o) noexcept = default;

  VTK_ITER_INLINE
  ConstTupleIterator& operator++() noexcept // prefix
  {
    this->Ref.Tuple += this->Ref.NumComps.value;
    return *this;
  }

  VTK_ITER_INLINE
  ConstTupleIterator operator++(int) noexcept // postfix
  {
    auto tuple = this->Ref.Tuple;
    this->Ref.Tuple += this->Ref.NumComps.value;
    return ConstTupleIterator{ tuple, this->Ref.NumComps };
  }

  VTK_ITER_INLINE
  ConstTupleIterator& operator--() noexcept // prefix
  {
    this->Ref.Tuple -= this->Ref.NumComps.value;
    return *this;
  }

  VTK_ITER_INLINE
  ConstTupleIterator operator--(int) noexcept // postfix
  {
    auto tuple = this->Ref.Tuple;
    this->Ref.Tuple -= this->Ref.NumComps.value;
    return ConstTupleIterator{ tuple, this->Ref.NumComps };
  }

  VTK_ITER_INLINE
  reference operator[](difference_type i) noexcept
  {
    return reference{ this->Ref.Tuple + i * this->Ref.NumComps, this->Ref.NumComps };
  }

  VTK_ITER_INLINE
  reference operator*() noexcept { return this->Ref; }

  VTK_ITER_INLINE
  pointer& operator->() noexcept { return this->Ref; }

#define VTK_TMP_MAKE_OPERATOR(OP)                                                                  \
  friend VTK_ITER_INLINE bool operator OP(                                                         \
    const ConstTupleIterator& lhs, const ConstTupleIterator& rhs) noexcept                         \
  {                                                                                                \
    return lhs.GetTuple() OP rhs.GetTuple();                                                       \
  }

  VTK_TMP_MAKE_OPERATOR(==)
  VTK_TMP_MAKE_OPERATOR(!=)
  VTK_TMP_MAKE_OPERATOR(<)
  VTK_TMP_MAKE_OPERATOR(>)
  VTK_TMP_MAKE_OPERATOR(<=)
  VTK_TMP_MAKE_OPERATOR(>=)

#undef VTK_TMP_MAKE_OPERATOR

  VTK_ITER_INLINE
  ConstTupleIterator& operator+=(difference_type offset) noexcept
  {
    this->Ref.Tuple += offset * this->Ref.NumComps.value;
    return *this;
  }

  friend VTK_ITER_INLINE ConstTupleIterator operator+(
    const ConstTupleIterator& it, difference_type offset) noexcept
  {
    return ConstTupleIterator{ it.GetTuple() + offset * it.GetNumComps().value, it.GetNumComps() };
  }

  friend VTK_ITER_INLINE ConstTupleIterator operator+(
    difference_type offset, const ConstTupleIterator& it) noexcept
  {
    return ConstTupleIterator{ it.GetTuple() + offset * it.GetNumComps().value, it.GetNumComps() };
  }

  VTK_ITER_INLINE
  ConstTupleIterator& operator-=(difference_type offset) noexcept
  {
    this->Ref.Tuple -= offset * this->Ref.NumComps.value;
    return *this;
  }

  friend VTK_ITER_INLINE ConstTupleIterator operator-(
    const ConstTupleIterator& it, difference_type offset) noexcept
  {
    return ConstTupleIterator{ it.GetTuple() - offset * it.GetNumComps().value, it.GetNumComps() };
  }

  friend VTK_ITER_INLINE difference_type operator-(
    const ConstTupleIterator& it1, const ConstTupleIterator& it2) noexcept
  {
    return static_cast<difference_type>(
      (it1.GetTuple() - it2.GetTuple()) / it1.GetNumComps().value);
  }

  friend VTK_ITER_INLINE void swap(ConstTupleIterator& lhs, ConstTupleIterator& rhs) noexcept
  {
    using std::swap;
    swap(lhs.GetTuple(), rhs.GetTuple());
    swap(lhs.GetNumComps(), rhs.GetNumComps());
  }

private:
  VTK_ITER_INLINE
  const ValueType*& GetTuple() noexcept { return this->Ref.Tuple; }
  VTK_ITER_INLINE
  const ValueType* GetTuple() const noexcept { return this->Ref.Tuple; }
  VTK_ITER_INLINE
  NumCompsType& GetNumComps() noexcept { return this->Ref.NumComps; }
  VTK_ITER_INLINE
  NumCompsType GetNumComps() const noexcept { return this->Ref.NumComps; }

  ConstTupleReference<ArrayType, TupleSize> Ref;
};

//------------------------------------------------------------------------------
// Tuple iterator
template <typename ValueType, ComponentIdType TupleSize>
struct TupleIterator<vtkAOSDataArrayTemplate<ValueType>, TupleSize>
{
private:
  using ArrayType = vtkAOSDataArrayTemplate<ValueType>;
  using NumCompsType = GenericTupleSize<TupleSize>;

public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = TupleReference<ArrayType, TupleSize>;
  using difference_type = TupleIdType;
  using pointer = TupleReference<ArrayType, TupleSize>;
  using reference = TupleReference<ArrayType, TupleSize>;

  VTK_ITER_INLINE
  TupleIterator() noexcept = default;

  VTK_ITER_INLINE
  TupleIterator(ValueType* tuple, NumCompsType numComps) noexcept
    : Ref(tuple, numComps)
  {
  }

  VTK_ITER_INLINE
  TupleIterator(const TupleIterator& o) noexcept = default;

  VTK_ITER_INLINE
  TupleIterator& operator=(const TupleIterator& o) noexcept
  {
    this->Ref.CopyReference(o.Ref);
    return *this;
  }

  VTK_ITER_INLINE
  TupleIterator& operator++() noexcept // prefix
  {
    this->Ref.Tuple += this->Ref.NumComps.value;
    return *this;
  }

  VTK_ITER_INLINE
  TupleIterator operator++(int) noexcept // postfix
  {
    auto tuple = this->Ref.Tuple;
    this->Ref.Tuple += this->Ref.NumComps.value;
    return TupleIterator{ tuple, this->Ref.NumComps };
  }

  VTK_ITER_INLINE
  TupleIterator& operator--() noexcept // prefix
  {
    this->Ref.Tuple -= this->Ref.NumComps.value;
    return *this;
  }

  VTK_ITER_INLINE
  TupleIterator operator--(int) noexcept // postfix
  {
    auto tuple = this->Ref.Tuple;
    this->Ref.Tuple -= this->Ref.NumComps.value;
    return TupleIterator{ tuple, this->Ref.NumComps };
  }

  VTK_ITER_INLINE
  reference operator[](difference_type i) noexcept
  {
    return reference{ this->Ref.Tuple + i * this->Ref.NumComps.value, this->Ref.NumComps };
  }

  reference operator*() noexcept { return this->Ref; }

  pointer& operator->() noexcept { return this->Ref; }

#define VTK_TMP_MAKE_OPERATOR(OP)                                                                  \
  friend VTK_ITER_INLINE bool operator OP(                                                         \
    const TupleIterator& lhs, const TupleIterator& rhs) noexcept                                   \
  {                                                                                                \
    return lhs.GetTuple() OP rhs.GetTuple();                                                       \
  }

  VTK_TMP_MAKE_OPERATOR(==)
  VTK_TMP_MAKE_OPERATOR(!=)
  VTK_TMP_MAKE_OPERATOR(<)
  VTK_TMP_MAKE_OPERATOR(>)
  VTK_TMP_MAKE_OPERATOR(<=)
  VTK_TMP_MAKE_OPERATOR(>=)

#undef VTK_TMP_MAKE_OPERATOR

  VTK_ITER_INLINE
  TupleIterator& operator+=(difference_type offset) noexcept
  {
    this->Ref.Tuple += offset * this->Ref.NumComps.value;
    return *this;
  }

  friend VTK_ITER_INLINE TupleIterator operator+(
    const TupleIterator& it, difference_type offset) noexcept
  {
    return TupleIterator{ it.GetTuple() + offset * it.GetNumComps().value, it.GetNumComps() };
  }

  friend VTK_ITER_INLINE TupleIterator operator+(
    difference_type offset, const TupleIterator& it) noexcept
  {
    return TupleIterator{ it.GetTuple() + offset * it.GetNumComps().value, it.GetNumComps() };
  }

  VTK_ITER_INLINE
  TupleIterator& operator-=(difference_type offset) noexcept
  {
    this->Ref.Tuple -= offset * this->Ref.NumComps.value;
    return *this;
  }

  friend VTK_ITER_INLINE TupleIterator operator-(
    const TupleIterator& it, difference_type offset) noexcept
  {
    return TupleIterator{ it.GetTuple() - offset * it.GetNumComps().value, it.GetNumComps() };
  }

  friend VTK_ITER_INLINE difference_type operator-(
    const TupleIterator& it1, const TupleIterator& it2) noexcept
  {
    return static_cast<difference_type>(
      (it1.GetTuple() - it2.GetTuple()) / it1.GetNumComps().value);
  }

  friend VTK_ITER_INLINE void swap(TupleIterator& lhs, TupleIterator& rhs) noexcept
  {
    using std::swap;
    swap(lhs.GetTuple(), rhs.GetTuple());
    swap(lhs.GetNumComps(), rhs.GetNumComps());
  }

  friend struct ConstTupleIterator<ArrayType, TupleSize>;

protected:
  VTK_ITER_INLINE
  ValueType* GetTuple() const noexcept { return this->Ref.Tuple; }
  VTK_ITER_INLINE
  ValueType*& GetTuple() noexcept { return this->Ref.Tuple; }
  VTK_ITER_INLINE
  NumCompsType GetNumComps() const noexcept { return this->Ref.NumComps; }
  VTK_ITER_INLINE
  NumCompsType& GetNumComps() noexcept { return this->Ref.NumComps; }

  TupleReference<ArrayType, TupleSize> Ref;
};

//------------------------------------------------------------------------------
// Tuple range
template <typename ValueType, ComponentIdType TupleSize>
struct TupleRange<vtkAOSDataArrayTemplate<ValueType>, TupleSize>
{
  using ArrayType = vtkAOSDataArrayTemplate<ValueType>;
  using APIType = GetAPIType<ArrayType>;

private:
  static_assert(IsValidTupleSize<TupleSize>::value, "Invalid tuple size.");
  static_assert(IsVtkDataArray<ArrayType>::value, "Invalid array type.");

  using NumCompsType = GenericTupleSize<TupleSize>;

public:
  using TupleIteratorType = TupleIterator<ArrayType, TupleSize>;
  using ConstTupleIteratorType = ConstTupleIterator<ArrayType, TupleSize>;
  using TupleReferenceType = TupleReference<ArrayType, TupleSize>;
  using ConstTupleReferenceType = ConstTupleReference<ArrayType, TupleSize>;
  using ComponentIteratorType = APIType*;
  using ConstComponentIteratorType = APIType const*;
  using ComponentReferenceType = APIType&;
  using ConstComponentReferenceType = const APIType&;
  using ComponentType = APIType;

  using size_type = TupleIdType;
  using iterator = TupleIteratorType;
  using const_iterator = ConstTupleIteratorType;
  using reference = TupleReferenceType;
  using const_reference = ConstTupleReferenceType;

  // May be DynamicTupleSize, or the actual tuple size.
  constexpr static ComponentIdType TupleSizeTag = TupleSize;

  VTK_ITER_INLINE
  TupleRange() noexcept = default;

  VTK_ITER_INLINE
  TupleRange(ArrayType* arr, TupleIdType beginTuple, TupleIdType endTuple) noexcept
    : Array(arr)
    , NumComps(arr)
    , BeginTuple(beginTuple)
    , EndTuple(endTuple)
  {
    assert(this->Array);
    assert(beginTuple >= 0 && beginTuple <= endTuple);
    assert(endTuple >= 0 && endTuple <= this->Array->GetNumberOfTuples());
  }

  VTK_ITER_INLINE
  TupleRange GetSubRange(TupleIdType beginTuple = 0, TupleIdType endTuple = -1) const noexcept
  {
    const TupleIdType curBegin = this->GetTupleId(this->GetTuplePointer(this->BeginTuple));
    const TupleIdType realBegin = curBegin + beginTuple;
    const TupleIdType realEnd =
      endTuple >= 0 ? curBegin + endTuple : this->GetTupleId(this->GetTuplePointer(this->EndTuple));

    return TupleRange{ this->Array, realBegin, realEnd };
  }

  VTK_ITER_INLINE
  ArrayType* GetArray() const noexcept { return this->Array; }

  VTK_ITER_INLINE
  ComponentIdType GetTupleSize() const noexcept { return this->NumComps.value; }

  VTK_ITER_INLINE
  TupleIdType GetBeginTupleId() const noexcept
  {
    return this->GetTupleId(this->GetTuplePointer(this->BeginTuple));
  }

  VTK_ITER_INLINE
  TupleIdType GetEndTupleId() const noexcept
  {
    return this->GetTupleId(this->GetTuplePointer(this->EndTuple));
  }

  VTK_ITER_INLINE
  size_type size() const noexcept
  {
    return static_cast<size_type>(
             this->GetTuplePointer(this->EndTuple) - this->GetTuplePointer(this->BeginTuple)) /
      static_cast<size_type>(this->NumComps.value);
  }

  VTK_ITER_INLINE
  iterator begin() noexcept
  {
    return iterator(this->GetTuplePointer(this->BeginTuple), this->NumComps);
  }

  VTK_ITER_INLINE
  iterator end() noexcept
  {
    return iterator(this->GetTuplePointer(this->EndTuple), this->NumComps);
  }

  VTK_ITER_INLINE
  const_iterator begin() const noexcept
  {
    return const_iterator(this->GetTuplePointer(this->BeginTuple), this->NumComps);
  }

  VTK_ITER_INLINE
  const_iterator end() const noexcept
  {
    return const_iterator(this->GetTuplePointer(this->EndTuple), this->NumComps);
  }

  VTK_ITER_INLINE
  const_iterator cbegin() const noexcept
  {
    return const_iterator(this->GetTuplePointer(this->BeginTuple), this->NumComps);
  }

  VTK_ITER_INLINE
  const_iterator cend() const noexcept
  {
    return const_iterator(this->GetTuplePointer(this->EndTuple), this->NumComps);
  }

  VTK_ITER_INLINE
  reference operator[](size_type i) noexcept
  {
    return reference{ this->Array->Buffer->GetBuffer() +
        (this->BeginTuple + i) * this->NumComps.value,
      this->NumComps };
  }

  VTK_ITER_INLINE
  const_reference operator[](size_type i) const noexcept
  {
    return const_reference{ this->Array->Buffer->GetBuffer() +
        (this->BeginTuple + i) * this->NumComps.value,
      this->NumComps };
  }

  VTK_ITER_INLINE void GetTuple(size_type i, ValueType* tuple) const noexcept
  {
    const ValueType* tuplePtr =
      this->Array->Buffer->GetBuffer() + (this->BeginTuple + i) * this->NumComps.value;
    for (ComponentIdType c = 0; c < this->NumComps.value; ++c)
    {
      tuple[c] = tuplePtr[c];
    }
  }

  template <typename VT = ValueType>
  typename std::enable_if<!std::is_same<VT, double>::value>::type VTK_ITER_INLINE GetTuple(
    size_type i, double* tuple) const noexcept
  {
    const ValueType* tuplePtr =
      this->Array->Buffer->GetBuffer() + (this->BeginTuple + i) * this->NumComps.value;
    for (ComponentIdType c = 0; c < this->NumComps.value; ++c)
    {
      tuple[c] = static_cast<double>(tuplePtr[c]);
    }
  }

  VTK_ITER_INLINE void SetTuple(size_type i, const ValueType* tuple) noexcept
  {
    ValueType* tuplePtr =
      this->Array->Buffer->GetBuffer() + (this->BeginTuple + i) * this->NumComps.value;
    for (ComponentIdType c = 0; c < this->NumComps.value; ++c)
    {
      tuplePtr[c] = tuple[c];
    }
  }

  template <typename VT = ValueType>
  typename std::enable_if<!std::is_same<VT, double>::value>::type VTK_ITER_INLINE SetTuple(
    size_type i, const double* tuple) noexcept
  {
    ValueType* tuplePtr =
      this->Array->Buffer->GetBuffer() + (this->BeginTuple + i) * this->NumComps.value;
    for (ComponentIdType c = 0; c < this->NumComps.value; ++c)
    {
      tuplePtr[c] = static_cast<ValueType>(tuple[c]);
    }
  }

private:
  VTK_ITER_INLINE
  ValueType* GetTuplePointer(vtkIdType tuple) const noexcept
  {
    return this->Array->Buffer->GetBuffer() + (tuple * this->NumComps.value);
  }

  VTK_ITER_INLINE
  TupleIdType GetTupleId(const ValueType* ptr) const noexcept
  {
    return static_cast<TupleIdType>((ptr - this->Array->GetPointer(0)) / this->NumComps.value);
  }

  mutable ArrayType* Array{ nullptr };
  NumCompsType NumComps{};
  TupleIdType BeginTuple{ 0 };
  TupleIdType EndTuple{ 0 };
};

// Unimplemented, only used inside decltype in SelectTupleRange:
template <typename ArrayType, ComponentIdType TupleSize,
  // Convenience:
  typename ValueType = typename ArrayType::ValueType,
  typename AOSArrayType = vtkAOSDataArrayTemplate<ValueType>,
  // SFINAE to select AOS arrays:
  typename = typename std::enable_if<IsAOSDataArray<ArrayType>::value>::type>
TupleRange<AOSArrayType, TupleSize> DeclareTupleRangeSpecialization(ArrayType*);

VTK_ABI_NAMESPACE_END
} // end namespace detail
} // end namespace vtk

VTK_ITER_OPTIMIZE_END

#endif // VTK_DEBUG_RANGE_ITERATORS
#endif // vtkDataArrayTupleRange_AOS_h

// VTK-HeaderTest-Exclude: vtkDataArrayTupleRange_AOS.h
