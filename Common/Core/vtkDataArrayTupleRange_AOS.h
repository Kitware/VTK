/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArrayTupleRange_AOS.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * Specialization of tuple ranges and iterators for vtkAOSDataArrayTemplate.
 */

#ifndef vtkDataArrayTupleRange_AOS_h
#define vtkDataArrayTupleRange_AOS_h

#include "vtkAOSDataArrayTemplate.h"
#include "vtkDataArrayMeta.h"
#include "vtkDataArrayTupleRange_Generic.h"

#include <cassert>
#include <algorithm>
#include <iterator>
#include <type_traits>

// Disable this specialization when iterator debugging is requested:
#ifndef VTK_DEBUG_RANGE_ITERATORS

namespace vtk
{

namespace detail
{

// Forward decs for friends/args
template <typename ArrayType, ComponentIdType> struct ConstTupleReference;
template <typename ArrayType, ComponentIdType> struct TupleReference;
template <typename ArrayType, ComponentIdType> struct ConstTupleIterator;
template <typename ArrayType, ComponentIdType> struct TupleIterator;
template <typename ArrayType, ComponentIdType> struct TupleRange;

//------------------------------------------------------------------------------
// Const tuple reference
template <typename ValueType,
          ComponentIdType TupleSize>
struct ConstTupleReference<vtkAOSDataArrayTemplate<ValueType>,
                           TupleSize>
{
private:
  using ArrayType = vtkAOSDataArrayTemplate<ValueType>;
  using NumCompsType = GenericTupleSize<TupleSize>;
  using APIType = ValueType;

public:
  using size_type = ComponentIdType;
  using value_type = APIType;
  using iterator = const ValueType*;
  using const_iterator = const ValueType*;

  ConstTupleReference(const ValueType *tuple, NumCompsType numComps) noexcept
    : Tuple(tuple)
    , NumComps(numComps)
  {
  }

  // Allow this type to masquerade as a pointer, so that tupleIiter->foo works.
  ConstTupleReference* operator->() noexcept { return this; }
  const ConstTupleReference* operator->() const noexcept { return this; }

  // Caller must ensure that there are size() elements in array.
  void GetTuple(volatile APIType *tuple) const noexcept
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
    VTK_ASSUME(this->NumComps.value > 0);
    for (ComponentIdType i = 0; i < this->NumComps.value; ++i)
    {
      tuple[i] = this->Tuple[i];
    }
  }

  // skips some runtime checks when both sizes are fixed:
  template <typename OArrayType,
            ComponentIdType OSize>
  EnableIfStaticTupleSizes<TupleSize, OSize, bool>
  operator==(const TupleReference<OArrayType, OSize> &other) const noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_convertible<OAPIType, APIType>{}),
                  "Incompatible types when assigning tuples.");

    // SFINAE guarantees that the tuple sizes are not dynamic in this overload:
    static_assert(TupleSize == OSize,
                  "Cannot assign tuples with different sizes.");

    return std::equal(this->cbegin(), this->cend(), other.cbegin());
  }

  // Needs a runtime check:
  template <typename OArrayType,
            ComponentIdType OSize>
  EnableIfEitherTupleSizeIsDynamic<TupleSize, OSize, bool>
  operator==(const TupleReference<OArrayType, OSize> &other) const noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_convertible<OAPIType, APIType>{}),
                  "Incompatible types when assigning tuples.");

    // Need to check the size at runtime :-(
    if (other.size() != this->NumComps.value)
    {
      return false;
    }

    return std::equal(this->cbegin(), this->cend(), other.cbegin());
  }

  // skips some runtime checks when both sizes are fixed:
  template <typename OArrayType,
            ComponentIdType OSize>
  EnableIfStaticTupleSizes<TupleSize, OSize, bool>
  operator==(const ConstTupleReference<OArrayType, OSize> &other) const noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_convertible<OAPIType, APIType>{}),
                  "Incompatible types when assigning tuples.");

    // SFINAE guarantees that the tuple sizes are not dynamic in this overload:
    static_assert(TupleSize == OSize,
                  "Cannot assign tuples with different sizes.");

    return std::equal(this->cbegin(), this->cend(), other.cbegin());
  }

  // Needs a runtime check:
  template <typename OArrayType,
            ComponentIdType OSize>
  EnableIfEitherTupleSizeIsDynamic<TupleSize, OSize, bool>
  operator==(const ConstTupleReference<OArrayType, OSize> &other) const noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_convertible<OAPIType, APIType>{}),
                  "Incompatible types when assigning tuples.");

    // Need to check the size at runtime :-(
    if (other.size() != this->NumComps.value)
    {
      return false;
    }

    return std::equal(this->cbegin(), this->cend(), other.cbegin());
  }

  template <typename OArrayType,
            ComponentIdType OSize>
  bool operator!=(const TupleReference<OArrayType, OSize>& o) const noexcept
  {
    return !(*this == o);
  }

  template <typename OArray,
            ComponentIdType OSize>
  bool operator!=(const ConstTupleReference<OArray, OSize>& o) const noexcept
  {
    return !(*this == o);
  }

  // Intentionally disabled. See vtk::DataArrayTupleRange documentation.
