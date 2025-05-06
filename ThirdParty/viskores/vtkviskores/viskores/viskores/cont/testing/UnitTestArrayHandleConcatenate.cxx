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

#include <viskores/cont/ArrayHandleConcatenate.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/Invoker.h>

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

constexpr viskores::Id ARRAY_SIZE = 10;

template <typename ValueType>
struct IndexSquared
{
  VISKORES_EXEC_CONT
  ValueType operator()(viskores::Id index) const
  {
    using ComponentType = typename viskores::VecTraits<ValueType>::ComponentType;
    return ValueType(static_cast<ComponentType>(index * index));
  }
};

struct PassThrough : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename InValue, typename OutValue>
  VISKORES_EXEC void operator()(const InValue& inValue, OutValue& outValue) const
  {
    outValue = inValue;
  }
};

VISKORES_CONT void TestConcatInvoke()
{
  using ValueType = viskores::Id;
  using FunctorType = IndexSquared<ValueType>;

  using ValueHandleType = viskores::cont::ArrayHandleImplicit<FunctorType>;
  using BasicArrayType = viskores::cont::ArrayHandle<ValueType>;
  using ConcatenateType = viskores::cont::ArrayHandleConcatenate<ValueHandleType, BasicArrayType>;

  FunctorType functor;
  for (viskores::Id start_pos = 0; start_pos < ARRAY_SIZE; start_pos += ARRAY_SIZE / 4)
  {
    viskores::Id implicitLen = ARRAY_SIZE - start_pos;
    viskores::Id basicLen = start_pos;

    // make an implicit array
    ValueHandleType implicit = viskores::cont::make_ArrayHandleImplicit(functor, implicitLen);
    // make a basic array
    std::vector<ValueType> basicVec;
    for (viskores::Id i = 0; i < basicLen; i++)
    {
      basicVec.push_back(ValueType(i));
    }
    BasicArrayType basic = viskores::cont::make_ArrayHandle(basicVec, viskores::CopyFlag::Off);

    // concatenate two arrays together
    ConcatenateType concatenate = viskores::cont::make_ArrayHandleConcatenate(implicit, basic);

    viskores::cont::ArrayHandle<ValueType> result;

    viskores::cont::Invoker invoke;
    invoke(PassThrough{}, concatenate, result);

    //verify that the control portal works
    auto resultPortal = result.ReadPortal();
    auto implicitPortal = implicit.ReadPortal();
    auto basicPortal = basic.ReadPortal();
    auto concatPortal = concatenate.ReadPortal();
    for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      const ValueType result_v = resultPortal.Get(i);
      ValueType correct_value;
      if (i < implicitLen)
        correct_value = implicitPortal.Get(i);
      else
        correct_value = basicPortal.Get(i - implicitLen);
      const ValueType control_value = concatPortal.Get(i);
      VISKORES_TEST_ASSERT(test_equal(result_v, correct_value),
                           "ArrayHandleConcatenate as Input Failed");
      VISKORES_TEST_ASSERT(test_equal(result_v, control_value),
                           "ArrayHandleConcatenate as Input Failed");
    }

    concatenate.ReleaseResources();
  }
}

void TestConcatOfConcat()
{
  std::cout << "Test concat of concat" << std::endl;

  viskores::cont::ArrayHandleIndex array1(ARRAY_SIZE);
  viskores::cont::ArrayHandleIndex array2(2 * ARRAY_SIZE);

  viskores::cont::ArrayHandleConcatenate<viskores::cont::ArrayHandleIndex,
                                         viskores::cont::ArrayHandleIndex>
    array3(array1, array2);

  viskores::cont::ArrayHandleIndex array4(ARRAY_SIZE);
  viskores::cont::ArrayHandleConcatenate<
    viskores::cont::ArrayHandleConcatenate<viskores::cont::ArrayHandleIndex,  // 1st
                                           viskores::cont::ArrayHandleIndex>, // ArrayHandle
    viskores::cont::ArrayHandleIndex>                                         // 2nd ArrayHandle
    array5;
  {
    array5 = viskores::cont::make_ArrayHandleConcatenate(array3, array4);
  }

  viskores::cont::printSummary_ArrayHandle(array5, std::cout, true);

  VISKORES_TEST_ASSERT(array5.GetNumberOfValues() == 4 * ARRAY_SIZE);
  VISKORES_TEST_ASSERT(array5.GetNumberOfComponentsFlat() == 1);

  // Check the values in array5. If array5 is correct, all the `ArrayHandleConcatinate`s
  // (such as in array3) must be working.
  auto portal = array5.ReadPortal();
  for (viskores::Id index = 0; index < ARRAY_SIZE; ++index)
  {
    VISKORES_TEST_ASSERT(portal.Get(index) == index);
    VISKORES_TEST_ASSERT(portal.Get(index + (3 * ARRAY_SIZE)) == index);
  }
  for (viskores::Id index = 0; index < (2 * ARRAY_SIZE); ++index)
  {
    VISKORES_TEST_ASSERT(portal.Get(index + ARRAY_SIZE) == index);
  }
}

