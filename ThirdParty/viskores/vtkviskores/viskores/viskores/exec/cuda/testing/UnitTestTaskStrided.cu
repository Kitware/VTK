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
#include <viskores/testing/Testing.h>

#include <viskores/cont/cuda/DeviceAdapterCuda.h>

#include <viskores/exec/FunctorBase.h>
#include <viskores/exec/arg/BasicArg.h>
#include <viskores/exec/arg/ThreadIndicesBasic.h>
#include <viskores/exec/cuda/internal/TaskStrided.h>

#include <viskores/StaticAssert.h>

#include <viskores/internal/FunctionInterface.h>
#include <viskores/internal/Invocation.h>

#if defined(VISKORES_MSVC)
#pragma warning(push)
#pragma warning(disable : 4068) //unknown pragma
#endif

#if defined(__NVCC__) && defined(__CUDACC_VER_MAJOR__)
// Disable warning "declared but never referenced"
// This file produces several false-positive warnings
// Eg: TestExecObject::TestExecObject, MyOutputToInputMapPortal::Get,
//     TestWorkletProxy::operator()
#pragma push
#if (CUDART_VERSION >= 11050)
#pragma nv_diag_suppress 177
#else
#pragma diag_suppress 177
#endif

#endif

namespace
{

struct TestExecObject
{
  using PortalType = viskores::cont::ArrayHandle<viskores::Id>::WritePortalType;

  VISKORES_EXEC_CONT
  TestExecObject(PortalType portal)
    : Portal(portal)
  {
  }

  PortalType Portal;
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
  VISKORES_EXEC_CONT
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
    return execObject.Portal.Get(indices.GetInputIndex()) + 10 * indices.GetInputIndex();
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
    execObject.Portal.Set(indices.GetOutputIndex(), value + 20 * indices.GetOutputIndex());
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

template <typename TaskType>
static __global__ void ScheduleTaskStrided(TaskType task, viskores::Id start, viskores::Id end)
{

  const viskores::Id index = blockIdx.x * blockDim.x + threadIdx.x;
  const viskores::Id inc = blockDim.x * gridDim.x;
  if (index >= start && index < end)
  {
    task(index, end, inc);
  }
}

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
            typename InputDomainType>
  VISKORES_EXEC viskores::exec::arg::ThreadIndicesBasic GetThreadIndices(
    const T& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutArrayType& threadToOut,
    const InputDomainType&) const
  {
    viskores::Id outIndex = threadToOut.Get(threadIndex);
    return viskores::exec::arg::ThreadIndicesBasic(
      threadIndex, outToIn.Get(outIndex), visit.Get(outIndex), outIndex);
  }
};

#define ERROR_MESSAGE "Expected worklet error."

// Not a full worklet, but provides operators that we expect in a worklet.
struct TestWorkletErrorProxy : viskores::exec::FunctorBase
{
  VISKORES_EXEC
  void operator()(viskores::Id, viskores::Id) const { this->RaiseError(ERROR_MESSAGE); }

