// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Generic implementation of tuple ranges and iterators, suitable for
 * vtkDataArray and all subclasses.
 */

#ifndef vtkDataArrayTupleRange_Generic_h
#define vtkDataArrayTupleRange_Generic_h

#include "vtkAssume.h"
#include "vtkDataArrayAccessor.h"
#include "vtkDataArrayMeta.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <type_traits>

VTK_ITER_OPTIMIZE_START

namespace vtk
{
namespace detail
{
VTK_ABI_NAMESPACE_BEGIN

// Forward decs for friends/args
template <typename ArrayType, ComponentIdType>
struct ConstComponentReference;
template <typename ArrayType, ComponentIdType>
struct ComponentReference;
template <typename ArrayType, ComponentIdType>
struct ConstComponentIterator;
template <typename ArrayType, ComponentIdType>
struct ComponentIterator;
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
// Const component reference
template <typename ArrayType, ComponentIdType TupleSize>
struct ConstComponentReference
{
private:
  static_assert(IsValidTupleSize<TupleSize>::value, "Invalid tuple size.");
  static_assert(IsVtkDataArray<ArrayType>::value, "Invalid array type.");

  using NumCompsType = GenericTupleSize<TupleSize>;
  using APIType = GetAPIType<ArrayType>;

public:
  using value_type = APIType;

  VTK_ITER_INLINE
  ConstComponentReference() noexcept
    : Array{ nullptr }
    , NumComps{}
    , TupleId{ 0 }
    , ComponentId{ 0 }
  {
  }

  VTK_ITER_INLINE
  ConstComponentReference(
    ArrayType* array, NumCompsType numComps, TupleIdType tuple, ComponentIdType comp) noexcept
    : Array{ array }
    , NumComps{ numComps }
    , TupleId{ tuple }
    , ComponentId{ comp }
  {
    VTK_ITER_ASSERT(array != nullptr, "Invalid array.");
    VTK_ITER_ASSERT(numComps.value > 0, "Invalid number of components.");
    VTK_ITER_ASSERT(
      tuple >= 0 && tuple <= array->GetNumberOfTuples(), "Invalid tuple accessed by iterator.");
    VTK_ITER_ASSERT(comp >= 0 && comp <= array->GetNumberOfComponents(),
      "Invalid component accessed by iterator.");
  }

  VTK_ITER_INLINE
  ConstComponentReference(const ComponentReference<ArrayType, TupleSize>& o)
    : Array{ o.Array }
    , NumComps{ o.NumComps }
    , TupleId{ o.TupleId }
    , ComponentId{ o.ComponentId }
  {
  }

  VTK_ITER_INLINE
  ConstComponentReference(const ConstComponentReference& o) noexcept = default;

  VTK_ITER_INLINE
  ConstComponentReference(ConstComponentReference&& o) noexcept = default;

  VTK_ITER_INLINE
  ConstComponentReference& operator=(const ConstComponentReference& o) noexcept
  {
    VTK_ITER_ASSERT(!this->Array, "Const reference already initialized.");
    // Initialize the reference.
    this->Array = o.Array;
    this->NumComps = o.NumComps;
    this->TupleId = o.TupleId;
    this->ComponentId = o.ComponentId;
  }

  VTK_ITER_INLINE
  ConstComponentReference& operator=(ConstComponentReference&& o) noexcept
  {
    VTK_ITER_ASSERT(!this->Array, "Const reference already initialized.");
    // Initialize the reference.
    this->Array = std::move(o.Array);
    this->NumComps = std::move(o.NumComps);
    this->TupleId = std::move(o.TupleId);
    this->ComponentId = std::move(o.ComponentId);
  }

  VTK_ITER_INLINE operator APIType() const noexcept { return this->castOperator(); }

protected:
  template <typename AT = ArrayType>
  typename std::enable_if<std::is_same<AT, vtkDataArray>::value, APIType>::type VTK_ITER_INLINE
  castOperator() const noexcept
  {
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    VTK_ITER_ASSUME(this->Array->GetNumberOfComponents() == this->NumComps.value);
    return this->Array->GetComponent(this->TupleId, this->ComponentId);
  }

  template <typename AT = ArrayType>
  typename std::enable_if<!std::is_same<AT, vtkDataArray>::value, APIType>::type VTK_ITER_INLINE
  castOperator() const noexcept
  {
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    VTK_ITER_ASSUME(this->Array->GetNumberOfComponents() == this->NumComps.value);
    return this->Array->GetTypedComponent(this->TupleId, this->ComponentId);
  }

  mutable ArrayType* Array;
  NumCompsType NumComps;
  TupleIdType TupleId;
  ComponentIdType ComponentId;
};

//------------------------------------------------------------------------------
// Component reference
template <typename ArrayType, ComponentIdType TupleSize>
struct ComponentReference
{
private:
  static_assert(IsValidTupleSize<TupleSize>::value, "Invalid tuple size.");
  static_assert(IsVtkDataArray<ArrayType>::value, "Invalid array type.");

  using NumCompsType = GenericTupleSize<TupleSize>;
  using APIType = GetAPIType<ArrayType>;

public:
  using value_type = APIType;

  VTK_ITER_INLINE
  ComponentReference() noexcept
    : Array{ nullptr }
    , NumComps{}
    , TupleId{ 0 }
    , ComponentId{ 0 }
  {
  }

  VTK_ITER_INLINE
  ComponentReference(
    ArrayType* array, NumCompsType numComps, TupleIdType tuple, ComponentIdType comp) noexcept
    : Array{ array }
    , NumComps{ numComps }
    , TupleId{ tuple }
    , ComponentId{ comp }
  {
    VTK_ITER_ASSERT(array != nullptr, "Invalid array.");
    VTK_ITER_ASSERT(numComps.value > 0, "Invalid number of components.");
    VTK_ITER_ASSERT(
      tuple >= 0 && tuple <= array->GetNumberOfTuples(), "Invalid tuple accessed by iterator.");
    VTK_ITER_ASSERT(comp >= 0 && comp <= array->GetNumberOfComponents(),
      "Invalid component accessed by iterator.");
  }

  VTK_ITER_INLINE
  ComponentReference(const ComponentReference& o) noexcept = default;
  VTK_ITER_INLINE
  ComponentReference(ComponentReference&& o) noexcept = default;

  VTK_ITER_INLINE
  ComponentReference operator=(const ComponentReference& o) noexcept
  {
    if (this->Array)
    { // Already initialized. Assign the value, not the reference
      return *this = static_cast<APIType>(o);
    }
    else
    { // Initialize the reference.
      this->Array = o.Array;
      this->NumComps = o.NumComps;
      this->TupleId = o.TupleId;
      this->ComponentId = o.ComponentId;

      return *this;
    }
  }

  VTK_ITER_INLINE
  ComponentReference operator=(ComponentReference&& o) noexcept
  {
    if (this->Array)
    { // Already initialized. Assign the value, not the reference
      return *this = std::move(static_cast<APIType>(o));
    }
    else
    { // Initialize the reference.
      this->Array = std::move(o.Array);
      this->NumComps = std::move(o.NumComps);
      this->TupleId = std::move(o.TupleId);
      this->ComponentId = std::move(o.ComponentId);

      return *this;
    }
  }

  template <typename OArray, ComponentIdType OSize>
  VTK_ITER_INLINE ComponentReference operator=(const ComponentReference<OArray, OSize>& o) noexcept
  { // Always copy the value for different reference types:
    const APIType tmp = o;
    return *this = std::move(tmp);
  }

  VTK_ITER_INLINE operator APIType() const noexcept { return this->castOperator(); }

  template <typename AT = ArrayType>
  typename std::enable_if<std::is_same<AT, vtkDataArray>::value, ComponentReference>::type
    VTK_ITER_INLINE
    operator=(APIType val) noexcept
  {
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    VTK_ITER_ASSUME(this->Array->GetNumberOfComponents() == this->NumComps.value);
    this->Array->SetComponent(this->TupleId, this->ComponentId, val);
    return *this;
  }

  template <typename AT = ArrayType>
  typename std::enable_if<!std::is_same<AT, vtkDataArray>::value, ComponentReference>::type
    VTK_ITER_INLINE
    operator=(APIType val) noexcept
  {
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    VTK_ITER_ASSUME(this->Array->GetNumberOfComponents() == this->NumComps.value);
    this->Array->SetTypedComponent(this->TupleId, this->ComponentId, val);
    return *this;
  }

