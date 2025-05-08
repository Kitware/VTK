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

#include <viskores/worklet/AverageByKey.h>

#include <viskores/Hash.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleCounting.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

static constexpr viskores::Id NUM_UNIQUE = 100;
static constexpr viskores::Id NUM_PER_GROUP = 10;
static constexpr viskores::Id ARRAY_SIZE = NUM_UNIQUE * NUM_PER_GROUP;

template <typename KeyArray, typename ValueArray>
void CheckAverageByKey(const KeyArray& uniqueKeys, const ValueArray& averagedValues)
{
  VISKORES_IS_ARRAY_HANDLE(KeyArray);
  VISKORES_IS_ARRAY_HANDLE(ValueArray);

  using KeyType = typename KeyArray::ValueType;

  VISKORES_TEST_ASSERT(uniqueKeys.GetNumberOfValues() == NUM_UNIQUE, "Bad number of keys.");
  VISKORES_TEST_ASSERT(averagedValues.GetNumberOfValues() == NUM_UNIQUE, "Bad number of values.");

  // We expect the unique keys to be sorted, and for the test values to be in order.
  auto keyPortal = uniqueKeys.ReadPortal();
  auto valuePortal = averagedValues.ReadPortal();
  for (viskores::Id index = 0; index < NUM_UNIQUE; ++index)
  {
    VISKORES_TEST_ASSERT(keyPortal.Get(index) == TestValue(index % NUM_UNIQUE, KeyType()),
                         "Unexpected key.");

    viskores::FloatDefault expectedAverage = static_cast<viskores::FloatDefault>(
      NUM_PER_GROUP * ((NUM_PER_GROUP - 1) * NUM_PER_GROUP) / 2 + index);
    VISKORES_TEST_ASSERT(test_equal(expectedAverage, valuePortal.Get(index)), "Bad average.");
  }
}

template <typename KeyType>
void TryKeyType(KeyType)
{
  std::cout << "Testing with " << viskores::testing::TypeName<KeyType>::Name() << " keys."
            << std::endl;

  // Create key array
  KeyType keyBuffer[ARRAY_SIZE];
  for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
  {
    keyBuffer[index] = TestValue(index % NUM_UNIQUE, KeyType());
  }
  viskores::cont::ArrayHandle<KeyType> keysArray =
    viskores::cont::make_ArrayHandle(keyBuffer, ARRAY_SIZE, viskores::CopyFlag::Off);

  // Create Keys object
  viskores::cont::ArrayHandle<KeyType> sortedKeys;
  viskores::cont::ArrayCopy(keysArray, sortedKeys);
  viskores::worklet::Keys<KeyType> keys(sortedKeys);
  VISKORES_TEST_ASSERT(keys.GetInputRange() == NUM_UNIQUE, "Keys has bad input range.");

  // Create values array
  viskores::cont::ArrayHandle<viskores::FloatDefault> valuesArray;
  viskores::cont::ArrayCopy(viskores::cont::ArrayHandleIndex(ARRAY_SIZE), valuesArray);

  std::cout << "  Try average with Keys object" << std::endl;
  CheckAverageByKey(keys.GetUniqueKeys(), viskores::worklet::AverageByKey::Run(keys, valuesArray));

  std::cout << "  Try average with device adapter's reduce by keys" << std::endl;
  viskores::cont::ArrayHandle<KeyType> outputKeys;
  viskores::cont::ArrayHandle<viskores::FloatDefault> outputValues;
  viskores::worklet::AverageByKey::Run(keysArray, valuesArray, outputKeys, outputValues);
  CheckAverageByKey(outputKeys, outputValues);
}

void DoTest()
{
  TryKeyType(viskores::Id());
  TryKeyType(viskores::IdComponent());
  TryKeyType(viskores::UInt8());
  TryKeyType(viskores::HashType());
  TryKeyType(viskores::Id3());
}

} // anonymous namespace

int UnitTestAverageByKey(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DoTest, argc, argv);
}
