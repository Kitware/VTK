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

#include <viskores/StaticAssert.h>

#include <viskores/cont/DeviceAdapterAlgorithm.h>
#include <viskores/testing/Testing.h>

#include <viskores/exec/FunctorBase.h>
#include <viskores/exec/arg/BasicArg.h>
#include <viskores/exec/arg/Fetch.h>
#include <viskores/exec/arg/ThreadIndicesBasic.h>

#include <viskores/internal/FunctionInterface.h>
#include <viskores/internal/Invocation.h>

#include <algorithm>
#include <vector>

namespace viskores
{
namespace exec
{
namespace internal
{
namespace testing
{

struct TestExecObject
{
  VISKORES_EXEC_CONT
  TestExecObject()
    : Values(nullptr)
  {
  }

  VISKORES_EXEC_CONT
  TestExecObject(std::vector<viskores::Id>& values)
    : Values(&values[0])
  {
  }

  VISKORES_EXEC_CONT
  TestExecObject(const TestExecObject& other) { Values = other.Values; }

  viskores::Id* Values;
};

struct MyOutputToInputMapPortal
{
  using ValueType = viskores::Id;
  VISKORES_EXEC_CONT
  viskores::Id Get(viskores::Id index) const { return index; }
};

struct MyVisitArrayPortal
{
  using ValueType = viskores::IdComponent;
  viskores::IdComponent Get(viskores::Id) const { return 1; }
};

struct MyThreadToOutputMapPortal
{
  using ValueType = viskores::Id;
  VISKORES_EXEC_CONT
  viskores::Id Get(viskores::Id index) const { return index; }
};

struct TestFetchTagInput
{
};
struct TestFetchTagOutput
{
};

// Missing TransportTag, but we are not testing that so we can leave it out.
struct TestControlSignatureTagInput
{
  using FetchTag = TestFetchTagInput;
};
struct TestControlSignatureTagOutput
{
  using FetchTag = TestFetchTagOutput;
};
}
}
}
}

namespace viskores
{
namespace exec
{
namespace arg
{

using namespace viskores::exec::internal::testing;

template <>
struct Fetch<TestFetchTagInput, viskores::exec::arg::AspectTagDefault, TestExecObject>
{
  using ValueType = viskores::Id;

  VISKORES_EXEC
  ValueType Load(const viskores::exec::arg::ThreadIndicesBasic& indices,
                 const TestExecObject& execObject) const
  {
    return execObject.Values[indices.GetInputIndex()] + 10 * indices.GetInputIndex();
  }

  VISKORES_EXEC
  void Store(const viskores::exec::arg::ThreadIndicesBasic&, const TestExecObject&, ValueType) const
  {
    // No-op
  }
};

template <>
struct Fetch<TestFetchTagOutput, viskores::exec::arg::AspectTagDefault, TestExecObject>
{
  using ValueType = viskores::Id;

  VISKORES_EXEC
  ValueType Load(const viskores::exec::arg::ThreadIndicesBasic&, const TestExecObject&) const
  {
    // No-op
    return ValueType();
  }

  VISKORES_EXEC
  void Store(const viskores::exec::arg::ThreadIndicesBasic& indices,
             const TestExecObject& execObject,
             ValueType value) const
  {
    execObject.Values[indices.GetOutputIndex()] = value + 20 * indices.GetOutputIndex();
  }
};
}
}
} // viskores::exec::arg