  friend VTK_ITER_INLINE void swap(ComponentReference lhs, ComponentReference rhs) noexcept
  { // Swap values, not references:
    APIType tmp = std::move(static_cast<APIType>(lhs));
    lhs = std::move(static_cast<APIType>(rhs));
    rhs = std::move(tmp);
  }

  template <typename OArray, ComponentIdType OSize>
  friend VTK_ITER_INLINE void swap(
    ComponentReference lhs, ComponentReference<OArray, OSize> rhs) noexcept
  { // Swap values, not references:
    using OAPIType = GetAPIType<OArray>;
    static_assert(
      std::is_same<APIType, OAPIType>::value, "Cannot swap components with different types.");

    APIType tmp = std::move(static_cast<APIType>(lhs));
    lhs = std::move(static_cast<APIType>(rhs));
    rhs = std::move(tmp);
  }

  friend VTK_ITER_INLINE void swap(ComponentReference lhs, APIType& rhs) noexcept
  {
    APIType tmp = std::move(static_cast<APIType>(lhs));
    lhs = std::move(rhs);
    rhs = std::move(tmp);
  }

  friend VTK_ITER_INLINE void swap(APIType& lhs, ComponentReference rhs) noexcept
  {
    APIType tmp = std::move(lhs);
    lhs = std::move(static_cast<APIType>(rhs));
    rhs = std::move(tmp);
  }

  VTK_ITER_INLINE
  ComponentReference operator++() noexcept // prefix
  {
    const APIType newVal = *this + 1;
    *this = newVal;
    return *this;
  }

  VTK_ITER_INLINE
  APIType operator++(int) noexcept // postfix
  {
    const APIType retVal = *this;
    *this = *this + 1;
    return retVal;
  }

  VTK_ITER_INLINE
  ComponentReference operator--() noexcept // prefix
  {
    const APIType newVal = *this - 1;
    *this = newVal;
    return *this;
  }

  VTK_ITER_INLINE
  APIType operator--(int) noexcept // postfix
  {
    const APIType retVal = *this;
    *this = *this - 1;
    return retVal;
  }

#define VTK_REF_OP_OVERLOADS(Op, ImplOp)                                                           \
  friend VTK_ITER_INLINE ComponentReference operator Op(                                           \
    ComponentReference lhs, APIType val) noexcept                                                  \
  {                                                                                                \
    const APIType newVal = lhs ImplOp val;                                                         \
    lhs = newVal;                                                                                  \
    return lhs;                                                                                    \
  }                                                                                                \
  friend VTK_ITER_INLINE ComponentReference operator Op(                                           \
    ComponentReference lhs, ComponentReference val) noexcept                                       \
  {                                                                                                \
    const APIType newVal = lhs ImplOp val;                                                         \
    lhs = newVal;                                                                                  \
    return lhs;                                                                                    \
  }                                                                                                \
  friend VTK_ITER_INLINE APIType& operator Op(APIType& lhs, ComponentReference val) noexcept       \
  {                                                                                                \
    const APIType newVal = lhs ImplOp val;                                                         \
    lhs = newVal;                                                                                  \
    return lhs;                                                                                    \
  }

  VTK_REF_OP_OVERLOADS(+=, +)
  VTK_REF_OP_OVERLOADS(-=, -)
  VTK_REF_OP_OVERLOADS(*=, *)
  VTK_REF_OP_OVERLOADS(/=, /)

#undef VTK_REF_OP_OVERLOADS

  friend struct ConstComponentReference<ArrayType, TupleSize>;
  friend struct ComponentIterator<ArrayType, TupleSize>;

protected:
  template <typename AT = ArrayType>
  typename std::enable_if<std::is_same<AT, vtkDataArray>::value, APIType>::type VTK_ITER_INLINE
  castOperator() const noexcept
  {
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    VTK_ITER_ASSUME(this->Array->GetNumberOfComponents() == this->NumComps.value);
    return this->Array->GetComponent(this->TupleId, this->ComponentId);
  }

  template <typename AT = ArrayType>
  typename std::enable_if<!std::is_same<AT, vtkDataArray>::value, APIType>::type VTK_ITER_INLINE
  castOperator() const noexcept
  {
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    VTK_ITER_ASSUME(this->Array->GetNumberOfComponents() == this->NumComps.value);
    return this->Array->GetTypedComponent(this->TupleId, this->ComponentId);
  }

  VTK_ITER_INLINE
  void CopyReference(const ComponentReference& o) noexcept
  {
    this->Array = o.Array;
    this->NumComps = o.NumComps;
    this->TupleId = o.TupleId;
    this->ComponentId = o.ComponentId;
  }

  mutable ArrayType* Array;
  NumCompsType NumComps;
  TupleIdType TupleId;
  ComponentIdType ComponentId;
};

//------------------------------------------------------------------------------
// Const component iterator
template <typename ArrayType, ComponentIdType TupleSize>
struct ConstComponentIterator
{
private:
  static_assert(IsValidTupleSize<TupleSize>::value, "Invalid tuple size.");
  static_assert(IsVtkDataArray<ArrayType>::value, "Invalid array type.");

  using NumCompsType = GenericTupleSize<TupleSize>;

public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = GetAPIType<ArrayType>;
  using difference_type = ComponentIdType;
  using pointer = void;
  using reference = ConstComponentReference<ArrayType, TupleSize>;

  VTK_ITER_INLINE
  ConstComponentIterator() noexcept
    : Array{ nullptr }
    , TupleId{ 0 }
    , ComponentId{ 0 }
  {
  }

  VTK_ITER_INLINE
  ConstComponentIterator(
    ArrayType* array, NumCompsType numComps, TupleIdType tupleId, ComponentIdType comp) noexcept
    : Array(array)
    , NumComps(numComps)
    , TupleId(tupleId)
    , ComponentId(comp)
  {
    VTK_ITER_ASSERT(array != nullptr, "Invalid array.");
    VTK_ITER_ASSERT(numComps.value > 0, "Invalid number of components.");
    VTK_ITER_ASSERT(tupleId >= 0 && tupleId <= array->GetNumberOfTuples(),
      "Const component iterator at invalid tuple id.");
    VTK_ITER_ASSERT(comp >= 0 && comp <= this->NumComps.value,
      "Const component iterator at invalid component id.");
  }

  VTK_ITER_INLINE
  ConstComponentIterator(const ComponentIterator<ArrayType, TupleSize>& o) noexcept
    : Array{ o.GetArray() }
    , NumComps{ o.GetNumComps() }
    , TupleId{ o.GetTupleId() }
    , ComponentId{ o.GetComponentId() }
  {
  }

  VTK_ITER_INLINE
  ConstComponentIterator(const ConstComponentIterator& o) noexcept = default;
  VTK_ITER_INLINE
  ConstComponentIterator& operator=(const ConstComponentIterator& o) noexcept = default;

  VTK_ITER_INLINE
  ConstComponentIterator& operator++() noexcept // prefix
  {
    ++this->ComponentId;
    VTK_ITER_ASSERT(this->ComponentId >= 0 && this->ComponentId <= this->NumComps.value,
      "Const component iterator at invalid component id.");
    return *this;
  }

  VTK_ITER_INLINE
  ConstComponentIterator operator++(int) noexcept // postfix
  {
    return ConstComponentIterator{ this->Array, this->NumComps, this->TupleId,
      this->ComponentId++ };
  }

  VTK_ITER_INLINE
  ConstComponentIterator& operator--() noexcept // prefix
  {
    --this->ComponentId;
    VTK_ITER_ASSERT(this->ComponentId >= 0 && this->ComponentId <= this->NumComps.value,
      "Const component iterator at invalid component id.");
    return *this;
  }

  VTK_ITER_INLINE
  ConstComponentIterator operator--(int) noexcept // postfix
  {
    return ConstComponentIterator{ this->Array, this->NumComps, this->TupleId,
      this->ComponentId-- };
  }

  VTK_ITER_INLINE
  reference operator[](difference_type i) const noexcept
  {
    return reference{ this->Array, this->NumComps, this->TupleId, this->ComponentId + i };
  }

