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

#include <viskores/cont/ArrayHandleGroupVec.h>

#include <viskores/cont/Invoker.h>

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

constexpr viskores::Id ARRAY_SIZE = 10;

struct PassThrough : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename InValue, typename OutValue>
  VISKORES_EXEC void operator()(const InValue& inValue, OutValue& outValue) const
  {
    outValue = inValue;
  }
};

template <viskores::IdComponent NUM_COMPONENTS>
struct TestGroupVecAsInput
{
  template <typename ComponentType>
  VISKORES_CONT void operator()(ComponentType) const
  {
    using ValueType = viskores::Vec<ComponentType, NUM_COMPONENTS>;

    viskores::cont::ArrayHandle<ComponentType> baseArray;
    baseArray.Allocate(ARRAY_SIZE * NUM_COMPONENTS);
    SetPortal(baseArray.WritePortal());

    viskores::cont::ArrayHandleGroupVec<viskores::cont::ArrayHandle<ComponentType>, NUM_COMPONENTS>
      groupArray(baseArray);
    VISKORES_TEST_ASSERT(groupArray.GetNumberOfValues() == ARRAY_SIZE,
                         "Group array reporting wrong array size.");
    VISKORES_TEST_ASSERT(groupArray.GetNumberOfComponentsFlat() ==
                         viskores::VecFlat<ComponentType>::NUM_COMPONENTS * NUM_COMPONENTS);

    viskores::cont::ArrayHandle<ValueType> resultArray;

    viskores::worklet::DispatcherMapField<PassThrough> dispatcher;
    dispatcher.Invoke(groupArray, resultArray);

    VISKORES_TEST_ASSERT(resultArray.GetNumberOfValues() == ARRAY_SIZE,
                         "Got bad result array size.");

    //verify that the control portal works
    viskores::Id totalIndex = 0;
    auto resultPortal = resultArray.ReadPortal();
    for (viskores::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      const ValueType result = resultPortal.Get(index);
      for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
           componentIndex++)
      {
        const ComponentType expectedValue = TestValue(totalIndex, ComponentType());
        VISKORES_TEST_ASSERT(test_equal(result[componentIndex], expectedValue),
                             "Result array got wrong value.");
        totalIndex++;
      }
    }

    groupArray.ReleaseResources();
  }
};

template <viskores::IdComponent NUM_COMPONENTS>
struct TestGroupVecAsOutput
{
  template <typename ComponentType>
  VISKORES_CONT void operator()(ComponentType) const
  {
    using ValueType = viskores::Vec<ComponentType, NUM_COMPONENTS>;

    viskores::cont::ArrayHandle<ValueType> baseArray;
    baseArray.Allocate(ARRAY_SIZE);
    SetPortal(baseArray.WritePortal());

    viskores::cont::ArrayHandle<ComponentType> resultArray;

    viskores::cont::ArrayHandleGroupVec<viskores::cont::ArrayHandle<ComponentType>, NUM_COMPONENTS>
      groupArray(resultArray);

    viskores::worklet::DispatcherMapField<PassThrough> dispatcher;
    dispatcher.Invoke(baseArray, groupArray);

    VISKORES_TEST_ASSERT(groupArray.GetNumberOfValues() == ARRAY_SIZE,
                         "Group array reporting wrong array size.");

    VISKORES_TEST_ASSERT(resultArray.GetNumberOfValues() == ARRAY_SIZE * NUM_COMPONENTS,
                         "Got bad result array size.");

    //verify that the control portal works
    viskores::Id totalIndex = 0;
    auto resultPortal = resultArray.ReadPortal();
    for (viskores::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      const ValueType expectedValue = TestValue(index, ValueType());
      for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
           componentIndex++)
      {
        const ComponentType result = resultPortal.Get(totalIndex);
        VISKORES_TEST_ASSERT(test_equal(result, expectedValue[componentIndex]),
                             "Result array got wrong value.");
        totalIndex++;
      }
    }
  }
};

void Run()
{
  using HandleTypesToTest =
    viskores::List<viskores::Id, viskores::Vec2i_32, viskores::FloatDefault, viskores::Vec3f_64>;
  using ScalarTypesToTest = viskores::List<viskores::UInt8, viskores::FloatDefault>;

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleGroupVec<3> as Input" << std::endl;
  viskores::testing::Testing::TryTypes(TestGroupVecAsInput<3>(), HandleTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleGroupVec<4> as Input" << std::endl;
  viskores::testing::Testing::TryTypes(TestGroupVecAsInput<4>(), HandleTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleGroupVec<2> as Output" << std::endl;
  viskores::testing::Testing::TryTypes(TestGroupVecAsOutput<2>(), ScalarTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleGroupVec<3> as Output" << std::endl;
  viskores::testing::Testing::TryTypes(TestGroupVecAsOutput<3>(), ScalarTypesToTest());
}

} // anonymous namespace

int UnitTestArrayHandleGroupVec(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
