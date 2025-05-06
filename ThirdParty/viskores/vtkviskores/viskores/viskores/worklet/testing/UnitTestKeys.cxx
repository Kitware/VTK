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

#include <viskores/worklet/Keys.h>

#include <viskores/cont/ArrayCopy.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

static constexpr viskores::Id ARRAY_SIZE = 1033;
static constexpr viskores::Id NUM_UNIQUE = ARRAY_SIZE / 10;

template <typename KeyPortal, typename IdPortal>
void CheckKeyReduce(const KeyPortal& originalKeys,
                    const KeyPortal& uniqueKeys,
                    const IdPortal& sortedValuesMap,
                    const IdPortal& offsets)
{
  using KeyType = typename KeyPortal::ValueType;
  viskores::Id originalSize = originalKeys.GetNumberOfValues();
  viskores::Id uniqueSize = uniqueKeys.GetNumberOfValues();
  VISKORES_TEST_ASSERT(originalSize == sortedValuesMap.GetNumberOfValues(),
                       "Inconsistent array size.");
  VISKORES_TEST_ASSERT(uniqueSize == offsets.GetNumberOfValues() - 1, "Inconsistent array size.");

  for (viskores::Id uniqueIndex = 0; uniqueIndex < uniqueSize; uniqueIndex++)
  {
    KeyType key = uniqueKeys.Get(uniqueIndex);
    viskores::Id offset = offsets.Get(uniqueIndex);
    viskores::IdComponent groupCount =
      static_cast<viskores::IdComponent>(offsets.Get(uniqueIndex + 1) - offset);
    for (viskores::IdComponent groupIndex = 0; groupIndex < groupCount; groupIndex++)
    {
      viskores::Id originalIndex = sortedValuesMap.Get(offset + groupIndex);
      KeyType originalKey = originalKeys.Get(originalIndex);
      VISKORES_TEST_ASSERT(key == originalKey, "Bad key lookup.");
    }
  }
}

template <typename KeyType>
void TryKeyType(KeyType)
{
  KeyType keyBuffer[ARRAY_SIZE];
  for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
  {
    keyBuffer[index] = TestValue(index % NUM_UNIQUE, KeyType());
  }

  viskores::cont::ArrayHandle<KeyType> keyArray =
    viskores::cont::make_ArrayHandle(keyBuffer, ARRAY_SIZE, viskores::CopyFlag::On);

  viskores::cont::ArrayHandle<KeyType> sortedKeys;
  viskores::cont::ArrayCopy(keyArray, sortedKeys);

  viskores::worklet::Keys<KeyType> keys(sortedKeys);
  VISKORES_TEST_ASSERT(keys.GetInputRange() == NUM_UNIQUE, "Keys has bad input range.");

  CheckKeyReduce(keyArray.ReadPortal(),
                 keys.GetUniqueKeys().ReadPortal(),
                 keys.GetSortedValuesMap().ReadPortal(),
                 keys.GetOffsets().ReadPortal());
}

void TestKeys()
{
  std::cout << "Testing viskores::Id keys." << std::endl;
  TryKeyType(viskores::Id());

  std::cout << "Testing viskores::IdComponent keys." << std::endl;
  TryKeyType(viskores::IdComponent());

  std::cout << "Testing viskores::UInt8 keys." << std::endl;
  TryKeyType(viskores::UInt8());

  std::cout << "Testing viskores::Id3 keys." << std::endl;
  TryKeyType(viskores::Id3());
}

} // anonymous namespace

int UnitTestKeys(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestKeys, argc, argv);
}