#if 0
  const_reference operator[](size_type i) const noexcept
  {
    return this->Tuple[i];
  }
#endif

  size_type size() const noexcept { return this->NumComps.value; }

  const_iterator begin() const noexcept
  {
    return const_iterator{this->Tuple};
  }

  const_iterator end() const noexcept
  {
    return const_iterator{this->Tuple + this->NumComps.value};
  }

  const_iterator cbegin() const noexcept
  {
    return const_iterator{this->Tuple};
  }

  const_iterator cend() const noexcept
  {
    return const_iterator{this->Tuple + this->NumComps.value};
  }

  friend struct ConstTupleIterator<ArrayType, TupleSize>;

protected:
  // Intentionally hidden. See vtk::DataArrayTupleRange documentation.
  ConstTupleReference(const ConstTupleReference&) noexcept = default;
  ConstTupleReference& operator=(const ConstTupleReference&) noexcept = default;

  const ValueType *Tuple;
  NumCompsType NumComps;
};

//------------------------------------------------------------------------------
// Tuple reference
template <typename ValueType,
          ComponentIdType TupleSize>
struct TupleReference<vtkAOSDataArrayTemplate<ValueType>,
                      TupleSize>
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

  TupleReference(ValueType *tuple, NumCompsType numComps) noexcept
    : Tuple(tuple)
    , NumComps(numComps)
  {
  }

  // Allow this type to masquerade as a pointer, so that tupleIiter->foo works.
  TupleReference* operator->() noexcept { return this; }
  const TupleReference* operator->() const noexcept { return this; }

  // Caller must ensure that there are size() elements in array.
  void GetTuple(volatile APIType *tuple) const noexcept
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
    VTK_ASSUME(this->NumComps.value > 0);
    for (ComponentIdType i = 0; i < this->NumComps.value; ++i)
    {
      tuple[i] = this->Tuple[i];
    }
  }

  // Caller must ensure that there are size() elements in array.
  void SetTuple(const APIType *tuple) noexcept
  {
    volatile APIType *out = this->Tuple;
    // Yes, this variable argument is marked volatile. See the explanation in
    // GetTuple.
    VTK_ASSUME(this->NumComps.value > 0);
    for (ComponentIdType i = 0; i < this->NumComps.value; ++i)
    {
      out[i] = tuple[i];
    }
  }

  TupleReference& operator=(const TupleReference& other) noexcept
  {
    std::copy_n(other.cbegin(), this->NumComps.value, this->begin());
    return *this;
  }

  template <typename OArrayType,
            ComponentIdType OSize>
  EnableIfStaticTupleSizes<TupleSize, OSize, TupleReference&>
  operator=(const TupleReference<OArrayType, OSize> &other) noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_convertible<OAPIType, APIType>{}),
                  "Incompatible types when assigning tuples.");

    // SFINAE guarantees that the tuple sizes are not dynamic in this overload:
    static_assert(TupleSize == OSize,
                  "Cannot assign tuples with different sizes.");

    std::copy_n(other.cbegin(), OSize, this->begin());
    return *this;
  }

  template <typename OArrayType,
            ComponentIdType OSize>
  EnableIfEitherTupleSizeIsDynamic<TupleSize, OSize, TupleReference&>
  operator=(const TupleReference<OArrayType, OSize> &other) noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_convertible<OAPIType, APIType>{}),
                  "Incompatible types when assigning tuples.");

    // Note that the sizes are not checked here. Enable
    // VTK_DEBUG_RANGE_ITERATORS to enable check.
    std::copy_n(other.cbegin(), this->NumComps.value, this->begin());
    return *this;
  }

  template <typename OArrayType,
            ComponentIdType OSize>
  EnableIfStaticTupleSizes<TupleSize, OSize, TupleReference&>
  operator=(const ConstTupleReference<OArrayType, OSize> &other) noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_convertible<OAPIType, APIType>{}),
                  "Incompatible types when assigning tuples.");

    // SFINAE guarantees that the tuple sizes are not dynamic in this overload:
    static_assert(TupleSize == OSize,
                  "Cannot assign tuples with different sizes.");

    std::copy_n(other.cbegin(), OSize, this->begin());
    return *this;
  }

  template <typename OArrayType,
            ComponentIdType OSize>
  EnableIfEitherTupleSizeIsDynamic<TupleSize, OSize, TupleReference&>
  operator=(const ConstTupleReference<OArrayType, OSize> &other) noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_convertible<OAPIType, APIType>{}),
                  "Incompatible types when assigning tuples.");

    // Note that the sizes are not checked here. Enable
    // VTK_DEBUG_RANGE_ITERATORS to enable check.
    std::copy_n(other.cbegin(), this->NumComps.value, this->begin());
    return *this;
  }

  // skips some runtime checks when both sizes are fixed:
  template <typename OArrayType,
            ComponentIdType OSize>
  EnableIfStaticTupleSizes<TupleSize, OSize, bool>
  operator==(const TupleReference<OArrayType, OSize> &other) const noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_convertible<OAPIType, APIType>{}),
                  "Incompatible types when assigning tuples.");

    // SFINAE guarantees that the tuple sizes are not dynamic in this overload:
    static_assert(TupleSize == OSize,
                  "Cannot assign tuples with different sizes.");

    return std::equal(this->cbegin(), this->cend(), other.cbegin());
  }

  // Needs a runtime check:
  template <typename OArrayType,
            ComponentIdType OSize>
  EnableIfEitherTupleSizeIsDynamic<TupleSize, OSize, bool>
  operator==(const TupleReference<OArrayType, OSize> &other) const noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_convertible<OAPIType, APIType>{}),
                  "Incompatible types when assigning tuples.");

    // Note that the sizes are not checked here. Enable
    // VTK_DEBUG_RANGE_ITERATORS to enable check.
    return std::equal(this->cbegin(), this->cend(), other.cbegin());
  }

  // skips some runtime checks when both sizes are fixed:
  template <typename OArrayType,
            ComponentIdType OSize>
  EnableIfStaticTupleSizes<TupleSize, OSize, bool>
  operator==(const ConstTupleReference<OArrayType, OSize> &other) const noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_convertible<OAPIType, APIType>{}),
                  "Incompatible types when assigning tuples.");

    // SFINAE guarantees that the tuple sizes are not dynamic in this overload:
    static_assert(TupleSize == OSize,
                  "Cannot assign tuples with different sizes.");

    return std::equal(this->cbegin(), this->cend(), other.cbegin());
  }

  // Needs a runtime check:
  template <typename OArrayType,
            ComponentIdType OSize>
  EnableIfEitherTupleSizeIsDynamic<TupleSize, OSize, bool>
  operator==(const ConstTupleReference<OArrayType, OSize> &other) const noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_convertible<OAPIType, APIType>{}),
                  "Incompatible types when assigning tuples.");

    // Note that the sizes are not checked here. Enable
    // VTK_DEBUG_RANGE_ITERATORS to enable check.
    return std::equal(this->cbegin(), this->cend(), other.cbegin());
  }

  template <typename OArrayType,
            ComponentIdType OSize>
  bool operator!=(const TupleReference<OArrayType, OSize>& o) const noexcept
  {
    return !(*this == o);
  }

  template <typename OArray,
            ComponentIdType OSize>
  bool operator!=(const ConstTupleReference<OArray, OSize>& o) const noexcept
  {
    return !(*this == o);
  }

  // skips some runtime checks:
  template <typename OArrayType,
            ComponentIdType OSize>
  EnableIfStaticTupleSizes<TupleSize, OSize, void>
  swap(TupleReference<OArrayType, OSize> &other) noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_same<OAPIType, APIType>{}),
                  "Incompatible types when swapping tuples.");

    // SFINAE guarantees that the tuple sizes are not dynamic in this overload:
    static_assert(TupleSize == OSize,
                  "Cannot swap tuples with different sizes.");

    std::swap_ranges(this->begin(), this->end(), other.begin());
  }

  // Needs a runtime check:
  template <typename OArrayType,
            ComponentIdType OSize>
  EnableIfEitherTupleSizeIsDynamic<TupleSize, OSize, void>
  swap(TupleReference<OArrayType, OSize> &other) noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_same<OAPIType, APIType>{}),
                  "Incompatible types when swapping tuples.");

    // Note that the sizes are not checked here. Enable
    // VTK_DEBUG_RANGE_ITERATORS to enable check.
    std::swap_ranges(this->begin(), this->end(), other.begin());
  }

  friend void swap(TupleReference &a, TupleReference &b) noexcept
  {
    a.swap(b);
  }

  template <typename OArray, ComponentIdType OSize>
  friend void swap(TupleReference &a, TupleReference<OArray, OSize> &b) noexcept
  {
    a.swap(b);
  }

  // Intentionally disabled. See vtk::DataArrayTupleRange documentation.
