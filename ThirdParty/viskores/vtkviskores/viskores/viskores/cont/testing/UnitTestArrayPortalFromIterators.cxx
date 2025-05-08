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

#include <viskores/cont/internal/ArrayPortalFromIterators.h>

#include <viskores/VecTraits.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

template <typename T>
struct TemplatedTests
{
  static constexpr viskores::Id ARRAY_SIZE = 10;

  using ValueType = T;
  using ComponentType = typename viskores::VecTraits<ValueType>::ComponentType;

  ValueType ExpectedValue(viskores::Id index, ComponentType value)
  {
    return ValueType(static_cast<ComponentType>(index + static_cast<viskores::Id>(value)));
  }

  template <class IteratorType>
  void FillIterator(IteratorType begin, IteratorType end, ComponentType value)
  {
    viskores::Id index = 0;
    for (IteratorType iter = begin; iter != end; iter++)
    {
      *iter = ExpectedValue(index, value);
      index++;
    }
  }

  template <class IteratorType>
  bool CheckIterator(IteratorType begin, IteratorType end, ComponentType value)
  {
    viskores::Id index = 0;
    for (IteratorType iter = begin; iter != end; iter++)
    {
      if (*iter != ExpectedValue(index, value))
      {
        return false;
      }
      index++;
    }
    return true;
  }

  template <class PortalType>
  bool CheckPortal(const PortalType& portal, ComponentType value)
  {
    viskores::cont::ArrayPortalToIterators<PortalType> iterators(portal);
    return CheckIterator(iterators.GetBegin(), iterators.GetEnd(), value);
  }

  void operator()()
  {
    ValueType array[ARRAY_SIZE];

    constexpr ComponentType ORIGINAL_VALUE = 109;
    FillIterator(array, array + ARRAY_SIZE, ORIGINAL_VALUE);

    ::viskores::cont::internal::ArrayPortalFromIterators<ValueType*> portal(array,
                                                                            array + ARRAY_SIZE);
    ::viskores::cont::internal::ArrayPortalFromIterators<const ValueType*> const_portal(
      array, array + ARRAY_SIZE);

    using PortalType = decltype(portal);
    using PortalConstType = decltype(const_portal);

    std::cout << "Check that PortalSupports* results are valid:" << std::endl;
    VISKORES_TEST_ASSERT(viskores::internal::PortalSupportsSets<PortalType>::value,
                         "Writable portals should support Set operations");
    VISKORES_TEST_ASSERT(viskores::internal::PortalSupportsGets<PortalType>::value,
                         "Writable portals should support Get operations");
    VISKORES_TEST_ASSERT(!viskores::internal::PortalSupportsSets<PortalConstType>::value,
                         "Read-only portals should not allow Set operations");
    VISKORES_TEST_ASSERT(viskores::internal::PortalSupportsGets<PortalConstType>::value,
                         "Read-only portals should support Get operations");

    std::cout << "  Check that ArrayPortalToIterators is not doing indirection." << std::endl;
    // If you get a compile error here about mismatched types, it might be
    // that ArrayPortalToIterators is not properly overloaded to return the
    // original iterator.
    VISKORES_TEST_ASSERT(viskores::cont::ArrayPortalToIteratorBegin(portal) == array,
                         "Begin iterator wrong.");
    VISKORES_TEST_ASSERT(viskores::cont::ArrayPortalToIteratorEnd(portal) == array + ARRAY_SIZE,
                         "End iterator wrong.");
    VISKORES_TEST_ASSERT(viskores::cont::ArrayPortalToIteratorBegin(const_portal) == array,
                         "Begin const iterator wrong.");
    VISKORES_TEST_ASSERT(viskores::cont::ArrayPortalToIteratorEnd(const_portal) ==
                           array + ARRAY_SIZE,
                         "End const iterator wrong.");

    VISKORES_TEST_ASSERT(portal.GetNumberOfValues() == ARRAY_SIZE, "Portal array size wrong.");
    VISKORES_TEST_ASSERT(const_portal.GetNumberOfValues() == ARRAY_SIZE,
                         "Const portal array size wrong.");

    std::cout << "  Check initial value." << std::endl;
    VISKORES_TEST_ASSERT(CheckPortal(portal, ORIGINAL_VALUE), "Portal iterator has bad value.");
    VISKORES_TEST_ASSERT(CheckPortal(const_portal, ORIGINAL_VALUE),
                         "Const portal iterator has bad value.");

    constexpr ComponentType SET_VALUE = 62;

    std::cout << "  Check get/set methods." << std::endl;
    for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
    {
      VISKORES_TEST_ASSERT(portal.Get(index) == ExpectedValue(index, ORIGINAL_VALUE),
                           "Bad portal value.");
      VISKORES_TEST_ASSERT(const_portal.Get(index) == ExpectedValue(index, ORIGINAL_VALUE),
                           "Bad const portal value.");

      portal.Set(index, ExpectedValue(index, SET_VALUE));
    }

    std::cout << "  Make sure set has correct value." << std::endl;
    VISKORES_TEST_ASSERT(CheckPortal(portal, SET_VALUE), "Portal iterator has bad value.");
    VISKORES_TEST_ASSERT(CheckIterator(array, array + ARRAY_SIZE, SET_VALUE),
                         "Array has bad value.");
  }
};

struct TestFunctor
{
  template <typename T>
  void operator()(T) const
  {
    TemplatedTests<T> tests;
    tests();
  }
};

void TestArrayPortalFromIterators()
{
  viskores::testing::Testing::TryTypes(TestFunctor());
}

} // Anonymous namespace

int UnitTestArrayPortalFromIterators(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestArrayPortalFromIterators, argc, argv);
}
