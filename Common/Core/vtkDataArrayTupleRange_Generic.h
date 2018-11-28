/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArrayTupleRange_Generic.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * Generic implementation of value ranges and iterators, suitable for
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

#ifndef __VTK_WRAP__

VTK_ITER_OPTIMIZE_START

namespace vtk
{

namespace detail
{

// Forward decs for friends/args
template <typename ArrayType, ComponentIdType> struct ComponentReference;
template <typename ArrayType, ComponentIdType> struct ConstComponentIterator;
template <typename ArrayType, ComponentIdType> struct ComponentIterator;
template <typename ArrayType, ComponentIdType> struct ConstTupleReference;
template <typename ArrayType, ComponentIdType> struct TupleReference;
template <typename ArrayType, ComponentIdType> struct ConstTupleIterator;
template <typename ArrayType, ComponentIdType> struct TupleIterator;
template <typename ArrayType, ComponentIdType> struct TupleRange;

//------------------------------------------------------------------------------
// Component reference
template <typename ArrayType,
          ComponentIdType TupleSize>
struct ComponentReference
{
private:
  static_assert(IsValidTupleSize<TupleSize>::value, "Invalid tuple size.");
  static_assert(IsVtkDataArray<ArrayType>::value, "Invalid array type.");

  using NumCompsType = GenericTupleSize<TupleSize>;
  using APIType = GetAPIType<ArrayType>;

public:

  VTK_ITER_INLINE
  ComponentReference(ArrayType* array,
                     NumCompsType numComps,
                     TupleIdType tuple,
                     ComponentIdType comp) noexcept
    : Data{array, numComps, tuple, comp}
    , IsValue(false)
  {
    VTK_ITER_ASSERT(array != nullptr, "Invalid array.");
    VTK_ITER_ASSERT(numComps.value > 0, "Invalid number of components.");
    VTK_ITER_ASSERT(tuple >= 0 && tuple <= array->GetNumberOfTuples(),
                    "Invalid tuple accessed by iterator.");
    VTK_ITER_ASSERT(comp >= 0 && comp <= array->GetNumberOfComponents(),
                    "Invalid component accessed by iterator.");
  }

  VTK_ITER_INLINE
  ComponentReference(const ComponentReference &o) noexcept
    : Data{static_cast<APIType>(o)}
    , IsValue(true)
  {
  }

  VTK_ITER_INLINE
  ComponentReference& operator=(const ComponentReference& o) noexcept
  {
    return *this = static_cast<APIType>(o);
  }

  template <typename OArray, ComponentIdType OSize>
  VTK_ITER_INLINE
  ComponentReference&
  operator=(const ComponentReference<OArray, OSize>& o) noexcept
  {
    APIType tmp = o;
    return *this = std::move(tmp);
  }

  VTK_ITER_INLINE
  operator APIType() const noexcept
  {
    if (this->IsValue)
    {
      return this->Data.Value;
    }
    else
    {
      VTK_ITER_ASSUME(this->Data.Lookup.NumComps.value > 0);
      VTK_ITER_ASSUME(this->Data.Lookup.Array->GetNumberOfComponents() ==
                      this->Data.Lookup.NumComps.value);
      vtkDataArrayAccessor<ArrayType> acc{this->Data.Lookup.Array};
      return acc.Get(this->Data.Lookup.TupleId, this->Data.Lookup.ComponentId);
    }
  }

  VTK_ITER_INLINE
  ComponentReference& operator= (APIType val) noexcept
  {
    if (this->IsValue)
    {
      this->Data.Value = val;
    }
    else
    {
      VTK_ITER_ASSUME(this->Data.Lookup.NumComps.value > 0);
      VTK_ITER_ASSUME(this->Data.Lookup.Array->GetNumberOfComponents() ==
                      this->Data.Lookup.NumComps.value);
      vtkDataArrayAccessor<ArrayType> acc{this->Data.Lookup.Array};
      acc.Set(this->Data.Lookup.TupleId, this->Data.Lookup.ComponentId, val);
    }
    return *this;
  }

  friend VTK_ITER_INLINE
  void swap(ComponentReference &lhs,
                   ComponentReference &rhs) noexcept
  {
    APIType tmp = std::move(static_cast<APIType>(lhs));
    lhs = std::move(static_cast<APIType>(rhs));
    rhs = std::move(tmp);
  }

  template <typename OArray, ComponentIdType OSize>
  friend VTK_ITER_INLINE
  void swap(ComponentReference &lhs,
                   ComponentReference<OArray, OSize> &rhs) noexcept
  {
    using OAPIType = GetAPIType<OArray>;
    static_assert(std::is_same<APIType, OAPIType>::value,
                  "Cannot swap components with different types.");

    APIType tmp = std::move(static_cast<APIType>(lhs));
    lhs = std::move(static_cast<APIType>(rhs));
    rhs = std::move(tmp);
  }

  friend VTK_ITER_INLINE
  void swap(ComponentReference &lhs, APIType &rhs) noexcept
  {
    APIType tmp = std::move(static_cast<APIType>(lhs));
    lhs = std::move(rhs);
    rhs = std::move(tmp);
  }

  friend VTK_ITER_INLINE
  void swap(APIType &lhs, ComponentReference &rhs) noexcept
  {
    APIType tmp = std::move(lhs);
    lhs = std::move(static_cast<APIType>(rhs));
    rhs = std::move(tmp);
  }

  friend struct ComponentIterator<ArrayType, TupleSize>;

protected:
  struct CopyStateTag {};

  VTK_ITER_INLINE
  ComponentReference(const ComponentReference &o, CopyStateTag) noexcept
    : Data{o.Data.Lookup.Array,
           o.Data.Lookup.NumComps,
           o.Data.Lookup.TupleId,
           o.Data.Lookup.ComponentId}
    , IsValue(false)
  {
    VTK_ITER_ASSERT(!o.IsValue,
                    "Cannot copy reference state from value reference.");
  }

  VTK_ITER_INLINE
  void CopyReference(const ComponentReference& o) noexcept
  {
    VTK_ITER_ASSERT(!this->IsValue && !o.IsValue,
                    "Iterator's internal ref cannot be in value state.");
    // Arrays must match (other array types may use different implementations.
    VTK_ITER_ASSERT(this->Data.Lookup.Array == o.Data.Lookup.Array,
                    "Cannot copy reference objects between arrays.");
    this->Data.Lookup = o.Data.Lookup;
  }