  VTK_ITER_INLINE
  reference operator*() const noexcept
  {
    return reference{ this->Array, this->NumComps, this->TupleId, this->ComponentId };
  }

#define VTK_TMP_MAKE_OPERATOR(OP)                                                                  \
  friend VTK_ITER_INLINE bool operator OP(                                                         \
    const ConstComponentIterator& lhs, const ConstComponentIterator& rhs) noexcept                 \
  {                                                                                                \
    VTK_ITER_ASSERT(lhs.Array == rhs.Array, "Mismatched arrays in iterator comparison.");          \
    VTK_ITER_ASSERT(lhs.TupleId == rhs.TupleId, "Mismatched tuple ids in iterator comparison.");   \
    VTK_ITER_ASSUME(lhs.NumComps.value > 0);                                                       \
    VTK_ITER_ASSUME(lhs.NumComps.value == rhs.NumComps.value);                                     \
    return lhs.ComponentId OP rhs.ComponentId;                                                     \
  }

  VTK_TMP_MAKE_OPERATOR(==)
  VTK_TMP_MAKE_OPERATOR(!=)
  VTK_TMP_MAKE_OPERATOR(<)
  VTK_TMP_MAKE_OPERATOR(>)
  VTK_TMP_MAKE_OPERATOR(<=)
  VTK_TMP_MAKE_OPERATOR(>=)

#undef VTK_TMP_MAKE_OPERATOR

  VTK_ITER_INLINE
  ConstComponentIterator& operator+=(difference_type offset) noexcept
  {
    this->ComponentId += offset;
    VTK_ITER_ASSERT(this->ComponentId >= 0 && this->ComponentId <= this->NumComps.value,
      "Const component iterator at invalid component id.");
    return *this;
  }

  friend VTK_ITER_INLINE ConstComponentIterator operator+(
    const ConstComponentIterator& it, difference_type offset) noexcept
  {
    return ConstComponentIterator{ it.Array, it.NumComps, it.TupleId, it.ComponentId + offset };
  }

  friend VTK_ITER_INLINE ConstComponentIterator operator+(
    difference_type offset, const ConstComponentIterator& it) noexcept
  {
    return ConstComponentIterator{ it.Array, it.NumComps, it.TupleId, it.ComponentId + offset };
  }

  VTK_ITER_INLINE
  ConstComponentIterator& operator-=(difference_type offset) noexcept
  {
    this->ComponentId -= offset;
    VTK_ITER_ASSERT(this->ComponentId >= 0 && this->ComponentId <= this->NumComps.value,
      "Const component iterator at invalid component id.");
    return *this;
  }

  friend VTK_ITER_INLINE ConstComponentIterator operator-(
    const ConstComponentIterator& it, difference_type offset) noexcept
  {
    return ConstComponentIterator{ it.Array, it.NumComps, it.TupleId, it.ComponentId - offset };
  }

  friend VTK_ITER_INLINE difference_type operator-(
    const ConstComponentIterator& it1, const ConstComponentIterator& it2) noexcept
  {
    VTK_ITER_ASSERT(it1.Array == it2.Array, "Cannot do math with iterators from different arrays.");
    VTK_ITER_ASSERT(it1.TupleId == it2.TupleId,
      "Cannot do math with component iterators from different "
      "tuples.");
    return it1.ComponentId - it2.ComponentId;
  }

  friend VTK_ITER_INLINE void swap(
    ConstComponentIterator& lhs, ConstComponentIterator& rhs) noexcept
  {
    // Different arrays may use different iterator implementations.
    VTK_ITER_ASSERT(lhs.Array == rhs.Array, "Cannot swap iterators from different arrays.");

    using std::swap;
    swap(lhs.TupleId, rhs.TupleId);
    swap(lhs.ComponentId, rhs.ComponentId);
  }

private:
  mutable ArrayType* Array;
  NumCompsType NumComps;
  TupleIdType TupleId;
  ComponentIdType ComponentId;
};

//------------------------------------------------------------------------------
// Component iterator
template <typename ArrayType, ComponentIdType TupleSize>
struct ComponentIterator
{
private:
  static_assert(IsValidTupleSize<TupleSize>::value, "Invalid tuple size.");
  static_assert(IsVtkDataArray<ArrayType>::value, "Invalid array type.");

  using NumCompsType = GenericTupleSize<TupleSize>;
  using APIType = GetAPIType<ArrayType>;

public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = APIType;
  using difference_type = ComponentIdType;
  using pointer = ComponentReference<ArrayType, TupleSize>;
  using reference = ComponentReference<ArrayType, TupleSize>;

  VTK_ITER_INLINE
  ComponentIterator() noexcept = default;

  VTK_ITER_INLINE
  ComponentIterator(
    ArrayType* array, NumCompsType numComps, TupleIdType tupleId, ComponentIdType comp) noexcept
    : Ref(array, numComps, tupleId, comp)
  {
    VTK_ITER_ASSERT(array != nullptr, "Invalid array.");
    VTK_ITER_ASSERT(numComps.value > 0, "Invalid number of components.");
    VTK_ITER_ASSERT(tupleId >= 0 && tupleId <= array->GetNumberOfTuples(),
      "Component iterator at invalid tuple id.");
    VTK_ITER_ASSERT(
      comp >= 0 && comp <= numComps.value, "Component iterator at invalid component id.");
  }

  VTK_ITER_INLINE
  ComponentIterator(const ComponentIterator& o) noexcept = default;

  VTK_ITER_INLINE
  ComponentIterator& operator=(const ComponentIterator& o) noexcept
  {
    this->Ref.CopyReference(o.Ref);
    return *this;
  }

  VTK_ITER_INLINE
  ComponentIterator& operator++() noexcept // prefix
  {
    ++this->Ref.ComponentId;
    VTK_ITER_ASSERT(this->Ref.ComponentId >= 0 && this->Ref.ComponentId <= this->Ref.NumComps.value,
      "Component iterator at invalid component id.");
    return *this;
  }

  VTK_ITER_INLINE
  ComponentIterator operator++(int) noexcept // postfix
  {
    return ComponentIterator{ this->Ref.Array, this->Ref.NumComps, this->Ref.TupleId,
      this->Ref.ComponentId++ };
  }

  VTK_ITER_INLINE
  ComponentIterator& operator--() noexcept // prefix
  {
    --this->Ref.ComponentId;
    VTK_ITER_ASSERT(this->Ref.ComponentId >= 0 && this->Ref.ComponentId <= this->Ref.NumComps.value,
      "Component iterator at invalid component id.");
    return *this;
  }

  VTK_ITER_INLINE
  ComponentIterator operator--(int) noexcept // postfix
  {
    return ComponentIterator{ this->Ref.Array, this->Ref.NumComps, this->Ref.TupleId,
      this->Ref.ComponentId-- };
  }

  VTK_ITER_INLINE
  reference operator[](difference_type i) const noexcept
  {
    return reference{ this->Ref.Array, this->Ref.NumComps, this->Ref.TupleId,
      this->Ref.ComponentId + i };
  }

  VTK_ITER_INLINE
  reference operator*() const noexcept { return this->Ref; }

  VTK_ITER_INLINE
  const pointer& operator->() const noexcept { return this->Ref; }

#define VTK_TMP_MAKE_OPERATOR(OP)                                                                  \
  friend VTK_ITER_INLINE bool operator OP(                                                         \
    const ComponentIterator& lhs, const ComponentIterator& rhs) noexcept                           \
  {                                                                                                \
    VTK_ITER_ASSERT(                                                                               \
      lhs.GetArray() == rhs.GetArray(), "Mismatched arrays in iterator comparison.");              \
    VTK_ITER_ASSERT(                                                                               \
      lhs.GetTupleId() == rhs.GetTupleId(), "Mismatched tuple ids in iterator comparison.");       \
    VTK_ITER_ASSUME(lhs.GetNumComps().value > 0);                                                  \
    VTK_ITER_ASSUME(lhs.GetNumComps().value == rhs.GetNumComps().value);                           \
    return lhs.GetComponentId() OP rhs.GetComponentId();                                           \
  }

  VTK_TMP_MAKE_OPERATOR(==)
  VTK_TMP_MAKE_OPERATOR(!=)
  VTK_TMP_MAKE_OPERATOR(<)
  VTK_TMP_MAKE_OPERATOR(>)
  VTK_TMP_MAKE_OPERATOR(<=)
  VTK_TMP_MAKE_OPERATOR(>=)

#undef VTK_TMP_MAKE_OPERATOR

