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

#include <viskores/cont/ArrayHandleSOAStride.h>

#include <viskores/cont/ArrayCopyDevice.h>
#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/cont/Invoker.h>

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

constexpr viskores::Id ARRAY_SIZE = 10;

using ScalarTypesToTest = viskores::List<viskores::UInt8, viskores::FloatDefault>;
using VectorTypesToTest = viskores::List<viskores::Vec2i_8, viskores::Vec3f_32>;

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

struct TestSOASAsInput
{
  template <typename ValueType>
  VISKORES_CONT void operator()(const ValueType viskoresNotUsed(v)) const
  {
    using VTraits = viskores::VecTraits<ValueType>;
    using ComponentType = typename VTraits::ComponentType;
    constexpr viskores::IdComponent NUM_COMPONENTS = VTraits::NUM_COMPONENTS;

    viskores::cont::ArrayHandleSOAStride<ValueType> soaStrideArray;
    {
      viskores::cont::ArrayHandle<ComponentType> dataArray;
      auto groupVec = viskores::cont::make_ArrayHandleGroupVec<NUM_COMPONENTS>(dataArray);
      groupVec.Allocate(ARRAY_SIZE);
      SetPortal(groupVec.WritePortal());
      for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
           ++componentIndex)
      {
        viskores::cont::ArrayHandleStride<ComponentType> componentArray =
          viskores::cont::make_ArrayHandleStride(
            dataArray, ARRAY_SIZE, NUM_COMPONENTS, componentIndex);
        soaStrideArray.SetArray(componentIndex, componentArray);
      }
    }

    VISKORES_TEST_ASSERT(soaStrideArray.GetNumberOfComponentsFlat() ==
                         viskores::VecFlat<ValueType>::NUM_COMPONENTS);
    VISKORES_TEST_ASSERT(soaStrideArray.GetNumberOfValues() == ARRAY_SIZE);
    VISKORES_TEST_ASSERT(soaStrideArray.ReadPortal().GetNumberOfValues() == ARRAY_SIZE);
    CheckPortal(soaStrideArray.ReadPortal());

    viskores::cont::ArrayHandle<ValueType> basicArray;
    viskores::cont::ArrayCopyDevice(soaStrideArray, basicArray);
    VISKORES_TEST_ASSERT(basicArray.GetNumberOfValues() == ARRAY_SIZE);
    CheckPortal(basicArray.ReadPortal());
  }
};

struct TestSOASAsOutput
{
  template <typename ValueType>
  VISKORES_CONT void operator()(const ValueType viskoresNotUsed(v)) const
  {
    using VTraits = viskores::VecTraits<ValueType>;
    using ComponentType = typename VTraits::ComponentType;
    constexpr viskores::IdComponent NUM_COMPONENTS = VTraits::NUM_COMPONENTS;

    viskores::cont::ArrayHandle<ValueType> basicArray;
    basicArray.Allocate(ARRAY_SIZE);
    SetPortal(basicArray.WritePortal());

    viskores::cont::ArrayHandleSOAStride<ValueType> soaStrideArray;
    viskores::cont::ArrayHandle<ComponentType> dataArray;
    {
      auto groupVec = viskores::cont::make_ArrayHandleGroupVec<NUM_COMPONENTS>(dataArray);
      for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
           ++componentIndex)
      {
        viskores::cont::ArrayHandleStride<ComponentType> componentArray =
          viskores::cont::make_ArrayHandleStride(dataArray, 0, NUM_COMPONENTS, componentIndex);
        soaStrideArray.SetArray(componentIndex, componentArray);
      }
    }

    soaStrideArray.Allocate(ARRAY_SIZE);
    auto portal = soaStrideArray.WritePortal();
    portal.Set(0, ValueType{});
    viskores::cont::Invoker{}(PassThrough{}, basicArray, soaStrideArray);

    VISKORES_TEST_ASSERT(soaStrideArray.GetNumberOfValues() == ARRAY_SIZE);
    for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
         ++componentIndex)
    {
      viskores::cont::ArrayHandleStride<ComponentType> componentArray =
        soaStrideArray.GetArray(componentIndex);
      auto componentPortal = componentArray.ReadPortal();
      for (viskores::Id valueIndex = 0; valueIndex < ARRAY_SIZE; ++valueIndex)
      {
        ComponentType expected =
          VTraits::GetComponent(TestValue(valueIndex, ValueType{}), componentIndex);
        ComponentType got = componentPortal.Get(valueIndex);
        VISKORES_TEST_ASSERT(test_equal(expected, got));
      }
    }
    CheckPortal(dataArray.ReadPortal());
  }
};

static void Run()
{
  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleSOAStride as Input" << std::endl;
  viskores::testing::Testing::TryTypes(TestSOASAsInput(), ScalarTypesToTest());
  viskores::testing::Testing::TryTypes(TestSOASAsInput(), VectorTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleSOAStride as Output" << std::endl;
  viskores::testing::Testing::TryTypes(TestSOASAsOutput(), ScalarTypesToTest());
  viskores::testing::Testing::TryTypes(TestSOASAsOutput(), VectorTypesToTest());
}

} // anonymous namespace

int UnitTestArrayHandleSOAStride(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
