//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_internal_ArrayPortalValueReference_h
#define viskores_internal_ArrayPortalValueReference_h

#include <viskores/TypeTraits.h>
#include <viskores/Types.h>
#include <viskores/VecTraits.h>

namespace viskores
{
namespace internal
{

/// \brief A value class for returning setable values of an ArrayPortal
///
/// \c ArrayPortal classes have a pair of \c Get and \c Set methods that
/// retrieve and store values in the array. This is to make it easy to
/// implement the \c ArrayPortal even it is not really an array. However, there
/// are some cases where the code structure expects a reference to a value that
/// can be set. For example, the \c IteratorFromArrayPortal class must return
/// something from its * operator that behaves like a reference.
///
/// For cases of this nature \c ArrayPortalValueReference can be used. This
/// class is constructured with an \c ArrayPortal and an index into the array.
/// The object then behaves like a reference to the value in the array. If you
/// set this reference object to a new value, it will call \c Set on the
/// \c ArrayPortal to insert the value into the array.
///
template <typename ArrayPortalType>
struct ArrayPortalValueReference
{
  using ValueType = typename ArrayPortalType::ValueType;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ArrayPortalValueReference(const ArrayPortalType& portal, viskores::Id index)
    : Portal(portal)
    , Index(index)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ArrayPortalValueReference(const ArrayPortalValueReference& ref)
    : Portal(ref.Portal)
    , Index(ref.Index)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ValueType Get() const { return this->Portal.Get(this->Index); }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  operator ValueType() const { return this->Get(); }

  // Declaring Set as const seems a little weird because we are changing the value. But remember
  // that ArrayPortalReference is only a reference class. The reference itself does not change,
  // just the thing that it is referencing. So declaring as const is correct and necessary so that
  // you can set the value of a reference returned from a function (which is a right hand side
  // value).

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  void Set(ValueType&& value) const { this->Portal.Set(this->Index, std::move(value)); }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  void Set(const ValueType& value) const { this->Portal.Set(this->Index, value); }

  VISKORES_CONT
  void Swap(const ArrayPortalValueReference<ArrayPortalType>& rhs) const noexcept
  {
    //we need use the explicit type not a proxy temp object
    //A proxy temp object would point to the same underlying data structure
    //and would not hold the old value of *this once *this was set to rhs.
    const ValueType aValue = *this;
    *this = rhs;
    rhs = aValue;
  }

