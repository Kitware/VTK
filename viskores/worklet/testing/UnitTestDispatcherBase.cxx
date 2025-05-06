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

#include <viskores/worklet/internal/DispatcherBase.h>

#include <viskores/worklet/internal/WorkletBase.h>

#include <viskores/cont/internal/Buffer.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

static constexpr viskores::Id ARRAY_SIZE = 10;

struct TestTypeCheckTag
{
};
struct TestTransportTagIn
{
};
struct TestTransportTagOut
{
};
struct TestFetchTagInput
{
};
struct TestFetchTagOutput
{
};

} // anonymous namespace

// not anonymous because nvcc sometimes complains about "unused" methods (that
// are definitely being used)
namespace ut_db
{

struct TestExecObjectIn
{
  VISKORES_EXEC_CONT
  TestExecObjectIn()
    : Array(nullptr)
  {
  }

  VISKORES_EXEC_CONT
  TestExecObjectIn(const viskores::Id* array)
    : Array(array)
  {
  }

  const viskores::Id* Array;
};

struct TestExecObjectOut
{
  VISKORES_EXEC_CONT
  TestExecObjectOut()
    : Array(nullptr)
  {
  }

  VISKORES_EXEC_CONT
  TestExecObjectOut(viskores::Id* array)
    : Array(array)
  {
  }

  viskores::Id* Array;
};

template <typename Device>
struct ExecutionObject
{
  viskores::Id Value;
};

struct TestExecObjectType : viskores::cont::ExecutionObjectBase
{
  template <typename Functor, typename... Args>
  void CastAndCall(Functor f, Args&&... args) const
  {
    f(*this, std::forward<Args>(args)...);
  }
  template <typename Device>
  VISKORES_CONT ExecutionObject<Device> PrepareForExecution(Device, viskores::cont::Token&) const
  {
    ExecutionObject<Device> object;
    object.Value = this->Value;
    return object;
  }
  viskores::Id Value;
};

struct TestExecObjectTypeBad
{ //this will fail as it doesn't inherit from viskores::cont::ExecutionObjectBase
  template <typename Functor, typename... Args>
  void CastAndCall(Functor f, Args&&... args) const
  {
    f(*this, std::forward<Args>(args)...);
  }
};

} // namespace ut_db

namespace viskores
{
namespace cont
{
namespace arg
{

template <>
struct TypeCheck<TestTypeCheckTag, viskores::cont::internal::Buffer>
{
  static constexpr bool value = true;
};

template <typename Device>
struct Transport<TestTransportTagIn, viskores::cont::internal::Buffer, Device>
{
  using ExecObjectType = ut_db::TestExecObjectIn;

  VISKORES_CONT
  ExecObjectType operator()(const viskores::cont::internal::Buffer& contData,
                            const viskores::cont::internal::Buffer&,
                            viskores::Id inputRange,
                            viskores::Id outputRange,
                            viskores::cont::Token& token) const
  {
    VISKORES_TEST_ASSERT(inputRange == ARRAY_SIZE, "Got unexpected size in test transport.");
    VISKORES_TEST_ASSERT(outputRange == ARRAY_SIZE, "Got unexpected size in test transport.");
    return reinterpret_cast<const viskores::Id*>(contData.ReadPointerDevice(Device{}, token));
  }
};

template <typename Device>
struct Transport<TestTransportTagOut, viskores::cont::internal::Buffer, Device>
{
  using ExecObjectType = ut_db::TestExecObjectOut;

  VISKORES_CONT
  ExecObjectType operator()(const viskores::cont::internal::Buffer& contData,
                            const viskores::cont::internal::Buffer&,
                            viskores::Id inputRange,
                            viskores::Id outputRange,
                            viskores::cont::Token& token) const
  {
    VISKORES_TEST_ASSERT(inputRange == ARRAY_SIZE, "Got unexpected size in test transport.");
    VISKORES_TEST_ASSERT(outputRange == ARRAY_SIZE, "Got unexpected size in test transport.");
    return reinterpret_cast<viskores::Id*>(contData.WritePointerDevice(Device{}, token));
  }
};
}
}
} // namespace viskores::cont::arg

namespace viskores
{
namespace cont
{
namespace internal
{

template <>
struct DynamicTransformTraits<ut_db::TestExecObjectType>
{
  using DynamicTag = viskores::cont::internal::DynamicTransformTagCastAndCall;
};
template <>
struct DynamicTransformTraits<ut_db::TestExecObjectTypeBad>
{
  using DynamicTag = viskores::cont::internal::DynamicTransformTagCastAndCall;
};
}
}
} // namespace viskores::cont::internal

namespace viskores
{
namespace exec
{
namespace arg
{

template <>
struct Fetch<TestFetchTagInput, viskores::exec::arg::AspectTagDefault, ut_db::TestExecObjectIn>
{
  using ValueType = viskores::Id;

  VISKORES_EXEC
  ValueType Load(const viskores::exec::arg::ThreadIndicesBasic indices,
                 const ut_db::TestExecObjectIn& execObject) const
  {
    return execObject.Array[indices.GetInputIndex()];
  }

