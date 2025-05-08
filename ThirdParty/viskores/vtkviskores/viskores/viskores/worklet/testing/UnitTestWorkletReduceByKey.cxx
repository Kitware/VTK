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

#include <viskores/worklet/DispatcherReduceByKey.h>
#include <viskores/worklet/WorkletReduceByKey.h>

#include <viskores/worklet/Keys.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/DeviceAdapterTag.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

#define STRINGIFY(x) STRINGIFY_IMPL(x)
#define STRINGIFY_IMPL(x) #x

#define TEST_ASSERT_WORKLET(condition)                                                           \
  do                                                                                             \
  {                                                                                              \
    if (!(condition))                                                                            \
    {                                                                                            \
      this->RaiseError("Test assert failed: " #condition "\n" __FILE__ ":" STRINGIFY(__LINE__)); \
      return;                                                                                    \
    }                                                                                            \
  } while (false)

#define ARRAY_SIZE 1033
#define GROUP_SIZE 10
#define NUM_UNIQUE (viskores::Id)(ARRAY_SIZE / GROUP_SIZE)

struct CheckKeyValuesWorklet : viskores::worklet::WorkletReduceByKey
{
  using ControlSignature = void(KeysIn keys,
                                ValuesIn keyMirror,
                                ValuesIn indexValues,
                                ValuesInOut valuesToModify,
                                ValuesOut writeKey);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, WorkIndex, ValueCount);
  using InputDomain = _1;

  template <typename T,
            typename KeyMirrorVecType,
            typename IndexValuesVecType,
            typename ValuesToModifyVecType,
            typename WriteKeysVecType>
  VISKORES_EXEC void operator()(const T& key,
                                const KeyMirrorVecType& keyMirror,
                                const IndexValuesVecType& valueIndices,
                                ValuesToModifyVecType& valuesToModify,
                                WriteKeysVecType& writeKey,
                                viskores::Id workIndex,
                                viskores::IdComponent numValues) const
  {
    // These tests only work if keys are in sorted order, which is how we group
    // them.

    TEST_ASSERT_WORKLET(key == TestValue(workIndex, T()));

    TEST_ASSERT_WORKLET(numValues >= GROUP_SIZE);
    TEST_ASSERT_WORKLET(keyMirror.GetNumberOfComponents() == numValues);
    TEST_ASSERT_WORKLET(valueIndices.GetNumberOfComponents() == numValues);
    TEST_ASSERT_WORKLET(valuesToModify.GetNumberOfComponents() == numValues);
    TEST_ASSERT_WORKLET(writeKey.GetNumberOfComponents() == numValues);

    for (viskores::IdComponent iComponent = 0; iComponent < numValues; iComponent++)
    {
      TEST_ASSERT_WORKLET(test_equal(keyMirror[iComponent], key));
      TEST_ASSERT_WORKLET(valueIndices[iComponent] % NUM_UNIQUE == workIndex);

      T value = valuesToModify[iComponent];
      valuesToModify[iComponent] = static_cast<T>(key + value);

      writeKey[iComponent] = key;
    }
  }
};

struct CheckReducedValuesWorklet : viskores::worklet::WorkletReduceByKey
{
  using ControlSignature = void(KeysIn,
                                ReducedValuesOut extractKeys,
                                ReducedValuesIn indexReference,
                                ReducedValuesInOut copyKeyPair);
  using ExecutionSignature = void(_1, _2, _3, _4, WorkIndex);

  template <typename T>
  VISKORES_EXEC void operator()(const T& key,
                                T& reducedValueOut,
                                viskores::Id indexReference,
                                viskores::Pair<T, T>& copyKeyPair,
                                viskores::Id workIndex) const
  {
    // This check only work if keys are in sorted order, which is how we group
    // them.
    TEST_ASSERT_WORKLET(key == TestValue(workIndex, T()));

    reducedValueOut = key;

    TEST_ASSERT_WORKLET(indexReference == workIndex);

    TEST_ASSERT_WORKLET(copyKeyPair.first == key);
    copyKeyPair.second = key;
  }
};

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
  viskores::cont::printSummary_ArrayHandle(keys.GetUniqueKeys(), std::cout);
  viskores::cont::printSummary_ArrayHandle(keys.GetOffsets(), std::cout);

  viskores::cont::ArrayHandle<KeyType> valuesToModify;
  valuesToModify.Allocate(ARRAY_SIZE);
  SetPortal(valuesToModify.WritePortal());

  viskores::cont::ArrayHandle<KeyType> writeKey;

  viskores::worklet::DispatcherReduceByKey<CheckKeyValuesWorklet> dispatcherCheckKeyValues;
  dispatcherCheckKeyValues.Invoke(
    keys, keyArray, viskores::cont::ArrayHandleIndex(ARRAY_SIZE), valuesToModify, writeKey);

  VISKORES_TEST_ASSERT(valuesToModify.GetNumberOfValues() == ARRAY_SIZE, "Bad array size.");
  VISKORES_TEST_ASSERT(writeKey.GetNumberOfValues() == ARRAY_SIZE, "Bad array size.");
  for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
  {
    KeyType key = TestValue(index % NUM_UNIQUE, KeyType());
    KeyType value = TestValue(index, KeyType());

    VISKORES_TEST_ASSERT(
      test_equal(static_cast<KeyType>(key + value), valuesToModify.ReadPortal().Get(index)),
      "Bad in/out value.");

    VISKORES_TEST_ASSERT(test_equal(key, writeKey.ReadPortal().Get(index)), "Bad out value.");
  }

  viskores::cont::ArrayHandle<KeyType> keyPairIn;
  keyPairIn.Allocate(NUM_UNIQUE);
  SetPortal(keyPairIn.WritePortal());

  viskores::cont::ArrayHandle<KeyType> keyPairOut;
  keyPairOut.Allocate(NUM_UNIQUE);

  viskores::worklet::DispatcherReduceByKey<CheckReducedValuesWorklet> dispatcherCheckReducedValues;
  dispatcherCheckReducedValues.Invoke(keys,
                                      writeKey,
                                      viskores::cont::ArrayHandleIndex(NUM_UNIQUE),
                                      viskores::cont::make_ArrayHandleZip(keyPairIn, keyPairOut));

  VISKORES_TEST_ASSERT(writeKey.GetNumberOfValues() == NUM_UNIQUE,
                       "Reduced values output not sized correctly.");
  CheckPortal(writeKey.ReadPortal());

  CheckPortal(keyPairOut.ReadPortal());
}

void TestReduceByKey(viskores::cont::DeviceAdapterId id)
{
  std::cout << "Testing Map Field on device adapter: " << id.GetName() << std::endl;

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

int UnitTestWorkletReduceByKey(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::RunOnDevice(TestReduceByKey, argc, argv);
}
