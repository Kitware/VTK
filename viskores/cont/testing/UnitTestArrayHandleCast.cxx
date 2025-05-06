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

#include <viskores/cont/ArrayHandleCast.h>

#include <viskores/cont/Invoker.h>

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

constexpr viskores::Id ARRAY_SIZE = 10;

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

struct TestCastAsInput
{
  template <typename CastToType>
  VISKORES_CONT void operator()(CastToType viskoresNotUsed(type)) const
  {
    viskores::cont::Invoker invoke;
    using InputArrayType = viskores::cont::ArrayHandleIndex;

    InputArrayType input(ARRAY_SIZE);
    viskores::cont::ArrayHandleCast<CastToType, InputArrayType> castArray =
      viskores::cont::make_ArrayHandleCast(input, CastToType());
    viskores::cont::ArrayHandle<CastToType> result;

    invoke(PassThrough{}, castArray, result);

    // verify results
    viskores::Id length = ARRAY_SIZE;
    auto resultPortal = result.ReadPortal();
    auto inputPortal = input.ReadPortal();
    for (viskores::Id i = 0; i < length; ++i)
    {
      VISKORES_TEST_ASSERT(resultPortal.Get(i) == static_cast<CastToType>(inputPortal.Get(i)),
                           "Casting ArrayHandle Failed");
    }

    castArray.ReleaseResources();
  }
};

struct TestCastAsOutput
{
  template <typename CastFromType>
  VISKORES_CONT void operator()(CastFromType viskoresNotUsed(type)) const
  {
    viskores::cont::Invoker invoke;

    using InputArrayType = viskores::cont::ArrayHandleIndex;
    using ResultArrayType = viskores::cont::ArrayHandle<CastFromType>;

    InputArrayType input(ARRAY_SIZE);

    ResultArrayType result;
    viskores::cont::ArrayHandleCast<viskores::Id, ResultArrayType> castArray =
      viskores::cont::make_ArrayHandleCast<CastFromType>(result);

    invoke(PassThrough{}, input, castArray);

    // verify results
    viskores::Id length = ARRAY_SIZE;
    auto inputPortal = input.ReadPortal();
    auto resultPortal = result.ReadPortal();
    for (viskores::Id i = 0; i < length; ++i)
    {
      VISKORES_TEST_ASSERT(inputPortal.Get(i) == static_cast<viskores::Id>(resultPortal.Get(i)),
                           "Casting ArrayHandle Failed");
    }
  }
};

void Run()
{
  using CastTypesToTest = viskores::List<viskores::Int32, viskores::UInt32>;

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleCast as Input" << std::endl;
  viskores::testing::Testing::TryTypes(TestCastAsInput(), CastTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleCast as Output" << std::endl;
  viskores::testing::Testing::TryTypes(TestCastAsOutput(), CastTypesToTest());
}

} // anonymous namespace

int UnitTestArrayHandleCast(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
