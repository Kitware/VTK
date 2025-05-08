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

#include <viskores/cont/arg/TransportTagExecObject.h>

#include <viskores/exec/FunctorBase.h>

#include <viskores/cont/DeviceAdapter.h>
#include <viskores/cont/ExecutionObjectBase.h>
#include <viskores/cont/TryExecute.h>

#include <viskores/cont/testing/Testing.h>

#define EXPECTED_NUMBER 42

namespace unittesttransportexecobject
{

struct NotAnExecutionObject
{
};
struct InvalidExecutionObject : viskores::cont::ExecutionObjectBase
{
};

template <typename Device>
struct ExecutionObject
{
  viskores::Int32 Number;
};

struct TestExecutionObject : public viskores::cont::ExecutionObjectBase
{
  viskores::Int32 Number;

  template <typename Device>
  VISKORES_CONT ExecutionObject<Device> PrepareForExecution(Device, viskores::cont::Token&) const
  {
    ExecutionObject<Device> object;
    object.Number = this->Number;
    return object;
  }
};

template <typename Device>
struct TestKernel : public viskores::exec::FunctorBase
{
  ExecutionObject<Device> Object;

  VISKORES_EXEC
  void operator()(viskores::Id) const
  {
    if (this->Object.Number != EXPECTED_NUMBER)
    {
      this->RaiseError("Got bad execution object.");
    }
  }
};

template <typename Device>
bool TryExecObjectTransport(Device device)
{
  std::cout << "Trying ExecObject transport with " << device.GetName() << std::endl;

  TestExecutionObject contObject;
  contObject.Number = EXPECTED_NUMBER;

  viskores::cont::arg::
    Transport<viskores::cont::arg::TransportTagExecObject, TestExecutionObject, Device>
      transport;

  viskores::cont::Token token;

  TestKernel<Device> kernel;
  kernel.Object = transport(contObject, nullptr, 1, 1, token);

  viskores::cont::DeviceAdapterAlgorithm<Device>::Schedule(kernel, 1);

  return true;
}

void TestExecObjectTransport()
{
  std::cout << "Checking ExecObject queries." << std::endl;
  VISKORES_TEST_ASSERT(
    !viskores::cont::internal::IsExecutionObjectBase<NotAnExecutionObject>::value, "Bad query");
  VISKORES_TEST_ASSERT(
    viskores::cont::internal::IsExecutionObjectBase<InvalidExecutionObject>::value, "Bad query");
  VISKORES_TEST_ASSERT(viskores::cont::internal::IsExecutionObjectBase<TestExecutionObject>::value,
                       "Bad query");

  VISKORES_TEST_ASSERT(
    !viskores::cont::internal::HasPrepareForExecution<NotAnExecutionObject>::value, "Bad query");
  VISKORES_TEST_ASSERT(
    !viskores::cont::internal::HasPrepareForExecution<InvalidExecutionObject>::value, "Bad query");
  VISKORES_TEST_ASSERT(viskores::cont::internal::HasPrepareForExecution<TestExecutionObject>::value,
                       "Bad query");

  VISKORES_TEST_ASSERT(
    viskores::cont::TryExecute([](auto device) { return TryExecObjectTransport(device); }));
}

} // namespace unittesttransportexecobject

int UnitTestTransportExecObject(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(
    unittesttransportexecobject::TestExecObjectTransport, argc, argv);
}