  VTK_ITER_INLINE
  ComponentIterator& operator+=(difference_type offset) noexcept
  {
    this->Ref.ComponentId += offset;
    VTK_ITER_ASSERT(this->Ref.ComponentId >= 0 && this->Ref.ComponentId <= this->Ref.NumComps.value,
      "Component iterator at invalid component id.");
    return *this;
  }

  friend VTK_ITER_INLINE ComponentIterator operator+(
    const ComponentIterator& it, difference_type offset) noexcept
  {
    return ComponentIterator{ it.GetArray(), it.GetNumComps(), it.GetTupleId(),
      it.GetComponentId() + offset };
  }

  friend VTK_ITER_INLINE ComponentIterator operator+(
    difference_type offset, const ComponentIterator& it) noexcept
  {
    return ComponentIterator{ it.GetArray(), it.GetNumComps(), it.GetTupleId(),
      it.GetComponentId() + offset };
  }

  VTK_ITER_INLINE
  ComponentIterator& operator-=(difference_type offset) noexcept
  {
    this->Ref.ComponentId -= offset;
    VTK_ITER_ASSERT(this->Ref.ComponentId >= 0 && this->Ref.ComponentId <= this->Ref.NumComps.value,
      "Component iterator at invalid component id.");
    return *this;
  }

  friend VTK_ITER_INLINE ComponentIterator operator-(
    const ComponentIterator& it, difference_type offset) noexcept
  {
    return ComponentIterator{ it.GetArray(), it.GetNumComps(), it.GetTupleId(),
      it.GetComponentId() - offset };
  }

  friend VTK_ITER_INLINE difference_type operator-(
    const ComponentIterator& it1, const ComponentIterator& it2) noexcept
  {
    VTK_ITER_ASSERT(it1.GetArray() == it2.GetArray(),
      "Cannot do math with component iterators from different "
      "arrays.");
    VTK_ITER_ASSERT(it1.GetTupleId() == it2.GetTupleId(),
      "Cannot do math with component iterators from different "
      "tuples.");
    return it1.GetComponentId() - it2.GetComponentId();
  }

  friend VTK_ITER_INLINE void swap(ComponentIterator& lhs, ComponentIterator& rhs) noexcept
  {
    // Different arrays may use different iterator implementations.
    VTK_ITER_ASSERT(
      lhs.GetArray() == rhs.GetArray(), "Cannot swap iterators from different arrays.");

    using std::swap;
    swap(lhs.GetTupleId(), rhs.GetTupleId());
    swap(lhs.GetComponentId(), rhs.GetComponentId());
  }

  friend struct ConstComponentIterator<ArrayType, TupleSize>;

protected:
  // Needed for access from friend functions. We could just store the array
  // and ID here instead of the ref, but meh.
  ArrayType* GetArray() const noexcept { return this->Ref.Array; }
  TupleIdType& GetTupleId() noexcept { return this->Ref.TupleId; }
  const TupleIdType& GetTupleId() const noexcept { return this->Ref.TupleId; }
  ComponentIdType& GetComponentId() noexcept { return this->Ref.ComponentId; }
  const ComponentIdType& GetComponentId() const noexcept { return this->Ref.ComponentId; }
  NumCompsType& GetNumComps() noexcept { return this->Ref.NumComps; }
  const NumCompsType& GetNumComps() const noexcept { return this->Ref.NumComps; }

  ComponentReference<ArrayType, TupleSize> Ref;
};

//------------------------------------------------------------------------------
// Const tuple reference
template <typename ArrayType, ComponentIdType TupleSize>
struct ConstTupleReference
{
private:
  static_assert(IsValidTupleSize<TupleSize>::value, "Invalid tuple size.");
  static_assert(IsVtkDataArray<ArrayType>::value, "Invalid array type.");

  using NumCompsType = GenericTupleSize<TupleSize>;
  using APIType = GetAPIType<ArrayType>;

public:
  using size_type = ComponentIdType;
  using value_type = APIType;
  using iterator = ConstComponentIterator<ArrayType, TupleSize>;
  using const_iterator = ConstComponentIterator<ArrayType, TupleSize>;
  using const_reference = ConstComponentReference<ArrayType, TupleSize>;

  VTK_ITER_INLINE
  ConstTupleReference() noexcept
    : Array(nullptr)
    , TupleId(0)
  {
  }

  VTK_ITER_INLINE
  ConstTupleReference(ArrayType* array, NumCompsType numComps, TupleIdType tupleId) noexcept
    : Array(array)
    , NumComps(numComps)
    , TupleId(tupleId)
  {
    VTK_ITER_ASSERT(array != nullptr, "Invalid array.");
    VTK_ITER_ASSERT(numComps.value > 0, "Invalid number of components.");
    VTK_ITER_ASSERT(tupleId >= 0 && tupleId <= array->GetNumberOfTuples(),
      "Const tuple reference at invalid tuple id.");
  }

  VTK_ITER_INLINE
  ConstTupleReference(const TupleReference<ArrayType, TupleSize>& o) noexcept
    : Array{ o.Array }
    , NumComps{ o.NumComps }
    , TupleId{ o.TupleId }
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
  template <typename AT = ArrayType>
  typename std::enable_if<std::is_same<AT, vtkDataArray>::value>::type VTK_ITER_INLINE GetTuple(
    APIType* tuple) const noexcept
  {
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    VTK_ITER_ASSUME(this->Array->GetNumberOfComponents() == this->NumComps.value);
    this->Array->GetTuple(this->TupleId, tuple);
  }

  template <typename AT = ArrayType>
  typename std::enable_if<!std::is_same<AT, vtkDataArray>::value>::type VTK_ITER_INLINE GetTuple(
    APIType* tuple) const noexcept
  {
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    VTK_ITER_ASSUME(this->Array->GetNumberOfComponents() == this->NumComps.value);
    this->Array->GetTypedTuple(this->TupleId, tuple);
  }

  template <typename VT = APIType, typename AT = ArrayType>
  typename std::enable_if<!std::is_same<VT, double>::value &&
    std::is_same<AT, vtkDataArray>::value>::type VTK_ITER_INLINE
  GetTuple(double* tuple) const noexcept
  {
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    VTK_ITER_ASSUME(this->Array->GetNumberOfComponents() == this->NumComps.value);
    for (ComponentIdType comp = 0; comp < this->NumComps.value; ++comp)
    {
      tuple[comp] = static_cast<double>(this->Array->GetComponent(this->TupleId, comp));
    }
  }

  template <typename VT = APIType, typename AT = ArrayType>
  typename std::enable_if<!std::is_same<VT, double>::value &&
    !std::is_same<AT, vtkDataArray>::value>::type VTK_ITER_INLINE
  GetTuple(double* tuple) const noexcept
  {
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    VTK_ITER_ASSUME(this->Array->GetNumberOfComponents() == this->NumComps.value);
    for (ComponentIdType comp = 0; comp < this->NumComps.value; ++comp)
    {
      tuple[comp] = static_cast<double>(this->Array->GetTypedComponent(this->TupleId, comp));
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
      (std::is_convertible<OAPIType, APIType>{}), "Incompatible types when comparing tuples.");

    // SFINAE guarantees that the tuple sizes are not dynamic in this overload:
    static_assert(TupleSize == OSize, "Cannot compare tuples with different sizes.");

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
      (std::is_convertible<OAPIType, APIType>{}), "Incompatible types when comparing tuples.");

    VTK_ITER_ASSERT(
      other.size() == this->NumComps.value, "Cannot compare tuples with different sizes.");

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
      (std::is_convertible<OAPIType, APIType>{}), "Incompatible types when comparing tuples.");

    // SFINAE guarantees that the tuple sizes are not dynamic in this overload:
    static_assert(TupleSize == OSize, "Cannot compare tuples with different sizes.");

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
      (std::is_convertible<OAPIType, APIType>{}), "Incompatible types when comparing tuples.");

    VTK_ITER_ASSERT(
      other.size() == this->NumComps.value, "Cannot compare tuples with different sizes.");

