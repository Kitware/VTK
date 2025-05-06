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

#include <viskores/cont/ArrayHandleZip.h>

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

struct InplaceFunctorPair : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldInOut);
  using ExecutionSignature = void(_1);

  template <typename T>
  VISKORES_EXEC void operator()(viskores::Pair<T, T>& value) const
  {
    value.second = value.first;
  }
};

struct TestZipAsInput
{
  viskores::cont::Invoker Invoke;

  template <typename KeyType, typename ValueType>
  VISKORES_CONT void operator()(viskores::Pair<KeyType, ValueType> viskoresNotUsed(pair)) const
  {
    using PairType = viskores::Pair<KeyType, ValueType>;
    using KeyComponentType = typename viskores::VecTraits<KeyType>::ComponentType;
    using ValueComponentType = typename viskores::VecTraits<ValueType>::ComponentType;

    KeyType testKeys[ARRAY_SIZE];
    ValueType testValues[ARRAY_SIZE];

    for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      testKeys[i] = KeyType(static_cast<KeyComponentType>(ARRAY_SIZE - i));
      testValues[i] = ValueType(static_cast<ValueComponentType>(i));
    }
    viskores::cont::ArrayHandle<KeyType> keys =
      viskores::cont::make_ArrayHandle(testKeys, ARRAY_SIZE, viskores::CopyFlag::Off);
    viskores::cont::ArrayHandle<ValueType> values =
      viskores::cont::make_ArrayHandle(testValues, ARRAY_SIZE, viskores::CopyFlag::Off);

    viskores::cont::ArrayHandleZip<viskores::cont::ArrayHandle<KeyType>,
                                   viskores::cont::ArrayHandle<ValueType>>
      zip = viskores::cont::make_ArrayHandleZip(keys, values);

    viskores::cont::ArrayHandle<PairType> result;

    this->Invoke(PassThrough{}, zip, result);

    //verify that the control portal works
    auto resultPortal = result.ReadPortal();
    for (int i = 0; i < ARRAY_SIZE; ++i)
    {
      const PairType result_v = resultPortal.Get(i);
      const PairType correct_value(KeyType(static_cast<KeyComponentType>(ARRAY_SIZE - i)),
                                   ValueType(static_cast<ValueComponentType>(i)));
      VISKORES_TEST_ASSERT(test_equal(result_v, correct_value), "ArrayHandleZip Failed as input");
    }

    zip.ReleaseResources();
  }
};

struct TestZipAsOutput
{
  viskores::cont::Invoker Invoke;

  template <typename KeyType, typename ValueType>
  VISKORES_CONT void operator()(viskores::Pair<KeyType, ValueType> viskoresNotUsed(pair)) const
  {
    using PairType = viskores::Pair<KeyType, ValueType>;
    using KeyComponentType = typename viskores::VecTraits<KeyType>::ComponentType;
    using ValueComponentType = typename viskores::VecTraits<ValueType>::ComponentType;

    PairType testKeysAndValues[ARRAY_SIZE];
    for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      testKeysAndValues[i] = PairType(KeyType(static_cast<KeyComponentType>(ARRAY_SIZE - i)),
                                      ValueType(static_cast<ValueComponentType>(i)));
    }
    viskores::cont::ArrayHandle<PairType> input =
      viskores::cont::make_ArrayHandle(testKeysAndValues, ARRAY_SIZE, viskores::CopyFlag::Off);

    viskores::cont::ArrayHandle<KeyType> result_keys;
    viskores::cont::ArrayHandle<ValueType> result_values;
    viskores::cont::ArrayHandleZip<viskores::cont::ArrayHandle<KeyType>,
                                   viskores::cont::ArrayHandle<ValueType>>
      result_zip = viskores::cont::make_ArrayHandleZip(result_keys, result_values);

    this->Invoke(PassThrough{}, input, result_zip);

    //now the two arrays we have zipped should have data inside them
    auto keysPortal = result_keys.ReadPortal();
    auto valsPortal = result_values.ReadPortal();
    for (int i = 0; i < ARRAY_SIZE; ++i)
    {
      const KeyType result_key = keysPortal.Get(i);
      const ValueType result_value = valsPortal.Get(i);

      VISKORES_TEST_ASSERT(
        test_equal(result_key, KeyType(static_cast<KeyComponentType>(ARRAY_SIZE - i))),
        "ArrayHandleZip Failed as input for key");
      VISKORES_TEST_ASSERT(test_equal(result_value, ValueType(static_cast<ValueComponentType>(i))),
                           "ArrayHandleZip Failed as input for value");
    }

    // Test filling the zipped array.
    viskores::cont::printSummary_ArrayHandle(result_zip, std::cout, true);
    PairType fillValue{ TestValue(1, KeyType{}), TestValue(2, ValueType{}) };
    result_zip.Fill(fillValue, 1);
    viskores::cont::printSummary_ArrayHandle(result_zip, std::cout, true);
    keysPortal = result_keys.ReadPortal();
    valsPortal = result_values.ReadPortal();
    // First entry should be the same.
    VISKORES_TEST_ASSERT(
      test_equal(keysPortal.Get(0), KeyType(static_cast<KeyComponentType>(ARRAY_SIZE))));
    VISKORES_TEST_ASSERT(
      test_equal(valsPortal.Get(0), ValueType(static_cast<ValueComponentType>(0))));
    // The rest should be fillValue
    for (viskores::Id index = 1; index < ARRAY_SIZE; ++index)
    {
      const KeyType result_key = keysPortal.Get(index);
      const ValueType result_value = valsPortal.Get(index);

      VISKORES_TEST_ASSERT(test_equal(result_key, fillValue.first));
      VISKORES_TEST_ASSERT(test_equal(result_value, fillValue.second));
    }
  }
};

struct TestZipAsInPlace
{
  viskores::cont::Invoker Invoke;

  template <typename ValueType>
  VISKORES_CONT void operator()(ValueType) const
  {
    viskores::cont::ArrayHandle<ValueType> inputValues;
    inputValues.Allocate(ARRAY_SIZE);
    SetPortal(inputValues.WritePortal());

    viskores::cont::ArrayHandle<ValueType> outputValues;
    outputValues.Allocate(ARRAY_SIZE);

    this->Invoke(InplaceFunctorPair{},
                 viskores::cont::make_ArrayHandleZip(inputValues, outputValues));

    CheckPortal(outputValues.ReadPortal());
  }
};

void Run()
{
  using ZipTypesToTest = viskores::List<viskores::Pair<viskores::UInt8, viskores::Id>,
                                        viskores::Pair<viskores::Float64, viskores::Vec4ui_8>,
                                        viskores::Pair<viskores::Vec3f_32, viskores::Vec4i_8>>;
  using HandleTypesToTest =
    viskores::List<viskores::Id, viskores::Vec2i_32, viskores::FloatDefault, viskores::Vec3f_64>;

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleZip as Input" << std::endl;
  viskores::testing::Testing::TryTypes(TestZipAsInput(), ZipTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleZip as Output" << std::endl;
  viskores::testing::Testing::TryTypes(TestZipAsOutput(), ZipTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleZip as In Place" << std::endl;
  viskores::testing::Testing::TryTypes(TestZipAsInPlace(), HandleTypesToTest());
}

} // anonymous namespace

int UnitTestArrayHandleZip(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