  VISKORES_EXEC
  void Store(const viskores::exec::arg::ThreadIndicesBasic,
             const ut_db::TestExecObjectIn&,
             ValueType) const
  {
    // No-op
  }
};

template <>
struct Fetch<TestFetchTagOutput, viskores::exec::arg::AspectTagDefault, ut_db::TestExecObjectOut>
{
  using ValueType = viskores::Id;

  VISKORES_EXEC
  ValueType Load(const viskores::exec::arg::ThreadIndicesBasic&,
                 const ut_db::TestExecObjectOut&) const
  {
    // No-op
    return ValueType();
  }

  VISKORES_EXEC
  void Store(const viskores::exec::arg::ThreadIndicesBasic& indices,
             const ut_db::TestExecObjectOut& execObject,
             ValueType value) const
  {
    execObject.Array[indices.GetOutputIndex()] = value;
  }
};
}
}
} // viskores::exec::arg

namespace
{

static constexpr viskores::Id EXPECTED_EXEC_OBJECT_VALUE = 123;

class TestWorkletBase : public viskores::worklet::internal::WorkletBase
{
public:
  struct TestIn : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = TestTypeCheckTag;
    using TransportTag = TestTransportTagIn;
    using FetchTag = TestFetchTagInput;
  };
  struct TestOut : viskores::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = TestTypeCheckTag;
    using TransportTag = TestTransportTagOut;
    using FetchTag = TestFetchTagOutput;
  };
};

class TestWorklet : public TestWorkletBase
{
public:
  using ControlSignature = void(TestIn, ExecObject, TestOut);
  using ExecutionSignature = _3(_1, _2, WorkIndex);

  template <typename ExecObjectType>
  VISKORES_EXEC viskores::Id operator()(viskores::Id value,
                                        ExecObjectType execObject,
                                        viskores::Id index) const
  {
    VISKORES_ASSERT(value == TestValue(index, viskores::Id()));
    VISKORES_ASSERT(execObject.Value == EXPECTED_EXEC_OBJECT_VALUE);
    return TestValue(index, viskores::Id()) + 1000;
  }
};

#define ERROR_MESSAGE "Expected worklet error."

class TestErrorWorklet : public TestWorkletBase
{
public:
  using ControlSignature = void(TestIn, ExecObject, TestOut);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename ExecObjectType>
  VISKORES_EXEC void operator()(viskores::Id, ExecObjectType, viskores::Id) const
  {
    this->RaiseError(ERROR_MESSAGE);
  }
};


inline viskores::Id SchedulingRange(const viskores::cont::internal::Buffer& inputDomain)
{
  return static_cast<viskores::Id>(inputDomain.GetNumberOfBytes() / sizeof(viskores::Id));
}
inline viskores::Id SchedulingRange(const viskores::cont::internal::Buffer* inputDomain)
{
  return static_cast<viskores::Id>(inputDomain->GetNumberOfBytes() / sizeof(viskores::Id));
}

template <typename WorkletType>
class TestDispatcher
  : public viskores::worklet::internal::
      DispatcherBase<TestDispatcher<WorkletType>, WorkletType, TestWorkletBase>
{
  using Superclass = viskores::worklet::internal::
    DispatcherBase<TestDispatcher<WorkletType>, WorkletType, TestWorkletBase>;
  using ScatterType = typename Superclass::ScatterType;

public:
  template <typename... T>
  VISKORES_CONT TestDispatcher(T&&... args)
    : Superclass(std::forward<T>(args)...)
  {
  }

  VISKORES_CONT
  template <typename Invocation>
  void DoInvoke(Invocation& invocation) const
  {
    std::cout << "In TestDispatcher::DoInvoke()" << std::endl;

    using namespace viskores::worklet::internal;

    // This is the type for the input domain
    using InputDomainType = typename Invocation::InputDomainType;

    // We can pull the input domain parameter (the data specifying the input
    // domain) from the invocation object.
    const InputDomainType& inputDomain = invocation.GetInputDomain();

    // For a DispatcherMapField, the inputDomain must be an ArrayHandle (or
    // an UncertainArrayHandle or an UnknownArrayHandle that gets cast to one).
    // The size of the domain (number of threads/worklet instances) is equal
    // to the size of the array.
    //verify the overloads for SchedulingRange work
    auto numInstances = SchedulingRange(inputDomain);

    // A MapField is a pretty straightforward dispatch. Once we know the number
    // of invocations, the superclass can take care of the rest.
    this->BasicInvoke(invocation, numInstances);
  }

private:
  WorkletType Worklet;
};

