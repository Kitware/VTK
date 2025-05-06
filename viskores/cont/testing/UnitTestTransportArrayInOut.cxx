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

#include <viskores/cont/arg/TransportTagArrayInOut.h>

#include <viskores/exec/FunctorBase.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DeviceAdapter.h>
#include <viskores/cont/TryExecute.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

static constexpr viskores::Id ARRAY_SIZE = 10;

template <typename PortalType>
struct TestKernelInOut : public viskores::exec::FunctorBase
{
  PortalType Portal;

  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    using ValueType = typename PortalType::ValueType;
    ValueType inValue = this->Portal.Get(index);
    this->Portal.Set(index, inValue + inValue);
  }
};

template <typename Device>
struct TryArrayInOutType
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

    using PortalType = typename ArrayHandleType::WritePortalType;

    viskores::cont::arg::
      Transport<viskores::cont::arg::TransportTagArrayInOut, ArrayHandleType, Device>
        transport;

    viskores::cont::Token token;

    TestKernelInOut<PortalType> kernel;
    kernel.Portal = transport(handle, handle, ARRAY_SIZE, ARRAY_SIZE, token);

    viskores::cont::DeviceAdapterAlgorithm<Device>::Schedule(kernel, ARRAY_SIZE);
    token.DetachFromAll();

    typename ArrayHandleType::ReadPortalType portal = handle.ReadPortal();
    VISKORES_TEST_ASSERT(portal.GetNumberOfValues() == ARRAY_SIZE,
                         "Portal has wrong number of values.");
    for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
    {
      T expectedValue = TestValue(index, T()) + TestValue(index, T());
      T retrievedValue = portal.Get(index);
      VISKORES_TEST_ASSERT(test_equal(expectedValue, retrievedValue),
                           "Functor did not modify in place.");
    }
  }
};

template <typename Device>
bool TryArrayInOutTransport(Device device)
{
  std::cout << "Trying ArrayInOut transport with " << device.GetName() << std::endl;
  viskores::testing::Testing::TryTypes(TryArrayInOutType<Device>(), viskores::TypeListCommon());
  return true;
}

void TestArrayInOutTransport()
{
  VISKORES_TEST_ASSERT(
    viskores::cont::TryExecute([](auto device) { return TryArrayInOutTransport(device); }));
}

} // anonymous namespace

int UnitTestTransportArrayInOut(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestArrayInOutTransport, argc, argv);
}