  // We switch to just using a value once copied. This allows us to use value
  // semantics when appropriate.
  union Storage
  {
    VTK_ITER_INLINE
    Storage() noexcept {}
    VTK_ITER_INLINE
    Storage(ArrayType* array,
            NumCompsType numComps,
            TupleIdType tupleId,
            ComponentIdType comp) noexcept
      : Lookup{array, numComps, tupleId, comp} {}
    VTK_ITER_INLINE
    Storage(const APIType& val) noexcept : Value(val) {}
    VTK_ITER_INLINE
    ~Storage() noexcept {}
    APIType Value;
    struct
    {
      mutable ArrayType* Array;
      NumCompsType NumComps;
      TupleIdType TupleId;
      ComponentIdType ComponentId;
    } Lookup;
  } Data;
  const bool IsValue;
};

//------------------------------------------------------------------------------
// Const component iterator
template <typename ArrayType,
          ComponentIdType TupleSize>
struct ConstComponentIterator :
    public std::iterator<std::random_access_iterator_tag,
                         GetAPIType<ArrayType>,
                         ComponentIdType,
                         // expected types don't have members, no op->().
                         void,
                         // ref is just a value type bc this is const.
                         GetAPIType<ArrayType>>
{
private:
  static_assert(IsValidTupleSize<TupleSize>::value, "Invalid tuple size.");
  static_assert(IsVtkDataArray<ArrayType>::value, "Invalid array type.");

  using NumCompsType = GenericTupleSize<TupleSize>;
  using Superclass = std::iterator<std::random_access_iterator_tag,
                                   GetAPIType<ArrayType>,
                                   ComponentIdType,
                                   void,
                                   GetAPIType<ArrayType>>;

public:
  using iterator_category = typename Superclass::iterator_category;
  using value_type = typename Superclass::value_type;
  using difference_type = typename Superclass::difference_type;
  using pointer = typename Superclass::pointer;
  using reference = typename Superclass::reference;

  VTK_ITER_INLINE
  ConstComponentIterator(ArrayType* array,
                         NumCompsType numComps,
                         TupleIdType tupleId,
                         ComponentIdType comp) noexcept
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
  ConstComponentIterator(const ConstComponentIterator& o) noexcept = default;
  VTK_ITER_INLINE
  ConstComponentIterator&
  operator=(const ConstComponentIterator& o) noexcept = default;

  VTK_ITER_INLINE
  ConstComponentIterator& operator++() noexcept // prefix
  {
    ++this->ComponentId;
    VTK_ITER_ASSERT(this->ComponentId >= 0 &&
                    this->ComponentId <= this->NumComps.value,
                    "Const component iterator at invalid component id.");
    return *this;
  }

  VTK_ITER_INLINE
  ConstComponentIterator operator++(int) noexcept // postfix
  {
    return ConstComponentIterator{this->Array,
                                  this->NumComps,
                                  this->TupleId,
                                  this->ComponentId++};
  }

  VTK_ITER_INLINE
  ConstComponentIterator& operator--() noexcept // prefix
  {
    --this->ComponentId;
    VTK_ITER_ASSERT(this->ComponentId >= 0 &&
                    this->ComponentId <= this->NumComps.value,
                    "Const component iterator at invalid component id.");
    return *this;
  }

  VTK_ITER_INLINE
  ConstComponentIterator operator--(int) noexcept // postfix
  {
    return ConstComponentIterator{this->Array,
                                  this->NumComps,
                                  this->TupleId,
                                  this->ComponentId--};
  }

  // operator[] is disabled. See vtk::DataArrayTupleRange documentation.
#if 0
  // Note that this is just a value_type, not an actual reference.
  VTK_ITER_INLINE
  value_type operator[](difference_type i) const noexcept
  {
    VTK_ITER_ASSUME(this->Array->GetNumberOfComponents() ==
                    this->NumComps.value);
    vtkDataArrayAccessor<ArrayType> acc{this->Array};
    return acc.Get(this->TupleId, this->ComponentId + i);
  }
#endif

  // Note that this is just a value_type, not an actual reference.
  VTK_ITER_INLINE
  value_type operator*() const noexcept
  {
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    VTK_ITER_ASSUME(this->Array->GetNumberOfComponents() == this->NumComps.value);
    vtkDataArrayAccessor<ArrayType> acc{this->Array};
    return acc.Get(this->TupleId, this->ComponentId);
  }

#define VTK_TMP_MAKE_OPERATOR(OP) \
  friend VTK_ITER_INLINE \
  bool operator OP (const ConstComponentIterator& lhs, \
                           const ConstComponentIterator& rhs) noexcept \
  { \
    VTK_ITER_ASSERT(lhs.Array == rhs.Array, \
                    "Mismatched arrays in iterator comparison."); \
    VTK_ITER_ASSERT(lhs.TupleId == rhs.TupleId, \
                    "Mismatched tuple ids in iterator comparison."); \
    VTK_ITER_ASSUME(lhs.NumComps.value > 0); \
    VTK_ITER_ASSUME(lhs.NumComps.value == rhs.NumComps.value); \
    return lhs.ComponentId OP rhs.ComponentId; \
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
    VTK_ITER_ASSERT(this->ComponentId >= 0 &&
                    this->ComponentId <= this->NumComps.value,
                    "Const component iterator at invalid component id.");
    return *this;
  }

  friend VTK_ITER_INLINE
  ConstComponentIterator operator+(const ConstComponentIterator& it,
                                   difference_type offset) noexcept
  {
    return ConstComponentIterator{it.Array,
                                  it.NumComps,
                                  it.TupleId,
                                  it.ComponentId + offset};
  }

  friend VTK_ITER_INLINE
  ConstComponentIterator operator+(difference_type offset,
                                   const ConstComponentIterator& it) noexcept
  {
    return ConstComponentIterator{it.Array,
                                  it.NumComps,
                                  it.TupleId,
                                  it.ComponentId + offset};
  }

  VTK_ITER_INLINE
  ConstComponentIterator& operator-=(difference_type offset) noexcept
  {
    this->ComponentId -= offset;
    VTK_ITER_ASSERT(this->ComponentId >= 0 &&
                    this->ComponentId <= this->NumComps.value,
                    "Const component iterator at invalid component id.");
    return *this;
  }

  friend VTK_ITER_INLINE
  ConstComponentIterator operator-(const ConstComponentIterator& it,
                                   difference_type offset) noexcept
  {
    return ConstComponentIterator{it.Array,
                                  it.NumComps,
                                  it.TupleId,
                                  it.ComponentId - offset};
  }

  friend VTK_ITER_INLINE
  difference_type operator-(const ConstComponentIterator& it1,
                            const ConstComponentIterator& it2) noexcept
  {
    VTK_ITER_ASSERT(it1.Array == it2.Array,
                    "Cannot do math with iterators from different arrays.");
    VTK_ITER_ASSERT(it1.TupleId == it2.TupleId,
                    "Cannot do math with component iterators from different "
                    "tuples.");
    return it1.ComponentId - it2.ComponentId;
  }

  friend VTK_ITER_INLINE
  void swap(ConstComponentIterator& lhs, ConstComponentIterator &rhs) noexcept
  {
    // Different arrays may use different iterator implementations.
    VTK_ITER_ASSERT(lhs.Array == rhs.Array,
                    "Cannot swap iterators from different arrays.");

    using std::swap;
    swap(lhs.TupleId, rhs.TupleId);
    swap(lhs.ComponentId, rhs.ComponentId);
  }

private:
  mutable ArrayType *Array;
  NumCompsType NumComps;
  TupleIdType TupleId;
  ComponentIdType ComponentId;
};

//------------------------------------------------------------------------------
// Component iterator
template <typename ArrayType,
          ComponentIdType TupleSize>
struct ComponentIterator :
    public std::iterator<std::random_access_iterator_tag,
                         GetAPIType<ArrayType>,
                         ComponentIdType,
                         ComponentReference<ArrayType, TupleSize>,
                         ComponentReference<ArrayType, TupleSize>>
{
private:
  static_assert(IsValidTupleSize<TupleSize>::value, "Invalid tuple size.");
  static_assert(IsVtkDataArray<ArrayType>::value, "Invalid array type.");

  using NumCompsType = GenericTupleSize<TupleSize>;
  using APIType = GetAPIType<ArrayType>;
  using Superclass = std::iterator<std::random_access_iterator_tag,
                                   APIType,
                                   ComponentIdType,
                                   ComponentReference<ArrayType, TupleSize>,
                                   ComponentReference<ArrayType, TupleSize>>;

public:
  using iterator_category = typename Superclass::iterator_category;
  using value_type = typename Superclass::value_type;
  using difference_type = typename Superclass::difference_type;
  using pointer = typename Superclass::pointer;
  using reference = typename Superclass::reference;

  VTK_ITER_INLINE
  ComponentIterator(ArrayType* array,
                    NumCompsType numComps,
                    TupleIdType tupleId,
                    ComponentIdType comp) noexcept
    : Ref(array, numComps, tupleId, comp)
  {
    VTK_ITER_ASSERT(array != nullptr, "Invalid array.");
    VTK_ITER_ASSERT(numComps.value > 0, "Invalid number of components.");
    VTK_ITER_ASSERT(tupleId >= 0 && tupleId <= array->GetNumberOfTuples(),
                    "Component iterator at invalid tuple id.");
    VTK_ITER_ASSERT(comp >= 0 && comp <= numComps.value,
                    "Component iterator at invalid component id.");
    VTK_ITER_ASSERT(this->Ref.IsValue == false,
                    "Iterator's internal ref cannot be in value state.");
  }

  VTK_ITER_INLINE
  ComponentIterator(const ComponentIterator& o) noexcept
    : Ref(o.Ref, typename reference::CopyStateTag{})
  {
    VTK_ITER_ASSERT(!this->Ref.IsValue && !o.Ref.IsValue,
                    "Iterator's internal ref cannot be in value state.");
  }

  VTK_ITER_INLINE
  ComponentIterator& operator=(const ComponentIterator& o) noexcept
  {
    VTK_ITER_ASSERT(this->Ref.IsValue == false,
                    "Iterator's internal ref cannot be in value state.");
    this->Ref.CopyReference(o.Ref);
    return *this;
  }

  VTK_ITER_INLINE
  ComponentIterator& operator++() noexcept // prefix
  {
    VTK_ITER_ASSERT(this->Ref.IsValue == false,
                    "Iterator's internal ref cannot be in value state.");
    ++this->Ref.Data.Lookup.ComponentId;
    VTK_ITER_ASSERT(this->Ref.Data.Lookup.ComponentId >= 0 &&
                    this->Ref.Data.Lookup.ComponentId <=
                    this->Ref.Data.Lookup.NumComps.value,
                    "Component iterator at invalid component id.");
    return *this;
  }

  VTK_ITER_INLINE
  ComponentIterator operator++(int) noexcept // postfix
  {
    VTK_ITER_ASSERT(this->Ref.IsValue == false,
                    "Iterator's internal ref cannot be in value state.");
    return ComponentIterator{this->Ref.Data.Lookup.Array,
                             this->Ref.Data.Lookup.NumComps,
                             this->Ref.Data.Lookup.TupleId,
                             this->Ref.Data.Lookup.ComponentId++};
  }

  VTK_ITER_INLINE
  ComponentIterator& operator--() noexcept // prefix
  {
    VTK_ITER_ASSERT(this->Ref.IsValue == false,
                    "Iterator's internal ref cannot be in value state.");
    --this->Ref.Data.Lookup.ComponentId;
    VTK_ITER_ASSERT(this->Ref.Data.Lookup.ComponentId >= 0 &&
                    this->Ref.Data.Lookup.ComponentId <=
                    this->Ref.Data.Lookup.NumComps.value,
                    "Component iterator at invalid component id.");
    return *this;
  }

  VTK_ITER_INLINE
  ComponentIterator operator--(int) noexcept // postfix
  {
    VTK_ITER_ASSERT(this->Ref.IsValue == false,
                    "Iterator's internal ref cannot be in value state.");
    return ComponentIterator{this->Ref.Data.Lookup.Array,
                             this->Ref.Data.Lookup.NumComps,
                             this->Ref.Data.Lookup.TupleId,
                             this->Ref.Data.Lookup.ComponentId--};
  }

  // Intentionally disabled. See vtk::DataArrayTupleRange documentation.
#if 0
  VTK_ITER_INLINE
  reference operator[](difference_type i) const noexcept
  {
    return reference{this->Ref.Array,
                     this->Ref.NumComps,
                     this->Ref.TupleId,
                     this->Ref.ComponentId + i};
  }
#endif

  VTK_ITER_INLINE
  const reference& operator*() const noexcept
  {
    return this->Ref;
  }

  VTK_ITER_INLINE
  reference& operator*() noexcept
  {
    return this->Ref;
  }

  VTK_ITER_INLINE
  const pointer& operator->() const noexcept
  {
    return this->Ref;
  }

  VTK_ITER_INLINE
  pointer& operator->() noexcept
  {
    return this->Ref;
  }

#define VTK_TMP_MAKE_OPERATOR(OP) \
  friend VTK_ITER_INLINE \
  bool operator OP (const ComponentIterator& lhs, \
                           const ComponentIterator& rhs) noexcept \
  { \
    VTK_ITER_ASSERT(!lhs.RefIsValue() && !rhs.RefIsValue(), \
                    "Iterator's internal ref must not be in value state."); \
    VTK_ITER_ASSERT(lhs.GetArray() == rhs.GetArray(), \
                    "Mismatched arrays in iterator comparison."); \
    VTK_ITER_ASSERT(lhs.GetTupleId() == rhs.GetTupleId(), \
                    "Mismatched tuple ids in iterator comparison."); \
    VTK_ITER_ASSUME(lhs.GetNumComps().value > 0); \
    VTK_ITER_ASSUME(lhs.GetNumComps().value == rhs.GetNumComps().value); \
    return lhs.GetComponentId() OP rhs.GetComponentId(); \
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
    VTK_ITER_ASSERT(this->Ref.IsValue == false,
                    "Iterator's internal ref cannot be in value state.");
    this->Ref.Data.Lookup.ComponentId += offset;
    VTK_ITER_ASSERT(this->Ref.Data.Lookup.ComponentId >= 0 &&
                    this->Ref.Data.Lookup.ComponentId <=
                    this->Ref.Data.Lookup.NumComps.value,
                    "Component iterator at invalid component id.");
    return *this;
  }

  friend VTK_ITER_INLINE
  ComponentIterator operator+(const ComponentIterator& it,
                                     difference_type offset) noexcept
  {
    VTK_ITER_ASSERT(!it.RefIsValue(),
                    "Iterator's internal ref cannot be in value state.");
    return ComponentIterator{it.GetArray(),
                             it.GetNumComps(),
                             it.GetTupleId(),
                             it.GetComponentId() + offset};
  }

  friend VTK_ITER_INLINE
  ComponentIterator operator+(difference_type offset,
                                     const ComponentIterator& it) noexcept
  {
    VTK_ITER_ASSERT(!it.RefIsValue(),
                    "Iterator's internal ref cannot be in value state.");
    return ComponentIterator{it.GetArray(),
                             it.GetNumComps(),
                             it.GetTupleId(),
                             it.GetComponentId() + offset};
  }

  VTK_ITER_INLINE
  ComponentIterator& operator-=(difference_type offset) noexcept
  {
    VTK_ITER_ASSERT(this->Ref.IsValue == false,
                    "Iterator's internal ref cannot be in value state.");
    this->Ref.Data.Lookup.ComponentId -= offset;
    VTK_ITER_ASSERT(this->Ref.Data.Lookup.ComponentId >= 0 &&
                    this->Ref.Data.Lookup.ComponentId <=
                    this->Ref.Data.Lookup.NumComps.value,
                    "Component iterator at invalid component id.");
    return *this;
  }

  friend VTK_ITER_INLINE
  ComponentIterator operator-(const ComponentIterator& it,
                                     difference_type offset) noexcept
  {
    VTK_ITER_ASSERT(!it.RefIsValue(),
                    "Iterator's internal ref cannot be in value state.");
    return ComponentIterator{it.GetArray(),
                             it.GetNumComps(),
                             it.GetTupleId(),
                             it.GetComponentId() - offset};
  }

  friend VTK_ITER_INLINE
  difference_type operator-(const ComponentIterator& it1,
                            const ComponentIterator& it2) noexcept
  {
    VTK_ITER_ASSERT(!it1.RefIsValue() && !it2.RefIsValue(),
                    "Iterator's internal ref cannot be in value state.");
    VTK_ITER_ASSERT(it1.GetArray() == it2.GetArray(),
                    "Cannot do math with component iterators from different "
                    "arrays.");
    VTK_ITER_ASSERT(it1.GetTupleId() == it2.GetTupleId(),
                    "Cannot do math with component iterators from different "
                    "tuples.");
    return it1.GetComponentId() - it2.GetComponentId();
  }

  friend VTK_ITER_INLINE
  void swap(ComponentIterator& lhs, ComponentIterator &rhs) noexcept
  {
    VTK_ITER_ASSERT(!lhs.RefIsValue() && !rhs.RefIsValue(),
                    "Iterator's internal ref cannot be in value state.");
    // Different arrays may use different iterator implementations.
    VTK_ITER_ASSERT(lhs.GetArray() == rhs.GetArray(),
                    "Cannot swap iterators from different arrays.");

    using std::swap;
    swap(lhs.GetTupleId(), rhs.GetTupleId());
    swap(lhs.GetComponentId(), rhs.GetComponentId());
  }

protected:
  VTK_ITER_INLINE
  bool RefIsValue() const noexcept { return this->Ref.IsValue; }
  VTK_ITER_INLINE
  ArrayType* GetArray() const noexcept { return this->Ref.Data.Lookup.Array; }
  VTK_ITER_INLINE
  ArrayType*& GetArray() noexcept { return this->Ref.Data.Lookup.Array; }
  VTK_ITER_INLINE
  const NumCompsType& GetNumComps() const noexcept
  {
    return this->Ref.Data.Lookup.NumComps;
  }
  VTK_ITER_INLINE
  NumCompsType& GetNumComps() noexcept
  {
    return this->Ref.Data.Lookup.NumComps;
  }
  VTK_ITER_INLINE
  const TupleIdType& GetTupleId() const noexcept
  {
    return this->Ref.Data.Lookup.TupleId;
  }
  VTK_ITER_INLINE
  TupleIdType& GetTupleId() noexcept { return this->Ref.Data.Lookup.TupleId; }
  VTK_ITER_INLINE
  const ComponentIdType& GetComponentId() const noexcept
  {
    return this->Ref.Data.Lookup.ComponentId;
  }
  VTK_ITER_INLINE
  ComponentIdType& GetComponentId() noexcept
  {
    return this->Ref.Data.Lookup.ComponentId;
  }

  ComponentReference<ArrayType, TupleSize> Ref;
};

//------------------------------------------------------------------------------
// Const tuple reference
template <typename ArrayType,
          ComponentIdType TupleSize>
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