  // Declaring operator= as const seems a little weird because we are changing the value. But
  // remember that ArrayPortalReference is only a reference class. The reference itself does not
  // change, just the thing that it is referencing. So declaring as const is correct and necessary
  // so that you can set the value of a reference returned from a function (which is a right hand
  // side value).

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  const ArrayPortalValueReference<ArrayPortalType>& operator=(ValueType&& value) const
  {
    this->Set(std::move(value));
    return *this;
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  const ArrayPortalValueReference<ArrayPortalType>& operator=(const ValueType& value) const
  {
    this->Set(value);
    return *this;
  }

  // This special overload of the operator= is to override the behavior of the default operator=
  // (which has the wrong behavior) with behavior consistent with the other operator= methods.
  // It is not declared const because the default operator= is not declared const. If you try
  // to do this assignment with a const object, you will get one of the operator= above.
  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT ArrayPortalValueReference<ArrayPortalType>& operator=(
    const ArrayPortalValueReference<ArrayPortalType>& rhs)
  {
    this->Set(static_cast<ValueType>(rhs.Portal.Get(rhs.Index)));
    return *this;
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  VISKORES_EXEC_CONT ValueType operator+=(const T& rhs) const
  {
    ValueType lhs = this->Get();
    lhs += rhs;
    this->Set(lhs);
    return lhs;
  }
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  VISKORES_EXEC_CONT ValueType operator+=(const ArrayPortalValueReference<T>& rhs) const
  {
    ValueType lhs = this->Get();
    lhs += rhs.Get();
    this->Set(lhs);
    return lhs;
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  VISKORES_EXEC_CONT ValueType operator-=(const T& rhs) const
  {
    ValueType lhs = this->Get();
    lhs -= rhs;
    this->Set(lhs);
    return lhs;
  }
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  VISKORES_EXEC_CONT ValueType operator-=(const ArrayPortalValueReference<T>& rhs) const
  {
    ValueType lhs = this->Get();
    lhs -= rhs.Get();
    this->Set(lhs);
    return lhs;
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  VISKORES_EXEC_CONT ValueType operator*=(const T& rhs) const
  {
    ValueType lhs = this->Get();
    lhs *= rhs;
    this->Set(lhs);
    return lhs;
  }
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  VISKORES_EXEC_CONT ValueType operator*=(const ArrayPortalValueReference<T>& rhs) const
  {
    ValueType lhs = this->Get();
    lhs *= rhs.Get();
    this->Set(lhs);
    return lhs;
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  VISKORES_EXEC_CONT ValueType operator/=(const T& rhs) const
  {
    ValueType lhs = this->Get();
    lhs /= rhs;
    this->Set(lhs);
    return lhs;
  }
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  VISKORES_EXEC_CONT ValueType operator/=(const ArrayPortalValueReference<T>& rhs) const
  {
    ValueType lhs = this->Get();
    lhs /= rhs.Get();
    this->Set(lhs);
    return lhs;
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  VISKORES_EXEC_CONT ValueType operator%=(const T& rhs) const
  {
    ValueType lhs = this->Get();
    lhs %= rhs;
    this->Set(lhs);
    return lhs;
  }
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  VISKORES_EXEC_CONT ValueType operator%=(const ArrayPortalValueReference<T>& rhs) const
  {
    ValueType lhs = this->Get();
    lhs %= rhs.Get();
    this->Set(lhs);
    return lhs;
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  VISKORES_EXEC_CONT ValueType operator&=(const T& rhs) const
  {
    ValueType lhs = this->Get();
    lhs &= rhs;
    this->Set(lhs);
    return lhs;
  }
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  VISKORES_EXEC_CONT ValueType operator&=(const ArrayPortalValueReference<T>& rhs) const
  {
    ValueType lhs = this->Get();
    lhs &= rhs.Get();
    this->Set(lhs);
    return lhs;
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  VISKORES_EXEC_CONT ValueType operator|=(const T& rhs) const
  {
    ValueType lhs = this->Get();
    lhs |= rhs;
    this->Set(lhs);
    return lhs;
  }
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  VISKORES_EXEC_CONT ValueType operator|=(const ArrayPortalValueReference<T>& rhs) const
  {
    ValueType lhs = this->Get();
    lhs |= rhs.Get();
    this->Set(lhs);
    return lhs;
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  VISKORES_EXEC_CONT ValueType operator^=(const T& rhs) const
  {
    ValueType lhs = this->Get();
    lhs ^= rhs;
    this->Set(lhs);
    return lhs;
  }
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  VISKORES_EXEC_CONT ValueType operator^=(const ArrayPortalValueReference<T>& rhs) const
  {
    ValueType lhs = this->Get();
    lhs ^= rhs.Get();
    this->Set(lhs);
    return lhs;
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  VISKORES_EXEC_CONT ValueType operator>>=(const T& rhs) const
  {
    ValueType lhs = this->Get();
    lhs >>= rhs;
    this->Set(lhs);
    return lhs;
  }
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  VISKORES_EXEC_CONT ValueType operator>>=(const ArrayPortalValueReference<T>& rhs) const
  {
    ValueType lhs = this->Get();
    lhs >>= rhs.Get();
    this->Set(lhs);
    return lhs;
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  VISKORES_EXEC_CONT ValueType operator<<=(const T& rhs) const
  {
    ValueType lhs = this->Get();
    lhs <<= rhs;
    this->Set(lhs);
    return lhs;
  }
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  VISKORES_EXEC_CONT ValueType operator<<=(const ArrayPortalValueReference<T>& rhs) const
  {
    ValueType lhs = this->Get();
    lhs <<= rhs.Get();
    this->Set(lhs);
    return lhs;
  }

  // Support Vec operations so that the reference can be treated as such Vec objects. Note
  // that although the [] operator is supported, you can only read components this way. You
  // cannot write components one at a time.
  VISKORES_EXEC_CONT viskores::IdComponent GetNumberOfComponents() const
  {
    return viskores::VecTraits<ValueType>::GetNumberOfComponents(this->Get());
  }
  VISKORES_EXEC_CONT
  typename viskores::VecTraits<ValueType>::ComponentType operator[](
    viskores::IdComponent index) const
  {
    return viskores::VecTraits<ValueType>::GetComponent(this->Get(), index);
  }

private:
  const ArrayPortalType& Portal;
  viskores::Id Index;
};

//implement a custom swap function, since the std::swap won't work
//since we return RValues instead of Lvalues
template <typename T>
void swap(const viskores::internal::ArrayPortalValueReference<T>& a,
          const viskores::internal::ArrayPortalValueReference<T>& b)
{
  a.Swap(b);
}

template <typename T>
void swap(const viskores::internal::ArrayPortalValueReference<T>& a,
          typename viskores::internal::ArrayPortalValueReference<T>::ValueType& b)
{
  using ValueType = typename viskores::internal::ArrayPortalValueReference<T>::ValueType;
  const ValueType tmp = a;
  a = b;
  b = tmp;
}

template <typename T>
void swap(typename viskores::internal::ArrayPortalValueReference<T>::ValueType& a,
          const viskores::internal::ArrayPortalValueReference<T>& b)
{
  using ValueType = typename viskores::internal::ArrayPortalValueReference<T>::ValueType;
  const ValueType tmp = b;
  b = a;
  a = tmp;
}

// The reason why all the operators on ArrayPortalValueReference are defined outside of the class
// is so that in the case that the operator in question is not defined in the value type, these
// operators will not be instantiated (and therefore cause a compile error) unless they are
// directly used (in which case a compile error is appropriate).

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
VISKORES_EXEC_CONT auto operator==(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                   const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() == rhs)
{
  return lhs.Get() == rhs;
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
VISKORES_EXEC_CONT auto operator==(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                   const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() == rhs.Get())
{
  return lhs.Get() == rhs.Get();
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
VISKORES_EXEC_CONT auto operator==(const typename RhsPortalType::ValueType& lhs,
                                   const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs == rhs.Get())
{
  return lhs == rhs.Get();
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
VISKORES_EXEC_CONT auto operator!=(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                   const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() != rhs)
{
  return lhs.Get() != rhs;
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
VISKORES_EXEC_CONT auto operator!=(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                   const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() != rhs.Get())
{
  return lhs.Get() != rhs.Get();
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
VISKORES_EXEC_CONT auto operator!=(const typename RhsPortalType::ValueType& lhs,
                                   const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs != rhs.Get())
{
  return lhs != rhs.Get();
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
VISKORES_EXEC_CONT auto operator<(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                  const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() < rhs)
{
  return lhs.Get() < rhs;
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
VISKORES_EXEC_CONT auto operator<(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                  const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() < rhs.Get())
{
  return lhs.Get() < rhs.Get();
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
VISKORES_EXEC_CONT auto operator<(const typename RhsPortalType::ValueType& lhs,
                                  const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs < rhs.Get())
{
  return lhs < rhs.Get();
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
VISKORES_EXEC_CONT auto operator>(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                  const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() > rhs)
{
  return lhs.Get() > rhs;
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
VISKORES_EXEC_CONT auto operator>(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                  const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() > rhs.Get())
{
  return lhs.Get() > rhs.Get();
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
VISKORES_EXEC_CONT auto operator>(const typename RhsPortalType::ValueType& lhs,
                                  const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs > rhs.Get())
{
  return lhs > rhs.Get();
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
VISKORES_EXEC_CONT auto operator<=(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                   const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() <= rhs)
{
  return lhs.Get() <= rhs;
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
VISKORES_EXEC_CONT auto operator<=(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                   const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() <= rhs.Get())
{
  return lhs.Get() <= rhs.Get();
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
VISKORES_EXEC_CONT auto operator<=(const typename RhsPortalType::ValueType& lhs,
                                   const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs <= rhs.Get())
{
  return lhs <= rhs.Get();
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
VISKORES_EXEC_CONT auto operator>=(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                   const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() >= rhs)
{
  return lhs.Get() >= rhs;
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
VISKORES_EXEC_CONT auto operator>=(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                   const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() >= rhs.Get())
{
  return lhs.Get() >= rhs.Get();
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
VISKORES_EXEC_CONT auto operator>=(const typename RhsPortalType::ValueType& lhs,
                                   const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs >= rhs.Get())
{
  return lhs >= rhs.Get();
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
VISKORES_EXEC_CONT auto operator+(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                  const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() + rhs)
{
  return lhs.Get() + rhs;
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
VISKORES_EXEC_CONT auto operator+(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                  const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() + rhs.Get())
{
  return lhs.Get() + rhs.Get();
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
VISKORES_EXEC_CONT auto operator+(const typename RhsPortalType::ValueType& lhs,
                                  const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs + rhs.Get())
{
  return lhs + rhs.Get();
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
VISKORES_EXEC_CONT auto operator-(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                  const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() - rhs)
{
  return lhs.Get() - rhs;
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
VISKORES_EXEC_CONT auto operator-(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                  const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() - rhs.Get())
{
  return lhs.Get() - rhs.Get();
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
VISKORES_EXEC_CONT auto operator-(const typename RhsPortalType::ValueType& lhs,
                                  const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs - rhs.Get())
{
  return lhs - rhs.Get();
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
VISKORES_EXEC_CONT auto operator*(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                  const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() * rhs)
{
  return lhs.Get() * rhs;
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
VISKORES_EXEC_CONT auto operator*(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                  const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() * rhs.Get())
{
  return lhs.Get() * rhs.Get();
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
VISKORES_EXEC_CONT auto operator*(const typename RhsPortalType::ValueType& lhs,
                                  const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs * rhs.Get())
{
  return lhs * rhs.Get();
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
VISKORES_EXEC_CONT auto operator/(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                  const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() / rhs)
{
  return lhs.Get() / rhs;
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
VISKORES_EXEC_CONT auto operator/(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                  const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() / rhs.Get())
{
  return lhs.Get() / rhs.Get();
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
VISKORES_EXEC_CONT auto operator/(const typename RhsPortalType::ValueType& lhs,
                                  const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs / rhs.Get())
{
  return lhs / rhs.Get();
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
VISKORES_EXEC_CONT auto operator%(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                  const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() % rhs)
{
  return lhs.Get() % rhs;
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
VISKORES_EXEC_CONT auto operator%(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                  const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() % rhs.Get())
{
  return lhs.Get() % rhs.Get();
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
VISKORES_EXEC_CONT auto operator%(const typename RhsPortalType::ValueType& lhs,
                                  const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs % rhs.Get())
{
  return lhs % rhs.Get();
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
VISKORES_EXEC_CONT auto operator^(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                  const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() ^ rhs)
{
  return lhs.Get() ^ rhs;
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
VISKORES_EXEC_CONT auto operator^(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                  const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() ^ rhs.Get())
{
  return lhs.Get() ^ rhs.Get();
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
VISKORES_EXEC_CONT auto operator^(const typename RhsPortalType::ValueType& lhs,
                                  const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs ^ rhs.Get())
{
  return lhs ^ rhs.Get();
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
VISKORES_EXEC_CONT auto operator|(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                  const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() | rhs)
{
  return lhs.Get() | rhs;
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
VISKORES_EXEC_CONT auto operator|(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                  const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() | rhs.Get())
{
  return lhs.Get() | rhs.Get();
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
VISKORES_EXEC_CONT auto operator|(const typename RhsPortalType::ValueType& lhs,
                                  const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs | rhs.Get())
{
  return lhs | rhs.Get();
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
VISKORES_EXEC_CONT auto operator&(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                  const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() & rhs)
{
  return lhs.Get() & rhs;
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
VISKORES_EXEC_CONT auto operator&(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                  const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() & rhs.Get())
{
  return lhs.Get() & rhs.Get();
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
VISKORES_EXEC_CONT auto operator&(const typename RhsPortalType::ValueType& lhs,
                                  const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs & rhs.Get())
{
  return lhs & rhs.Get();
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
VISKORES_EXEC_CONT auto operator<<(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                   const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() << rhs)
{
  return lhs.Get() << rhs;
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
VISKORES_EXEC_CONT auto operator<<(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                   const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() << rhs.Get())
{
  return lhs.Get() << rhs.Get();
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
VISKORES_EXEC_CONT auto operator<<(const typename RhsPortalType::ValueType& lhs,
                                   const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs << rhs.Get())
{
  return lhs << rhs.Get();
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
VISKORES_EXEC_CONT auto operator>>(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                   const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() >> rhs)
{
  return lhs.Get() >> rhs;
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
VISKORES_EXEC_CONT auto operator>>(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                   const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() >> rhs.Get())
{
  return lhs.Get() >> rhs.Get();
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
VISKORES_EXEC_CONT auto operator>>(const typename RhsPortalType::ValueType& lhs,
                                   const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs >> rhs.Get())
{
  return lhs >> rhs.Get();
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename PortalType>
VISKORES_EXEC_CONT auto operator~(const ArrayPortalValueReference<PortalType>& ref)
  -> decltype(~ref.Get())
{
  return ~ref.Get();
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename PortalType>
VISKORES_EXEC_CONT auto operator!(const ArrayPortalValueReference<PortalType>& ref)
  -> decltype(!ref.Get())
{
  return !ref.Get();
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
VISKORES_EXEC_CONT auto operator&&(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                   const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() && rhs)
{
  return lhs.Get() && rhs;
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
VISKORES_EXEC_CONT auto operator&&(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                   const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() && rhs.Get())
{
  return lhs.Get() && rhs.Get();
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
VISKORES_EXEC_CONT auto operator&&(const typename RhsPortalType::ValueType& lhs,
                                   const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs && rhs.Get())
{
  return lhs && rhs.Get();
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType>
VISKORES_EXEC_CONT auto operator||(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                   const typename LhsPortalType::ValueType& rhs)
  -> decltype(lhs.Get() || rhs)
{
  return lhs.Get() || rhs;
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename LhsPortalType, typename RhsPortalType>
VISKORES_EXEC_CONT auto operator||(const ArrayPortalValueReference<LhsPortalType>& lhs,
                                   const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs.Get() || rhs.Get())
{
  return lhs.Get() || rhs.Get();
}
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename RhsPortalType>
VISKORES_EXEC_CONT auto operator||(const typename RhsPortalType::ValueType& lhs,
                                   const ArrayPortalValueReference<RhsPortalType>& rhs)
  -> decltype(lhs || rhs.Get())
{
  return lhs || rhs.Get();
}
}
} // namespace viskores::internal

namespace viskores
{

// Make specialization for TypeTraits and VecTraits so that the reference
// behaves the same as the value.

template <typename PortalType>
struct TypeTraits<viskores::internal::ArrayPortalValueReference<PortalType>>
  : viskores::TypeTraits<
      typename viskores::internal::ArrayPortalValueReference<PortalType>::ValueType>
{
};

template <typename PortalType>
struct VecTraits<viskores::internal::ArrayPortalValueReference<PortalType>>
  : viskores::VecTraits<
      typename viskores::internal::ArrayPortalValueReference<PortalType>::ValueType>
{
};

} // namespace viskores

#endif //viskores_internal_ArrayPortalValueReference_h