#if 0
  reference operator[](size_type i) const noexcept
  {
    return this->Tuple[i];
  }
#endif

  void fill(const value_type &v) noexcept
  {
    std::fill(this->begin(), this->end(), v);
  }

  size_type size() const noexcept { return this->NumComps.value; }

  iterator begin() noexcept
  {
    return iterator{this->Tuple};
  }

  iterator end() noexcept
  {
    return iterator{this->Tuple + this->NumComps.value};
  }

  const_iterator begin() const noexcept
  {
    return const_iterator{this->Tuple};
  }

  const_iterator end() const noexcept
  {
    return const_iterator{this->Tuple + this->NumComps.value};
  }

  const_iterator cbegin() const noexcept
  {
    return const_iterator{this->Tuple};
  }

  const_iterator cend() const noexcept
  {
    return const_iterator{this->Tuple + this->NumComps.value};
  }

  friend struct TupleIterator<ArrayType, TupleSize>;

protected:

  // Intentionally hidden. See vtk::DataArrayTupleRange documentation.
  TupleReference(const TupleReference&) noexcept = default;

  void CopyReference(const TupleReference& o) noexcept
  {
    this->Tuple = o.Tuple;
    this->NumComps = o.NumComps;
  }

  ValueType *Tuple;
  NumCompsType NumComps;
};