  VTK_ITER_INLINE
  ConstTupleReference(ArrayType* array,
                      NumCompsType numComps,
                      TupleIdType tupleId) noexcept
    : Array(array)
    , NumComps(numComps)
    , TupleId(tupleId)
  {
    VTK_ITER_ASSERT(array != nullptr, "Invalid array.");
    VTK_ITER_ASSERT(numComps.value > 0, "Invalid number of components.");
    VTK_ITER_ASSERT(tupleId >= 0 && tupleId <= array->GetNumberOfTuples(),
                    "Const tuple reference at invalid tuple id.");
  }

  // Allow this type to masquerade as a pointer, so that tupleIiter->foo works.
  VTK_ITER_INLINE
  ConstTupleReference* operator->() noexcept { return this; }
  VTK_ITER_INLINE
  const ConstTupleReference* operator->() const noexcept { return this; }

  // Caller must ensure that there are size() elements in array.
  VTK_ITER_INLINE
  void GetTuple(APIType *tuple) const noexcept
  {
    vtkDataArrayAccessor<ArrayType> acc{this->Array};
    acc.Get(this->TupleId, tuple);
  }

  // skips some runtime checks when both sizes are fixed:
  template <typename OArrayType,
            ComponentIdType OSize>
  VTK_ITER_INLINE
  EnableIfStaticTupleSizes<TupleSize, OSize, bool>
  operator==(const TupleReference<OArrayType, OSize> &other) const noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_convertible<OAPIType, APIType>{}),
                  "Incompatible types when comparing tuples.");

    // SFINAE guarantees that the tuple sizes are not dynamic in this overload:
    static_assert(TupleSize == OSize,
                  "Cannot compare tuples with different sizes.");

    return std::equal(this->cbegin(), this->cend(), other.cbegin());
  }

  // Needs a runtime check:
  template <typename OArrayType,
            ComponentIdType OSize>
  VTK_ITER_INLINE
  EnableIfEitherTupleSizeIsDynamic<TupleSize, OSize, bool>
  operator==(const TupleReference<OArrayType, OSize> &other) const noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_convertible<OAPIType, APIType>{}),
                  "Incompatible types when comparing tuples.");

    VTK_ITER_ASSERT(other.size() == this->NumComps.value,
                    "Cannot compare tuples with different sizes.");

    return std::equal(this->cbegin(), this->cend(), other.cbegin());
  }

  // skips some runtime checks when both sizes are fixed:
  template <typename OArrayType,
            ComponentIdType OSize>
  VTK_ITER_INLINE
  EnableIfStaticTupleSizes<TupleSize, OSize, bool>
  operator==(const ConstTupleReference<OArrayType, OSize> &other) const noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_convertible<OAPIType, APIType>{}),
                  "Incompatible types when comparing tuples.");

    // SFINAE guarantees that the tuple sizes are not dynamic in this overload:
    static_assert(TupleSize == OSize,
                  "Cannot compare tuples with different sizes.");

    return std::equal(this->cbegin(), this->cend(), other.cbegin());
  }

  // Needs a runtime check:
  template <typename OArrayType,
            ComponentIdType OSize>
  VTK_ITER_INLINE
  EnableIfEitherTupleSizeIsDynamic<TupleSize, OSize, bool>
  operator==(const ConstTupleReference<OArrayType, OSize> &other) const noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_convertible<OAPIType, APIType>{}),
                  "Incompatible types when comparing tuples.");

    VTK_ITER_ASSERT(other.size() == this->NumComps.value,
                    "Cannot compare tuples with different sizes.");

    return std::equal(this->cbegin(), this->cend(), other.cbegin());
  }

  template <typename OArrayType,
            ComponentIdType OSize>
  VTK_ITER_INLINE
  bool operator!=(const TupleReference<OArrayType, OSize>& o) const noexcept
  {
    return !(*this == o);
  }

  template <typename OArrayT,
            ComponentIdType OSize>
  VTK_ITER_INLINE
  bool operator!=(const ConstTupleReference<OArrayT, OSize>& o) const noexcept
  {
    return !(*this == o);
  }

  // operator[] disabled. See vtk::DataArrayTupleRange documentation.
