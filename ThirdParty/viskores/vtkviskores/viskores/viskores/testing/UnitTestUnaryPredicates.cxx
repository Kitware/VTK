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

#include <viskores/UnaryPredicates.h>

#include <viskores/testing/Testing.h>

namespace
{

template <typename T>
void UnaryPredicateTest()
{
  //test IsZeroInitialized
  {
    viskores::IsZeroInitialized is_default;
    VISKORES_TEST_ASSERT(is_default(viskores::TypeTraits<T>::ZeroInitialization()) == true,
                         "IsZeroInitialized wrong.");
    VISKORES_TEST_ASSERT(is_default(TestValue(1, T())) == false, "IsZeroInitialized wrong.");
  }

  //test NotZeroInitialized
  {
    viskores::NotZeroInitialized not_default;
    VISKORES_TEST_ASSERT(not_default(viskores::TypeTraits<T>::ZeroInitialization()) == false,
                         "NotZeroInitialized wrong.");
    VISKORES_TEST_ASSERT(not_default(TestValue(1, T())) == true, "NotZeroInitialized wrong.");
  }
}

struct UnaryPredicateTestFunctor
{
  template <typename T>
  void operator()(const T&) const
  {
    UnaryPredicateTest<T>();
  }
};

void TestUnaryPredicates()
{
  viskores::testing::Testing::TryTypes(UnaryPredicateTestFunctor());

  //test LogicalNot
  {
    viskores::LogicalNot logical_not;
    VISKORES_TEST_ASSERT(logical_not(true) == false, "logical_not true wrong.");
    VISKORES_TEST_ASSERT(logical_not(false) == true, "logical_not false wrong.");
  }
}

} // anonymous namespace

int UnitTestUnaryPredicates(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestUnaryPredicates, argc, argv);
}
