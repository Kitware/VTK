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

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleDiscard.h>
#include <viskores/cont/DeviceAdapterAlgorithm.h>

#include <viskores/cont/serial/internal/DeviceAdapterAlgorithmSerial.h>
#include <viskores/cont/serial/internal/DeviceAdapterTagSerial.h>

#include <viskores/cont/testing/Testing.h>

#include <viskores/BinaryOperators.h>

#include <algorithm>

namespace UnitTestArrayHandleDiscardDetail
{

template <typename ValueType>
struct Test
{
  static constexpr viskores::Id ARRAY_SIZE = 100;
  static constexpr viskores::Id NUM_KEYS = 3;

  using DeviceTag = viskores::cont::DeviceAdapterTagSerial;
  using Algorithm = viskores::cont::DeviceAdapterAlgorithm<DeviceTag>;
  using Handle = viskores::cont::ArrayHandle<ValueType>;
  using DiscardHandle = viskores::cont::ArrayHandleDiscard<ValueType>;
  using OutputPortal = typename Handle::WritePortalType;
  using ReduceOp = viskores::Add;

  // Test discard arrays by using the ReduceByKey algorithm. Two regular arrays
  // handles are provided as inputs, but the keys_output array is a discard
  // array handle. The values_output array should still be populated correctly.
  void TestReduceByKey()
  {
    // The reduction operator:
    ReduceOp op;

    // Prepare inputs / reference data:
    ValueType keyData[ARRAY_SIZE];
    ValueType valueData[ARRAY_SIZE];
    ValueType refData[NUM_KEYS];
    std::fill(refData, refData + NUM_KEYS, ValueType(0));
    for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      const viskores::Id key = i % NUM_KEYS;
      keyData[i] = static_cast<ValueType>(key);
      valueData[i] = static_cast<ValueType>(i * 2);
      refData[key] = op(refData[key], valueData[i]);
    }

    // Prepare array handles:
    Handle keys = viskores::cont::make_ArrayHandle(keyData, ARRAY_SIZE, viskores::CopyFlag::Off);
    Handle values =
      viskores::cont::make_ArrayHandle(valueData, ARRAY_SIZE, viskores::CopyFlag::Off);
    DiscardHandle output_keys;
    Handle output_values;

    Algorithm::SortByKey(keys, values);
    Algorithm::ReduceByKey(keys, values, output_keys, output_values, op);

    OutputPortal outputs = output_values.WritePortal();

    VISKORES_TEST_ASSERT(outputs.GetNumberOfValues() == NUM_KEYS,
                         "Unexpected number of output values from ReduceByKey.");

    for (viskores::Id i = 0; i < NUM_KEYS; ++i)
    {
      VISKORES_TEST_ASSERT(test_equal(outputs.Get(i), refData[i]),
                           "Unexpected output value after ReduceByKey.");
    }
  }

  void TestPrepareExceptions()
  {
    viskores::cont::Token token;
    DiscardHandle handle;
    handle.Allocate(50);

    try
    {
      handle.PrepareForInput(DeviceTag(), token);
    }
    catch (viskores::cont::ErrorBadValue&)
    {
      // Expected failure.
    }

    try
    {
      handle.PrepareForInPlace(DeviceTag(), token);
    }
    catch (viskores::cont::ErrorBadValue&)
    {
      // Expected failure.
    }

    // Shouldn't fail:
    handle.PrepareForOutput(ARRAY_SIZE, DeviceTag(), token);
  }

  void TestFill()
  {
    DiscardHandle array;
    array.AllocateAndFill(ARRAY_SIZE, ValueType{ 0 });
    VISKORES_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE);
  }

  void operator()()
  {
    TestReduceByKey();
    TestPrepareExceptions();
    TestFill();
  }
};

void TestArrayHandleDiscard()
{
  Test<viskores::UInt8>()();
  Test<viskores::Int16>()();
  Test<viskores::Int32>()();
  Test<viskores::Int64>()();
  Test<viskores::Float32>()();
  Test<viskores::Float64>()();
}

} // end namespace UnitTestArrayHandleDiscardDetail

int UnitTestArrayHandleDiscard(int argc, char* argv[])
{
  using namespace UnitTestArrayHandleDiscardDetail;
  return viskores::cont::testing::Testing::Run(TestArrayHandleDiscard, argc, argv);
}