#if 0
  const_reference operator[](size_type i) const noexcept
  {
    VTK_ITER_ASSUME(this->Array->GetNumberOfComponents() ==
                    this->NumComps.value);
    vtkDataArrayAccessor<ArrayType> acc{this->Array};
    return acc.Get(this->TupleId, i);
  }
#endif

  VTK_ITER_INLINE
  size_type size() const noexcept { return this->NumComps.value; }

  VTK_ITER_INLINE
  const_iterator begin() const noexcept { return this->NewConstIterator(0); }
  VTK_ITER_INLINE
  const_iterator end() const noexcept
  {
    return this->NewConstIterator(this->NumComps.value);
  }

  VTK_ITER_INLINE
  const_iterator cbegin() const noexcept { return this->NewConstIterator(0); }
  VTK_ITER_INLINE
  const_iterator cend() const noexcept
  {
    return this->NewConstIterator(this->NumComps.value);
  }

  friend struct ConstTupleIterator<ArrayType, TupleSize>;

protected:

  VTK_ITER_INLINE
  const_iterator NewConstIterator(ComponentIdType comp) const noexcept
  {
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    return const_iterator{this->Array,
                          this->NumComps,
                          this->TupleId,
                          comp};
  }

  // Intentionally hidden. See vtk::DataArrayTupleRange documentation.
  VTK_ITER_INLINE
  ConstTupleReference(const ConstTupleReference&) noexcept = default;
  VTK_ITER_INLINE
  ConstTupleReference& operator=(const ConstTupleReference&) noexcept = default;

  VTK_ITER_INLINE
  void CopyReference(const ConstTupleReference& o) noexcept
  {
    // Must use same array, other array types may use different implementations.
    VTK_ITER_ASSERT(this->Array == o.Array,
                    "Cannot copy reference objects between arrays.");
    this->NumComps = o.NumComps;
    this->TupleId = o.TupleId;
  }

  mutable ArrayType* Array;
  NumCompsType NumComps;
  TupleIdType TupleId;
};

