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

#include <viskores/worklet/MaskSelect.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DeviceAdapterAlgorithm.h>

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/Testing.h>

#include <vector>

namespace
{

constexpr viskores::Id nullValue = -2;

struct TestMaskArrays
{
  viskores::cont::ArrayHandle<viskores::IdComponent> SelectArray;
  viskores::cont::ArrayHandle<viskores::Id> ThreadToOutputMap;
};

TestMaskArrays MakeMaskArraysShort()
{
  TestMaskArrays arrays;

  arrays.SelectArray = viskores::cont::make_ArrayHandle<viskores::IdComponent>(
    { 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 });
  arrays.ThreadToOutputMap = viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 1, 6, 17 });

  return arrays;
}

TestMaskArrays MakeMaskArraysLong()
{
  TestMaskArrays arrays;

  arrays.SelectArray = viskores::cont::make_ArrayHandle<viskores::IdComponent>({
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
    1,
    1,
    1,
    1,
    1,
  });
  arrays.ThreadToOutputMap = viskores::cont::make_ArrayHandle<viskores::Id>(
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 14, 15 });

  return arrays;
}

TestMaskArrays MakeMaskArraysZero()
{
  TestMaskArrays arrays;

  arrays.SelectArray =
    viskores::cont::make_ArrayHandle<viskores::IdComponent>({ 0, 0, 0, 0, 0, 0 });
  arrays.ThreadToOutputMap.Allocate(0);

  return arrays;
}

struct TestMaskSelectWorklet : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn inputIndices, FieldInOut copyIndices);
  using ExecutionSignature = void(_1, _2);

  using MaskType = viskores::worklet::MaskSelect;

  VISKORES_EXEC
  void operator()(viskores::Id inputIndex, viskores::Id& indexCopy) const
  {
    indexCopy = inputIndex;
  }
};

template <typename T, typename SelectArrayType>
void CompareArrays(viskores::cont::ArrayHandle<T> array1,
                   viskores::cont::ArrayHandle<T> array2,
                   const SelectArrayType& selectArray)
{
  VISKORES_IS_ARRAY_HANDLE(SelectArrayType);

  auto portal1 = array1.ReadPortal();
  auto portal2 = array2.ReadPortal();
  auto selectPortal = selectArray.ReadPortal();

  VISKORES_TEST_ASSERT(portal1.GetNumberOfValues() == portal2.GetNumberOfValues());
  VISKORES_TEST_ASSERT(portal1.GetNumberOfValues() == selectArray.GetNumberOfValues());

  for (viskores::Id index = 0; index < portal1.GetNumberOfValues(); index++)
  {
    if (selectPortal.Get(index))
    {
      T value1 = portal1.Get(index);
      T value2 = portal2.Get(index);
      VISKORES_TEST_ASSERT(
        value1 == value2, "Array values not equal (", index, ": ", value1, " ", value2, ")");
    }
    else
    {
      T value = portal2.Get(index);
      VISKORES_TEST_ASSERT(value == nullValue, "Expected null value, got ", value);
    }
  }
}

template <typename T>
void CompareArrays(viskores::cont::ArrayHandle<T> array1, viskores::cont::ArrayHandle<T> array2)
{
  CompareArrays(array1,
                array2,
                viskores::cont::make_ArrayHandleConstant<bool>(true, array1.GetNumberOfValues()));
}

// This unit test makes sure the ScatterCounting generates the correct map
// and visit arrays.
void TestMaskArrayGeneration(const TestMaskArrays& arrays)
{
  std::cout << "  Testing array generation" << std::endl;

  viskores::worklet::MaskSelect mask(arrays.SelectArray, viskores::cont::DeviceAdapterTagAny());

  viskores::Id inputSize = arrays.SelectArray.GetNumberOfValues();

  std::cout << "    Checking thread to output map ";
  viskores::cont::printSummary_ArrayHandle(mask.GetThreadToOutputMap(inputSize), std::cout);
  CompareArrays(arrays.ThreadToOutputMap, mask.GetThreadToOutputMap(inputSize));
}

// This is more of an integration test that makes sure the scatter works with a
// worklet invocation.
void TestMaskWorklet(const TestMaskArrays& arrays)
{
  std::cout << "  Testing mask select in a worklet." << std::endl;

  viskores::worklet::DispatcherMapField<TestMaskSelectWorklet> dispatcher(
    viskores::worklet::MaskSelect(arrays.SelectArray));

  viskores::Id inputSize = arrays.SelectArray.GetNumberOfValues();

  viskores::cont::ArrayHandle<viskores::Id> inputIndices;
  viskores::cont::ArrayCopy(viskores::cont::ArrayHandleIndex(inputSize), inputIndices);

  viskores::cont::ArrayHandle<viskores::Id> selectIndexCopy;
  viskores::cont::ArrayCopy(viskores::cont::ArrayHandleConstant<viskores::Id>(nullValue, inputSize),
                            selectIndexCopy);

  std::cout << "    Invoke worklet" << std::endl;
  dispatcher.Invoke(inputIndices, selectIndexCopy);

  std::cout << "    Check copied indices." << std::endl;
  CompareArrays(inputIndices, selectIndexCopy, arrays.SelectArray);
}

void TestMaskSelectWithArrays(const TestMaskArrays& arrays)
{
  TestMaskArrayGeneration(arrays);
  TestMaskWorklet(arrays);
}

void TestMaskSelect()
{
  std::cout << "Testing arrays with output smaller than input." << std::endl;
  TestMaskSelectWithArrays(MakeMaskArraysShort());

  std::cout << "Testing arrays with output larger than input." << std::endl;
  TestMaskSelectWithArrays(MakeMaskArraysLong());

  std::cout << "Testing arrays with zero output." << std::endl;
  TestMaskSelectWithArrays(MakeMaskArraysZero());
}

} // anonymous namespace

int UnitTestMaskSelect(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestMaskSelect, argc, argv);
}
