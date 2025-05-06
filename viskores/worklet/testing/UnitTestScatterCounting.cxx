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

#include <viskores/worklet/ScatterCounting.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DeviceAdapterAlgorithm.h>

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/Testing.h>

#include <vector>

namespace
{

struct TestScatterArrays
{
  viskores::cont::ArrayHandle<viskores::IdComponent> CountArray;
  viskores::cont::ArrayHandle<viskores::Id> InputToOutputMap;
  viskores::cont::ArrayHandle<viskores::Id> OutputToInputMap;
  viskores::cont::ArrayHandle<viskores::IdComponent> VisitArray;
};

TestScatterArrays MakeScatterArraysShort()
{
  TestScatterArrays arrays;
  arrays.CountArray = viskores::cont::make_ArrayHandle<viskores::IdComponent>(
    { 1, 2, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 });
  arrays.InputToOutputMap = viskores::cont::make_ArrayHandle<viskores::Id>(
    { 0, 1, 3, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6 });
  arrays.OutputToInputMap = viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 1, 1, 4, 6, 14 });
  arrays.VisitArray = viskores::cont::make_ArrayHandle<viskores::IdComponent>({ 0, 0, 1, 0, 0, 0 });

  return arrays;
}

TestScatterArrays MakeScatterArraysLong()
{
  TestScatterArrays arrays;
  arrays.CountArray = viskores::cont::make_ArrayHandle<viskores::IdComponent>({ 0, 1, 2, 3, 4, 5 });
  arrays.InputToOutputMap = viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 0, 1, 3, 6, 10 });
  arrays.OutputToInputMap =
    viskores::cont::make_ArrayHandle<viskores::Id>({ 1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5 });
  arrays.VisitArray = viskores::cont::make_ArrayHandle<viskores::IdComponent>(
    { 0, 0, 1, 0, 1, 2, 0, 1, 2, 3, 0, 1, 2, 3, 4 });

  return arrays;
}

TestScatterArrays MakeScatterArraysZero()
{
  TestScatterArrays arrays;
  arrays.CountArray = viskores::cont::make_ArrayHandle<viskores::IdComponent>({ 0, 0, 0, 0, 0, 0 });
  arrays.InputToOutputMap = viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 0, 0, 0, 0, 0 });
  arrays.OutputToInputMap.Allocate(0);
  arrays.VisitArray.Allocate(0);

  return arrays;
}

struct TestScatterCountingWorklet : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn inputIndices,
                                FieldOut copyIndices,
                                FieldOut recordVisit,
                                FieldOut recordWorkId);
  using ExecutionSignature = void(_1, _2, _3, _4, VisitIndex, WorkIndex);

  using ScatterType = viskores::worklet::ScatterCounting;

  template <typename CountArrayType>
  VISKORES_CONT static ScatterType MakeScatter(const CountArrayType& countArray)
  {
    return ScatterType(countArray);
  }

  VISKORES_EXEC
  void operator()(viskores::Id inputIndex,
                  viskores::Id& indexCopy,
                  viskores::IdComponent& writeVisit,
                  viskores::Float32& captureWorkId,
                  viskores::IdComponent visitIndex,
                  viskores::Id workId) const
  {
    indexCopy = inputIndex;
    writeVisit = visitIndex;
    captureWorkId = TestValue(workId, viskores::Float32());
  }
};

template <typename T>
void CompareArrays(viskores::cont::ArrayHandle<T> array1, viskores::cont::ArrayHandle<T> array2)
{
  using PortalType = typename viskores::cont::ArrayHandle<T>::ReadPortalType;
  PortalType portal1 = array1.ReadPortal();
  PortalType portal2 = array2.ReadPortal();

  VISKORES_TEST_ASSERT(portal1.GetNumberOfValues() == portal2.GetNumberOfValues(),
                       "Arrays are not the same length.");

  for (viskores::Id index = 0; index < portal1.GetNumberOfValues(); index++)
  {
    T value1 = portal1.Get(index);
    T value2 = portal2.Get(index);
    VISKORES_TEST_ASSERT(value1 == value2, "Array values not equal.");
  }
}

// This unit test makes sure the ScatterCounting generates the correct map
// and visit arrays.
void TestScatterArrayGeneration(const TestScatterArrays& arrays)
{
  std::cout << "  Testing array generation" << std::endl;

  viskores::worklet::ScatterCounting scatter(
    arrays.CountArray, viskores::cont::DeviceAdapterTagAny(), true);

  viskores::Id inputSize = arrays.CountArray.GetNumberOfValues();

  std::cout << "    Checking input to output map." << std::endl;
  CompareArrays(arrays.InputToOutputMap, scatter.GetInputToOutputMap());

  std::cout << "    Checking output to input map." << std::endl;
  CompareArrays(arrays.OutputToInputMap, scatter.GetOutputToInputMap(inputSize));

  std::cout << "    Checking visit array." << std::endl;
  CompareArrays(arrays.VisitArray, scatter.GetVisitArray(inputSize));
}

// This is more of an integration test that makes sure the scatter works with a
// worklet invocation.
void TestScatterWorklet(const TestScatterArrays& arrays)
{
  std::cout << "  Testing scatter counting in a worklet." << std::endl;

  viskores::worklet::DispatcherMapField<TestScatterCountingWorklet> dispatcher(
    TestScatterCountingWorklet::MakeScatter(arrays.CountArray));

  viskores::Id inputSize = arrays.CountArray.GetNumberOfValues();
  viskores::cont::ArrayHandleIndex inputIndices(inputSize);
  viskores::cont::ArrayHandle<viskores::Id> outputToInputMapCopy;
  viskores::cont::ArrayHandle<viskores::IdComponent> visitCopy;
  viskores::cont::ArrayHandle<viskores::Float32> captureWorkId;

  std::cout << "    Invoke worklet" << std::endl;
  dispatcher.Invoke(inputIndices, outputToInputMapCopy, visitCopy, captureWorkId);

  std::cout << "    Check output to input map." << std::endl;
  CompareArrays(outputToInputMapCopy, arrays.OutputToInputMap);
  std::cout << "    Check visit." << std::endl;
  CompareArrays(visitCopy, arrays.VisitArray);
  std::cout << "    Check work id." << std::endl;
  CheckPortal(captureWorkId.ReadPortal());
}

void TestScatterCountingWithArrays(const TestScatterArrays& arrays)
{
  TestScatterArrayGeneration(arrays);
  TestScatterWorklet(arrays);
}

void TestScatterCounting()
{
  std::cout << "Testing arrays with output smaller than input." << std::endl;
  TestScatterCountingWithArrays(MakeScatterArraysShort());

  std::cout << "Testing arrays with output larger than input." << std::endl;
  TestScatterCountingWithArrays(MakeScatterArraysLong());

  std::cout << "Testing arrays with zero output." << std::endl;
  TestScatterCountingWithArrays(MakeScatterArraysZero());
}

} // anonymous namespace

int UnitTestScatterCounting(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestScatterCounting, argc, argv);
}