  template <typename T,
            typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType>
  VISKORES_EXEC viskores::exec::arg::ThreadIndicesBasic GetThreadIndices(
    const T& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutArrayType& threadToOut,
    const InputDomainType&) const
  {
    viskores::Id outIndex = threadToOut.Get(threadIndex);
    return viskores::exec::arg::ThreadIndicesBasic(
      threadIndex, outToIn.Get(outIndex), visit.Get(outIndex), outIndex);
  }
};

// Check behavior of InvocationToFetch helper class.

VISKORES_STATIC_ASSERT(
  (std::is_same<
    viskores::exec::internal::detail::
      InvocationToFetch<viskores::exec::arg::ThreadIndicesBasic, InvocationType1, 1>::type,
    viskores::exec::arg::Fetch<TestFetchTagInput,
                               viskores::exec::arg::AspectTagDefault,
                               TestExecObject>>::type::value));

VISKORES_STATIC_ASSERT(
  (std::is_same<
    viskores::exec::internal::detail::
      InvocationToFetch<viskores::exec::arg::ThreadIndicesBasic, InvocationType1, 2>::type,
    viskores::exec::arg::Fetch<TestFetchTagOutput,
                               viskores::exec::arg::AspectTagDefault,
                               TestExecObject>>::type::value));

VISKORES_STATIC_ASSERT(
  (std::is_same<
    viskores::exec::internal::detail::
      InvocationToFetch<viskores::exec::arg::ThreadIndicesBasic, InvocationType2, 0>::type,
    viskores::exec::arg::Fetch<TestFetchTagOutput,
                               viskores::exec::arg::AspectTagDefault,
                               TestExecObject>>::type::value));

template <typename DeviceAdapter>
void TestNormalFunctorInvoke()
{
  std::cout << "Testing normal worklet invoke." << std::endl;

  viskores::cont::Token token;

  viskores::Id inputTestValues[3] = { 5, 5, 6 };

  viskores::cont::ArrayHandle<viskores::Id> input =
    viskores::cont::make_ArrayHandle(inputTestValues, 3, viskores::CopyFlag::Off);
  viskores::cont::ArrayHandle<viskores::Id> output;

  viskores::internal::FunctionInterface<void(TestExecObject, TestExecObject)> execObjects =
    viskores::internal::make_FunctionInterface<void>(
      TestExecObject(input.PrepareForInPlace(DeviceAdapter(), token)),
      TestExecObject(output.PrepareForOutput(3, DeviceAdapter(), token)));

  std::cout << "  Try void return." << std::endl;
  TestWorkletProxy worklet;
  InvocationType1 invocation1(execObjects);

  using TaskTypes = typename viskores::cont::DeviceTaskTypes<DeviceAdapter>;
  auto task1 = TaskTypes::MakeTask(worklet, invocation1, viskores::Id());

  ScheduleTaskStrided<decltype(task1)><<<32, 256>>>(task1, 1, 2);
  cudaDeviceSynchronize();
  token.DetachFromAll();
  input.SyncControlArray();
  output.SyncControlArray();

  VISKORES_TEST_ASSERT(inputTestValues[1] == 5, "Input value changed.");
  VISKORES_TEST_ASSERT(output.ReadPortal().Get(1) == inputTestValues[1] + 100 + 30,
                       "Output value not set right.");

  std::cout << "  Try return value." << std::endl;

  execObjects = viskores::internal::make_FunctionInterface<void>(
    TestExecObject(input.PrepareForInPlace(DeviceAdapter(), token)),
    TestExecObject(output.PrepareForOutput(3, DeviceAdapter(), token)));

  InvocationType2 invocation2(execObjects);

  using TaskTypes = typename viskores::cont::DeviceTaskTypes<DeviceAdapter>;
  auto task2 = TaskTypes::MakeTask(worklet, invocation2, viskores::Id());

  ScheduleTaskStrided<decltype(task2)><<<32, 256>>>(task2, 2, 3);
  cudaDeviceSynchronize();
  token.DetachFromAll();
  input.SyncControlArray();
  output.SyncControlArray();

  VISKORES_TEST_ASSERT(inputTestValues[2] == 6, "Input value changed.");
  VISKORES_TEST_ASSERT(output.ReadPortal().Get(2) == inputTestValues[2] + 200 + 30 * 2,
                       "Output value not set right.");
}

template <typename DeviceAdapter>
void TestErrorFunctorInvoke()
{
  std::cout << "Testing invoke with an error raised in the worklet." << std::endl;

  viskores::cont::Token token;

  viskores::Id inputTestValue = 5;
  viskores::Id outputTestValue = static_cast<viskores::Id>(0xDEADDEAD);

  viskores::cont::ArrayHandle<viskores::Id> input =
    viskores::cont::make_ArrayHandle(&inputTestValue, 1, viskores::CopyFlag::Off);
  viskores::cont::ArrayHandle<viskores::Id> output =
    viskores::cont::make_ArrayHandle(&outputTestValue, 1, viskores::CopyFlag::Off);

  viskores::internal::FunctionInterface<void(TestExecObject, TestExecObject)> execObjects =
    viskores::internal::make_FunctionInterface<void>(
      TestExecObject(input.PrepareForInPlace(DeviceAdapter(), token)),
      TestExecObject(output.PrepareForInPlace(DeviceAdapter(), token)));

  using TaskStrided1 = viskores::exec::cuda::internal::
    TaskStrided1D<TestWorkletErrorProxy, InvocationType1, viskores::cont::internal::HintList<>>;
  TestWorkletErrorProxy worklet;
  InvocationType1 invocation(execObjects);

  using TaskTypes = typename viskores::cont::DeviceTaskTypes<DeviceAdapter>;
  using Algorithm = viskores::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

  auto task = TaskTypes::MakeTask(worklet, invocation, viskores::Id());

  auto errorArray = Algorithm::GetPinnedErrorArray();
  viskores::exec::internal::ErrorMessageBuffer errorMessage(errorArray.DevicePtr, errorArray.Size);
  task.SetErrorMessageBuffer(errorMessage);

  ScheduleTaskStrided<decltype(task)><<<32, 256>>>(task, 1, 2);
  cudaDeviceSynchronize();

  VISKORES_TEST_ASSERT(errorMessage.IsErrorRaised(), "Error not raised correctly.");
  VISKORES_TEST_ASSERT(errorArray.HostPtr == std::string(ERROR_MESSAGE),
                       "Got wrong error message.");
}

template <typename DeviceAdapter>
void TestTaskStrided()
{
  TestNormalFunctorInvoke<DeviceAdapter>();
  TestErrorFunctorInvoke<DeviceAdapter>();
}

} // anonymous namespace

int UnitTestTaskStrided(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(
    TestTaskStrided<viskores::cont::DeviceAdapterTagCuda>, argc, argv);
}

#if defined(__NVCC__) && defined(__CUDACC_VER_MAJOR__)
#pragma pop
#endif

#if defined(VISKORES_MSVC)
#pragma warning(pop)
#endif
