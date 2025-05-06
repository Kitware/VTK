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
#ifndef viskores_cont_testing_TestingArrayHandleMultiplexer_h
#define viskores_cont_testing_TestingArrayHandleMultiplexer_h

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleImplicit.h>
#include <viskores/cont/ArrayHandleMultiplexer.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>

#include <viskores/BinaryOperators.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

template <typename T>
struct TestValueFunctor
{
  VISKORES_EXEC_CONT T operator()(viskores::Id index) const { return TestValue(index, T()); }
};

constexpr viskores::Id ARRAY_SIZE = 10;

template <typename... Ts0, typename... Ts1>
static void CheckArray(const viskores::cont::ArrayHandleMultiplexer<Ts0...>& multiplexerArray,
                       const viskores::cont::ArrayHandle<Ts1...>& expectedArray)
{
  using T = typename std::remove_reference<decltype(multiplexerArray)>::type::ValueType;

  viskores::cont::printSummary_ArrayHandle(multiplexerArray, std::cout);
  VISKORES_TEST_ASSERT(
    test_equal_portals(multiplexerArray.ReadPortal(), expectedArray.ReadPortal()),
    "Multiplexer array gave wrong result in control environment");

  viskores::cont::ArrayHandle<T> copy;
  viskores::cont::Algorithm::Copy(multiplexerArray, copy);
  VISKORES_TEST_ASSERT(test_equal_portals(copy.ReadPortal(), expectedArray.ReadPortal()),
                       "Multiplexer did not copy correctly in execution environment");
}

void BasicSwitch()
{
  std::cout << "\n--- Basic switch" << std::endl;

  using ValueType = viskores::FloatDefault;

  using ArrayType1 = viskores::cont::ArrayHandleConstant<ValueType>;
  ArrayType1 array1(TestValue(0, viskores::FloatDefault{}), ARRAY_SIZE);

  using ArrayType2 = viskores::cont::ArrayHandleCounting<ValueType>;
  ArrayType2 array2(TestValue(1, viskores::FloatDefault{}), 1.0f, ARRAY_SIZE);

  auto array3 = viskores::cont::make_ArrayHandleImplicit(TestValueFunctor<ValueType>{}, ARRAY_SIZE);
  using ArrayType3 = decltype(array3);

  viskores::cont::ArrayHandleMultiplexer<ArrayType1, ArrayType2, ArrayType3> multiplexer;

  std::cout << "Check array1" << std::endl;
  multiplexer = array1;
  CheckArray(multiplexer, array1);

  std::cout << "Check array2" << std::endl;
  multiplexer = array2;
  CheckArray(multiplexer, array2);

  std::cout << "Check array3" << std::endl;
  multiplexer = array3;
  CheckArray(multiplexer, array3);
}

void Reduce()
{
  // Regression test for an issue with compiling ArrayHandleMultiplexer with the thrust reduce
  // algorithm on CUDA. Most likely related to:
  // https://github.com/thrust/thrust/issues/928
  // https://github.com/thrust/thrust/issues/1044
  std::cout << "\n--- Reduce" << std::endl;

  using ValueType = viskores::Vec3f;
  using MultiplexerType =
    viskores::cont::ArrayHandleMultiplexer<viskores::cont::ArrayHandleConstant<ValueType>,
                                           viskores::cont::ArrayHandleCounting<ValueType>,
                                           viskores::cont::ArrayHandle<ValueType>,
                                           viskores::cont::ArrayHandleUniformPointCoordinates,
                                           viskores::cont::ArrayHandleCartesianProduct<
                                             viskores::cont::ArrayHandle<viskores::FloatDefault>,
                                             viskores::cont::ArrayHandle<viskores::FloatDefault>,
                                             viskores::cont::ArrayHandle<viskores::FloatDefault>>>;

  MultiplexerType multiplexer = viskores::cont::ArrayHandleCounting<ValueType>(
    viskores::Vec3f(1), viskores::Vec3f(1), ARRAY_SIZE);

  {
    std::cout << "Basic Reduce" << std::endl;
    ValueType result = viskores::cont::Algorithm::Reduce(multiplexer, ValueType(0.0));
    VISKORES_TEST_ASSERT(test_equal(result, ValueType(0.5 * (ARRAY_SIZE * (ARRAY_SIZE + 1)))));
  }

  {
    std::cout << "Reduce with custom operator" << std::endl;
    viskores::Vec<ValueType, 2> initial(ValueType(10000), ValueType(0));
    viskores::Vec<ValueType, 2> result =
      viskores::cont::Algorithm::Reduce(multiplexer, initial, viskores::MinAndMax<ValueType>{});
    VISKORES_TEST_ASSERT(test_equal(result[0], ValueType(1)));
    VISKORES_TEST_ASSERT(
      test_equal(result[1], ValueType(static_cast<viskores::FloatDefault>(ARRAY_SIZE))));
  }
}

void Fill()
{
  std::cout << "\n--- Fill" << std::endl;

  using ValueType = viskores::Vec3f;
  using MultiplexerType =
    viskores::cont::ArrayHandleMultiplexer<viskores::cont::ArrayHandleConstant<ValueType>,
                                           viskores::cont::ArrayHandleCounting<ValueType>,
                                           viskores::cont::ArrayHandle<ValueType>,
                                           viskores::cont::ArrayHandleUniformPointCoordinates,
                                           viskores::cont::ArrayHandleCartesianProduct<
                                             viskores::cont::ArrayHandle<viskores::FloatDefault>,
                                             viskores::cont::ArrayHandle<viskores::FloatDefault>,
                                             viskores::cont::ArrayHandle<viskores::FloatDefault>>>;

  const ValueType testValue1 = TestValue(1, ValueType{});
  const ValueType testValue2 = TestValue(2, ValueType{});

  MultiplexerType multiplexer = viskores::cont::ArrayHandle<ValueType>{};

  multiplexer.AllocateAndFill(ARRAY_SIZE, testValue1);
  VISKORES_TEST_ASSERT(multiplexer.GetNumberOfComponentsFlat() ==
                       viskores::VecFlat<ValueType>::NUM_COMPONENTS);
  {
    auto portal = multiplexer.ReadPortal();
    VISKORES_TEST_ASSERT(portal.GetNumberOfValues() == ARRAY_SIZE);
    for (viskores::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      VISKORES_TEST_ASSERT(portal.Get(index) == testValue1);
    }
  }

  viskores::cont::ArrayHandle<viskores::FloatDefault> array1;
  array1.Allocate(ARRAY_SIZE);
  viskores::cont::ArrayHandle<viskores::FloatDefault> array2;
  array2.Allocate(ARRAY_SIZE);
  viskores::cont::ArrayHandle<viskores::FloatDefault> array3;
  array3.Allocate(ARRAY_SIZE);
  multiplexer = viskores::cont::make_ArrayHandleCartesianProduct(array1, array2, array3);

  multiplexer.Fill(testValue2);
  {
    auto portal1 = array1.ReadPortal();
    auto portal2 = array2.ReadPortal();
    auto portal3 = array3.ReadPortal();
    for (viskores::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      VISKORES_TEST_ASSERT(portal1.Get(index) == testValue2[0]);
      VISKORES_TEST_ASSERT(portal2.Get(index) == testValue2[1]);
      VISKORES_TEST_ASSERT(portal3.Get(index) == testValue2[2]);
    }
  }
}

void TestAll()
{
  BasicSwitch();
  Reduce();
  Fill();
}

} // anonymous namespace

int UnitTestArrayHandleMultiplexer(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestAll, argc, argv);
}

#endif //viskores_cont_testing_TestingArrayHandleMultiplexer_h
