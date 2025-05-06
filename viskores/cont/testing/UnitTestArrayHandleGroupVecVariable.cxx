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

#include <viskores/cont/ArrayHandleGroupVecVariable.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ConvertNumComponentsToOffsets.h>
#include <viskores/cont/Invoker.h>

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

constexpr viskores::Id ARRAY_SIZE = 10;

// GroupVecVariable is a bit strange because it supports values of different
// lengths, so a simple pass through worklet will not work. Use custom
// worklets.
struct GroupVariableInputWorklet : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, WorkIndex, _2);

  template <typename InputType>
  VISKORES_EXEC void operator()(const InputType& input,
                                viskores::Id workIndex,
                                viskores::Id& dummyOut) const
  {
    using ComponentType = typename InputType::ComponentType;
    viskores::IdComponent expectedSize = static_cast<viskores::IdComponent>(workIndex);
    if (expectedSize != input.GetNumberOfComponents())
    {
      this->RaiseError("Got unexpected number of components.");
    }

    viskores::Id valueIndex = workIndex * (workIndex - 1) / 2;
    dummyOut = valueIndex;
    for (viskores::IdComponent componentIndex = 0; componentIndex < expectedSize; componentIndex++)
    {
      ComponentType expectedValue = TestValue(valueIndex, ComponentType());
      if (viskores::Abs(expectedValue - input[componentIndex]) > 0.000001)
      {
        this->RaiseError("Got bad value in GroupVariableInputWorklet.");
      }
      valueIndex++;
    }
  }
};

struct TestGroupVecVariableAsInput
{
  template <typename ComponentType>
  VISKORES_CONT void operator()(ComponentType) const
  {
    viskores::cont::Invoker invoke;
    viskores::Id sourceArraySize;

    viskores::cont::ArrayHandle<viskores::Id> numComponentsArray;
    viskores::cont::ArrayCopy(viskores::cont::ArrayHandleIndex(ARRAY_SIZE), numComponentsArray);
    viskores::cont::ArrayHandle<viskores::Id> offsetsArray =
      viskores::cont::ConvertNumComponentsToOffsets(numComponentsArray, sourceArraySize);

    viskores::cont::ArrayHandle<ComponentType> sourceArray;
    sourceArray.Allocate(sourceArraySize);
    SetPortal(sourceArray.WritePortal());

    viskores::cont::ArrayHandle<viskores::Id> dummyArray;

    auto groupVecArray =
      viskores::cont::make_ArrayHandleGroupVecVariable(sourceArray, offsetsArray);

    VISKORES_TEST_ASSERT(groupVecArray.GetNumberOfValues() == ARRAY_SIZE);
    // Num components is inconsistent, so you should just get 0.
    VISKORES_TEST_ASSERT(groupVecArray.GetNumberOfComponentsFlat() == 0);

    invoke(GroupVariableInputWorklet{}, groupVecArray, dummyArray);

    dummyArray.ReadPortal();

    groupVecArray.ReleaseResources();
  }
};

// GroupVecVariable is a bit strange because it supports values of different
// lengths, so a simple pass through worklet will not work. Use custom
// worklets.
struct GroupVariableOutputWorklet : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_2, WorkIndex);

  template <typename OutputType>
  VISKORES_EXEC void operator()(OutputType& output, viskores::Id workIndex) const
  {
    using ComponentType = typename OutputType::ComponentType;
    viskores::IdComponent expectedSize = static_cast<viskores::IdComponent>(workIndex);
    if (expectedSize != output.GetNumberOfComponents())
    {
      this->RaiseError("Got unexpected number of components.");
    }

    viskores::Id valueIndex = workIndex * (workIndex - 1) / 2;
    for (viskores::IdComponent componentIndex = 0; componentIndex < expectedSize; componentIndex++)
    {
      output[componentIndex] = TestValue(valueIndex, ComponentType());
      valueIndex++;
    }
  }
};

struct TestGroupVecVariableAsOutput
{
  template <typename ComponentType>
  VISKORES_CONT void operator()(ComponentType) const
  {
    viskores::Id sourceArraySize;

    viskores::cont::ArrayHandle<viskores::Id> numComponentsArray;
    viskores::cont::ArrayCopy(viskores::cont::ArrayHandleIndex(ARRAY_SIZE), numComponentsArray);
    viskores::cont::ArrayHandle<viskores::Id> offsetsArray =
      viskores::cont::ConvertNumComponentsToOffsets(numComponentsArray, sourceArraySize);

    viskores::cont::ArrayHandle<ComponentType> sourceArray;
    sourceArray.Allocate(sourceArraySize);

    viskores::worklet::DispatcherMapField<GroupVariableOutputWorklet> dispatcher;
    dispatcher.Invoke(viskores::cont::ArrayHandleIndex(ARRAY_SIZE),
                      viskores::cont::make_ArrayHandleGroupVecVariable(sourceArray, offsetsArray));

    CheckPortal(sourceArray.ReadPortal());
  }
};

void Run()
{
  using ScalarTypesToTest = viskores::List<viskores::UInt8, viskores::FloatDefault>;

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleGroupVecVariable as Input" << std::endl;
  viskores::testing::Testing::TryTypes(TestGroupVecVariableAsInput(), ScalarTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleGroupVecVariable as Output" << std::endl;
  viskores::testing::Testing::TryTypes(TestGroupVecVariableAsOutput(), ScalarTypesToTest());
}

} // anonymous namespace

int UnitTestArrayHandleGroupVecVariable(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