namespace viskores
{
namespace exec
{
namespace internal
{
namespace testing
{

using TestControlSignature = void(TestControlSignatureTagInput, TestControlSignatureTagOutput);
using TestControlInterface = viskores::internal::FunctionInterface<TestControlSignature>;

using TestExecutionSignature1 = void(viskores::exec::arg::BasicArg<1>,
                                     viskores::exec::arg::BasicArg<2>);
using TestExecutionInterface1 = viskores::internal::FunctionInterface<TestExecutionSignature1>;

using TestExecutionSignature2 = viskores::exec::arg::BasicArg<2>(viskores::exec::arg::BasicArg<1>);
using TestExecutionInterface2 = viskores::internal::FunctionInterface<TestExecutionSignature2>;

using ExecutionParameterInterface =
  viskores::internal::FunctionInterface<void(TestExecObject, TestExecObject)>;

using InvocationType1 = viskores::internal::Invocation<ExecutionParameterInterface,
                                                       TestControlInterface,
                                                       TestExecutionInterface1,
                                                       1,
                                                       MyOutputToInputMapPortal,
                                                       MyVisitArrayPortal,
                                                       MyThreadToOutputMapPortal>;

using InvocationType2 = viskores::internal::Invocation<ExecutionParameterInterface,
                                                       TestControlInterface,
                                                       TestExecutionInterface2,
                                                       1,
                                                       MyOutputToInputMapPortal,
                                                       MyVisitArrayPortal,
                                                       MyThreadToOutputMapPortal>;

// Not a full worklet, but provides operators that we expect in a worklet.
struct TestWorkletProxy : viskores::exec::FunctorBase
{
  VISKORES_EXEC
  void operator()(viskores::Id input, viskores::Id& output) const { output = input + 100; }