//------------------------------------------------------------------------------
// Const tuple iterator
template <typename ValueType,
          ComponentIdType TupleSize>
struct ConstTupleIterator<vtkAOSDataArrayTemplate<ValueType>,
                          TupleSize> :
    public std::iterator<std::random_access_iterator_tag,
                         ConstTupleReference<vtkAOSDataArrayTemplate<ValueType>, TupleSize>,
                         TupleIdType,
                         ConstTupleReference<vtkAOSDataArrayTemplate<ValueType>, TupleSize>,
                         ConstTupleReference<vtkAOSDataArrayTemplate<ValueType>, TupleSize>>
{
private:
  using ArrayType = vtkAOSDataArrayTemplate<ValueType>;
  using NumCompsType = GenericTupleSize<TupleSize>;
  using Superclass = std::iterator<std::random_access_iterator_tag,
                                   ConstTupleReference<ArrayType, TupleSize>,
                                   TupleIdType,
                                   ConstTupleReference<ArrayType, TupleSize>,
                                   ConstTupleReference<ArrayType, TupleSize>>;

public:
  using iterator_category = typename Superclass::iterator_category;
  using value_type = typename Superclass::value_type;
  using difference_type = typename Superclass::difference_type;
  using pointer = typename Superclass::pointer;
  using reference = typename Superclass::reference;

  ConstTupleIterator(const ValueType* tuple, NumCompsType numComps) noexcept
    : Ref(tuple, numComps)
  {
  }

  ConstTupleIterator(const ConstTupleIterator& o) noexcept = default;
  ConstTupleIterator& operator=(const ConstTupleIterator& o) noexcept = default;

  ConstTupleIterator& operator++() noexcept // prefix
  {
    this->Ref.Tuple += this->Ref.NumComps.value;
    return *this;
  }

  ConstTupleIterator operator++(int) noexcept // postfix
  {
    auto tuple = this->Ref.Tuple;
    this->Ref.Tuple += this->Ref.NumComps.value;
    return ConstTupleIterator{tuple, this->Ref.NumComps};
  }

  ConstTupleIterator& operator--() noexcept // prefix
  {
    this->Ref.Tuple -= this->Ref.NumComps.value;
    return *this;
  }

  ConstTupleIterator operator--(int) noexcept // postfix
  {
    auto tuple = this->Ref.Tuple;
    this->Ref.Tuple -= this->Ref.NumComps.value;
    return ConstTupleIterator{tuple, this->Ref.NumComps};
  }

  // Intentionally disabled. See vtk::DataArrayRange documentation.
#if 0
  reference operator[](difference_type i) const noexcept
  {
    return reference{this->Ref.Tuple + i * this->Ref.NumComps,
                     this->Ref.NumComps};
  }
#endif

  reference& operator*() noexcept
  {
    return this->Ref;
  }

  pointer& operator->() noexcept
  {
    return this->Ref;
  }

#define VTK_TMP_MAKE_OPERATOR(OP) \
  friend bool operator OP (const ConstTupleIterator& lhs, \
                           const ConstTupleIterator& rhs) noexcept \
  { \
    return lhs.GetTuple() OP rhs.GetTuple(); \
  }

  VTK_TMP_MAKE_OPERATOR(==)
  VTK_TMP_MAKE_OPERATOR(!=)
  VTK_TMP_MAKE_OPERATOR(<)
  VTK_TMP_MAKE_OPERATOR(>)
  VTK_TMP_MAKE_OPERATOR(<=)
  VTK_TMP_MAKE_OPERATOR(>=)

#undef VTK_TMP_MAKE_OPERATOR

  ConstTupleIterator& operator+=(difference_type offset) noexcept
  {
    this->Ref.Tuple += offset * this->Ref.NumComps.value;
    return *this;
  }

  friend ConstTupleIterator operator+(const ConstTupleIterator& it,
                                      difference_type offset) noexcept
  {
    return ConstTupleIterator{it.GetTuple() + offset * it.GetNumComps().value,
                              it.GetNumComps()};
  }

  friend ConstTupleIterator operator+(difference_type offset,
                                      const ConstTupleIterator& it) noexcept
  {
    return ConstTupleIterator{it.GetTuple() + offset * it.GetNumComps().value,
                              it.GetNumComps()};
  }

  ConstTupleIterator& operator-=(difference_type offset) noexcept
  {
    this->Ref.Tuple -= offset * this->Ref.NumComps.value;
    return *this;
  }

  friend ConstTupleIterator operator-(const ConstTupleIterator& it,
                                      difference_type offset) noexcept
  {
    return ConstTupleIterator{it.GetTuple() - offset * it.GetNumComps().value,
                              it.GetNumComps()};
  }

  friend difference_type operator-(const ConstTupleIterator& it1,
                                   const ConstTupleIterator& it2) noexcept
  {
    return static_cast<difference_type>((it1.GetTuple() - it2.GetTuple()) /
                                        it1.GetNumComps().value);
  }

  friend void swap(ConstTupleIterator& lhs, ConstTupleIterator &rhs) noexcept
  {
    using std::swap;
    swap(lhs.GetTuple(), rhs.GetTuple());
    swap(lhs.GetNumComps(), rhs.GetNumComps());
  }

private:
   const ValueType*& GetTuple() noexcept { return this->Ref.Tuple; }
   const ValueType* GetTuple() const noexcept { return this->Ref.Tuple; }
   NumCompsType& GetNumComps() noexcept { return this->Ref.NumComps; }
   NumCompsType GetNumComps() const noexcept { return this->Ref.NumComps; }

   ConstTupleReference<ArrayType, TupleSize> Ref;
};

