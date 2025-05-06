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

#include <viskores/exec/internal/TaskSingular.h>

#include <viskores/exec/FunctorBase.h>
#include <viskores/exec/arg/BasicArg.h>
#include <viskores/exec/arg/ThreadIndicesBasic.h>

#include <viskores/StaticAssert.h>

#include <viskores/internal/FunctionInterface.h>
#include <viskores/internal/Invocation.h>

#include <viskores/testing/Testing.h>

namespace
{

struct TestExecObject
{
  VISKORES_EXEC_CONT
  TestExecObject()
    : Value(nullptr)
  {
  }

  VISKORES_EXEC_CONT
  TestExecObject(viskores::Id* value)
    : Value(value)
  {
  }

  viskores::Id* Value;
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

} // anonymous namespace

namespace viskores
{
namespace exec
{
namespace arg
{

template <>
struct Fetch<TestFetchTagInput, viskores::exec::arg::AspectTagDefault, TestExecObject>
{
  using ValueType = viskores::Id;

  VISKORES_EXEC
  ValueType Load(const viskores::exec::arg::ThreadIndicesBasic& indices,
                 const TestExecObject& execObject) const
  {
    return *execObject.Value + 10 * indices.GetInputIndex();
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
    *execObject.Value = value + 20 * indices.GetOutputIndex();
  }
};
}
}
} // viskores::exec::arg

namespace
{

using TestControlSignature = void(TestControlSignatureTagInput, TestControlSignatureTagOutput);
using TestControlInterface = viskores::internal::FunctionInterface<TestControlSignature>;

using TestExecutionSignature1 = void(viskores::exec::arg::BasicArg<1>,
                                     viskores::exec::arg::BasicArg<2>);
using TestExecutionInterface1 = viskores::internal::FunctionInterface<TestExecutionSignature1>;

using TestExecutionSignature2 = viskores::exec::arg::BasicArg<2>(viskores::exec::arg::BasicArg<1>);
using TestExecutionInterface2 = viskores::internal::FunctionInterface<TestExecutionSignature2>;

// Not a full worklet, but provides operators that we expect in a worklet.
struct TestWorkletProxy : viskores::exec::FunctorBase
{
  VISKORES_EXEC
  void operator()(viskores::Id input, viskores::Id& output) const { output = input + 100; }

  VISKORES_EXEC
  viskores::Id operator()(viskores::Id input) const { return input + 200; }

  template <typename T,
            typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType,
            typename G>
  VISKORES_EXEC viskores::exec::arg::ThreadIndicesBasic GetThreadIndices(
    const T& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutArrayType& threadToOut,
    const InputDomainType&,
    const G& globalThreadIndexOffset) const
  {
    const viskores::Id outIndex = threadToOut.Get(threadIndex);
    return viskores::exec::arg::ThreadIndicesBasic(
      threadIndex, outToIn.Get(outIndex), visit.Get(outIndex), outIndex, globalThreadIndexOffset);
  }
};

template <typename Invocation>
void CallDoWorkletInvokeFunctor(const Invocation& invocation, viskores::Id index)
{
  const viskores::Id outputIndex = invocation.ThreadToOutputMap.Get(index);
  viskores::exec::internal::detail::DoWorkletInvokeFunctor(
    TestWorkletProxy(),
    invocation,
    viskores::exec::arg::ThreadIndicesBasic(index,
                                            invocation.OutputToInputMap.Get(outputIndex),
                                            invocation.VisitArray.Get(outputIndex),
                                            outputIndex));
}

void TestDoWorkletInvoke()
{
  std::cout << "Testing internal worklet invoke." << std::endl;

  viskores::Id inputTestValue;
  viskores::Id outputTestValue;
  viskores::internal::FunctionInterface<void(TestExecObject, TestExecObject)> execObjects =
    viskores::internal::make_FunctionInterface<void>(TestExecObject(&inputTestValue),
                                                     TestExecObject(&outputTestValue));

  std::cout << "  Try void return." << std::endl;
  inputTestValue = 5;
  outputTestValue = static_cast<viskores::Id>(0xDEADDEAD);
  CallDoWorkletInvokeFunctor(viskores::internal::make_Invocation<1>(execObjects,
                                                                    TestControlInterface(),
                                                                    TestExecutionInterface1(),
                                                                    MyOutputToInputMapPortal(),
                                                                    MyVisitArrayPortal(),
                                                                    MyThreadToOutputMapPortal()),
                             1);
  VISKORES_TEST_ASSERT(inputTestValue == 5, "Input value changed.");
  VISKORES_TEST_ASSERT(outputTestValue == inputTestValue + 100 + 30, "Output value not set right.");

  std::cout << "  Try return value." << std::endl;
  inputTestValue = 6;
  outputTestValue = static_cast<viskores::Id>(0xDEADDEAD);
  CallDoWorkletInvokeFunctor(viskores::internal::make_Invocation<1>(execObjects,
                                                                    TestControlInterface(),
                                                                    TestExecutionInterface2(),
                                                                    MyOutputToInputMapPortal(),
                                                                    MyVisitArrayPortal(),
                                                                    MyThreadToOutputMapPortal()),
                             2);
  VISKORES_TEST_ASSERT(inputTestValue == 6, "Input value changed.");
  VISKORES_TEST_ASSERT(outputTestValue == inputTestValue + 200 + 30 * 2,
                       "Output value not set right.");
}

void TestWorkletInvokeFunctor()
{
  TestDoWorkletInvoke();
}

} // anonymous namespace

int UnitTestWorkletInvokeFunctor(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestWorkletInvokeFunctor, argc, argv);
}