  VISKORES_EXEC
  viskores::Id operator()(viskores::Id input) const { return input + 200; }

  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutputArrayType,
            typename InputDomainType>
  VISKORES_EXEC viskores::exec::arg::ThreadIndicesBasic GetThreadIndices(
    const viskores::Id& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutputArrayType& threadToOut,
    const InputDomainType&) const
  {
    const viskores::Id outIndex = threadToOut.Get(threadIndex);
    return viskores::exec::arg::ThreadIndicesBasic(
      threadIndex, outToIn.Get(outIndex), visit.Get(outIndex), outIndex);
  }

  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType>
  VISKORES_EXEC viskores::exec::arg::ThreadIndicesBasic GetThreadIndices(
    const viskores::Id3& viskoresNotUsed(iterationSpace),
    const viskores::Id3& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutArrayType& threadToOut,
    const InputDomainType&) const
  {
    const viskores::Id flatThreadIndex = viskores::Dot(threadIndex, viskores::Id3(1, 8, 64));
    const viskores::Id outIndex = threadToOut.Get(flatThreadIndex);
    return viskores::exec::arg::ThreadIndicesBasic(
      flatThreadIndex, outToIn.Get(outIndex), visit.Get(outIndex), outIndex);
  }
};

#define ERROR_MESSAGE "Expected worklet error."

// Not a full worklet, but provides operators that we expect in a worklet.
struct TestWorkletErrorProxy : viskores::exec::FunctorBase
{
  VISKORES_EXEC
  void operator()(viskores::Id, viskores::Id) const { this->RaiseError(ERROR_MESSAGE); }

  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType>
  VISKORES_EXEC viskores::exec::arg::ThreadIndicesBasic GetThreadIndices(
    const viskores::Id& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutArrayType& threadToOut,
    const InputDomainType&) const
  {
    const viskores::Id outIndex = threadToOut.Get(threadIndex);
    return viskores::exec::arg::ThreadIndicesBasic(
      threadIndex, outToIn.Get(outIndex), visit.Get(outIndex), outIndex);
  }

  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutputArrayType,
            typename InputDomainType>
  VISKORES_EXEC viskores::exec::arg::ThreadIndicesBasic GetThreadIndices(
    const viskores::Id3& viskoresNotUsed(iterationSpace),
    const viskores::Id3& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutputArrayType& threadToOutput,
    const InputDomainType&) const
  {
    const viskores::Id index = viskores::Dot(threadIndex, viskores::Id3(1, 8, 64));
    const viskores::Id outputIndex = threadToOutput.Get(index);
    return viskores::exec::arg::ThreadIndicesBasic(
      index, outToIn.Get(outputIndex), visit.Get(outputIndex), outputIndex);
  }
};

template <typename DeviceAdapter>
void Test1DNormalTaskTilingInvoke()
{

  std::cout << "Testing TaskTiling1D." << std::endl;

  std::vector<viskores::Id> inputTestValues(100, 5);
  std::vector<viskores::Id> outputTestValues(100, static_cast<viskores::Id>(0xDEADDEAD));
  viskores::internal::FunctionInterface<void(TestExecObject, TestExecObject)> execObjects =
    viskores::internal::make_FunctionInterface<void>(TestExecObject(inputTestValues),
                                                     TestExecObject(outputTestValues));

  std::cout << "  Try void return." << std::endl;
  TestWorkletProxy worklet;
  InvocationType1 invocation1(execObjects);

  using TaskTypes = typename viskores::cont::DeviceTaskTypes<DeviceAdapter>;
  auto task1 = TaskTypes::MakeTask(worklet, invocation1, viskores::Id());

  viskores::exec::internal::ErrorMessageBuffer errorMessage(nullptr, 0);
  task1.SetErrorMessageBuffer(errorMessage);

  task1(0, 90);
  task1(90, 99);
  task1(99, 100); //verify single value ranges work

  for (std::size_t i = 0; i < 100; ++i)
  {
    VISKORES_TEST_ASSERT(inputTestValues[i] == 5, "Input value changed.");
    VISKORES_TEST_ASSERT(outputTestValues[i] ==
                           inputTestValues[i] + 100 + (30 * static_cast<viskores::Id>(i)),
                         "Output value not set right.");
  }

  std::cout << "  Try return value." << std::endl;
  std::fill(inputTestValues.begin(), inputTestValues.end(), 6);
  std::fill(
    outputTestValues.begin(), outputTestValues.end(), static_cast<viskores::Id>(0xDEADDEAD));

  InvocationType2 invocation2(execObjects);

  using TaskTypes = typename viskores::cont::DeviceTaskTypes<DeviceAdapter>;
  auto task2 = TaskTypes::MakeTask(worklet, invocation2, viskores::Id());

  task2.SetErrorMessageBuffer(errorMessage);

  task2(0, 0); //verify zero value ranges work
  task2(0, 90);
  task2(90, 100);

  task2(0, 100); //verify that you can invoke worklets multiple times

  for (std::size_t i = 0; i < 100; ++i)
  {
    VISKORES_TEST_ASSERT(inputTestValues[i] == 6, "Input value changed.");
    VISKORES_TEST_ASSERT(outputTestValues[i] ==
                           inputTestValues[i] + 200 + (30 * static_cast<viskores::Id>(i)),
                         "Output value not set right.");
  }
}

template <typename DeviceAdapter>
void Test1DErrorTaskTilingInvoke()
{

  std::cout << "Testing TaskTiling1D with an error raised in the worklet." << std::endl;

  std::vector<viskores::Id> inputTestValues(100, 5);
  std::vector<viskores::Id> outputTestValues(100, static_cast<viskores::Id>(0xDEADDEAD));

  TestExecObject arg1(inputTestValues);
  TestExecObject arg2(outputTestValues);

  viskores::internal::FunctionInterface<void(TestExecObject, TestExecObject)> execObjects =
    viskores::internal::make_FunctionInterface<void>(arg1, arg2);

  TestWorkletErrorProxy worklet;
  InvocationType1 invocation(execObjects);

  using TaskTypes = typename viskores::cont::DeviceTaskTypes<DeviceAdapter>;
  auto task = TaskTypes::MakeTask(worklet, invocation, viskores::Id());

  char message[1024];
  message[0] = '\0';
  viskores::exec::internal::ErrorMessageBuffer errorMessage(message, 1024);
  task.SetErrorMessageBuffer(errorMessage);

  task(0, 100);

  VISKORES_TEST_ASSERT(errorMessage.IsErrorRaised(), "Error not raised correctly.");
  VISKORES_TEST_ASSERT(message == std::string(ERROR_MESSAGE), "Got wrong error message.");
}

template <typename DeviceAdapter>
void Test3DNormalTaskTilingInvoke()
{
  std::cout << "Testing TaskTiling3D." << std::endl;

  std::vector<viskores::Id> inputTestValues((8 * 8 * 8), 5);
  std::vector<viskores::Id> outputTestValues((8 * 8 * 8), static_cast<viskores::Id>(0xDEADDEAD));
  viskores::internal::FunctionInterface<void(TestExecObject, TestExecObject)> execObjects =
    viskores::internal::make_FunctionInterface<void>(TestExecObject(inputTestValues),
                                                     TestExecObject(outputTestValues));

  std::cout << "  Try void return." << std::endl;

  TestWorkletProxy worklet;
  InvocationType1 invocation1(execObjects);

  using TaskTypes = typename viskores::cont::DeviceTaskTypes<DeviceAdapter>;
  auto task1 = TaskTypes::MakeTask(worklet, invocation1, viskores::Id3());
  for (viskores::Id k = 0; k < 8; ++k)
  {
    for (viskores::Id j = 0; j < 8; j += 2)
    {
      //verify that order is not required
      task1(viskores::Id3{ 8, 8, 8 }, 0, 8, j + 1, k);
      task1(viskores::Id3{ 8, 8, 8 }, 0, 8, j, k);
    }
  }

  for (std::size_t i = 0; i < (8 * 8 * 8); ++i)
  {
    VISKORES_TEST_ASSERT(inputTestValues[i] == 5, "Input value changed.");
    VISKORES_TEST_ASSERT(outputTestValues[i] ==
                           inputTestValues[i] + 100 + (30 * static_cast<viskores::Id>(i)),
                         "Output value not set right.");
  }

  std::cout << "  Try return value." << std::endl;
  std::fill(inputTestValues.begin(), inputTestValues.end(), 6);
  std::fill(
    outputTestValues.begin(), outputTestValues.end(), static_cast<viskores::Id>(0xDEADDEAD));

  InvocationType2 invocation2(execObjects);
  using TaskTypes = typename viskores::cont::DeviceTaskTypes<DeviceAdapter>;
  auto task2 = TaskTypes::MakeTask(worklet, invocation2, viskores::Id3());

  //verify that linear order of values being processed is not presumed
  for (viskores::Id i = 0; i < 8; ++i)
  {
    for (viskores::Id j = 0; j < 8; ++j)
    {
      for (viskores::Id k = 0; k < 8; ++k)
      {
        task2(viskores::Id3{ 8, 8, 8 }, i, i + 1, j, k);
      }
    }
  }

  for (std::size_t i = 0; i < (8 * 8 * 8); ++i)
  {
    VISKORES_TEST_ASSERT(inputTestValues[i] == 6, "Input value changed.");
    VISKORES_TEST_ASSERT(outputTestValues[i] ==
                           inputTestValues[i] + 200 + (30 * static_cast<viskores::Id>(i)),
                         "Output value not set right.");
  }
}

template <typename DeviceAdapter>
void Test3DErrorTaskTilingInvoke()
{
  std::cout << "Testing TaskTiling3D with an error raised in the worklet." << std::endl;

  std::vector<viskores::Id> inputTestValues((8 * 8 * 8), 5);
  std::vector<viskores::Id> outputTestValues((8 * 8 * 8), static_cast<viskores::Id>(0xDEADDEAD));
  viskores::internal::FunctionInterface<void(TestExecObject, TestExecObject)> execObjects =
    viskores::internal::make_FunctionInterface<void>(TestExecObject(inputTestValues),
                                                     TestExecObject(outputTestValues));

  TestWorkletErrorProxy worklet;
  InvocationType1 invocation(execObjects);

  using TaskTypes = typename viskores::cont::DeviceTaskTypes<DeviceAdapter>;
  auto task1 = TaskTypes::MakeTask(worklet, invocation, viskores::Id3());

  char message[1024];
  message[0] = '\0';
  viskores::exec::internal::ErrorMessageBuffer errorMessage(message, 1024);
  task1.SetErrorMessageBuffer(errorMessage);

  for (viskores::Id k = 0; k < 8; ++k)
  {
    for (viskores::Id j = 0; j < 8; ++j)
    {
      task1(viskores::Id3{ 8, 8, 8 }, 0, 8, j, k);
    }
  }

  VISKORES_TEST_ASSERT(errorMessage.IsErrorRaised(), "Error not raised correctly.");
  VISKORES_TEST_ASSERT(message == std::string(ERROR_MESSAGE), "Got wrong error message.");
}

template <typename DeviceAdapter>
void TestTaskTiling()
{
  Test1DNormalTaskTilingInvoke<DeviceAdapter>();
  Test1DErrorTaskTilingInvoke<DeviceAdapter>();

  Test3DNormalTaskTilingInvoke<DeviceAdapter>();
  Test3DErrorTaskTilingInvoke<DeviceAdapter>();
}
}
}
}
}
