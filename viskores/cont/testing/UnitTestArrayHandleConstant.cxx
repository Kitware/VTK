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

#include <viskores/cont/ArrayHandleConstant.h>

#include <viskores/cont/Invoker.h>

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

constexpr viskores::Id ARRAY_SIZE = 10;

using HandleTypesToTest =
  viskores::List<viskores::Id, viskores::Vec2i_32, viskores::FloatDefault, viskores::Vec3f_64>;

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

struct TestConstantAsInput
{
  template <typename ValueType>
  VISKORES_CONT void operator()(const ValueType viskoresNotUsed(v)) const
  {
    const ValueType value = TestValue(43, ValueType());

    viskores::cont::ArrayHandleConstant<ValueType> constant =
      viskores::cont::make_ArrayHandleConstant(value, ARRAY_SIZE);

    VISKORES_TEST_ASSERT(constant.GetValue() == value);
    VISKORES_TEST_ASSERT(constant.GetNumberOfValues() == ARRAY_SIZE);
    VISKORES_TEST_ASSERT(constant.GetNumberOfComponentsFlat() ==
                         viskores::VecFlat<ValueType>::NUM_COMPONENTS);

    viskores::cont::ArrayHandle<ValueType> result;

    viskores::cont::Invoker invoke;
    invoke(PassThrough{}, constant, result);

    //verify that the control portal works
    auto resultPortal = result.ReadPortal();
    auto constantPortal = constant.ReadPortal();
    for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      const ValueType result_v = resultPortal.Get(i);
      const ValueType control_value = constantPortal.Get(i);
      VISKORES_TEST_ASSERT(test_equal(result_v, value), "Counting Handle Failed");
      VISKORES_TEST_ASSERT(test_equal(result_v, control_value), "Counting Handle Control Failed");
    }

    constant.ReleaseResources();
  }
};

void Run()
{
  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleConstant as Input" << std::endl;
  viskores::testing::Testing::TryTypes(TestConstantAsInput(), HandleTypesToTest());
}

} // anonymous namespace

int UnitTestArrayHandleConstant(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