    return std::equal(this->cbegin(), this->cend(), other.cbegin());
  }

  template <typename OArrayType, ComponentIdType OSize>
  VTK_ITER_INLINE bool operator!=(const TupleReference<OArrayType, OSize>& o) const noexcept
  {
    return !(*this == o);
  }

  template <typename OArrayT, ComponentIdType OSize>
  VTK_ITER_INLINE bool operator!=(const ConstTupleReference<OArrayT, OSize>& o) const noexcept
  {
    return !(*this == o);
  }

  VTK_ITER_INLINE
  const_reference operator[](size_type i) const noexcept
  {
    return const_reference{ this->Array, this->NumComps, this->TupleId, i };
  }

  VTK_ITER_INLINE
  size_type size() const noexcept { return this->NumComps.value; }

  VTK_ITER_INLINE
  const_iterator begin() const noexcept { return this->NewConstIterator(0); }
  VTK_ITER_INLINE
  const_iterator end() const noexcept { return this->NewConstIterator(this->NumComps.value); }

  VTK_ITER_INLINE
  const_iterator cbegin() const noexcept { return this->NewConstIterator(0); }
  VTK_ITER_INLINE
  const_iterator cend() const noexcept { return this->NewConstIterator(this->NumComps.value); }

  friend struct ConstTupleIterator<ArrayType, TupleSize>;

protected:
  // Intentionally hidden:
  VTK_ITER_INLINE
  ConstTupleReference& operator=(const ConstTupleReference&) noexcept = default;

  VTK_ITER_INLINE
  const_iterator NewConstIterator(ComponentIdType comp) const noexcept
  {
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    return const_iterator{ this->Array, this->NumComps, this->TupleId, comp };
  }

  VTK_ITER_INLINE
  void CopyReference(const ConstTupleReference& o) noexcept
  {
    // Must use same array, other array types may use different implementations.
    VTK_ITER_ASSERT(this->Array == o.Array, "Cannot copy reference objects between arrays.");
    this->NumComps = o.NumComps;
    this->TupleId = o.TupleId;
  }

  mutable ArrayType* Array;
  NumCompsType NumComps;
  TupleIdType TupleId;
};

//------------------------------------------------------------------------------
// Tuple reference
template <typename ArrayType, ComponentIdType TupleSize>
struct TupleReference
{
private:
  static_assert(IsValidTupleSize<TupleSize>::value, "Invalid tuple size.");
  static_assert(IsVtkDataArray<ArrayType>::value, "Invalid array type.");

  using NumCompsType = GenericTupleSize<TupleSize>;
  using APIType = GetAPIType<ArrayType>;

public:
  using size_type = ComponentIdType;
  using value_type = APIType;
  using iterator = ComponentIterator<ArrayType, TupleSize>;
  using const_iterator = ConstComponentIterator<ArrayType, TupleSize>;
  using reference = ComponentReference<ArrayType, TupleSize>;
  using const_reference = ConstComponentReference<ArrayType, TupleSize>;

  VTK_ITER_INLINE
  TupleReference() noexcept
    : Array(nullptr)
    , TupleId(0)
  {
  }

  VTK_ITER_INLINE
  TupleReference(ArrayType* array, NumCompsType numComps, TupleIdType tupleId) noexcept
    : Array(array)
    , NumComps(numComps)
    , TupleId(tupleId)
  {
    VTK_ITER_ASSERT(array != nullptr, "Invalid array.");
    VTK_ITER_ASSERT(numComps.value > 0, "Invalid number of components.");
    VTK_ITER_ASSERT(tupleId >= 0 && tupleId <= array->GetNumberOfTuples(),
      "Tuple reference at invalid tuple id.");
  }

  VTK_ITER_INLINE
  TupleReference(const TupleReference&) = default;
  VTK_ITER_INLINE
  TupleReference(TupleReference&&) noexcept = default;

  // Allow this type to masquerade as a pointer, so that tupleIiter->foo works.
  VTK_ITER_INLINE
  TupleReference* operator->() noexcept { return this; }
  VTK_ITER_INLINE
  const TupleReference* operator->() const noexcept { return this; }

  // Caller must ensure that there are size() elements in array.
  template <typename AT = ArrayType>
  typename std::enable_if<std::is_same<AT, vtkDataArray>::value>::type VTK_ITER_INLINE GetTuple(
    APIType* tuple) const noexcept
  {
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    VTK_ITER_ASSUME(this->Array->GetNumberOfComponents() == this->NumComps.value);
    this->Array->GetTuple(this->TupleId, tuple);
  }

  template <typename AT = ArrayType>
  typename std::enable_if<!std::is_same<AT, vtkDataArray>::value>::type VTK_ITER_INLINE GetTuple(
    APIType* tuple) const noexcept
  {
    this->Array->GetTypedTuple(this->TupleId, tuple);
  }

  template <typename VT = APIType, typename AT = ArrayType>
  typename std::enable_if<!std::is_same<VT, double>::value &&
    std::is_same<AT, vtkDataArray>::value>::type VTK_ITER_INLINE
  GetTuple(double* tuple) const noexcept
  {
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    VTK_ITER_ASSUME(this->Array->GetNumberOfComponents() == this->NumComps.value);
    for (ComponentIdType comp = 0; comp < this->NumComps.value; ++comp)
    {
      tuple[comp] = static_cast<double>(this->Array->GetComponent(this->TupleId, comp));
    }
  }

  template <typename VT = APIType, typename AT = ArrayType>
  typename std::enable_if<!std::is_same<VT, double>::value &&
    !std::is_same<AT, vtkDataArray>::value>::type VTK_ITER_INLINE
  GetTuple(double* tuple) const noexcept
  {
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    VTK_ITER_ASSUME(this->Array->GetNumberOfComponents() == this->NumComps.value);
    for (ComponentIdType comp = 0; comp < this->NumComps.value; ++comp)
    {
      tuple[comp] = static_cast<double>(this->Array->GetTypedComponent(this->TupleId, comp));
    }
  }

  // Caller must ensure that there are size() elements in array.
  template <typename AT = ArrayType>
  typename std::enable_if<std::is_same<AT, vtkDataArray>::value>::type VTK_ITER_INLINE SetTuple(
    const APIType* tuple) noexcept
  {
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    VTK_ITER_ASSUME(this->Array->GetNumberOfComponents() == this->NumComps.value);
    this->Array->SetTuple(this->TupleId, tuple);
  }

  template <typename AT = ArrayType>
  typename std::enable_if<!std::is_same<AT, vtkDataArray>::value>::type VTK_ITER_INLINE SetTuple(
    const APIType* tuple) noexcept
  {
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    VTK_ITER_ASSUME(this->Array->GetNumberOfComponents() == this->NumComps.value);
    this->Array->SetTypedTuple(this->TupleId, tuple);
  }

  template <typename VT = APIType, typename AT = ArrayType>
  typename std::enable_if<!std::is_same<VT, double>::value &&
    std::is_same<AT, vtkDataArray>::value>::type VTK_ITER_INLINE
  SetTuple(const double* tuple) noexcept
  {
    for (ComponentIdType comp = 0; comp < this->NumComps.value; ++comp)
    {
      VTK_ITER_ASSUME(this->NumComps.value > 0);
      VTK_ITER_ASSUME(this->Array->GetNumberOfComponents() == this->NumComps.value);
      this->Array->SetComponent(this->TupleId, comp, tuple[comp]);
    }
  }

  template <typename VT = APIType, typename AT = ArrayType>
  typename std::enable_if<!std::is_same<VT, double>::value &&
    !std::is_same<AT, vtkDataArray>::value>::type VTK_ITER_INLINE
  SetTuple(const double* tuple) noexcept
  {
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    VTK_ITER_ASSUME(this->Array->GetNumberOfComponents() == this->NumComps.value);
    for (ComponentIdType comp = 0; comp < this->NumComps.value; ++comp)
    {
      this->Array->SetTypedComponent(this->TupleId, comp, static_cast<APIType>(tuple[comp]));
    }
  }

  VTK_ITER_INLINE
  TupleReference& operator=(const TupleReference& other) noexcept
  {
    std::copy_n(other.cbegin(), this->NumComps.value, this->begin());
    return *this;
  }

  // skips some runtime checks when both sizes are fixed:
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

  // Needs a runtime check:
  template <typename OArrayType, ComponentIdType OSize>
  VTK_ITER_INLINE EnableIfEitherTupleSizeIsDynamic<TupleSize, OSize, TupleReference&> operator=(
    const TupleReference<OArrayType, OSize>& other) noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert(
      (std::is_convertible<OAPIType, APIType>{}), "Incompatible types when assigning tuples.");

    VTK_ITER_ASSERT(
      other.size() == this->NumComps.value, "Cannot assign tuples with different sizes.");