//------------------------------------------------------------------------------
// Tuple iterator
template <typename ValueType,
          ComponentIdType TupleSize>
struct TupleIterator<vtkAOSDataArrayTemplate<ValueType>,
                     TupleSize> :
    public std::iterator<std::random_access_iterator_tag,
                         TupleReference<vtkAOSDataArrayTemplate<ValueType>, TupleSize>,
                         TupleIdType,
                         TupleReference<vtkAOSDataArrayTemplate<ValueType>, TupleSize>,
                         TupleReference<vtkAOSDataArrayTemplate<ValueType>, TupleSize>>
{
private:
  using ArrayType = vtkAOSDataArrayTemplate<ValueType>;
  using NumCompsType = GenericTupleSize<TupleSize>;
  using Superclass = std::iterator<std::random_access_iterator_tag,
                                   TupleReference<ArrayType, TupleSize>,
                                   TupleIdType,
                                   TupleReference<ArrayType, TupleSize>,
                                   TupleReference<ArrayType, TupleSize>>;

public:
  using iterator_category = typename Superclass::iterator_category;
  using value_type = typename Superclass::value_type;
  using difference_type = typename Superclass::difference_type;
  using pointer = typename Superclass::pointer;
  using reference = typename Superclass::reference;

  TupleIterator(ValueType* tuple, NumCompsType numComps) noexcept
    : Ref(tuple, numComps)
  {
  }

  TupleIterator(const TupleIterator& o) noexcept = default;

  TupleIterator& operator=(const TupleIterator& o) noexcept
  {
    this->Ref.CopyReference(o.Ref);
    return *this;
  }

  TupleIterator& operator++() noexcept // prefix
  {
    this->Ref.Tuple += this->Ref.NumComps.value;
    return *this;
  }

  TupleIterator operator++(int) noexcept // postfix
  {
    auto tuple = this->Ref.Tuple;
    this->Ref.Tuple += this->Ref.NumComps.value;
    return TupleIterator{tuple, this->Ref.NumComps};
  }

  TupleIterator& operator--() noexcept // prefix
  {
    this->Ref.Tuple -= this->Ref.NumComps.value;
    return *this;
  }

  TupleIterator operator--(int) noexcept // postfix
  {
    auto tuple = this->Ref.Tuple;
    this->Ref.Tuple -= this->Ref.NumComps.value;
    return TupleIterator{tuple, this->Ref.NumComps};
  }

  // Intentionally disabled. See vtk::DataArrayTupleRange documentation.
#if 0
  reference operator[](difference_type i) const noexcept
  {
    return reference{this->Ref.Tuple + i * this->Ref.NumComps.value,
                     this->Ref.NumComps};
  }
#endif

  reference& operator*() noexcept
  {
    return this->Ref;
  }

  pointer& operator->() noexcept
  {
    return this->Ref;
  }

#define VTK_TMP_MAKE_OPERATOR(OP) \
  friend bool operator OP (const TupleIterator& lhs, \
                           const TupleIterator& rhs) noexcept \
  { \
    return lhs.GetTuple() OP rhs.GetTuple(); \
  }

  VTK_TMP_MAKE_OPERATOR(==)
  VTK_TMP_MAKE_OPERATOR(!=)
  VTK_TMP_MAKE_OPERATOR(<)
  VTK_TMP_MAKE_OPERATOR(>)
  VTK_TMP_MAKE_OPERATOR(<=)
  VTK_TMP_MAKE_OPERATOR(>=)

#undef VTK_TMP_MAKE_OPERATOR

  TupleIterator& operator+=(difference_type offset) noexcept
  {
    this->Ref.Tuple += offset * this->Ref.NumComps.value;
    return *this;
  }

  friend TupleIterator operator+(const TupleIterator& it,
                                 difference_type offset) noexcept
  {
    return TupleIterator{it.GetTuple() + offset * it.GetNumComps().value,
                         it.GetNumComps()};
  }

  friend TupleIterator operator+(difference_type offset,
                                 const TupleIterator& it) noexcept
  {
    return TupleIterator{it.GetTuple() + offset * it.GetNumComps().value,
                         it.GetNumComps()};
  }

  TupleIterator& operator-=(difference_type offset) noexcept
  {
    this->Ref.Tuple -= offset * this->Ref.NumComps.value;
    return *this;
  }

  friend TupleIterator operator-(const TupleIterator& it,
                                 difference_type offset) noexcept
  {
    return TupleIterator{it.GetTuple() - offset * it.GetNumComps().value,
                         it.GetNumComps()};
  }

  friend difference_type operator-(const TupleIterator& it1,
                                   const TupleIterator& it2) noexcept
  {
    return static_cast<difference_type>((it1.GetTuple() - it2.GetTuple()) /
                                        it1.GetNumComps().value);
  }

  friend void swap(TupleIterator& lhs, TupleIterator &rhs) noexcept
  {
    using std::swap;
    swap(lhs.GetTuple(), rhs.GetTuple());
    swap(lhs.GetNumComps(), rhs.GetNumComps());
  }

private:
  ValueType* GetTuple() const noexcept { return this->Ref.Tuple; }
  ValueType*& GetTuple() noexcept { return this->Ref.Tuple; }
  NumCompsType GetNumComps() const noexcept { return this->Ref.NumComps; }
  NumCompsType& GetNumComps() noexcept { return this->Ref.NumComps; }

  TupleReference<ArrayType, TupleSize> Ref;
};

