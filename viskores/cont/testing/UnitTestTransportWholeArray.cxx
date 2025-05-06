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

#include <viskores/cont/arg/TransportTagAtomicArray.h>
#include <viskores/cont/arg/TransportTagWholeArrayIn.h>
#include <viskores/cont/arg/TransportTagWholeArrayInOut.h>
#include <viskores/cont/arg/TransportTagWholeArrayOut.h>

#include <viskores/exec/FunctorBase.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DeviceAdapter.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

static constexpr viskores::Id ARRAY_SIZE = 10;

#define OFFSET 10

template <typename PortalType>
struct TestOutKernel : public viskores::exec::FunctorBase
{
  PortalType Portal;

  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    if (this->Portal.GetNumberOfValues() != ARRAY_SIZE)
    {
      this->RaiseError("Out whole array has wrong size.");
    }
    using ValueType = typename PortalType::ValueType;
    this->Portal.Set(index, TestValue(index, ValueType()));
  }
};

template <typename PortalType>
struct TestInKernel : public viskores::exec::FunctorBase
{
  PortalType Portal;

  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    if (this->Portal.GetNumberOfValues() != ARRAY_SIZE)
    {
      this->RaiseError("In whole array has wrong size.");
    }
    using ValueType = typename PortalType::ValueType;
    if (!test_equal(this->Portal.Get(index), TestValue(index, ValueType())))
    {
      this->RaiseError("Got bad execution object.");
    }
  }
};

template <typename PortalType>
struct TestInOutKernel : public viskores::exec::FunctorBase
{
  PortalType Portal;

  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    if (this->Portal.GetNumberOfValues() != ARRAY_SIZE)
    {
      this->RaiseError("In/Out whole array has wrong size.");
    }
    using ValueType = typename PortalType::ValueType;
    this->Portal.Set(index, this->Portal.Get(index) + ValueType(OFFSET));
  }
};

template <typename AtomicType>
struct TestAtomicKernel : public viskores::exec::FunctorBase
{
  VISKORES_CONT
  TestAtomicKernel(const AtomicType& atomicArray)
    : AtomicArray(atomicArray)
  {
  }

  AtomicType AtomicArray;

  VISKORES_EXEC
  void operator()(viskores::Id index) const
  {
    using ValueType = typename AtomicType::ValueType;
    this->AtomicArray.Add(0, static_cast<ValueType>(index));
  }
};

template <typename Device>
struct TryWholeArrayType
{
  template <typename T>
  void operator()(T) const
  {
    using ArrayHandleType = viskores::cont::ArrayHandle<T>;

    using InTransportType = viskores::cont::arg::
      Transport<viskores::cont::arg::TransportTagWholeArrayIn, ArrayHandleType, Device>;
    using InOutTransportType = viskores::cont::arg::
      Transport<viskores::cont::arg::TransportTagWholeArrayInOut, ArrayHandleType, Device>;
    using OutTransportType = viskores::cont::arg::
      Transport<viskores::cont::arg::TransportTagWholeArrayOut, ArrayHandleType, Device>;

    ArrayHandleType array;
    array.Allocate(ARRAY_SIZE);

    viskores::cont::Token token;

    std::cout << "Check Transport WholeArrayOut" << std::endl;
    TestOutKernel<typename OutTransportType::ExecObjectType> outKernel;
    outKernel.Portal = OutTransportType()(array, nullptr, -1, -1, token);

    viskores::cont::DeviceAdapterAlgorithm<Device>::Schedule(outKernel, ARRAY_SIZE);
    token.DetachFromAll();

    CheckPortal(array.ReadPortal());

    std::cout << "Check Transport WholeArrayIn" << std::endl;
    TestInKernel<typename InTransportType::ExecObjectType> inKernel;
    inKernel.Portal = InTransportType()(array, nullptr, -1, -1, token);

    viskores::cont::DeviceAdapterAlgorithm<Device>::Schedule(inKernel, ARRAY_SIZE);
    token.DetachFromAll();

    std::cout << "Check Transport WholeArrayInOut" << std::endl;
    TestInOutKernel<typename InOutTransportType::ExecObjectType> inOutKernel;
    inOutKernel.Portal = InOutTransportType()(array, nullptr, -1, -1, token);

    viskores::cont::DeviceAdapterAlgorithm<Device>::Schedule(inOutKernel, ARRAY_SIZE);
    token.DetachFromAll();

    VISKORES_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE, "Array size wrong?");
    auto portal = array.ReadPortal();
    for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
    {
      T expectedValue = TestValue(index, T()) + T(OFFSET);
      T retrievedValue = portal.Get(index);
      VISKORES_TEST_ASSERT(test_equal(expectedValue, retrievedValue),
                           "In/Out array not set correctly.");
    }
  }
};

template <typename Device>
struct TryAtomicArrayType
{
  template <typename T>
  void operator()(T) const
  {
    using ArrayHandleType = viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic>;

    using TransportType = viskores::cont::arg::
      Transport<viskores::cont::arg::TransportTagAtomicArray, ArrayHandleType, Device>;

    ArrayHandleType array;
    array.Allocate(1);
    array.WritePortal().Set(0, 0);

    viskores::cont::Token token;

    std::cout << "Check Transport AtomicArray" << std::endl;
    TestAtomicKernel<typename TransportType::ExecObjectType> kernel(
      TransportType()(array, nullptr, -1, -1, token));

    viskores::cont::DeviceAdapterAlgorithm<Device>::Schedule(kernel, ARRAY_SIZE);
    token.DetachFromAll();

    T result = array.ReadPortal().Get(0);
    VISKORES_TEST_ASSERT(result == ((ARRAY_SIZE - 1) * ARRAY_SIZE) / 2,
                         "Got wrong summation in atomic array.");
  }
};

template <typename Device>
void TryArrayOutTransport(Device)
{
  viskores::testing::Testing::TryTypes(TryWholeArrayType<Device>(), viskores::TypeListCommon());
  viskores::testing::Testing::TryTypes(TryAtomicArrayType<Device>(),
                                       viskores::cont::AtomicArrayTypeList());
}

void TestWholeArrayTransport()
{
  std::cout << "Trying WholeArray transport." << std::endl;
  TryArrayOutTransport(viskores::cont::DeviceAdapterTagSerial());
}

} // Anonymous namespace

int UnitTestTransportWholeArray(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestWholeArrayTransport, argc, argv);
}
