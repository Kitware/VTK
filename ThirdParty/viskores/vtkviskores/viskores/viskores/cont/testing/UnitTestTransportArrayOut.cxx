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

#include <viskores/cont/arg/TransportTagArrayOut.h>

#include <viskores/exec/FunctorBase.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/DeviceAdapter.h>
#include <viskores/cont/TryExecute.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

static constexpr viskores::Id ARRAY_SIZE = 10;

template <typename PortalType>
struct TestKernelOut : public viskores::exec::FunctorBase
{
  PortalType Portal;

  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    using ValueType = typename PortalType::ValueType;
    this->Portal.Set(index, TestValue(index, ValueType()));
  }
};

template <typename Device>
struct TryArrayOutType
{
  template <typename T>
  void operator()(T) const
  {
    using ArrayHandleType = viskores::cont::ArrayHandle<T>;
    ArrayHandleType handle;

    using PortalType = typename ArrayHandleType::WritePortalType;

    viskores::cont::arg::
      Transport<viskores::cont::arg::TransportTagArrayOut, ArrayHandleType, Device>
        transport;

    viskores::cont::Token token;

    TestKernelOut<PortalType> kernel;
    kernel.Portal = transport(
      handle, viskores::cont::ArrayHandleIndex(ARRAY_SIZE), ARRAY_SIZE, ARRAY_SIZE, token);

    VISKORES_TEST_ASSERT(handle.GetNumberOfValues() == ARRAY_SIZE,
                         "ArrayOut transport did not allocate array correctly.");

    viskores::cont::DeviceAdapterAlgorithm<Device>::Schedule(kernel, ARRAY_SIZE);
    token.DetachFromAll();

    CheckPortal(handle.ReadPortal());
  }
};

template <typename Device>
bool TryArrayOutTransport(Device device)
{
  std::cout << "Trying ArrayOut transport with " << device.GetName() << std::endl;
  viskores::testing::Testing::TryTypes(TryArrayOutType<Device>());
  return true;
}

void TestArrayOutTransport()
{
  VISKORES_TEST_ASSERT(
    viskores::cont::TryExecute([](auto device) { return TryArrayOutTransport(device); }));
}

} // Anonymous namespace

int UnitTestTransportArrayOut(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestArrayOutTransport, argc, argv);
}
