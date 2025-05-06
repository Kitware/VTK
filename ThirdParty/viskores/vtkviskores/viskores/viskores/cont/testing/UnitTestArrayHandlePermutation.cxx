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

#include <viskores/cont/ArrayHandlePermutation.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleImplicit.h>
#include <viskores/cont/Invoker.h>

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/Testing.h>

#include <vector>

namespace
{

const viskores::Id ARRAY_SIZE = 10;

struct DoubleIndexFunctor
{
  VISKORES_EXEC_CONT
  viskores::Id operator()(viskores::Id index) const { return 2 * index; }
};

using DoubleIndexArrayType = viskores::cont::ArrayHandleImplicit<DoubleIndexFunctor>;

struct CheckPermutationWorklet : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn permutationArray);
  using ExecutionSignature = void(WorkIndex, _1);

  template <typename T>
  VISKORES_EXEC void operator()(viskores::Id index, const T& value) const
  {
    viskores::Id permutedIndex = 2 * index;
    T expectedValue = TestValue(permutedIndex, T());

    if (!test_equal(value, expectedValue))
    {
      this->RaiseError("Encountered bad transformed value.");
    }
  }
};

struct InPlacePermutationWorklet : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldInOut permutationArray);

  template <typename T>
  VISKORES_EXEC void operator()(T& value) const
  {
    value = value + T(1000);
  }
};

template <typename PortalType>
VISKORES_CONT void CheckInPlaceResult(PortalType portal)
{
  using T = typename PortalType::ValueType;
  for (viskores::Id permutedIndex = 0; permutedIndex < 2 * ARRAY_SIZE; permutedIndex++)
  {
    if (permutedIndex % 2 == 0)
    {
      // This index was part of the permuted array; has a value changed
      T expectedValue = TestValue(permutedIndex, T()) + T(1000);
      T retrievedValue = portal.Get(permutedIndex);
      VISKORES_TEST_ASSERT(test_equal(expectedValue, retrievedValue),
                           "Permuted set unexpected value.");
    }
    else
    {
      // This index was not part of the permuted array; has original value
      T expectedValue = TestValue(permutedIndex, T());
      T retrievedValue = portal.Get(permutedIndex);
      VISKORES_TEST_ASSERT(test_equal(expectedValue, retrievedValue),
                           "Permuted array modified value it should not have.");
    }
  }
}

struct OutputPermutationWorklet : viskores::worklet::WorkletMapField
{
  // Note: Using a FieldOut for the input domain is rare (and mostly discouraged),
  // but it works as long as the array is allocated to the size desired.
  using ControlSignature = void(FieldOut permutationArray);
  using ExecutionSignature = void(WorkIndex, _1);

  template <typename T>
  VISKORES_EXEC void operator()(viskores::Id index, T& value) const
  {
    value = TestValue(static_cast<viskores::Id>(index), T());
  }
};

template <typename PortalType>
VISKORES_CONT void CheckOutputResult(PortalType portal)
{
  using T = typename PortalType::ValueType;
  for (viskores::IdComponent permutedIndex = 0; permutedIndex < 2 * ARRAY_SIZE; permutedIndex++)
  {
    if (permutedIndex % 2 == 0)
    {
      // This index was part of the permuted array; has a value changed
      viskores::Id originalIndex = permutedIndex / 2;
      T expectedValue = TestValue(originalIndex, T());
      T retrievedValue = portal.Get(permutedIndex);
      VISKORES_TEST_ASSERT(test_equal(expectedValue, retrievedValue),
                           "Permuted set unexpected value.");
    }
    else
    {
      // This index was not part of the permuted array; has original value
      T expectedValue = TestValue(permutedIndex, T());
      T retrievedValue = portal.Get(permutedIndex);
      VISKORES_TEST_ASSERT(test_equal(expectedValue, retrievedValue),
                           "Permuted array modified value it should not have.");
    }
  }
}

template <typename ValueType>
struct PermutationTests
{
  using IndexArrayType = viskores::cont::ArrayHandleImplicit<DoubleIndexFunctor>;
  using ValueArrayType = viskores::cont::ArrayHandle<ValueType, viskores::cont::StorageTagBasic>;
  using PermutationArrayType =
    viskores::cont::ArrayHandlePermutation<IndexArrayType, ValueArrayType>;

  ValueArrayType MakeValueArray() const
  {
    // Allocate a buffer and set initial values
    std::vector<ValueType> buffer(2 * ARRAY_SIZE);
    for (viskores::IdComponent index = 0; index < 2 * ARRAY_SIZE; index++)
    {
      viskores::UInt32 i = static_cast<viskores::UInt32>(index);
      buffer[i] = TestValue(index, ValueType());
    }

    // Create an ArrayHandle from the buffer
    return viskores::cont::make_ArrayHandle(buffer, viskores::CopyFlag::On);
  }

  void operator()() const
  {
    std::cout << "Create ArrayHandlePermutation" << std::endl;
    IndexArrayType indexArray(DoubleIndexFunctor(), ARRAY_SIZE);

    ValueArrayType valueArray = this->MakeValueArray();

    PermutationArrayType permutationArray(indexArray, valueArray);

    VISKORES_TEST_ASSERT(permutationArray.GetNumberOfValues() == ARRAY_SIZE,
                         "Permutation array wrong size.");
    VISKORES_TEST_ASSERT(permutationArray.WritePortal().GetNumberOfValues() == ARRAY_SIZE,
                         "Permutation portal wrong size.");
    VISKORES_TEST_ASSERT(permutationArray.ReadPortal().GetNumberOfValues() == ARRAY_SIZE,
                         "Permutation portal wrong size.");
    VISKORES_TEST_ASSERT(permutationArray.GetNumberOfComponentsFlat() ==
                         viskores::VecFlat<ValueType>::NUM_COMPONENTS);

    viskores::cont::Invoker invoke;

    std::cout << "Test initial values in execution environment" << std::endl;
    invoke(CheckPermutationWorklet{}, permutationArray);

    std::cout << "Try in place operation" << std::endl;
    invoke(InPlacePermutationWorklet{}, permutationArray);
    CheckInPlaceResult(valueArray.WritePortal());
    CheckInPlaceResult(valueArray.ReadPortal());

    std::cout << "Try output operation" << std::endl;
    invoke(OutputPermutationWorklet{}, permutationArray);
    CheckOutputResult(valueArray.ReadPortal());
    CheckOutputResult(valueArray.WritePortal());
  }
};

struct TryInputType
{
  template <typename InputType>
  void operator()(InputType) const
  {
    PermutationTests<InputType>()();
  }
};

void TestArrayHandlePermutation()
{
  viskores::testing::Testing::TryTypes(TryInputType(), viskores::TypeListCommon());
}

} // anonymous namespace

int UnitTestArrayHandlePermutation(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestArrayHandlePermutation, argc, argv);
}
