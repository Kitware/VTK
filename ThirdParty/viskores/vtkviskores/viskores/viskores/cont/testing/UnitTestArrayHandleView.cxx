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

#include <viskores/cont/ArrayHandleView.h>

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

struct TestViewAsInput
{
  template <typename ValueType>
  VISKORES_CONT void operator()(const ValueType viskoresNotUsed(v)) const
  {
    using FunctorType = IndexSquared<ValueType>;

    using ValueHandleType = viskores::cont::ArrayHandleImplicit<FunctorType>;
    using ViewHandleType = viskores::cont::ArrayHandleView<ValueHandleType>;

    FunctorType functor;
    for (viskores::Id start_pos = 0; start_pos < ARRAY_SIZE; start_pos += ARRAY_SIZE / 4)
    {
      const viskores::Id counting_ARRAY_SIZE = ARRAY_SIZE - start_pos;

      ValueHandleType implicit = viskores::cont::make_ArrayHandleImplicit(functor, ARRAY_SIZE);

      ViewHandleType view =
        viskores::cont::make_ArrayHandleView(implicit, start_pos, counting_ARRAY_SIZE);
      VISKORES_TEST_ASSERT(view.GetNumberOfComponentsFlat() ==
                           viskores::VecFlat<ValueType>::NUM_COMPONENTS);
      VISKORES_TEST_ASSERT(view.GetNumberOfValues() == counting_ARRAY_SIZE);

      viskores::cont::ArrayHandle<ValueType> result;

      viskores::cont::Invoker invoke;
      invoke(PassThrough{}, view, result);

      //verify that the control portal works
      auto resultPortal = result.ReadPortal();
      auto implicitPortal = implicit.ReadPortal();
      auto viewPortal = view.ReadPortal();
      for (viskores::Id i = 0; i < counting_ARRAY_SIZE; ++i)
      {
        const viskores::Id value_index = i;
        const viskores::Id key_index = start_pos + i;

        const ValueType result_v = resultPortal.Get(value_index);
        const ValueType correct_value = implicitPortal.Get(key_index);
        const ValueType control_value = viewPortal.Get(value_index);
        VISKORES_TEST_ASSERT(test_equal(result_v, correct_value), "Implicit Handle Failed");
        VISKORES_TEST_ASSERT(test_equal(result_v, control_value), "Implicit Handle Failed");
      }

      view.ReleaseResources();
    }
  }
};

struct TestViewAsOutput
{
  template <typename ValueType>
  VISKORES_CONT void operator()(const ValueType viskoresNotUsed(v)) const
  {
    using ValueHandleType = viskores::cont::ArrayHandle<ValueType>;
    using ViewHandleType = viskores::cont::ArrayHandleView<ValueHandleType>;

    viskores::cont::ArrayHandle<ValueType> input;
    input.Allocate(ARRAY_SIZE);
    SetPortal(input.WritePortal());

    ValueHandleType values;
    values.Allocate(ARRAY_SIZE * 2);

    ViewHandleType view = viskores::cont::make_ArrayHandleView(values, ARRAY_SIZE, ARRAY_SIZE);
    viskores::cont::Invoker invoke;
    invoke(PassThrough{}, input, view);

    //verify that the control portal works
    CheckPortal(view.ReadPortal());

    //verify that filling works
    const ValueType expected = TestValue(20, ValueType{});
    view.Fill(expected);
    auto valuesPortal = values.ReadPortal();
    for (viskores::Id index = ARRAY_SIZE; index < 2 * ARRAY_SIZE; ++index)
    {
      VISKORES_TEST_ASSERT(valuesPortal.Get(index) == expected);
    }
  }
};

void Run()
{
  using HandleTypesToTest =
    viskores::List<viskores::Id, viskores::Vec2i_32, viskores::FloatDefault, viskores::Vec3f_64>;

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleView as Input" << std::endl;
  viskores::testing::Testing::TryTypes(TestViewAsInput(), HandleTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleView as Output" << std::endl;
  viskores::testing::Testing::TryTypes(TestViewAsOutput(), HandleTypesToTest());
}

} // anonymous namespace

int UnitTestArrayHandleView(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