//------------------------------------------------------------------------------
// Tuple reference
template <typename ArrayType,
          ComponentIdType TupleSize>
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

  VTK_ITER_INLINE
  TupleReference(ArrayType* array,
                 NumCompsType numComps,
                 TupleIdType tupleId) noexcept
    : Array(array)
    , NumComps(numComps)
    , TupleId(tupleId)
  {
    VTK_ITER_ASSERT(array != nullptr, "Invalid array.");
    VTK_ITER_ASSERT(numComps.value > 0, "Invalid number of components.");
    VTK_ITER_ASSERT(tupleId >= 0 && tupleId <= array->GetNumberOfTuples(),
                    "Tuple reference at invalid tuple id.");
  }

  // Allow this type to masquerade as a pointer, so that tupleIiter->foo works.
  VTK_ITER_INLINE
  TupleReference* operator->() noexcept { return this; }
  VTK_ITER_INLINE
  const TupleReference* operator->() const noexcept { return this; }

  // Caller must ensure that there are size() elements in array.
  VTK_ITER_INLINE
  void GetTuple(APIType *tuple) const noexcept
  {
    vtkDataArrayAccessor<ArrayType> acc{this->Array};
    acc.Get(this->TupleId, tuple);
  }

  // Caller must ensure that there are size() elements in array.
  VTK_ITER_INLINE
  void SetTuple(const APIType *tuple) noexcept
  {
    vtkDataArrayAccessor<ArrayType> acc{this->Array};
    acc.Set(this->TupleId, tuple);
  }

  VTK_ITER_INLINE
  TupleReference& operator=(const TupleReference& other) noexcept
  {
    std::copy_n(other.cbegin(), this->NumComps.value, this->begin());
    return *this;
  }

  // skips some runtime checks when both sizes are fixed:
  template <typename OArrayType,
            ComponentIdType OSize>
  VTK_ITER_INLINE
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

  // Needs a runtime check:
  template <typename OArrayType,
            ComponentIdType OSize>
  VTK_ITER_INLINE
  EnableIfEitherTupleSizeIsDynamic<TupleSize, OSize, TupleReference&>
  operator=(const TupleReference<OArrayType, OSize> &other) noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_convertible<OAPIType, APIType>{}),
                  "Incompatible types when assigning tuples.");

    VTK_ITER_ASSERT(other.size() == this->NumComps.value,
                    "Cannot assign tuples with different sizes.");

    std::copy_n(other.cbegin(), this->NumComps.value, this->begin());
    return *this;
  }

  // skips some runtime checks when both sizes are fixed:
  template <typename OArrayType,
            ComponentIdType OSize>
  VTK_ITER_INLINE
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

  // Needs a runtime check:
  template <typename OArrayType,
            ComponentIdType OSize>
  VTK_ITER_INLINE
  EnableIfEitherTupleSizeIsDynamic<TupleSize, OSize, TupleReference&>
  operator=(const ConstTupleReference<OArrayType, OSize> &other) noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_convertible<OAPIType, APIType>{}),
                  "Incompatible types when assigning tuples.");

    VTK_ITER_ASSERT(other.size() == this->NumComps.value,
                    "Cannot assign tuples with different sizes.");

    std::copy_n(other.cbegin(), this->NumComps.value, this->begin());
    return *this;
  }

  // skips some runtime checks when both sizes are fixed:
  template <typename OArrayType,
            ComponentIdType OSize>
  VTK_ITER_INLINE
  EnableIfStaticTupleSizes<TupleSize, OSize, bool>
  operator==(const TupleReference<OArrayType, OSize> &other) const noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_convertible<OAPIType, APIType>{}),
                  "Incompatible types when comparing tuples.");

    // SFINAE guarantees that the tuple sizes are not dynamic in this overload:
    static_assert(TupleSize == OSize,
                  "Cannot compare tuples with different sizes.");

    return std::equal(this->cbegin(), this->cend(), other.cbegin());
  }

  // Needs a runtime check:
  template <typename OArrayType,
            ComponentIdType OSize>
  VTK_ITER_INLINE
  EnableIfEitherTupleSizeIsDynamic<TupleSize, OSize, bool>
  operator==(const TupleReference<OArrayType, OSize> &other) const noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_convertible<OAPIType, APIType>{}),
                  "Incompatible types when comparing tuples.");

    VTK_ITER_ASSERT(other.size() == this->NumComps.value,
                    "Cannot compare tuples with different sizes.");

    return std::equal(this->cbegin(), this->cend(), other.cbegin());
  }

  // skips some runtime checks when both sizes are fixed:
  template <typename OArrayType,
            ComponentIdType OSize>
  VTK_ITER_INLINE
  EnableIfStaticTupleSizes<TupleSize, OSize, bool>
  operator==(const ConstTupleReference<OArrayType, OSize> &other) const noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_convertible<OAPIType, APIType>{}),
                  "Incompatible types when comparing tuples.");

    // SFINAE guarantees that the tuple sizes are not dynamic in this overload:
    static_assert(TupleSize == OSize,
                  "Cannot compare tuples with different sizes.");

    return std::equal(this->cbegin(), this->cend(), other.cbegin());
  }

  // Needs a runtime check:
  template <typename OArrayType,
            ComponentIdType OSize>
  VTK_ITER_INLINE
  EnableIfEitherTupleSizeIsDynamic<TupleSize, OSize, bool>
  operator==(const ConstTupleReference<OArrayType, OSize> &other) const noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_convertible<OAPIType, APIType>{}),
                  "Incompatible types when comparing tuples.");

    VTK_ITER_ASSERT(other.size() == this->NumComps.value,
                    "Cannot compare tuples with different sizes.");

    return std::equal(this->cbegin(), this->cend(), other.cbegin());
  }

  template <typename OArrayType,
            ComponentIdType OSize>
  VTK_ITER_INLINE
  bool operator!=(const TupleReference<OArrayType, OSize>& o) const noexcept
  {
    return !(*this == o);
  }

  template <typename OArray,
            ComponentIdType OSize>
  VTK_ITER_INLINE
  bool operator!=(const ConstTupleReference<OArray, OSize>& o) const noexcept
  {
    return !(*this == o);
  }

  // skips some runtime checks:
  template <typename OArrayType,
            ComponentIdType OSize>
  VTK_ITER_INLINE
  EnableIfStaticTupleSizes<TupleSize, OSize, void>
  swap(TupleReference<OArrayType, OSize> &other) noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_convertible<OAPIType, APIType>{}),
                  "Incompatible types when swapping tuples.");

    // SFINAE guarantees that the tuple sizes are not dynamic in this overload:
    static_assert(TupleSize == OSize,
                  "Cannot swap tuples with different sizes.");

    std::swap_ranges(this->begin(), this->end(), other.begin());
  }

  // Needs a runtime check:
  template <typename OArrayType,
            ComponentIdType OSize>
  VTK_ITER_INLINE
  EnableIfEitherTupleSizeIsDynamic<TupleSize, OSize, void>
  swap(TupleReference<OArrayType, OSize> &other) noexcept
  {
    // Check that types are convertible:
    using OAPIType = GetAPIType<OArrayType>;
    static_assert((std::is_convertible<OAPIType, APIType>{}),
                  "Incompatible types when swapping tuples.");

    VTK_ITER_ASSERT(other.size() == this->NumComps.value,
                    "Cannot swap tuples with different sizes.");

    std::swap_ranges(this->begin(), this->end(), other.begin());
  }

  friend VTK_ITER_INLINE
  void swap(TupleReference &a,
                   TupleReference &b) noexcept
  {
    a.swap(b);
  }

  template <typename OArray, ComponentIdType OSize>
  friend VTK_ITER_INLINE
  void swap(TupleReference &a,
                   TupleReference<OArray, OSize> &b) noexcept
  {
    a.swap(b);
  }

  // Intentionally disabled. See vtk::DataArrayTupleRange documentation.
