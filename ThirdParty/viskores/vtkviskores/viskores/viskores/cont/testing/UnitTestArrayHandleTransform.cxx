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

#include <viskores/cont/ArrayHandleTransform.h>

#include <viskores/VecTraits.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/Invoker.h>

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

const viskores::Id ARRAY_SIZE = 10;

struct MySquare
{
  template <typename U>
  VISKORES_EXEC auto operator()(U u) const -> decltype(viskores::Dot(u, u))
  {
    return viskores::Dot(u, u);
  }
};

struct CheckTransformWorklet : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn original, FieldIn transformed);

  template <typename T, typename U>
  VISKORES_EXEC void operator()(const T& original, const U& transformed) const
  {
    if (!test_equal(transformed, MySquare{}(original)))
    {
      this->RaiseError("Encountered bad transformed value.");
    }
  }
};

template <typename OriginalArrayHandleType, typename TransformedArrayHandleType>
VISKORES_CONT void CheckControlPortals(const OriginalArrayHandleType& originalArray,
                                       const TransformedArrayHandleType& transformedArray)
{
  std::cout << "  Verify that the control portal works" << std::endl;

  using OriginalPortalType = typename OriginalArrayHandleType::ReadPortalType;
  using TransformedPortalType = typename TransformedArrayHandleType::ReadPortalType;

  VISKORES_TEST_ASSERT(originalArray.GetNumberOfValues() == transformedArray.GetNumberOfValues(),
                       "Number of values in transformed array incorrect.");

  OriginalPortalType originalPortal = originalArray.ReadPortal();
  TransformedPortalType transformedPortal = transformedArray.ReadPortal();

  VISKORES_TEST_ASSERT(originalPortal.GetNumberOfValues() == transformedPortal.GetNumberOfValues(),
                       "Number of values in transformed portal incorrect.");

  for (viskores::Id index = 0; index < originalArray.GetNumberOfValues(); index++)
  {
    using T = typename TransformedPortalType::ValueType;
    typename OriginalPortalType::ValueType original = originalPortal.Get(index);
    T transformed = transformedPortal.Get(index);
    VISKORES_TEST_ASSERT(test_equal(transformed, MySquare{}(original)), "Bad transform value.");
  }
}

struct ValueScale
{
  ValueScale()
    : Factor(1.0)
  {
  }

  ValueScale(viskores::Float64 factor)
    : Factor(factor)
  {
  }

  template <typename ValueType>
  VISKORES_EXEC_CONT ValueType operator()(const ValueType& v) const
  {
    using Traits = viskores::VecTraits<ValueType>;
    using TTraits = viskores::TypeTraits<ValueType>;
    using ComponentType = typename Traits::ComponentType;

    ValueType result = TTraits::ZeroInitialization();
    for (viskores::IdComponent i = 0; i < Traits::GetNumberOfComponents(v); ++i)
    {
      viskores::Float64 vi = static_cast<viskores::Float64>(Traits::GetComponent(v, i));
      viskores::Float64 ri = vi * this->Factor;
      Traits::SetComponent(result, i, static_cast<ComponentType>(ri));
    }
    return result;
  }

private:
  viskores::Float64 Factor;
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

template <typename InputValueType>
struct TransformTests
{
  using OutputValueType = typename viskores::VecTraits<InputValueType>::ComponentType;

  using TransformHandle =
    viskores::cont::ArrayHandleTransform<viskores::cont::ArrayHandle<InputValueType>, MySquare>;

  using CountingTransformHandle =
    viskores::cont::ArrayHandleTransform<viskores::cont::ArrayHandleCounting<InputValueType>,
                                         MySquare>;

  using Device = viskores::cont::DeviceAdapterTagSerial;
  using Algorithm = viskores::cont::DeviceAdapterAlgorithm<Device>;

  void operator()() const
  {
    MySquare functor;
    viskores::cont::Invoker invoke;

    std::cout << "Test a transform handle with a counting handle as the values" << std::endl;
    viskores::cont::ArrayHandleCounting<InputValueType> counting =
      viskores::cont::make_ArrayHandleCounting(
        InputValueType(OutputValueType(0)), InputValueType(1), ARRAY_SIZE);
    CountingTransformHandle countingTransformed =
      viskores::cont::make_ArrayHandleTransform(counting, functor);

    CheckControlPortals(counting, countingTransformed);

    std::cout << "  Verify that the execution portal works" << std::endl;
    invoke(CheckTransformWorklet{}, counting, countingTransformed);

    std::cout << "Test a transform handle with a normal handle as the values" << std::endl;
    //we are going to connect the two handles up, and than fill
    //the values and make sure the transform sees the new values in the handle
    viskores::cont::ArrayHandle<InputValueType> input;
    TransformHandle thandle(input, functor);

    using Portal = typename viskores::cont::ArrayHandle<InputValueType>::WritePortalType;
    input.Allocate(ARRAY_SIZE);
    SetPortal(input.WritePortal());

    CheckControlPortals(input, thandle);

    std::cout << "  Verify that the execution portal works" << std::endl;
    invoke(CheckTransformWorklet{}, input, thandle);

    std::cout << "Modify array handle values to ensure transform gets updated" << std::endl;
    {
      Portal portal = input.WritePortal();
      for (viskores::Id index = 0; index < ARRAY_SIZE; ++index)
      {
        portal.Set(index, TestValue(index * index, InputValueType()));
      }
    }

    CheckControlPortals(input, thandle);

    std::cout << "  Verify that the execution portal works" << std::endl;
    invoke(CheckTransformWorklet{}, input, thandle);

    std::cout << "Write to a transformed array with an inverse transform" << std::endl;
    {
      ValueScale scaleUp(2.0);
      ValueScale scaleDown(1.0 / 2.0);

      input.Allocate(ARRAY_SIZE);
      SetPortal(input.WritePortal());

      viskores::cont::ArrayHandle<InputValueType> output;
      auto transformed = viskores::cont::make_ArrayHandleTransform(output, scaleUp, scaleDown);

      invoke(PassThrough{}, input, transformed);

      //verify that the control portal works
      auto outputPortal = output.ReadPortal();
      auto transformedPortal = transformed.ReadPortal();
      for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
      {
        const InputValueType result_v = outputPortal.Get(i);
        const InputValueType correct_value = scaleDown(TestValue(i, InputValueType()));
        const InputValueType control_value = transformedPortal.Get(i);
        VISKORES_TEST_ASSERT(test_equal(result_v, correct_value), "Transform Handle Failed");
        VISKORES_TEST_ASSERT(test_equal(scaleUp(result_v), control_value),
                             "Transform Handle Control Failed");
      }
    }
  }
};

struct TryInputType
{
  template <typename InputType>
  void operator()(InputType) const
  {
    TransformTests<InputType>()();
  }
};

void TestArrayHandleTransform()
{
  viskores::testing::Testing::TryTypes(TryInputType());
}

} // anonymous namespace

int UnitTestArrayHandleTransform(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestArrayHandleTransform, argc, argv);
}
