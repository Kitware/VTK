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
#ifndef viskores_cont_internal_IteratorFromArrayPortal_h
#define viskores_cont_internal_IteratorFromArrayPortal_h

#include <viskores/Assert.h>
#include <viskores/cont/ArrayPortal.h>
#include <viskores/internal/ArrayPortalValueReference.h>

namespace viskores
{
namespace cont
{
namespace internal
{

template <class ArrayPortalType>
class IteratorFromArrayPortal
{
public:
  using value_type = typename std::remove_const<typename ArrayPortalType::ValueType>::type;
  using reference = viskores::internal::ArrayPortalValueReference<ArrayPortalType>;
  using pointer = typename std::add_pointer<value_type>::type;

  using difference_type = std::ptrdiff_t;

  using iterator_category = std::random_access_iterator_tag;

  using iter = IteratorFromArrayPortal<ArrayPortalType>;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  IteratorFromArrayPortal()
    : Portal()
    , Index(0)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  explicit IteratorFromArrayPortal(const ArrayPortalType& portal, viskores::Id index = 0)
    : Portal(portal)
    , Index(index)
  {
    VISKORES_ASSERT(index >= 0);
    VISKORES_ASSERT(index <= portal.GetNumberOfValues());
  }

  VISKORES_EXEC_CONT
  reference operator*() const { return reference(this->Portal, this->Index); }

  VISKORES_EXEC_CONT
  reference operator->() const { return reference(this->Portal, this->Index); }

  VISKORES_EXEC_CONT
  reference operator[](difference_type idx) const
  {
    return reference(this->Portal, this->Index + static_cast<viskores::Id>(idx));
  }

  VISKORES_EXEC_CONT
  iter& operator++()
  {
    this->Index++;
    VISKORES_ASSERT(this->Index <= this->Portal.GetNumberOfValues());
    return *this;
  }

  VISKORES_EXEC_CONT
  iter operator++(int) { return iter(this->Portal, this->Index++); }

  VISKORES_EXEC_CONT
  iter& operator--()
  {
    this->Index--;
    VISKORES_ASSERT(this->Index >= 0);
    return *this;
  }

  VISKORES_EXEC_CONT
  iter operator--(int) { return iter(this->Portal, this->Index--); }

  VISKORES_EXEC_CONT
  iter& operator+=(difference_type n)
  {
    this->Index += static_cast<viskores::Id>(n);
    VISKORES_ASSERT(this->Index <= this->Portal.GetNumberOfValues());
    return *this;
  }

  VISKORES_EXEC_CONT
  iter& operator-=(difference_type n)
  {
    this->Index -= static_cast<viskores::Id>(n);
    VISKORES_ASSERT(this->Index >= 0);
    return *this;
  }

  VISKORES_EXEC_CONT
  iter operator-(difference_type n) const
  {
    return iter(this->Portal, this->Index - static_cast<viskores::Id>(n));
  }

  ArrayPortalType Portal;
  viskores::Id Index;
};

template <class ArrayPortalType>
VISKORES_EXEC_CONT IteratorFromArrayPortal<ArrayPortalType> make_IteratorBegin(
  const ArrayPortalType& portal)
{
  return IteratorFromArrayPortal<ArrayPortalType>(portal, 0);
}

template <class ArrayPortalType>
VISKORES_EXEC_CONT IteratorFromArrayPortal<ArrayPortalType> make_IteratorEnd(
  const ArrayPortalType& portal)
{
  return IteratorFromArrayPortal<ArrayPortalType>(portal, portal.GetNumberOfValues());
}

template <typename PortalType>
VISKORES_EXEC_CONT bool operator==(
  viskores::cont::internal::IteratorFromArrayPortal<PortalType> const& lhs,
  viskores::cont::internal::IteratorFromArrayPortal<PortalType> const& rhs)
{
  return lhs.Index == rhs.Index;
}

template <typename PortalType>
VISKORES_EXEC_CONT bool operator!=(
  viskores::cont::internal::IteratorFromArrayPortal<PortalType> const& lhs,
  viskores::cont::internal::IteratorFromArrayPortal<PortalType> const& rhs)
{
  return lhs.Index != rhs.Index;
}

template <typename PortalType>
VISKORES_EXEC_CONT bool operator<(
  viskores::cont::internal::IteratorFromArrayPortal<PortalType> const& lhs,
  viskores::cont::internal::IteratorFromArrayPortal<PortalType> const& rhs)
{
  return lhs.Index < rhs.Index;
}

template <typename PortalType>
VISKORES_EXEC_CONT bool operator<=(
  viskores::cont::internal::IteratorFromArrayPortal<PortalType> const& lhs,
  viskores::cont::internal::IteratorFromArrayPortal<PortalType> const& rhs)
{
  return lhs.Index <= rhs.Index;
}

template <typename PortalType>
VISKORES_EXEC_CONT bool operator>(
  viskores::cont::internal::IteratorFromArrayPortal<PortalType> const& lhs,
  viskores::cont::internal::IteratorFromArrayPortal<PortalType> const& rhs)
{
  return lhs.Index > rhs.Index;
}

template <typename PortalType>
VISKORES_EXEC_CONT bool operator>=(
  viskores::cont::internal::IteratorFromArrayPortal<PortalType> const& lhs,
  viskores::cont::internal::IteratorFromArrayPortal<PortalType> const& rhs)
{
  return lhs.Index >= rhs.Index;
}

template <typename PortalType>
VISKORES_EXEC_CONT std::ptrdiff_t operator-(
  viskores::cont::internal::IteratorFromArrayPortal<PortalType> const& lhs,
  viskores::cont::internal::IteratorFromArrayPortal<PortalType> const& rhs)
{
  return lhs.Index - rhs.Index;
}

template <typename PortalType>
VISKORES_EXEC_CONT viskores::cont::internal::IteratorFromArrayPortal<PortalType> operator+(
  viskores::cont::internal::IteratorFromArrayPortal<PortalType> const& iter,
  std::ptrdiff_t n)
{
  return viskores::cont::internal::IteratorFromArrayPortal<PortalType>(
    iter.Portal, iter.Index + static_cast<viskores::Id>(n));
}

template <typename PortalType>
VISKORES_EXEC_CONT viskores::cont::internal::IteratorFromArrayPortal<PortalType> operator+(
  std::ptrdiff_t n,
  viskores::cont::internal::IteratorFromArrayPortal<PortalType> const& iter)
{
  return viskores::cont::internal::IteratorFromArrayPortal<PortalType>(
    iter.Portal, iter.Index + static_cast<viskores::Id>(n));
}
}
}
} // namespace viskores::cont::internal

#endif //viskores_cont_internal_IteratorFromArrayPortal_h