#if 0
  VTK_ITER_INLINE
  reference operator[](size_type i) const
  {
    return reference{this->Array, this->NumComps, this->TupleId, i};
  }
#endif

  VTK_ITER_INLINE
  void fill(const value_type &v) noexcept
  {
    std::fill(this->begin(), this->end(), v);
  }

  VTK_ITER_INLINE
  size_type size() const noexcept { return this->NumComps.value; }

  VTK_ITER_INLINE
  iterator begin() noexcept { return this->NewIterator(0); }
  VTK_ITER_INLINE
  iterator end() noexcept { return this->NewIterator(this->NumComps.value); }

  VTK_ITER_INLINE
  const_iterator begin() const noexcept { return this->NewConstIterator(0); }
  VTK_ITER_INLINE
  const_iterator end() const noexcept
  {
    return this->NewConstIterator(this->NumComps.value);
  }

  VTK_ITER_INLINE
  const_iterator cbegin() const noexcept { return this->NewConstIterator(0); }
  VTK_ITER_INLINE
  const_iterator cend() const noexcept
  {
    return this->NewConstIterator(this->NumComps.value);
  }

  friend struct TupleIterator<ArrayType, TupleSize>;

protected:

  VTK_ITER_INLINE
  iterator NewIterator(ComponentIdType comp) const noexcept
  {
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    return iterator{this->Array, this->NumComps, this->TupleId, comp};
  }

  VTK_ITER_INLINE
  const_iterator NewConstIterator(ComponentIdType comp) const noexcept
  {
    VTK_ITER_ASSUME(this->NumComps.value > 0);
    return const_iterator{this->Array,
                               this->NumComps,
                               this->TupleId,
                               comp};
  }

  // Intentionally hidden. See vtk::DataArrayTupleRange documentation.
  VTK_ITER_INLINE
  TupleReference(const TupleReference&) = default;

  VTK_ITER_INLINE
  void CopyReference(const TupleReference& o) noexcept
  {
    // Must use same array, other array types may use different implementations.
    VTK_ITER_ASSERT(this->Array == o.Array,
                    "Cannot copy reference objects between arrays.");
    this->NumComps = o.NumComps;
    this->TupleId = o.TupleId;
  }

  mutable ArrayType* Array;
  NumCompsType NumComps;
  TupleIdType TupleId;
};

