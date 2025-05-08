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

#include <viskores/cont/ArrayHandleRuntimeVec.h>

#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/cont/Invoker.h>

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

constexpr viskores::Id ARRAY_SIZE = 10;

struct UnusualType
{
  viskores::Id X;
};

} // anonymous namespace

namespace detail
{

template <>
struct TestValueImpl<UnusualType>
{
  VISKORES_EXEC_CONT UnusualType operator()(viskores::Id index) const
  {
    return { TestValue(index, decltype(UnusualType::X){}) };
  }
};

template <>
struct TestEqualImpl<UnusualType, UnusualType>
{
  VISKORES_EXEC_CONT bool operator()(UnusualType value1,
                                     UnusualType value2,
                                     viskores::Float64 tolerance) const
  {
    return test_equal(value1.X, value2.X, tolerance);
  }
};

} // namespace detail

namespace
{

struct PassThrough : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename InValue, typename OutValue>
  VISKORES_EXEC void operator()(const InValue& inValue, OutValue& outValue) const
  {
    viskores::IdComponent inIndex = 0;
    viskores::IdComponent outIndex = 0;
    this->FlatCopy(inValue, inIndex, outValue, outIndex);
  }

  template <typename InValue, typename OutValue>
  VISKORES_EXEC void FlatCopy(const InValue& inValue,
                              viskores::IdComponent& inIndex,
                              OutValue& outValue,
                              viskores::IdComponent& outIndex) const
  {
    using VTraitsIn = viskores::VecTraits<InValue>;
    using VTraitsOut = viskores::VecTraits<OutValue>;
    VTraitsOut::SetComponent(outValue, outIndex, VTraitsIn::GetComponent(inValue, inIndex));
    inIndex++;
    outIndex++;
  }

  template <typename InComponent, viskores::IdComponent InN, typename OutValue>
  VISKORES_EXEC void FlatCopy(const viskores::Vec<InComponent, InN>& inValue,
                              viskores::IdComponent& inIndex,
                              OutValue& outValue,
                              viskores::IdComponent& outIndex) const
  {
    VISKORES_ASSERT(inIndex == 0);
    for (viskores::IdComponent i = 0; i < InN; ++i)
    {
      FlatCopy(inValue[i], inIndex, outValue, outIndex);
      inIndex = 0;
    }
  }

  template <typename InValue, typename OutComponent, viskores::IdComponent OutN>
  VISKORES_EXEC void FlatCopy(const InValue& inValue,
                              viskores::IdComponent& inIndex,
                              viskores::Vec<OutComponent, OutN>& outValue,
                              viskores::IdComponent& outIndex) const
  {
    VISKORES_ASSERT(outIndex == 0);
    for (viskores::IdComponent i = 0; i < OutN; ++i)
    {
      OutComponent outComponent;
      FlatCopy(inValue, inIndex, outComponent, outIndex);
      outValue[i] = outComponent;
      outIndex = 0;
    }
  }
};

template <viskores::IdComponent NUM_COMPONENTS>
struct TestRuntimeVecAsInput
{
  template <typename ComponentType>
  VISKORES_CONT void operator()(ComponentType) const
  {
    using ValueType = viskores::Vec<ComponentType, NUM_COMPONENTS>;

    viskores::cont::ArrayHandle<ComponentType> baseArray;
    baseArray.Allocate(ARRAY_SIZE * NUM_COMPONENTS);
    SetPortal(baseArray.WritePortal());

    auto runtimeVecArray = viskores::cont::make_ArrayHandleRuntimeVec(NUM_COMPONENTS, baseArray);
    VISKORES_TEST_ASSERT(runtimeVecArray.GetNumberOfValues() == ARRAY_SIZE,
                         "Group array reporting wrong array size.");
    VISKORES_TEST_ASSERT(runtimeVecArray.GetNumberOfComponentsFlat() ==
                         NUM_COMPONENTS * viskores::VecFlat<ComponentType>::NUM_COMPONENTS);

    viskores::cont::ArrayHandle<ValueType> resultArray;

    viskores::cont::Invoker{}(PassThrough{}, runtimeVecArray, resultArray);

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

    //verify that you can get the data as a basic array
    viskores::cont::ArrayHandle<viskores::Vec<ComponentType, NUM_COMPONENTS>> flatComponents;
    runtimeVecArray.AsArrayHandleBasic(flatComponents);
    VISKORES_TEST_ASSERT(test_equal_ArrayHandles(
      flatComponents, viskores::cont::make_ArrayHandleGroupVec<NUM_COMPONENTS>(baseArray)));

    viskores::cont::ArrayHandle<viskores::Vec<viskores::Vec<ComponentType, 1>, NUM_COMPONENTS>>
      nestedComponents;
    runtimeVecArray.AsArrayHandleBasic(nestedComponents);
    auto flatPortal = flatComponents.ReadPortal();
    auto nestedPortal = nestedComponents.ReadPortal();
    for (viskores::Id index = 0; index < flatPortal.GetNumberOfValues(); ++index)
    {
      VISKORES_TEST_ASSERT(test_equal(viskores::make_VecFlat(flatPortal.Get(index)),
                                      viskores::make_VecFlat(nestedPortal.Get(index))));
    }

    runtimeVecArray.ReleaseResources();
  }
};

template <viskores::IdComponent NUM_COMPONENTS>
struct TestRuntimeVecAsOutput
{
  template <typename ComponentType>
  VISKORES_CONT void operator()(ComponentType) const
  {
    using ValueType = viskores::Vec<ComponentType, NUM_COMPONENTS>;

    viskores::cont::ArrayHandle<ValueType> baseArray;
    baseArray.Allocate(ARRAY_SIZE);
    SetPortal(baseArray.WritePortal());

    viskores::cont::ArrayHandle<ComponentType> resultArray;

    viskores::cont::ArrayHandleRuntimeVec<ComponentType> runtimeVecArray(NUM_COMPONENTS,
                                                                         resultArray);

    viskores::cont::Invoker{}(PassThrough{}, baseArray, runtimeVecArray);

    VISKORES_TEST_ASSERT(runtimeVecArray.GetNumberOfValues() == ARRAY_SIZE,
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
  std::cout << "Testing ArrayHandleRuntimeVec(3) as Input" << std::endl;
  viskores::testing::Testing::TryTypes(TestRuntimeVecAsInput<3>(), HandleTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleRuntimeVec(4) as Input" << std::endl;
  viskores::testing::Testing::TryTypes(TestRuntimeVecAsInput<4>(), HandleTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleRuntimeVec(2) as Output" << std::endl;
  viskores::testing::Testing::TryTypes(TestRuntimeVecAsOutput<2>(), ScalarTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleRuntimeVec(3) as Output" << std::endl;
  viskores::testing::Testing::TryTypes(TestRuntimeVecAsOutput<3>(), ScalarTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleRuntimeVec(3) as Input with unusual type" << std::endl;
  TestRuntimeVecAsInput<3>{}(UnusualType{});
}

} // anonymous namespace

int UnitTestArrayHandleRuntimeVec(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
