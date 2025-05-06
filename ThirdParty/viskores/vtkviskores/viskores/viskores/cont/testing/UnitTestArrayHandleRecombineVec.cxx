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

#include <viskores/cont/ArrayHandleRecombineVec.h>

#include <viskores/cont/ArrayHandleReverse.h>
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

struct TestRecombineVecAsInput
{
  template <typename T>
  VISKORES_CONT void operator()(T) const
  {
    viskores::cont::ArrayHandle<T> baseArray;
    baseArray.Allocate(ARRAY_SIZE);
    SetPortal(baseArray.WritePortal());

    using VTraits = viskores::VecTraits<T>;
    viskores::cont::ArrayHandleRecombineVec<typename VTraits::ComponentType> recombinedArray;
    for (viskores::IdComponent cIndex = 0; cIndex < VTraits::NUM_COMPONENTS; ++cIndex)
    {
      recombinedArray.AppendComponentArray(
        viskores::cont::ArrayExtractComponent(baseArray, cIndex));
    }
    VISKORES_TEST_ASSERT(recombinedArray.GetNumberOfComponents() == VTraits::NUM_COMPONENTS);
    VISKORES_TEST_ASSERT(recombinedArray.GetNumberOfComponentsFlat() ==
                         viskores::VecFlat<T>::NUM_COMPONENTS);
    VISKORES_TEST_ASSERT(recombinedArray.GetNumberOfValues() == ARRAY_SIZE);

    viskores::cont::ArrayHandle<T> outputArray;
    viskores::cont::Invoker invoke;
    invoke(PassThrough{}, recombinedArray, outputArray);

    VISKORES_TEST_ASSERT(test_equal_ArrayHandles(baseArray, outputArray));
  }
};

struct TestRecombineVecAsOutput
{
  template <typename T>
  VISKORES_CONT void operator()(T) const
  {
    viskores::cont::ArrayHandle<T> baseArray;
    baseArray.Allocate(ARRAY_SIZE);
    SetPortal(baseArray.WritePortal());

    viskores::cont::ArrayHandle<T> outputArray;

    using VTraits = viskores::VecTraits<T>;
    viskores::cont::ArrayHandleRecombineVec<typename VTraits::ComponentType> recombinedArray;
    for (viskores::IdComponent cIndex = 0; cIndex < VTraits::NUM_COMPONENTS; ++cIndex)
    {
      recombinedArray.AppendComponentArray(
        viskores::cont::ArrayExtractComponent(outputArray, cIndex));
    }
    VISKORES_TEST_ASSERT(recombinedArray.GetNumberOfComponents() == VTraits::NUM_COMPONENTS);

    viskores::cont::Invoker invoke;
    invoke(PassThrough{}, baseArray, recombinedArray);
    VISKORES_TEST_ASSERT(test_equal_ArrayHandles(baseArray, outputArray));

    // Try outputing to a recombine vec inside of another fancy ArrayHandle.
    auto reverseOutput = viskores::cont::make_ArrayHandleReverse(recombinedArray);
    invoke(PassThrough{}, baseArray, reverseOutput);
    VISKORES_TEST_ASSERT(test_equal_ArrayHandles(baseArray, reverseOutput));
  }
};

void Run()
{
  using HandleTypesToTest =
    viskores::List<viskores::Id, viskores::Vec2i_32, viskores::FloatDefault, viskores::Vec3f_64>;

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleRecombineVec as Input" << std::endl;
  viskores::testing::Testing::TryTypes(TestRecombineVecAsInput(), HandleTypesToTest{});

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleRecombineVec as Output" << std::endl;
  viskores::testing::Testing::TryTypes(TestRecombineVecAsOutput(), HandleTypesToTest{});
}

} // anonymous namespace

int UnitTestArrayHandleRecombineVec(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
