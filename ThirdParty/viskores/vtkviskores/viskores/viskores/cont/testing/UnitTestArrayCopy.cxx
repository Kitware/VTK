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

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayCopyDevice.h>
#include <viskores/cont/ArrayHandleConcatenate.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/ArrayHandleReverse.h>
#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/ArrayHandleView.h>
#include <viskores/cont/UncertainArrayHandle.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <viskores/TypeTraits.h>
#include <viskores/VecTraits.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

static constexpr viskores::Id ARRAY_SIZE = 10;

viskores::cont::UnknownArrayHandle MakeComparable(const viskores::cont::UnknownArrayHandle& array,
                                                  std::false_type)
{
  return array;
}

template <typename T>
viskores::cont::UnknownArrayHandle MakeComparable(const viskores::cont::ArrayHandle<T>& array,
                                                  std::true_type)
{
  return array;
}

template <typename ArrayType>
viskores::cont::UnknownArrayHandle MakeComparable(const ArrayType& array, std::true_type)
{
  viskores::cont::ArrayHandle<typename ArrayType::ValueType> simpleArray;
  viskores::cont::ArrayCopyDevice(array, simpleArray);
  return simpleArray;
}

void TestValuesImpl(const viskores::cont::UnknownArrayHandle& refArray,
                    const viskores::cont::UnknownArrayHandle& testArray)
{
  auto result = test_equal_ArrayHandles(refArray, testArray);
  VISKORES_TEST_ASSERT(result, result.GetMergedMessage());
}

template <typename RefArrayType, typename TestArrayType>
void TestValues(const RefArrayType& refArray, const TestArrayType& testArray)
{
  TestValuesImpl(
    MakeComparable(refArray,
                   typename viskores::cont::internal::ArrayHandleCheck<RefArrayType>::type{}),
    MakeComparable(testArray,
                   typename viskores::cont::internal::ArrayHandleCheck<TestArrayType>::type{}));
}

template <typename ValueType>
viskores::cont::ArrayHandle<ValueType> MakeInputArray()
{
  viskores::cont::ArrayHandle<ValueType> input;
  input.Allocate(ARRAY_SIZE);
  SetPortal(input.WritePortal());
  return input;
}