void TestConcatenateEmptyArray()
{
  std::cout << "Test empty array" << std::endl;

  std::vector<viskores::Float64> vec;
  for (viskores::Id i = 0; i < ARRAY_SIZE; i++)
  {
    vec.push_back(viskores::Float64(i) * 1.5);
  }

  using CoeffValueType = viskores::Float64;
  using CoeffArrayTypeTmp = viskores::cont::ArrayHandle<CoeffValueType>;
  using ArrayConcat = viskores::cont::ArrayHandleConcatenate<CoeffArrayTypeTmp, CoeffArrayTypeTmp>;
  using ArrayConcat2 = viskores::cont::ArrayHandleConcatenate<ArrayConcat, CoeffArrayTypeTmp>;

  CoeffArrayTypeTmp arr1 = viskores::cont::make_ArrayHandle(vec, viskores::CopyFlag::Off);
  CoeffArrayTypeTmp arr2, arr3;

  ArrayConcat arrConc(arr2, arr1);
  ArrayConcat2 arrConc2(arrConc, arr3);

  viskores::cont::printSummary_ArrayHandle(arrConc2, std::cout, true);

  VISKORES_TEST_ASSERT(arrConc2.GetNumberOfValues() == ARRAY_SIZE);
  VISKORES_TEST_ASSERT(arrConc2.GetNumberOfComponentsFlat() == 1);
}

void TestConcatenateFill()
{
  std::cout << "Test fill" << std::endl;

  using T = viskores::FloatDefault;
  viskores::cont::ArrayHandle<T> array1;
  viskores::cont::ArrayHandle<T> array2;
  array1.Allocate(ARRAY_SIZE);
  array2.Allocate(ARRAY_SIZE);

  auto concatArray = viskores::cont::make_ArrayHandleConcatenate(array1, array2);

  const T value0 = TestValue(0, T{});
  const T value1 = TestValue(1, T{});
  const T value2 = TestValue(2, T{});

  VISKORES_STATIC_ASSERT_MSG((ARRAY_SIZE % 2) == 0, "ARRAY_SIZE must be even for this test.");

  concatArray.Fill(value2, 3 * ARRAY_SIZE / 2);
  concatArray.Fill(value1, ARRAY_SIZE / 2, 3 * ARRAY_SIZE / 2);
  concatArray.Fill(value0, 0, ARRAY_SIZE / 2);

  viskores::cont::printSummary_ArrayHandle(concatArray, std::cout, true);

  auto portal = concatArray.ReadPortal();
  for (viskores::Id index = 0; index < (ARRAY_SIZE / 2); ++index)
  {
    VISKORES_TEST_ASSERT(portal.Get(index) == value0);
  }
  for (viskores::Id index = (ARRAY_SIZE / 2); index < (3 * ARRAY_SIZE / 2); ++index)
  {
    VISKORES_TEST_ASSERT(portal.Get(index) == value1);
  }
  for (viskores::Id index = (3 * ARRAY_SIZE / 2); index < (2 * ARRAY_SIZE); ++index)
  {
    VISKORES_TEST_ASSERT(portal.Get(index) == value2);
  }
}

void TestArrayHandleConcatenate()
{
  TestConcatInvoke();
  TestConcatOfConcat();
  TestConcatenateEmptyArray();
  TestConcatenateFill();
}

} // anonymous namespace

int UnitTestArrayHandleConcatenate(int argc, char* argv[])
{
  //TestConcatenateEmptyArray();
  return viskores::cont::testing::Testing::Run(TestArrayHandleConcatenate, argc, argv);
}