    std::copy_n(other.cbegin(), this->NumComps.value, this->begin());
    return *this;
  }

  // skips some runtime checks when both sizes are fixed:
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

  // Needs a runtime check:
  template <typename OArrayType, ComponentIdType OSize>
  VTK_ITER_INLINE EnableIfEitherTupleSizeIsDynamic<TupleSize, OSize, TupleReference&> operator=(
    const ConstTupleReference<OArrayType, OSize>& other) noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert(
      (std::is_convertible<OAPIType, APIType>{}), "Incompatible types when assigning tuples.");

    VTK_ITER_ASSERT(
      other.size() == this->NumComps.value, "Cannot assign tuples with different sizes.");

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
      (std::is_convertible<OAPIType, APIType>{}), "Incompatible types when comparing tuples.");

    // SFINAE guarantees that the tuple sizes are not dynamic in this overload:
    static_assert(TupleSize == OSize, "Cannot compare tuples with different sizes.");

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
      (std::is_convertible<OAPIType, APIType>{}), "Incompatible types when comparing tuples.");

    VTK_ITER_ASSERT(
      other.size() == this->NumComps.value, "Cannot compare tuples with different sizes.");

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
      (std::is_convertible<OAPIType, APIType>{}), "Incompatible types when comparing tuples.");

    // SFINAE guarantees that the tuple sizes are not dynamic in this overload:
    static_assert(TupleSize == OSize, "Cannot compare tuples with different sizes.");

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
      (std::is_convertible<OAPIType, APIType>{}), "Incompatible types when comparing tuples.");

    VTK_ITER_ASSERT(
      other.size() == this->NumComps.value, "Cannot compare tuples with different sizes.");

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
    static_assert(
      (std::is_convertible<OAPIType, APIType>{}), "Incompatible types when swapping tuples.");

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
    static_assert(
      (std::is_convertible<OAPIType, APIType>{}), "Incompatible types when swapping tuples.");

    VTK_ITER_ASSERT(
      other.size() == this->NumComps.value, "Cannot swap tuples with different sizes.");

    std::swap_ranges(this->begin(), this->end(), other.begin());
  }

  friend VTK_ITER_INLINE void swap(TupleReference a, TupleReference b) noexcept { a.swap(b); }

  template <typename OArray, ComponentIdType OSize>
  friend VTK_ITER_INLINE void swap(TupleReference a, TupleReference<OArray, OSize> b) noexcept
  {
    a.swap(b);
  }

  VTK_ITER_INLINE
  reference operator[](size_type i) noexcept
  {
    return reference{ this->Array, this->NumComps, this->TupleId, i };
  }

  VTK_ITER_INLINE
  const_reference operator[](size_type i) const noexcept
  {
    // Let the reference type do the lookup during implicit conversion.
    return const_reference{ this->Array, this->NumComps, this->TupleId, i };
  }

  VTK_ITER_INLINE
  void fill(const value_type& v) noexcept { std::fill(this->begin(), this->end(), v); }

  VTK_ITER_INLINE
  size_type size() const noexcept { return this->NumComps.value; }

  VTK_ITER_INLINE
  iterator begin() noexcept { return this->NewIterator(0); }
  VTK_ITER_INLINE
  iterator end() noexcept { return this->NewIterator(this->NumComps.value); }

  VTK_ITER_INLINE
  const_iterator begin() const noexcept { return this->NewConstIterator(0); }
  VTK_ITER_INLINE
  const_iterator end() const noexcept { return this->NewConstIterator(this->NumComps.value); }

  VTK_ITER_INLINE
  const_iterator cbegin() const noexcept { return this->NewConstIterator(0); }
  VTK_ITER_INLINE
  const_iterator cend() const noexcept { return this->NewConstIterator(this->NumComps.value); }

  friend struct ConstTupleReference<ArrayType, TupleSize>;
  friend struct TupleIterator<ArrayType, TupleSize>;

protected:
  VTK_ITER_INLINE
  iterator NewIterator(ComponentIdType comp) const noexcept
  {
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    return iterator{ this->Array, this->NumComps, this->TupleId, comp };
  }

  VTK_ITER_INLINE
  const_iterator NewConstIterator(ComponentIdType comp) const noexcept
  {
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    return const_iterator{ this->Array, this->NumComps, this->TupleId, comp };
  }

  VTK_ITER_INLINE
  void CopyReference(const TupleReference& o) noexcept
  {
    // Must use same array, other array types may use different implementations.
    VTK_ITER_ASSERT(this->Array == o.Array, "Cannot copy reference objects between arrays.");
    this->NumComps = o.NumComps;
    this->TupleId = o.TupleId;
  }

  mutable ArrayType* Array;
  NumCompsType NumComps;
  TupleIdType TupleId;
};

//------------------------------------------------------------------------------
// Const tuple iterator
template <typename ArrayType, ComponentIdType TupleSize>
struct ConstTupleIterator
{
private:
  static_assert(IsValidTupleSize<TupleSize>::value, "Invalid tuple size.");
  static_assert(IsVtkDataArray<ArrayType>::value, "Invalid array type.");

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
  ConstTupleIterator(ArrayType* array, NumCompsType numComps, TupleIdType tupleId) noexcept
    : Ref(array, numComps, tupleId)
  {
    VTK_ITER_ASSERT(array != nullptr, "Invalid array.");
    VTK_ITER_ASSERT(numComps.value > 0, "Invalid number of components.");
    VTK_ITER_ASSERT(tupleId >= 0 && tupleId <= array->GetNumberOfTuples(),
      "Const tuple iterator at invalid tuple id.");
  }

  VTK_ITER_INLINE
  ConstTupleIterator(const TupleIterator<ArrayType, TupleSize>& o) noexcept
    : Ref{ o.Ref }
  {
  }

  VTK_ITER_INLINE
  ConstTupleIterator(const ConstTupleIterator& o) noexcept = default;
  VTK_ITER_INLINE
  ConstTupleIterator& operator=(const ConstTupleIterator& o) noexcept
  {
    this->Ref.CopyReference(o.Ref);
    return *this;
  }

  VTK_ITER_INLINE
  ConstTupleIterator& operator++() noexcept // prefix
  {
    ++this->Ref.TupleId;
    VTK_ITER_ASSERT(
      this->Ref.TupleId >= 0 && this->Ref.TupleId <= this->Ref.Array->GetNumberOfTuples(),
      "Const tuple iterator at invalid component id.");
    return *this;
  }

  VTK_ITER_INLINE
  ConstTupleIterator operator++(int) noexcept // postfix
  {
    return ConstTupleIterator{ this->Ref.Array, this->Ref.NumComps, this->Ref.TupleId++ };
  }

  VTK_ITER_INLINE
  ConstTupleIterator& operator--() noexcept // prefix
  {
    --this->Ref.TupleId;
    VTK_ITER_ASSERT(
      this->Ref.TupleId >= 0 && this->Ref.TupleId <= this->Ref.Array->GetNumberOfTuples(),
      "Const tuple iterator at invalid component id.");
    return *this;
  }

  VTK_ITER_INLINE
  ConstTupleIterator operator--(int) noexcept // postfix
  {
    return ConstTupleIterator{ this->Ref.Array, this->Ref.NumComps, this->Ref.TupleId-- };
  }

  VTK_ITER_INLINE
  reference operator[](difference_type i) noexcept
  {
    return reference{ this->GetArray(), this->GetNumComps(), this->GetTupleId() + i };
  }

  VTK_ITER_INLINE
  reference operator*() noexcept { return this->Ref; }

  VTK_ITER_INLINE
  pointer operator->() noexcept { return this->Ref; }

#define VTK_TMP_MAKE_OPERATOR(OP)                                                                  \
  friend VTK_ITER_INLINE bool operator OP(                                                         \
    const ConstTupleIterator& lhs, const ConstTupleIterator& rhs) noexcept                         \
  {                                                                                                \
    VTK_ITER_ASSERT(                                                                               \
      lhs.GetArray() == rhs.GetArray(), "Cannot compare iterators from different arrays.");        \
    VTK_ITER_ASSUME(lhs.GetNumComps().value > 0);                                                  \
    VTK_ITER_ASSUME(lhs.GetNumComps().value == rhs.GetNumComps().value);                           \
    return lhs.GetTupleId() OP rhs.GetTupleId();                                                   \
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
    this->Ref.TupleId += offset;
    VTK_ITER_ASSERT(
      this->Ref.TupleId >= 0 && this->Ref.TupleId <= this->Ref.Array->GetNumberOfTuples(),
      "Const tuple iterator at invalid component id.");
    return *this;
  }

