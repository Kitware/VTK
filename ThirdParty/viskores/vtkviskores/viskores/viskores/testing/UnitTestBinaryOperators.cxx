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

#include <viskores/BinaryOperators.h>

#include <viskores/testing/Testing.h>

namespace
{

//general pair test
template <typename T>
void BinaryOperatorTest()
{

  //Not using TestValue method as it causes roll-over to occur with
  //uint8 and int8 leading to unexpected comparisons.

  //test Sum
  {
    viskores::Sum sum;
    T result;

    result = sum(viskores::TypeTraits<T>::ZeroInitialization(), T(1));
    VISKORES_TEST_ASSERT(result == T(1), "Sum wrong.");

    result = sum(T(1), T(1));
    VISKORES_TEST_ASSERT(result == T(2), "Sum wrong.");
  }

  //test Product
  {
    viskores::Product product;
    T result;

    result = product(viskores::TypeTraits<T>::ZeroInitialization(), T(1));
    VISKORES_TEST_ASSERT(result == viskores::TypeTraits<T>::ZeroInitialization(), "Product wrong.");

    result = product(T(1), T(1));
    VISKORES_TEST_ASSERT(result == T(1), "Product wrong.");

    result = product(T(2), T(3));
    VISKORES_TEST_ASSERT(result == T(6), "Product wrong.");
  }

  //test Maximum
  {
    viskores::Maximum maximum;
    VISKORES_TEST_ASSERT(maximum(T(1), T(2)) == T(2), "Maximum wrong.");
    VISKORES_TEST_ASSERT(maximum(T(2), T(2)) == T(2), "Maximum wrong.");
    VISKORES_TEST_ASSERT(maximum(T(2), T(1)) == T(2), "Maximum wrong.");
  }

  //test Minimum
  {
    viskores::Minimum minimum;
    VISKORES_TEST_ASSERT(minimum(T(1), T(2)) == T(1), "Minimum wrong.");
    VISKORES_TEST_ASSERT(minimum(T(1), T(1)) == T(1), "Minimum wrong.");
    VISKORES_TEST_ASSERT(minimum(T(3), T(2)) == T(2), "Minimum wrong.");
  }

  //test MinAndMax
  {
    viskores::MinAndMax<T> min_and_max;
    viskores::Vec<T, 2> result;

    // Test1: basic param
    {
      result = min_and_max(T(1));
      VISKORES_TEST_ASSERT(test_equal(result, viskores::Vec<T, 2>(T(1), T(1))),
                           "Test1 MinAndMax wrong");
    }

    // Test2: basic param
    {
      result = min_and_max(viskores::TypeTraits<T>::ZeroInitialization(), T(1));
      VISKORES_TEST_ASSERT(
        test_equal(result,
                   viskores::Vec<T, 2>(viskores::TypeTraits<T>::ZeroInitialization(), T(1))),
        "Test2 MinAndMax wrong");

      result = min_and_max(T(2), T(1));
      VISKORES_TEST_ASSERT(test_equal(result, viskores::Vec<T, 2>(T(1), T(2))),
                           "Test2 MinAndMax wrong");
    }

    // Test3: 1st param vector, 2nd param basic
    {
      result = min_and_max(viskores::Vec<T, 2>(3, 5), T(7));
      VISKORES_TEST_ASSERT(test_equal(result, viskores::Vec<T, 2>(T(3), T(7))),
                           "Test3 MinAndMax Wrong");

      result = min_and_max(viskores::Vec<T, 2>(3, 5), T(2));
      VISKORES_TEST_ASSERT(test_equal(result, viskores::Vec<T, 2>(T(2), T(5))),
                           "Test3 MinAndMax Wrong");
    }

    // Test4: 1st param basic, 2nd param vector
    {
      result = min_and_max(T(7), viskores::Vec<T, 2>(3, 5));
      VISKORES_TEST_ASSERT(test_equal(result, viskores::Vec<T, 2>(T(3), T(7))),
                           "Test4 MinAndMax Wrong");

      result = min_and_max(T(2), viskores::Vec<T, 2>(3, 5));
      VISKORES_TEST_ASSERT(test_equal(result, viskores::Vec<T, 2>(T(2), T(5))),
                           "Test4 MinAndMax Wrong");
    }

    // Test5: 2 vector param
    {
      result = min_and_max(viskores::Vec<T, 2>(2, 4), viskores::Vec<T, 2>(3, 5));
      VISKORES_TEST_ASSERT(test_equal(result, viskores::Vec<T, 2>(T(2), T(5))),
                           "Test5 MinAndMax Wrong");

      result = min_and_max(viskores::Vec<T, 2>(2, 7), viskores::Vec<T, 2>(3, 5));
      VISKORES_TEST_ASSERT(test_equal(result, viskores::Vec<T, 2>(T(2), T(7))),
                           "Test5 MinAndMax Wrong");

      result = min_and_max(viskores::Vec<T, 2>(4, 4), viskores::Vec<T, 2>(1, 8));
      VISKORES_TEST_ASSERT(test_equal(result, viskores::Vec<T, 2>(T(1), T(8))),
                           "Test5 MinAndMax Wrong");

      result = min_and_max(viskores::Vec<T, 2>(4, 4), viskores::Vec<T, 2>(3, 3));
      VISKORES_TEST_ASSERT(test_equal(result, viskores::Vec<T, 2>(T(3), T(4))),
                           "Test5 MinAndMax Wrong");
    }
  }
}

struct BinaryOperatorTestFunctor
{
  template <typename T>
  void operator()(const T&) const
  {
    BinaryOperatorTest<T>();
  }
};

void TestBinaryOperators()
{
  viskores::testing::Testing::TryTypes(BinaryOperatorTestFunctor());

  viskores::UInt32 v1 = 0xccccccccu;
  viskores::UInt32 v2 = 0xffffffffu;
  viskores::UInt32 v3 = 0x0u;

  //test BitwiseAnd
  {
    viskores::BitwiseAnd bitwise_and;
    VISKORES_TEST_ASSERT(bitwise_and(v1, v2) == (v1 & v2), "bitwise_and wrong.");
    VISKORES_TEST_ASSERT(bitwise_and(v1, v3) == (v1 & v3), "bitwise_and wrong.");
    VISKORES_TEST_ASSERT(bitwise_and(v2, v3) == (v2 & v3), "bitwise_and wrong.");
  }

  //test BitwiseOr
  {
    viskores::BitwiseOr bitwise_or;
    VISKORES_TEST_ASSERT(bitwise_or(v1, v2) == (v1 | v2), "bitwise_or wrong.");
    VISKORES_TEST_ASSERT(bitwise_or(v1, v3) == (v1 | v3), "bitwise_or wrong.");
    VISKORES_TEST_ASSERT(bitwise_or(v2, v3) == (v2 | v3), "bitwise_or wrong.");
  }

  //test BitwiseXor
  {
    viskores::BitwiseXor bitwise_xor;
    VISKORES_TEST_ASSERT(bitwise_xor(v1, v2) == (v1 ^ v2), "bitwise_xor wrong.");
    VISKORES_TEST_ASSERT(bitwise_xor(v1, v3) == (v1 ^ v3), "bitwise_xor wrong.");
    VISKORES_TEST_ASSERT(bitwise_xor(v2, v3) == (v2 ^ v3), "bitwise_xor wrong.");
  }
}

} // anonymous namespace

int UnitTestBinaryOperators(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestBinaryOperators, argc, argv);
}