//------------------------------------------------------------------------------
// Const tuple iterator
template <typename ArrayType,
          ComponentIdType TupleSize>
struct ConstTupleIterator :
    public std::iterator<std::random_access_iterator_tag,
                         ConstTupleReference<ArrayType, TupleSize>,
                         TupleIdType,
                         ConstTupleReference<ArrayType, TupleSize>,
                         ConstTupleReference<ArrayType, TupleSize>>
{
private:
  static_assert(IsValidTupleSize<TupleSize>::value, "Invalid tuple size.");
  static_assert(IsVtkDataArray<ArrayType>::value, "Invalid array type.");

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

  VTK_ITER_INLINE
  ConstTupleIterator(ArrayType* array,
                     NumCompsType numComps,
                     TupleIdType tupleId) noexcept
    : Ref(array, numComps, tupleId)
  {
    VTK_ITER_ASSERT(array != nullptr, "Invalid array.");
    VTK_ITER_ASSERT(numComps.value > 0, "Invalid number of components.");
    VTK_ITER_ASSERT(tupleId >= 0 && tupleId <= array->GetNumberOfTuples(),
                    "Const tuple iterator at invalid tuple id.");
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
    VTK_ITER_ASSERT(this->Ref.TupleId >= 0 &&
                    this->Ref.TupleId <= this->Ref.Array->GetNumberOfTuples(),
                    "Const tuple iterator at invalid component id.");
    return *this;
  }

  VTK_ITER_INLINE
  ConstTupleIterator operator++(int) noexcept // postfix
  {
    return ConstTupleIterator{this->Ref.Array,
                              this->Ref.NumComps,
                              this->Ref.TupleId++};
  }

  VTK_ITER_INLINE
  ConstTupleIterator& operator--() noexcept // prefix
  {
    --this->Ref.TupleId;
    VTK_ITER_ASSERT(this->Ref.TupleId >= 0 &&
                    this->Ref.TupleId <= this->Ref.Array->GetNumberOfTuples(),
                    "Const tuple iterator at invalid component id.");
    return *this;
  }

  VTK_ITER_INLINE
  ConstTupleIterator operator--(int) noexcept // postfix
  {
    return ConstTupleIterator{this->Ref.Array,
                              this->Ref.NumComps,
                              this->Ref.TupleId--};
  }

  // Intentionally disabled. See vtk::DataArrayRange documentation.
#if 0
  VTK_ITER_INLINE
  reference operator[](difference_type i) const noexcept
  {
    return reference{this->Array, this->NumComps, TupleId + i};
  }
#endif

  VTK_ITER_INLINE
  reference& operator*() noexcept
  {
    return this->Ref;
  }

  VTK_ITER_INLINE
  pointer& operator->() noexcept
  {
    return this->Ref;
  }

#define VTK_TMP_MAKE_OPERATOR(OP) \
  friend VTK_ITER_INLINE \
  bool operator OP (const ConstTupleIterator& lhs, \
                           const ConstTupleIterator& rhs) noexcept \
  { \
    VTK_ITER_ASSERT(lhs.GetArray() == rhs.GetArray(), \
                    "Cannot compare iterators from different arrays."); \
    VTK_ITER_ASSUME(lhs.GetNumComps().value > 0); \
    VTK_ITER_ASSUME(lhs.GetNumComps().value == rhs.GetNumComps().value); \
    return lhs.GetTupleId() OP rhs.GetTupleId(); \
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
    VTK_ITER_ASSERT(this->Ref.TupleId >= 0 &&
                    this->Ref.TupleId <= this->Ref.Array->GetNumberOfTuples(),
                    "Const tuple iterator at invalid component id.");
    return *this;
  }

  friend VTK_ITER_INLINE
  ConstTupleIterator operator+(const ConstTupleIterator& it,
                               difference_type offset) noexcept
  {
    return ConstTupleIterator{it.GetArray(),
                              it.GetNumComps(),
                              it.GetTupleId() + offset};
  }

  friend VTK_ITER_INLINE
  ConstTupleIterator operator+(difference_type offset,
                               const ConstTupleIterator& it) noexcept
  {
    return ConstTupleIterator{it.GetArray(),
                              it.GetNumComps(),
                              it.GetTupleId() + offset};
  }

  VTK_ITER_INLINE
  ConstTupleIterator& operator-=(difference_type offset) noexcept
  {
    this->Ref.TupleId -= offset;
    VTK_ITER_ASSERT(this->Ref.TupleId >= 0 &&
                    this->Ref.TupleId <= this->Ref.Array->GetNumberOfTuples(),
                    "Const tuple iterator at invalid component id.");
    return *this;
  }

  friend VTK_ITER_INLINE
  ConstTupleIterator operator-(const ConstTupleIterator& it,
                               difference_type offset) noexcept
  {
    return ConstTupleIterator{it.GetArray(),
                              it.GetNumComps(),
                              it.GetTupleId() - offset};
  }

  friend VTK_ITER_INLINE
  difference_type operator-(const ConstTupleIterator& it1,
                            const ConstTupleIterator& it2) noexcept
  {
    VTK_ITER_ASSERT(it1.GetArray() == it2.GetArray(),
                    "Cannot do math with tuple iterators from different "
                    "arrays.");
    return it1.GetTupleId() - it2.GetTupleId();
  }

  friend VTK_ITER_INLINE
  void swap(ConstTupleIterator& lhs, ConstTupleIterator &rhs) noexcept
  {
    // Different arrays may use different iterator implementations.
    VTK_ITER_ASSERT(lhs.GetArray() == rhs.GetArray(),
                    "Cannot swap iterators from different arrays.");

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
template <typename ArrayType,
          ComponentIdType TupleSize>
struct TupleIterator : public std::iterator<std::random_access_iterator_tag,
                                            TupleReference<ArrayType, TupleSize>,
                                            TupleIdType,
                                            TupleReference<ArrayType, TupleSize>,
                                            TupleReference<ArrayType, TupleSize>>
{
private:
  static_assert(IsValidTupleSize<TupleSize>::value, "Invalid tuple size.");
  static_assert(IsVtkDataArray<ArrayType>::value, "Invalid array type.");

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

  VTK_ITER_INLINE
  TupleIterator(ArrayType* array,
                NumCompsType numComps,
                TupleIdType tupleId) noexcept
    : Ref(array, numComps, tupleId)
  {
    VTK_ITER_ASSERT(array != nullptr, "Invalid array.");
    VTK_ITER_ASSERT(numComps.value > 0, "Invalid number of components.");
    VTK_ITER_ASSERT(tupleId >= 0 && tupleId <= array->GetNumberOfTuples(),
                    "Tuple iterator at invalid tuple id.");
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
    VTK_ITER_ASSERT(this->Ref.TupleId >= 0 &&
                    this->Ref.TupleId <= this->Ref.Array->GetNumberOfTuples(),
                    "Tuple iterator at invalid component id.");
    return *this;
  }

  VTK_ITER_INLINE
  TupleIterator operator++(int) noexcept // postfix
  {
    return TupleIterator{this->Ref.Array,
                         this->Ref.NumComps,
                         this->Ref.TupleId++};
  }

  VTK_ITER_INLINE
  TupleIterator& operator--() noexcept // prefix
  {
    --this->Ref.TupleId;
    VTK_ITER_ASSERT(this->Ref.TupleId >= 0 &&
                    this->Ref.TupleId <= this->Ref.Array->GetNumberOfTuples(),
                    "Tuple iterator at invalid component id.");
    return *this;
  }

  VTK_ITER_INLINE
  TupleIterator operator--(int) noexcept // postfix
  {
    return TupleIterator{this->Ref.Array,
                         this->Ref.NumComps,
                         this->Ref.TupleId--};
  }

  // Intentionally disabled. See vtk::DataArrayTupleRange documentation.
#if 0
  VTK_ITER_INLINE
  reference operator[](difference_type i) const
  {
    return reference{this->Ref.Array,
                     this->Ref.NumComps,
                     this->Ref.TupleId + i};
  }
#endif

  VTK_ITER_INLINE
  reference& operator*() noexcept
  {
    return this->Ref;
  }

  VTK_ITER_INLINE
  pointer& operator->() noexcept
  {
    return this->Ref;
  }

#define VTK_TMP_MAKE_OPERATOR(OP) \
  friend VTK_ITER_INLINE \
  bool operator OP (const TupleIterator& lhs, \
                           const TupleIterator& rhs) noexcept \
  { \
    VTK_ITER_ASSERT(lhs.GetArray() == rhs.GetArray(), \
                    "Cannot compare iterators from different arrays."); \
    VTK_ITER_ASSUME(lhs.GetNumComps().value > 0); \
    VTK_ITER_ASSUME(lhs.GetNumComps().value == rhs.GetNumComps().value); \
    return lhs.GetTupleId() OP rhs.GetTupleId(); \
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
    VTK_ITER_ASSERT(this->Ref.TupleId >= 0 &&
                    this->Ref.TupleId <= this->Ref.Array->GetNumberOfTuples(),
                    "Tuple iterator at invalid component id.");
    return *this;
  }

  friend VTK_ITER_INLINE
  TupleIterator operator+(const TupleIterator& it,
                          difference_type offset) noexcept
  {
    return TupleIterator{it.GetArray(),
                         it.GetNumComps(),
                         it.GetTupleId() + offset};
  }

  friend VTK_ITER_INLINE
  TupleIterator operator+(difference_type offset,
                          const TupleIterator& it) noexcept
  {
    return TupleIterator{it.GetArray(),
                         it.GetNumComps(),
                         it.GetTupleId() + offset};
  }

  VTK_ITER_INLINE
  TupleIterator& operator-=(difference_type offset) noexcept
  {
    this->Ref.TupleId -= offset;
    VTK_ITER_ASSERT(this->Ref.TupleId >= 0 &&
                    this->Ref.TupleId <= this->Ref.Array->GetNumberOfTuples(),
                    "Tuple iterator at invalid component id.");
    return *this;
  }

  friend VTK_ITER_INLINE
  TupleIterator operator-(const TupleIterator& it,
                          difference_type offset) noexcept
  {
    return TupleIterator{it.GetArray(),
                         it.GetNumComps(),
                         it.GetTupleId() - offset};
  }

  friend VTK_ITER_INLINE
  difference_type operator-(const TupleIterator& it1,
                            const TupleIterator& it2) noexcept
  {
    VTK_ITER_ASSERT(it1.GetArray() == it2.GetArray(),
                    "Cannot do math with tuple iterators from different "
                    "arrays.");
    return it1.GetTupleId() - it2.GetTupleId();
  }

  friend VTK_ITER_INLINE
  void swap(TupleIterator& lhs, TupleIterator &rhs) noexcept
  {
    // Different arrays may use different iterator implementations.
    VTK_ITER_ASSERT(lhs.GetArray() == rhs.GetArray(),
                    "Cannot swap iterators from different arrays.");

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

  TupleReference<ArrayType, TupleSize> Ref;
};

//------------------------------------------------------------------------------
// Tuple range
template <typename ArrayTypeT,
          ComponentIdType TupleSize>
struct TupleRange
{
private:
  using APIType = GetAPIType<ArrayTypeT>;
  using NumCompsType = GenericTupleSize<TupleSize>;
  static_assert(IsValidTupleSize<TupleSize>::value, "Invalid tuple size.");
  static_assert(IsVtkDataArray<ArrayTypeT>::value, "Invalid array type.");

public:
  using ArrayType = ArrayTypeT;
  using ComponentType = APIType;

  // May be DynamicTupleSize, or the actual tuple size.
  constexpr static ComponentIdType TupleSizeTag = TupleSize;

  using size_type = TupleIdType;
  using iterator = TupleIterator<ArrayType, TupleSize>;
  using const_iterator = ConstTupleIterator<ArrayType, TupleSize>;
  using reference = TupleReference<ArrayType, TupleSize>;
  using const_reference = ConstTupleReference<ArrayType, TupleSize>;

  VTK_ITER_INLINE
  TupleRange(ArrayType *arr,
             TupleIdType beginTuple,
             TupleIdType endTuple) noexcept
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
  const_iterator begin() const noexcept
  {
    return this->NewCIter(this->BeginTuple);
  }
  VTK_ITER_INLINE
  const_iterator end() const noexcept
  {
    return this->NewCIter(this->EndTuple);
  }

  VTK_ITER_INLINE
  const_iterator cbegin() const noexcept
  {
    return this->NewCIter(this->BeginTuple);
  }
  VTK_ITER_INLINE
  const_iterator cend() const noexcept
  {
    return this->NewCIter(this->EndTuple);
  }

private:

  VTK_ITER_INLINE
  iterator NewIter(TupleIdType t) const
  {
    return iterator{this->Array, this->NumComps, t};
  }

  VTK_ITER_INLINE
  const_iterator NewCIter(TupleIdType t) const
  {
    return const_iterator{this->Array, this->NumComps, t};
  }

  mutable vtkSmartPointer<ArrayType> Array;
  NumCompsType NumComps;
  TupleIdType BeginTuple;
  TupleIdType EndTuple;
};

} // end namespace detail
} // end namespace vtk

VTK_ITER_OPTIMIZE_END

#endif // __VTK_WRAP__
#endif // vtkDataArrayTupleRange_Generic_h

// VTK-HeaderTest-Exclude: vtkDataArrayTupleRange_Generic.h