void TestBasicInvoke()
{
  std::cout << "Test basic invoke" << std::endl;
  std::cout << "  Set up data." << std::endl;
  viskores::cont::internal::Buffer inputBuffer;
  viskores::cont::internal::Buffer outputBuffer;
  ut_db::TestExecObjectType execObject;
  execObject.Value = EXPECTED_EXEC_OBJECT_VALUE;

  {
    viskores::cont::Token token;
    inputBuffer.SetNumberOfBytes(
      static_cast<viskores::BufferSizeType>(ARRAY_SIZE * sizeof(viskores::Id)),
      viskores::CopyFlag::Off,
      token);
    outputBuffer.SetNumberOfBytes(
      static_cast<viskores::BufferSizeType>(ARRAY_SIZE * sizeof(viskores::Id)),
      viskores::CopyFlag::Off,
      token);
    auto inputArray = reinterpret_cast<viskores::Id*>(inputBuffer.WritePointerHost(token));
    auto outputArray = reinterpret_cast<viskores::Id*>(outputBuffer.WritePointerHost(token));
    std::size_t i = 0;
    for (viskores::Id index = 0; index < ARRAY_SIZE; index++, i++)
    {
      inputArray[i] = TestValue(index, viskores::Id());
      outputArray[i] = static_cast<viskores::Id>(0xDEADDEAD);
    }
  }

  std::cout << "  Create and run dispatcher." << std::endl;
  TestDispatcher<TestWorklet> dispatcher;
  dispatcher.Invoke(inputBuffer, execObject, &outputBuffer);

  std::cout << "  Check output of invoke." << std::endl;
  {
    viskores::cont::Token token;
    auto outputArray = reinterpret_cast<const viskores::Id*>(outputBuffer.ReadPointerHost(token));
    std::size_t i = 0;
    for (viskores::Id index = 0; index < ARRAY_SIZE; index++, i++)
    {
      VISKORES_TEST_ASSERT(outputArray[i] == TestValue(index, viskores::Id()) + 1000,
                           "Got bad value from testing.");
    }
  }
}

void TestInvokeWithError()
{
  std::cout << "Test invoke with error raised" << std::endl;
  std::cout << "  Set up data." << std::endl;
  viskores::cont::internal::Buffer inputBuffer;
  viskores::cont::internal::Buffer outputBuffer;
  ut_db::TestExecObjectType execObject;
  execObject.Value = EXPECTED_EXEC_OBJECT_VALUE;

  {
    viskores::cont::Token token;
    inputBuffer.SetNumberOfBytes(
      static_cast<viskores::BufferSizeType>(ARRAY_SIZE * sizeof(viskores::Id)),
      viskores::CopyFlag::Off,
      token);
    outputBuffer.SetNumberOfBytes(
      static_cast<viskores::BufferSizeType>(ARRAY_SIZE * sizeof(viskores::Id)),
      viskores::CopyFlag::Off,
      token);
    auto inputArray = reinterpret_cast<viskores::Id*>(inputBuffer.WritePointerHost(token));
    auto outputArray = reinterpret_cast<viskores::Id*>(outputBuffer.WritePointerHost(token));
    std::size_t i = 0;
    for (viskores::Id index = 0; index < ARRAY_SIZE; index++, i++)
    {
      inputArray[i] = TestValue(index, viskores::Id());
      outputArray[i] = static_cast<viskores::Id>(0xDEADDEAD);
    }
  }

  try
  {
    std::cout << "  Create and run dispatcher that raises error." << std::endl;
    TestDispatcher<TestErrorWorklet> dispatcher;
    dispatcher.Invoke(&inputBuffer, execObject, outputBuffer);
    // Make sure the invocation finishes by moving data to host. Asynchronous launches
    // might not throw an error right away.
    viskores::cont::Token token;
    outputBuffer.ReadPointerHost(token);
    VISKORES_TEST_FAIL("Exception not thrown.");
  }
  catch (viskores::cont::ErrorExecution& error)
  {
    std::cout << "  Got expected exception." << std::endl;
    std::cout << "  Exception message: " << error.GetMessage() << std::endl;
    VISKORES_TEST_ASSERT(error.GetMessage() == ERROR_MESSAGE, "Got unexpected error message.");
  }
}

void TestInvokeWithBadDynamicType()
{
  std::cout << "Test invoke with bad type" << std::endl;

  std::vector<viskores::Id> inputArray(ARRAY_SIZE);
  std::vector<viskores::Id> outputArray(ARRAY_SIZE);
  ut_db::TestExecObjectTypeBad execObject;
  TestDispatcher<TestWorklet> dispatcher;

  try
  {
    std::cout << "  Second argument bad." << std::endl;
    dispatcher.Invoke(inputArray, execObject, outputArray);
    VISKORES_TEST_FAIL("Dispatcher did not throw expected error.");
  }
  catch (viskores::cont::ErrorBadType& error)
  {
    std::cout << "    Got expected exception." << std::endl;
    std::cout << "    " << error.GetMessage() << std::endl;
    VISKORES_TEST_ASSERT(error.GetMessage().find(" 2 ") != std::string::npos,
                         "Parameter index not named in error message.");
  }
}

void TestDispatcherBase()
{
  TestBasicInvoke();
  TestInvokeWithError();
  TestInvokeWithBadDynamicType();
}

} // anonymous namespace

int UnitTestDispatcherBase(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestDispatcherBase, argc, argv);
}