//------------------------------------------------------------------------------
// Tuple range
template <typename ValueType,
          ComponentIdType TupleSize>
struct TupleRange<vtkAOSDataArrayTemplate<ValueType>,
                  TupleSize>
{
  using ArrayType = vtkAOSDataArrayTemplate<ValueType>;

private:
  static_assert(IsValidTupleSize<TupleSize>::value, "Invalid tuple size.");
  static_assert(IsVtkDataArray<ArrayType>::value, "Invalid array type.");

  using APIType = GetAPIType<ArrayType>;
  using NumCompsType = GenericTupleSize<TupleSize>;

public:
  using ComponentType = APIType;

  using size_type = TupleIdType;
  using iterator = TupleIterator<ArrayType, TupleSize>;
  using const_iterator = ConstTupleIterator<ArrayType, TupleSize>;
  using reference = TupleReference<ArrayType, TupleSize>;
  using const_reference = ConstTupleReference<ArrayType, TupleSize>;

  // May be DynamicTupleSize, or the actual tuple size.
  constexpr static ComponentIdType TupleSizeTag = TupleSize;

  TupleRange(ArrayType *arr,
             TupleIdType beginTuple,
             TupleIdType endTuple) noexcept
    : Array(arr)
    , NumComps(arr)
    , BeginTuple(TupleRange::GetTuplePointer(arr, beginTuple))
    , EndTuple(TupleRange::GetTuplePointer(arr, endTuple))
  {
    assert(this->Array);
    assert(beginTuple >= 0 && beginTuple <= endTuple);
    assert(endTuple >= 0 && endTuple <= this->Array->GetNumberOfTuples());
  }

  ArrayType* GetArray() const noexcept { return this->Array; }

  ComponentIdType GetTupleSize() const noexcept { return this->NumComps.value; }

  TupleIdType GetBeginTupleId() const noexcept
  {
    return this->GetTupleId(this->BeginTuple);
  }

  TupleIdType GetEndTupleId() const noexcept
  {
    return this->GetTupleId(this->EndTuple);
  }

  size_type size() const noexcept
  {
    return static_cast<size_type>(this->EndTuple - this->BeginTuple) /
           static_cast<size_type>(this->NumComps.value);
  }

  iterator begin() noexcept
  {
    return iterator(this->BeginTuple, this->NumComps);
  }

  iterator end() noexcept
  {
    return iterator(this->EndTuple, this->NumComps);
  }

  const_iterator begin() const noexcept
  {
    return const_iterator(this->BeginTuple, this->NumComps);
  }

  const_iterator end() const noexcept
  {
    return const_iterator(this->EndTuple, this->NumComps);
  }

  const_iterator cbegin() const noexcept
  {
    return const_iterator(this->BeginTuple, this->NumComps);
  }

  const_iterator cend() const noexcept
  {
    return const_iterator(this->EndTuple, this->NumComps);
  }

private:

  ValueType* GetTuplePointer(ArrayType *array, vtkIdType tuple) const noexcept
  {
    return array->GetPointer(tuple * this->NumComps.value);
  }

  TupleIdType GetTupleId(const ValueType* ptr) const noexcept
  {
    return static_cast<TupleIdType>((ptr - this->Array->GetPointer(0)) /
                                    this->NumComps.value);
  }

  // Store a ref to the array to ensure it stays in memory:
  mutable vtkSmartPointer<ArrayType> Array;
  NumCompsType NumComps;
  ValueType *const BeginTuple;
  ValueType *const EndTuple;
};

} // end namespace detail
} // end namespace vtk

#endif // VTK_DEBUG_RANGE_ITERATORS
#endif // vtkDataArrayTupleRange_AOS_h

// VTK-HeaderTest-Exclude: vtkDataArrayTupleRange_AOS.h
