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

#include <viskores/BinaryPredicates.h>

#include <viskores/testing/Testing.h>

namespace
{

//general pair test
template <typename T>
void BinaryPredicateTest()
{
  //Not using TestValue method as it causes roll-over to occur with
  //uint8 and int8 leading to unexpected comparisons.

  //test Equal
  {
    viskores::Equal is_equal;
    VISKORES_TEST_ASSERT(is_equal(viskores::TypeTraits<T>::ZeroInitialization(),
                                  viskores::TypeTraits<T>::ZeroInitialization()),
                         "Equal wrong.");
    VISKORES_TEST_ASSERT(is_equal(T(1), T(2)) == false, "Equal wrong.");
  }

  //test NotEqual
  {
    viskores::NotEqual not_equal;
    VISKORES_TEST_ASSERT(not_equal(viskores::TypeTraits<T>::ZeroInitialization(), T(1)),
                         "NotEqual wrong.");
    VISKORES_TEST_ASSERT(not_equal(T(1), T(1)) == false, "NotEqual wrong.");
  }

  //test SortLess
  {
    viskores::SortLess sort_less;
    VISKORES_TEST_ASSERT(sort_less(T(1), T(2)) == true, "SortLess wrong.");
    VISKORES_TEST_ASSERT(sort_less(T(2), T(2)) == false, "SortLess wrong.");
    VISKORES_TEST_ASSERT(sort_less(T(2), T(1)) == false, "SortLess wrong.");
  }

  //test SortGreater
  {
    viskores::SortGreater sort_greater;
    VISKORES_TEST_ASSERT(sort_greater(T(1), T(2)) == false, "SortGreater wrong.");
    VISKORES_TEST_ASSERT(sort_greater(T(1), T(1)) == false, "SortGreater wrong.");
    VISKORES_TEST_ASSERT(sort_greater(T(3), T(2)) == true, "SortGreater wrong.");
  }
}

struct BinaryPredicateTestFunctor
{
  template <typename T>
  void operator()(const T&) const
  {
    BinaryPredicateTest<T>();
  }
};

void TestBinaryPredicates()
{
  viskores::testing::Testing::TryTypes(BinaryPredicateTestFunctor());

  //test LogicalAnd
  {
    viskores::LogicalAnd logical_and;
    VISKORES_TEST_ASSERT(logical_and(true, true) == true, "logical_and true wrong.");
    VISKORES_TEST_ASSERT(logical_and(true, false) == false, "logical_and true wrong.");
    VISKORES_TEST_ASSERT(logical_and(false, true) == false, "logical_and true wrong.");
    VISKORES_TEST_ASSERT(logical_and(false, false) == false, "logical_and true wrong.");
  }

  //test LogicalOr
  {
    viskores::LogicalOr logical_or;
    VISKORES_TEST_ASSERT(logical_or(true, true) == true, "logical_or true wrong.");
    VISKORES_TEST_ASSERT(logical_or(true, false) == true, "logical_or true wrong.");
    VISKORES_TEST_ASSERT(logical_or(false, true) == true, "logical_or true wrong.");
    VISKORES_TEST_ASSERT(logical_or(false, false) == false, "logical_or true wrong.");
  }
}

} // anonymous namespace

int UnitTestBinaryPredicates(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestBinaryPredicates, argc, argv);
}
