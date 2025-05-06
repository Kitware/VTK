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
#include <viskores/cont/ArrayHandleCounting.h>

#include <viskores/cont/testing/Testing.h>

#include <string>

namespace UnitTestArrayHandleCountingNamespace
{

const viskores::Id ARRAY_SIZE = 10;

} // namespace UnitTestArrayHandleCountingNamespace

namespace UnitTestArrayHandleCountingNamespace
{

template <typename ValueType>
struct TemplatedTests
{
  using ArrayHandleType = viskores::cont::ArrayHandleCounting<ValueType>;
  using PortalType = typename ArrayHandleType::ReadPortalType;

  void operator()(const ValueType& startingValue, const ValueType& step)
  {
    ArrayHandleType arrayConst(startingValue, step, ARRAY_SIZE);

    ArrayHandleType arrayMake =
      viskores::cont::make_ArrayHandleCounting(startingValue, step, ARRAY_SIZE);

    typename ArrayHandleType::Superclass arrayHandle =
      ArrayHandleType(startingValue, step, ARRAY_SIZE);

    VISKORES_TEST_ASSERT(arrayConst.GetNumberOfValues() == ARRAY_SIZE,
                         "Counting array using constructor has wrong size.");

    VISKORES_TEST_ASSERT(arrayMake.GetNumberOfValues() == ARRAY_SIZE,
                         "Counting array using make has wrong size.");

    VISKORES_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE,
                         "Counting array using raw array handle + tag has wrong size.");

    ValueType properValue = startingValue;
    auto arrayConstPortal = arrayConst.ReadPortal();
    auto arrayMakePortal = arrayMake.ReadPortal();
    auto arrayHandlePortal = arrayHandle.ReadPortal();
    for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
    {
      VISKORES_TEST_ASSERT(arrayConstPortal.Get(index) == properValue,
                           "Counting array using constructor has unexpected value.");
      VISKORES_TEST_ASSERT(arrayMakePortal.Get(index) == properValue,
                           "Counting array using make has unexpected value.");

      VISKORES_TEST_ASSERT(arrayHandlePortal.Get(index) == properValue,
                           "Counting array using raw array handle + tag has unexpected value.");
      properValue = properValue + step;
    }
  }
};

void TestArrayHandleCounting()
{
  TemplatedTests<viskores::Id>()(0, 1);
  TemplatedTests<viskores::Id>()(8, 2);
  TemplatedTests<viskores::Float32>()(0.0f, 1.0f);
  TemplatedTests<viskores::Float32>()(3.0f, -0.5f);
  TemplatedTests<viskores::Float64>()(0.0, 1.0);
  TemplatedTests<viskores::Float64>()(-3.0, 2.0);
}


} // namespace UnitTestArrayHandleCountingNamespace

int UnitTestArrayHandleCounting(int argc, char* argv[])
{
  using namespace UnitTestArrayHandleCountingNamespace;
  return viskores::cont::testing::Testing::Run(TestArrayHandleCounting, argc, argv);
}