  friend VTK_ITER_INLINE ConstTupleIterator operator+(
    const ConstTupleIterator& it, difference_type offset) noexcept
  {
    return ConstTupleIterator{ it.GetArray(), it.GetNumComps(), it.GetTupleId() + offset };
  }

  friend VTK_ITER_INLINE ConstTupleIterator operator+(
    difference_type offset, const ConstTupleIterator& it) noexcept
  {
    return ConstTupleIterator{ it.GetArray(), it.GetNumComps(), it.GetTupleId() + offset };
  }

  VTK_ITER_INLINE
  ConstTupleIterator& operator-=(difference_type offset) noexcept
  {
    this->Ref.TupleId -= offset;
    VTK_ITER_ASSERT(
      this->Ref.TupleId >= 0 && this->Ref.TupleId <= this->Ref.Array->GetNumberOfTuples(),
      "Const tuple iterator at invalid component id.");
    return *this;
  }

  friend VTK_ITER_INLINE ConstTupleIterator operator-(
    const ConstTupleIterator& it, difference_type offset) noexcept
  {
    return ConstTupleIterator{ it.GetArray(), it.GetNumComps(), it.GetTupleId() - offset };
  }

  friend VTK_ITER_INLINE difference_type operator-(
    const ConstTupleIterator& it1, const ConstTupleIterator& it2) noexcept
  {
    VTK_ITER_ASSERT(it1.GetArray() == it2.GetArray(),
      "Cannot do math with tuple iterators from different "
      "arrays.");
    return it1.GetTupleId() - it2.GetTupleId();
  }

  friend VTK_ITER_INLINE void swap(ConstTupleIterator& lhs, ConstTupleIterator& rhs) noexcept
  {
    // Different arrays may use different iterator implementations.
    VTK_ITER_ASSERT(
      lhs.GetArray() == rhs.GetArray(), "Cannot swap iterators from different arrays.");

    using std::swap;
    swap(lhs.GetTupleId(), rhs.GetTupleId());
  }

private:
  VTK_ITER_INLINE
  ArrayType* GetArray() const noexcept { return this->Ref.Array; }
  VTK_ITER_INLINE
  ArrayType*& GetArray() noexcept { return this->Ref.Array; }
  VTK_ITER_INLINE
  NumCompsType GetNumComps() const noexcept { return this->Ref.NumComps; }
  VTK_ITER_INLINE
  NumCompsType& GetNumComps() noexcept { return this->Ref.NumComps; }
  VTK_ITER_INLINE
  TupleIdType GetTupleId() const noexcept { return this->Ref.TupleId; }
  VTK_ITER_INLINE
  TupleIdType& GetTupleId() noexcept { return this->Ref.TupleId; }

  ConstTupleReference<ArrayType, TupleSize> Ref;
};

//------------------------------------------------------------------------------
// Tuple iterator
template <typename ArrayType, ComponentIdType TupleSize>
struct TupleIterator
{
private:
  static_assert(IsValidTupleSize<TupleSize>::value, "Invalid tuple size.");
  static_assert(IsVtkDataArray<ArrayType>::value, "Invalid array type.");

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
  TupleIterator(ArrayType* array, NumCompsType numComps, TupleIdType tupleId) noexcept
    : Ref(array, numComps, tupleId)
  {
    VTK_ITER_ASSERT(array != nullptr, "Invalid array.");
    VTK_ITER_ASSERT(numComps.value > 0, "Invalid number of components.");
    VTK_ITER_ASSERT(
      tupleId >= 0 && tupleId <= array->GetNumberOfTuples(), "Tuple iterator at invalid tuple id.");
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
    ++this->Ref.TupleId;
    VTK_ITER_ASSERT(
      this->Ref.TupleId >= 0 && this->Ref.TupleId <= this->Ref.Array->GetNumberOfTuples(),
      "Tuple iterator at invalid component id.");
    return *this;
  }

  VTK_ITER_INLINE
  TupleIterator operator++(int) noexcept // postfix
  {
    return TupleIterator{ this->Ref.Array, this->Ref.NumComps, this->Ref.TupleId++ };
  }

  VTK_ITER_INLINE
  TupleIterator& operator--() noexcept // prefix
  {
    --this->Ref.TupleId;
    VTK_ITER_ASSERT(
      this->Ref.TupleId >= 0 && this->Ref.TupleId <= this->Ref.Array->GetNumberOfTuples(),
      "Tuple iterator at invalid component id.");
    return *this;
  }

  VTK_ITER_INLINE
  TupleIterator operator--(int) noexcept // postfix
  {
    return TupleIterator{ this->Ref.Array, this->Ref.NumComps, this->Ref.TupleId-- };
  }

  VTK_ITER_INLINE
  reference operator[](difference_type i) noexcept
  {
    return reference{ this->Ref.Array, this->Ref.NumComps, this->Ref.TupleId + i };
  }

  VTK_ITER_INLINE
  reference operator*() noexcept { return this->Ref; }

  VTK_ITER_INLINE
  pointer& operator->() noexcept { return this->Ref; }

#define VTK_TMP_MAKE_OPERATOR(OP)                                                                  \
  friend VTK_ITER_INLINE bool operator OP(                                                         \
    const TupleIterator& lhs, const TupleIterator& rhs) noexcept                                   \
  {                                                                                                \
    VTK_ITER_ASSERT(                                                                               \
      lhs.GetArray() == rhs.GetArray(), "Cannot compare iterators from different arrays.");        \
    VTK_ITER_ASSUME(lhs.GetNumComps().value > 0);                                                  \
    VTK_ITER_ASSUME(lhs.GetNumComps().value == rhs.GetNumComps().value);                           \
    return lhs.GetTupleId() OP rhs.GetTupleId();                                                   \
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
    this->Ref.TupleId += offset;
    VTK_ITER_ASSERT(
      this->Ref.TupleId >= 0 && this->Ref.TupleId <= this->Ref.Array->GetNumberOfTuples(),
      "Tuple iterator at invalid component id.");
    return *this;
  }

  friend VTK_ITER_INLINE TupleIterator operator+(
    const TupleIterator& it, difference_type offset) noexcept
  {
    return TupleIterator{ it.GetArray(), it.GetNumComps(), it.GetTupleId() + offset };
  }

  friend VTK_ITER_INLINE TupleIterator operator+(
    difference_type offset, const TupleIterator& it) noexcept
  {
    return TupleIterator{ it.GetArray(), it.GetNumComps(), it.GetTupleId() + offset };
  }

  VTK_ITER_INLINE
  TupleIterator& operator-=(difference_type offset) noexcept
  {
    this->Ref.TupleId -= offset;
    VTK_ITER_ASSERT(
      this->Ref.TupleId >= 0 && this->Ref.TupleId <= this->Ref.Array->GetNumberOfTuples(),
      "Tuple iterator at invalid component id.");
    return *this;
  }

  friend VTK_ITER_INLINE TupleIterator operator-(
    const TupleIterator& it, difference_type offset) noexcept
  {
    return TupleIterator{ it.GetArray(), it.GetNumComps(), it.GetTupleId() - offset };
  }

  friend VTK_ITER_INLINE difference_type operator-(
    const TupleIterator& it1, const TupleIterator& it2) noexcept
  {
    VTK_ITER_ASSERT(it1.GetArray() == it2.GetArray(),
      "Cannot do math with tuple iterators from different "
      "arrays.");
    return it1.GetTupleId() - it2.GetTupleId();
  }

  friend VTK_ITER_INLINE void swap(TupleIterator& lhs, TupleIterator& rhs) noexcept
  {
    // Different arrays may use different iterator implementations.
    VTK_ITER_ASSERT(
      lhs.GetArray() == rhs.GetArray(), "Cannot swap iterators from different arrays.");

    using std::swap;
    swap(lhs.GetTupleId(), rhs.GetTupleId());
  }