template <typename ValueType>
void TryCopy()
{
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 "Trying type: " << viskores::testing::TypeName<ValueType>::Name());
  using VTraits = viskores::VecTraits<ValueType>;

  {
    std::cout << "implicit -> basic" << std::endl;
    viskores::cont::ArrayHandleIndex input(ARRAY_SIZE);
    viskores::cont::ArrayHandle<typename VTraits::BaseComponentType> output;
    viskores::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "basic -> basic" << std::endl;
    using SourceType = typename VTraits::template ReplaceComponentType<viskores::Id>;
    viskores::cont::ArrayHandle<SourceType> input = MakeInputArray<SourceType>();
    viskores::cont::ArrayHandle<ValueType> output;
    viskores::cont::ArrayCopy(input, output);
    TestValues(input, output);

    output.ReleaseResources();
    viskores::cont::ArrayCopy(viskores::cont::UnknownArrayHandle(input), output);
    TestValues(input, output);
  }

  {
    std::cout << "implicit -> implicit (index)" << std::endl;
    viskores::cont::ArrayHandleIndex input(ARRAY_SIZE);
    viskores::cont::ArrayHandleIndex output;
    viskores::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "implicit -> implicit (constant)" << std::endl;
    viskores::cont::ArrayHandleConstant<int> input(41, ARRAY_SIZE);
    viskores::cont::ArrayHandleConstant<int> output;
    viskores::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "implicit -> implicit (base->derived, constant)" << std::endl;
    viskores::cont::ArrayHandle<int, viskores::cont::StorageTagConstant> input =
      viskores::cont::make_ArrayHandleConstant<int>(41, ARRAY_SIZE);
    viskores::cont::ArrayHandleConstant<int> output;
    viskores::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "constant -> basic" << std::endl;
    viskores::cont::ArrayHandleConstant<ValueType> input(TestValue(2, ValueType{}), ARRAY_SIZE);
    viskores::cont::ArrayHandle<ValueType> output;
    viskores::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "counting -> basic" << std::endl;
    viskores::cont::ArrayHandleCounting<ValueType> input(ValueType(-4), ValueType(3), ARRAY_SIZE);
    viskores::cont::ArrayHandle<ValueType> output;
    viskores::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "view -> basic" << std::endl;
    viskores::cont::ArrayHandle<ValueType> input = MakeInputArray<ValueType>();
    viskores::cont::make_ArrayHandleView(input, 1, ARRAY_SIZE / 2);
    viskores::cont::ArrayHandle<ValueType> output;
    viskores::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "concatinate -> basic" << std::endl;
    viskores::cont::ArrayHandle<ValueType> input1 = MakeInputArray<ValueType>();
    viskores::cont::ArrayHandleConstant<ValueType> input2(TestValue(6, ValueType{}),
                                                          ARRAY_SIZE / 2);
    auto concatInput = viskores::cont::make_ArrayHandleConcatenate(input1, input2);
    viskores::cont::ArrayHandle<ValueType> output;
    viskores::cont::ArrayCopy(concatInput, output);
    TestValues(concatInput, output);
  }

  {
    std::cout << "permutation -> basic" << std::endl;
    viskores::cont::ArrayHandle<viskores::Id> indices;
    viskores::cont::ArrayCopy(
      viskores::cont::make_ArrayHandleCounting<viskores::Id>(0, 2, ARRAY_SIZE / 2), indices);
    auto input = viskores::cont::make_ArrayHandlePermutation(indices, MakeInputArray<ValueType>());
    viskores::cont::ArrayHandle<ValueType> output;
    viskores::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "unknown -> unknown" << std::endl;
    viskores::cont::UnknownArrayHandle input = MakeInputArray<ValueType>();
    viskores::cont::UnknownArrayHandle output;
    viskores::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "unknown -> basic (same type)" << std::endl;
    viskores::cont::UnknownArrayHandle input = MakeInputArray<ValueType>();
    viskores::cont::ArrayHandle<ValueType> output;
    viskores::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "unknown -> basic (different type)" << std::endl;
    using SourceType = typename VTraits::template ReplaceComponentType<viskores::UInt8>;
    viskores::cont::UnknownArrayHandle input = MakeInputArray<SourceType>();
    viskores::cont::ArrayHandle<ValueType> output;
    viskores::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "unknown -> basic (different type, unsupported device)" << std::endl;
    // Force the source to be on the Serial device. If the --viskores-device argument was
    // given with a different device (which is how ctest is set up if compiled with
    // any device), then Serial will be turned off.
    using SourceType = typename VTraits::template ReplaceComponentType<viskores::UInt8>;
    auto rawInput = MakeInputArray<SourceType>();
    {
      // Force moving the data to the Serial device.
      viskores::cont::Token token;
      rawInput.PrepareForInput(viskores::cont::DeviceAdapterTagSerial{}, token);
    }
    viskores::cont::UnknownArrayHandle input = rawInput;
    viskores::cont::ArrayHandle<ValueType> output;
    viskores::cont::ArrayCopy(input, output);
    TestValues(input, output);
  }

  {
    std::cout << "runtime vec size -> runtime vec size" << std::endl;
    using ComponentType = typename VTraits::BaseComponentType;
    viskores::cont::ArrayHandle<ValueType> staticVecArray = MakeInputArray<ValueType>();
    viskores::cont::ArrayHandleRuntimeVec<ComponentType> input =
      viskores::cont::make_ArrayHandleRuntimeVec(staticVecArray);
    viskores::cont::ArrayHandleRuntimeVec<ComponentType> output(input.GetNumberOfComponents());
    viskores::cont::ArrayCopy(input, output);
    // Convert the arrays back to static vec sizes for comparison, because TestValues
    // uses a device array copy that may not work on runtime vec sizes.
    TestValues(staticVecArray,
               output.template AsArrayHandleBasic<viskores::cont::ArrayHandle<ValueType>>());
  }

  {
    std::cout << "runtime vec size reverse -> runtime vec size view" << std::endl;
    using ComponentType = typename VTraits::BaseComponentType;
    viskores::cont::ArrayHandle<ValueType> staticVecArray = MakeInputArray<ValueType>();
    viskores::cont::ArrayHandleRuntimeVec<ComponentType> inputRuntimeVec =
      viskores::cont::make_ArrayHandleRuntimeVec(staticVecArray);
    auto input = viskores::cont::make_ArrayHandleReverse(inputRuntimeVec);
    viskores::cont::ArrayHandleRuntimeVec<ComponentType> outputBase(
      inputRuntimeVec.GetNumberOfComponents());
    outputBase.Allocate(ARRAY_SIZE * 2);
    auto output = viskores::cont::make_ArrayHandleView(outputBase, 2, ARRAY_SIZE);
    viskores::cont::ArrayCopy(input, output);
    // Convert the arrays back to static vec sizes for comparison, because TestValues
    // uses a device array copy that may not work on runtime vec sizes.
    TestValues(viskores::cont::make_ArrayHandleReverse(staticVecArray),
               viskores::cont::make_ArrayHandleView(
                 outputBase.template AsArrayHandleBasic<viskores::cont::ArrayHandle<ValueType>>(),
                 2,
                 ARRAY_SIZE));
  }

  {
    std::cout << "runtime vec size -> runtime vec size (different type)" << std::endl;
    using ComponentType = typename VTraits::BaseComponentType;
    using SourceType = typename VTraits::template ReplaceComponentType<viskores::UInt8>;
    viskores::cont::ArrayHandle<SourceType> staticVecArray = MakeInputArray<SourceType>();
    viskores::cont::ArrayHandleRuntimeVec<viskores::UInt8> input =
      viskores::cont::make_ArrayHandleRuntimeVec(staticVecArray);
    viskores::cont::ArrayHandleRuntimeVec<ComponentType> output(input.GetNumberOfComponents());
    viskores::cont::ArrayCopy(input, output);
    // Convert the arrays back to static vec sizes for comparison, because TestValues
    // uses a device array copy that may not work on runtime vec sizes.
    TestValues(staticVecArray,
               output.template AsArrayHandleBasic<viskores::cont::ArrayHandle<ValueType>>());
  }

  {
    std::cout << "basic -> recombined vec" << std::endl;
    using ComponentType = typename VTraits::BaseComponentType;
    viskores::cont::ArrayHandle<ValueType> input = MakeInputArray<ValueType>();
    viskores::cont::ArrayHandle<ValueType> output;
    auto recombinedVec =
      viskores::cont::UnknownArrayHandle{ output }.ExtractArrayFromComponents<ComponentType>();
    viskores::cont::ArrayCopy(input, recombinedVec);
    TestValues(input, output);
  }

  {
    std::cout << "basic -> recombined vec (different type)" << std::endl;
    using SourceType = typename VTraits::template ReplaceComponentType<viskores::Id>;
    using ComponentType = typename VTraits::BaseComponentType;
    viskores::cont::ArrayHandle<SourceType> input = MakeInputArray<SourceType>();
    viskores::cont::ArrayHandle<ValueType> output;
    auto recombinedVec =
      viskores::cont::UnknownArrayHandle{ output }.ExtractArrayFromComponents<ComponentType>();
    viskores::cont::ArrayCopy(input, recombinedVec);
    TestValues(input, output);
  }

  {
    std::cout << "constant -> extracted component" << std::endl;
    using ComponentType = typename VTraits::BaseComponentType;
    viskores::cont::ArrayHandle<ValueType> output;
    output.Allocate(ARRAY_SIZE);
    ValueType invalue = TestValue(7, ValueType{});
    for (viskores::IdComponent component = 0; component < VTraits::NUM_COMPONENTS; ++component)
    {
      viskores::cont::ArrayHandleConstant<ComponentType> input(
        VTraits::GetComponent(invalue, component), ARRAY_SIZE);
      auto extractedComponent =
        viskores::cont::ArrayExtractComponent(output, component, viskores::CopyFlag::Off);
      viskores::cont::ArrayCopy(input, extractedComponent);
    }
    TestValues(viskores::cont::make_ArrayHandleConstant(invalue, ARRAY_SIZE), output);
  }

  // Test the copy methods in UnknownArrayHandle. Although this would be appropriate in
  // UnitTestUnknownArrayHandle, it is easier to test copies here.
  {
    std::cout << "unknown.DeepCopyFrom(same type)" << std::endl;
    viskores::cont::ArrayHandle<ValueType> input = MakeInputArray<ValueType>();
    viskores::cont::ArrayHandle<ValueType> outputArray;
    viskores::cont::UnknownArrayHandle(outputArray).DeepCopyFrom(input);
    // Should be different arrays with same content.
    VISKORES_TEST_ASSERT(input != outputArray);
    TestValues(input, outputArray);

    viskores::cont::UnknownArrayHandle outputUnknown;
    outputUnknown.DeepCopyFrom(input);
    // Should be different arrays with same content.
    VISKORES_TEST_ASSERT(input !=
                         outputUnknown.AsArrayHandle<viskores::cont::ArrayHandle<ValueType>>());
    TestValues(input, outputUnknown);
  }

  {
    std::cout << "unknown.DeepCopyFrom(different type)" << std::endl;
    using SourceType = typename VTraits::template ReplaceComponentType<viskores::UInt8>;
    viskores::cont::ArrayHandle<SourceType> input = MakeInputArray<SourceType>();
    viskores::cont::ArrayHandle<ValueType> outputArray;
    viskores::cont::UnknownArrayHandle(outputArray).DeepCopyFrom(input);
    TestValues(input, outputArray);

    outputArray.ReleaseResources();
    viskores::cont::UnknownArrayHandle outputUnknown(outputArray);
    outputUnknown.DeepCopyFrom(input);
    TestValues(input, outputUnknown);
  }

  {
    std::cout << "unknown.CopyShallowIfPossible(same type)" << std::endl;
    viskores::cont::ArrayHandle<ValueType> input = MakeInputArray<ValueType>();
    viskores::cont::UnknownArrayHandle outputUnknown;
    outputUnknown.CopyShallowIfPossible(input);
    VISKORES_TEST_ASSERT(input ==
                         outputUnknown.AsArrayHandle<viskores::cont::ArrayHandle<ValueType>>());

    viskores::cont::ArrayHandle<ValueType> outputArray;
    outputUnknown = outputArray;
    outputUnknown.CopyShallowIfPossible(input);
    outputUnknown.AsArrayHandle(outputArray);
    VISKORES_TEST_ASSERT(input == outputArray);
  }

  {
    std::cout << "unknown.CopyShallowIfPossible(different type)" << std::endl;
    using SourceType = typename VTraits::template ReplaceComponentType<viskores::UInt8>;
    viskores::cont::ArrayHandle<SourceType> input = MakeInputArray<SourceType>();
    viskores::cont::ArrayHandle<ValueType> outputArray;
    viskores::cont::UnknownArrayHandle(outputArray).CopyShallowIfPossible(input);
    TestValues(input, outputArray);

    outputArray.ReleaseResources();
    viskores::cont::UnknownArrayHandle outputUnknown(outputArray);
    outputUnknown.CopyShallowIfPossible(input);
    TestValues(input, outputUnknown);
  }
}

void TryArrayCopyShallowIfPossible()
{
  viskores::cont::ArrayHandle<viskores::Float32> input = MakeInputArray<viskores::Float32>();
  viskores::cont::UnknownArrayHandle unknownInput = input;

  {
    std::cout << "shallow copy" << std::endl;
    viskores::cont::ArrayHandle<viskores::Float32> output;
    viskores::cont::ArrayCopyShallowIfPossible(unknownInput, output);
    VISKORES_TEST_ASSERT(input == output, "Copy was not shallow");
  }

  {
    std::cout << "cannot shallow copy" << std::endl;
    viskores::cont::ArrayHandle<viskores::Float64> output;
    viskores::cont::ArrayCopyShallowIfPossible(unknownInput, output);
    TestValues(input, output);
  }
}

void TestArrayCopy()
{
  TryCopy<viskores::Id>();
  TryCopy<viskores::IdComponent>();
  TryCopy<viskores::Float32>();
  TryCopy<viskores::Vec3f>();
  TryCopy<viskores::Vec4i_16>();
  TryArrayCopyShallowIfPossible();
}

} // anonymous namespace

int UnitTestArrayCopy(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestArrayCopy, argc, argv);
}
