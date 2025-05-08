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

#include <viskores/cont/arg/TransportTagArrayIn.h>

#include <viskores/exec/FunctorBase.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DeviceAdapter.h>
#include <viskores/cont/TryExecute.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

static constexpr viskores::Id ARRAY_SIZE = 10;

template <typename PortalType>
struct TestKernelIn : public viskores::exec::FunctorBase
{
  PortalType Portal;

  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    using ValueType = typename PortalType::ValueType;
    if (!test_equal(this->Portal.Get(index), TestValue(index, ValueType())))
    {
      this->RaiseError("Got bad execution object.");
    }
  }
};

template <typename Device>
struct TryArrayInType
{
  template <typename T>
  void operator()(T) const
  {
    T array[ARRAY_SIZE];
    for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
    {
      array[index] = TestValue(index, T());
    }

    using ArrayHandleType = viskores::cont::ArrayHandle<T>;
    ArrayHandleType handle =
      viskores::cont::make_ArrayHandle(array, ARRAY_SIZE, viskores::CopyFlag::Off);

    using PortalType = typename ArrayHandleType::ReadPortalType;

    viskores::cont::arg::
      Transport<viskores::cont::arg::TransportTagArrayIn, ArrayHandleType, Device>
        transport;

    viskores::cont::Token token;

    TestKernelIn<PortalType> kernel;
    kernel.Portal = transport(handle, handle, ARRAY_SIZE, ARRAY_SIZE, token);

    viskores::cont::DeviceAdapterAlgorithm<Device>::Schedule(kernel, ARRAY_SIZE);
  }
};

template <typename Device>
bool TryArrayInTransport(Device device)
{
  std::cout << "Trying ArrayIn transport with " << device.GetName() << std::endl;
  viskores::testing::Testing::TryTypes(TryArrayInType<Device>());
  return true;
}

void TestArrayInTransport()
{
  VISKORES_TEST_ASSERT(
    viskores::cont::TryExecute([](auto device) { return TryArrayInTransport(device); }));
}

} // Anonymous namespace

int UnitTestTransportArrayIn(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestArrayInTransport, argc, argv);
}
