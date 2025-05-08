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

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleImplicit.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/serial/DeviceAdapterSerial.h>

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/VecTraits.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

const viskores::Id ARRAY_SIZE = 10;

template <typename ValueType>
struct IndexSquared
{
  VISKORES_EXEC_CONT
  ValueType operator()(viskores::Id i) const
  {
    using ComponentType = typename viskores::VecTraits<ValueType>::ComponentType;
    return ValueType(static_cast<ComponentType>(i * i));
  }
};

struct PassThrough : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename InValue, typename OutValue>
  VISKORES_EXEC void operator()(const InValue& inValue, OutValue& outValue) const
  {
    outValue = inValue;
  }
};

struct ImplicitTests
{
  template <typename ValueType>
  void operator()(const ValueType) const
  {
    using FunctorType = IndexSquared<ValueType>;
    FunctorType functor;

    using ImplicitHandle = viskores::cont::ArrayHandleImplicit<FunctorType>;

    ImplicitHandle implicit = viskores::cont::make_ArrayHandleImplicit(functor, ARRAY_SIZE);

    std::cout << "verify that the control portal works" << std::endl;
    auto implicitPortal = implicit.ReadPortal();
    for (int i = 0; i < ARRAY_SIZE; ++i)
    {
      const ValueType v = implicitPortal.Get(i);
      const ValueType correct_value = functor(i);
      VISKORES_TEST_ASSERT(v == correct_value, "Implicit Handle Failed");
    }

    std::cout << "verify that the execution portal works" << std::endl;
    viskores::cont::Token token;
    using Device = viskores::cont::DeviceAdapterTagSerial;
    using CEPortal = typename ImplicitHandle::ReadPortalType;
    CEPortal execPortal = implicit.PrepareForInput(Device(), token);
    for (int i = 0; i < ARRAY_SIZE; ++i)
    {
      const ValueType v = execPortal.Get(i);
      const ValueType correct_value = functor(i);
      VISKORES_TEST_ASSERT(v == correct_value, "Implicit Handle Failed");
    }

    std::cout << "verify that the array handle works in a worklet on the device" << std::endl;
    viskores::cont::Invoker invoke;
    viskores::cont::ArrayHandle<ValueType> result;
    invoke(PassThrough{}, implicit, result);
    auto resultPortal = result.ReadPortal();
    for (int i = 0; i < ARRAY_SIZE; ++i)
    {
      const ValueType value = resultPortal.Get(i);
      const ValueType correctValue = functor(i);
      VISKORES_TEST_ASSERT(test_equal(value, correctValue));
    }
  }
};

void TestArrayHandleImplicit()
{
  viskores::testing::Testing::TryTypes(ImplicitTests(), viskores::TypeListCommon());
}

} // anonymous namespace

int UnitTestArrayHandleImplicit(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestArrayHandleImplicit, argc, argv);
}
