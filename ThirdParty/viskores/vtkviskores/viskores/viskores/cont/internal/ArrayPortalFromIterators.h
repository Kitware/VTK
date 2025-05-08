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
#ifndef viskores_cont_internal_ArrayPortalFromIterators_h
#define viskores_cont_internal_ArrayPortalFromIterators_h

#include <viskores/Assert.h>
#include <viskores/Types.h>
#include <viskores/cont/ArrayPortal.h>
#include <viskores/cont/ArrayPortalToIterators.h>
#include <viskores/cont/ErrorBadAllocation.h>

#include <iterator>
#include <limits>

#include <type_traits>

namespace viskores
{
namespace cont
{
namespace internal
{

template <typename IteratorT, typename Enable = void>
class ArrayPortalFromIterators;

/// This templated implementation of an ArrayPortal allows you to adapt a pair
/// of begin/end iterators to an ArrayPortal interface.
///
template <class IteratorT>
class ArrayPortalFromIterators<IteratorT,
                               typename std::enable_if<!std::is_const<
                                 typename std::remove_pointer<IteratorT>::type>::value>::type>
{
public:
  using ValueType = typename std::iterator_traits<IteratorT>::value_type;
  using IteratorType = IteratorT;

  ArrayPortalFromIterators() = default;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_CONT
  ArrayPortalFromIterators(IteratorT begin, IteratorT end)
    : BeginIterator(begin)
  {
    typename std::iterator_traits<IteratorT>::difference_type numberOfValues =
      std::distance(begin, end);
    VISKORES_ASSERT(numberOfValues >= 0);
#ifndef VISKORES_USE_64BIT_IDS
    if (numberOfValues > (std::numeric_limits<viskores::Id>::max)())
    {
      throw viskores::cont::ErrorBadAllocation(
        "Distance of iterators larger than maximum array size. "
        "To support larger arrays, try turning on VISKORES_USE_64BIT_IDS.");
    }
#endif // !VISKORES_USE_64BIT_IDS
    this->NumberOfValues = static_cast<viskores::Id>(numberOfValues);
  }

  /// Copy constructor for any other ArrayPortalFromIterators with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template <class OtherIteratorT>
  VISKORES_EXEC_CONT ArrayPortalFromIterators(const ArrayPortalFromIterators<OtherIteratorT>& src)
    : BeginIterator(src.GetIteratorBegin())
    , NumberOfValues(src.GetNumberOfValues())
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id index) const { return *this->IteratorAt(index); }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  void Set(viskores::Id index, const ValueType& value) const
  {
    *(this->BeginIterator + index) = value;
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  IteratorT GetIteratorBegin() const { return this->BeginIterator; }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  IteratorT GetIteratorEnd() const
  {
    IteratorType iterator = this->BeginIterator;
    using difference_type = typename std::iterator_traits<IteratorType>::difference_type;
    iterator += static_cast<difference_type>(this->NumberOfValues);
    return iterator;
  }

private:
  IteratorT BeginIterator;
  viskores::Id NumberOfValues;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  IteratorT IteratorAt(viskores::Id index) const
  {
    VISKORES_ASSERT(index >= 0);
    VISKORES_ASSERT(index < this->GetNumberOfValues());

    return this->BeginIterator + index;
  }
};

template <class IteratorT>
class ArrayPortalFromIterators<IteratorT,
                               typename std::enable_if<std::is_const<
                                 typename std::remove_pointer<IteratorT>::type>::value>::type>
{
public:
  using ValueType = typename std::iterator_traits<IteratorT>::value_type;
  using IteratorType = IteratorT;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ArrayPortalFromIterators()
    : BeginIterator(nullptr)
    , NumberOfValues(0)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_CONT
  ArrayPortalFromIterators(IteratorT begin, IteratorT end)
    : BeginIterator(begin)
  {
    typename std::iterator_traits<IteratorT>::difference_type numberOfValues =
      std::distance(begin, end);
    VISKORES_ASSERT(numberOfValues >= 0);
#ifndef VISKORES_USE_64BIT_IDS
    if (numberOfValues > (std::numeric_limits<viskores::Id>::max)())
    {
      throw viskores::cont::ErrorBadAllocation(
        "Distance of iterators larger than maximum array size. "
        "To support larger arrays, try turning on VISKORES_USE_64BIT_IDS.");
    }
#endif // !VISKORES_USE_64BIT_IDS
    this->NumberOfValues = static_cast<viskores::Id>(numberOfValues);
  }

  /// Copy constructor for any other ArrayPortalFromIterators with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <class OtherIteratorT>
  VISKORES_EXEC_CONT ArrayPortalFromIterators(const ArrayPortalFromIterators<OtherIteratorT>& src)
    : BeginIterator(src.GetIteratorBegin())
    , NumberOfValues(src.GetNumberOfValues())
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id index) const { return *this->IteratorAt(index); }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  IteratorT GetIteratorBegin() const { return this->BeginIterator; }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  IteratorT GetIteratorEnd() const
  {
    using difference_type = typename std::iterator_traits<IteratorType>::difference_type;
    IteratorType iterator = this->BeginIterator;
    iterator += static_cast<difference_type>(this->NumberOfValues);
    return iterator;
  }

private:
  IteratorT BeginIterator;
  viskores::Id NumberOfValues;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  IteratorT IteratorAt(viskores::Id index) const
  {
    VISKORES_ASSERT(index >= 0);
    VISKORES_ASSERT(index < this->GetNumberOfValues());

    return this->BeginIterator + index;
  }
};
}
}
} // namespace viskores::cont::internal

#endif //viskores_cont_internal_ArrayPortalFromIterators_h