  friend struct ConstTupleIterator<ArrayType, TupleSize>;
  friend struct ConstTupleReference<ArrayType, TupleSize>;

protected:
  VTK_ITER_INLINE
  ArrayType* GetArray() const noexcept { return this->Ref.Array; }
  VTK_ITER_INLINE
  ArrayType*& GetArray() noexcept { return this->Ref.Array; }
  VTK_ITER_INLINE
  NumCompsType GetNumComps() const noexcept { return this->Ref.NumComps; }
  VTK_ITER_INLINE
  NumCompsType& GetNumComps() noexcept { return this->Ref.NumComps; }
  VTK_ITER_INLINE
  TupleIdType GetTupleId() const noexcept { return this->Ref.TupleId; }
  VTK_ITER_INLINE
  TupleIdType& GetTupleId() noexcept { return this->Ref.TupleId; }

  TupleReference<ArrayType, TupleSize> Ref;
};

//------------------------------------------------------------------------------
// Tuple range
template <typename ArrayTypeT, ComponentIdType TupleSize>
struct TupleRange
{
  using ArrayType = ArrayTypeT;
  using APIType = GetAPIType<ArrayType>;
  using ValueType = APIType;

private:
  static_assert(IsValidTupleSize<TupleSize>::value, "Invalid tuple size.");
  static_assert(IsVtkDataArray<ArrayTypeT>::value, "Invalid array type.");

  using NumCompsType = GenericTupleSize<TupleSize>;

public:
  using TupleIteratorType = TupleIterator<ArrayType, TupleSize>;
  using ConstTupleIteratorType = ConstTupleIterator<ArrayType, TupleSize>;
  using TupleReferenceType = TupleReference<ArrayType, TupleSize>;
  using ConstTupleReferenceType = ConstTupleReference<ArrayType, TupleSize>;
  using ComponentIteratorType = ComponentIterator<ArrayType, TupleSize>;
  using ConstComponentIteratorType = ConstComponentIterator<ArrayType, TupleSize>;
  using ComponentReferenceType = ComponentReference<ArrayType, TupleSize>;
  using ConstComponentReferenceType = ConstComponentReference<ArrayType, TupleSize>;
  using ComponentType = APIType;

  // May be DynamicTupleSize, or the actual tuple size.
  constexpr static ComponentIdType TupleSizeTag = TupleSize;

  using size_type = TupleIdType;
  using iterator = TupleIteratorType;
  using const_iterator = ConstTupleIteratorType;
  using reference = TupleReferenceType;
  using const_reference = ConstTupleReferenceType;

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
    const TupleIdType realBegin = this->BeginTuple + beginTuple;
    const TupleIdType realEnd = endTuple >= 0 ? this->BeginTuple + endTuple : this->EndTuple;

    return TupleRange{ this->Array, realBegin, realEnd };
  }

  VTK_ITER_INLINE
  ArrayType* GetArray() const noexcept { return this->Array; }
  VTK_ITER_INLINE
  ComponentIdType GetTupleSize() const noexcept { return this->NumComps.value; }
  VTK_ITER_INLINE
  TupleIdType GetBeginTupleId() const noexcept { return this->BeginTuple; }
  VTK_ITER_INLINE
  TupleIdType GetEndTupleId() const noexcept { return this->EndTuple; }

  VTK_ITER_INLINE
  size_type size() const noexcept { return this->EndTuple - this->BeginTuple; }

  VTK_ITER_INLINE
  iterator begin() noexcept { return this->NewIter(this->BeginTuple); }
  VTK_ITER_INLINE
  iterator end() noexcept { return this->NewIter(this->EndTuple); }

  VTK_ITER_INLINE
  const_iterator begin() const noexcept { return this->NewCIter(this->BeginTuple); }
  VTK_ITER_INLINE
  const_iterator end() const noexcept { return this->NewCIter(this->EndTuple); }

  VTK_ITER_INLINE
  const_iterator cbegin() const noexcept { return this->NewCIter(this->BeginTuple); }
  VTK_ITER_INLINE
  const_iterator cend() const noexcept { return this->NewCIter(this->EndTuple); }

  VTK_ITER_INLINE
  reference operator[](size_type i) noexcept
  {
    return reference{ this->Array, this->NumComps, this->BeginTuple + i };
  }

  VTK_ITER_INLINE
  const_reference operator[](size_type i) const noexcept
  {
    return const_reference{ this->Array, this->NumComps, this->BeginTuple + i };
  }

  template <typename AT = ArrayType>
  typename std::enable_if<std::is_same<AT, vtkDataArray>::value>::type VTK_ITER_INLINE GetTuple(
    size_type i, ValueType* tuple) const noexcept
  {
    this->Array->GetTuple(this->BeginTuple + i, tuple);
  }

  template <typename AT = ArrayType>
  typename std::enable_if<!std::is_same<AT, vtkDataArray>::value>::type VTK_ITER_INLINE GetTuple(
    size_type i, ValueType* tuple) const noexcept
  {
    this->Array->GetTypedTuple(this->BeginTuple + i, tuple);
  }

  template <typename VT = ValueType, typename AT = ArrayType>
  typename std::enable_if<!std::is_same<VT, double>::value &&
    std::is_same<AT, vtkDataArray>::value>::type VTK_ITER_INLINE
  GetTuple(size_type i, double* tuple) const noexcept
  {
    for (ComponentIdType comp = 0; comp < this->NumComps.value; ++comp)
    {
      tuple[comp] = static_cast<double>(this->Array->GetComponent(i, comp));
    }
  }

  template <typename VT = ValueType, typename AT = ArrayType>
  typename std::enable_if<!std::is_same<VT, double>::value &&
    !std::is_same<AT, vtkDataArray>::value>::type VTK_ITER_INLINE
  GetTuple(size_type i, double* tuple) const noexcept
  {
    for (ComponentIdType comp = 0; comp < this->NumComps.value; ++comp)
    {
      tuple[comp] = static_cast<double>(this->Array->GetTypedComponent(i, comp));
    }
  }

  template <typename AT = ArrayType>
  typename std::enable_if<std::is_same<AT, vtkDataArray>::value>::type VTK_ITER_INLINE SetTuple(
    size_type i, const ValueType* tuple) noexcept
  {
    this->Array->SetTuple(this->BeginTuple + i, tuple);
  }

  template <typename AT = ArrayType>
  typename std::enable_if<!std::is_same<AT, vtkDataArray>::value>::type VTK_ITER_INLINE SetTuple(
    size_type i, const ValueType* tuple) noexcept
  {
    this->Array->SetTypedTuple(this->BeginTuple + i, tuple);
  }

  template <typename VT = ValueType, typename AT = ArrayType>
  typename std::enable_if<!std::is_same<VT, double>::value &&
    std::is_same<AT, vtkDataArray>::value>::type VTK_ITER_INLINE
  SetTuple(size_type i, const double* tuple) noexcept
  {
    for (ComponentIdType comp = 0; comp < this->NumComps.value; ++comp)
    {
      this->Array->SetComponent(this->BeginTuple + i, comp, static_cast<ValueType>(tuple[comp]));
    }
  }

  template <typename VT = ValueType, typename AT = ArrayType>
  typename std::enable_if<!std::is_same<VT, double>::value &&
    !std::is_same<AT, vtkDataArray>::value>::type VTK_ITER_INLINE
  SetTuple(size_type i, const double* tuple) noexcept
  {
    for (ComponentIdType comp = 0; comp < this->NumComps.value; ++comp)
    {
      this->Array->SetTypedComponent(
        this->BeginTuple + i, comp, static_cast<ValueType>(tuple[comp]));
    }
  }

private:
  VTK_ITER_INLINE
  iterator NewIter(TupleIdType t) const { return iterator{ this->Array, this->NumComps, t }; }

  VTK_ITER_INLINE
  const_iterator NewCIter(TupleIdType t) const
  {
    return const_iterator{ this->Array, this->NumComps, t };
  }

  mutable ArrayType* Array{ nullptr };
  NumCompsType NumComps{};
  TupleIdType BeginTuple{ 0 };
  TupleIdType EndTuple{ 0 };
};

// Unimplemented, only used inside decltype in SelectTupleRange:
template <typename ArrayType, ComponentIdType TupleSize>
TupleRange<ArrayType, TupleSize> DeclareTupleRangeSpecialization(vtkDataArray*);

VTK_ABI_NAMESPACE_END
} // end namespace detail
} // end namespace vtk

VTK_ITER_OPTIMIZE_END

#endif // __VTK_WRAP__

// VTK-HeaderTest-Exclude: vtkDataArrayTupleRange_Generic.h
