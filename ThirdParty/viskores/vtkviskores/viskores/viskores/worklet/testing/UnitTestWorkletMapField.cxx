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
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>

#include <viskores/cont/UncertainArrayHandle.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/Testing.h>

class TestMapFieldWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut, FieldInOut);
  using ExecutionSignature = _3(_1, _2, _3, WorkIndex);

  template <typename T>
  VISKORES_EXEC T operator()(const T& in, T& out, T& inout, viskores::Id workIndex) const
  {
    if (!test_equal(in, TestValue(workIndex, T()) + T(100)))
    {
      this->RaiseError("Got wrong input value.");
    }
    out = static_cast<T>(in - T(100));
    if (!test_equal(inout, TestValue(workIndex, T()) + T(100)))
    {
      this->RaiseError("Got wrong in-out value.");
    }

    // We return the new value of inout. Since _3 is both an arg and return,
    // this tests that the return value is set after updating the arg values.
    return static_cast<T>(inout - T(100));
  }

  template <typename T1, typename T2, typename T3>
  VISKORES_EXEC T3 operator()(const T1&, const T2&, const T3&, viskores::Id) const
  {
    this->RaiseError("Cannot call this worklet with different types.");
    return viskores::TypeTraits<T3>::ZeroInitialization();
  }
};

namespace mapfield
{
static constexpr viskores::Id ARRAY_SIZE = 10;

template <typename WorkletType>
struct DoStaticTestWorklet
{
  template <typename T>
  VISKORES_CONT void operator()(T) const
  {
    std::cout << "Set up data." << std::endl;
    T inputArray[ARRAY_SIZE];

    for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
    {
      inputArray[index] = static_cast<T>(TestValue(index, T()) + T(100));
    }

    viskores::cont::ArrayHandle<T> inputHandle =
      viskores::cont::make_ArrayHandle(inputArray, ARRAY_SIZE, viskores::CopyFlag::On);
    viskores::cont::ArrayHandle<T> outputHandle, outputHandleAsPtr;
    viskores::cont::ArrayHandle<T> inoutHandle, inoutHandleAsPtr;

    viskores::cont::ArrayCopy(inputHandle, inoutHandle);
    viskores::cont::ArrayCopy(inputHandle, inoutHandleAsPtr);

    std::cout << "Create and run dispatchers." << std::endl;
    viskores::worklet::DispatcherMapField<WorkletType> dispatcher;
    dispatcher.Invoke(inputHandle, outputHandle, inoutHandle);
    dispatcher.Invoke(&inputHandle, &outputHandleAsPtr, &inoutHandleAsPtr);

    std::cout << "Check results." << std::endl;
    CheckPortal(outputHandle.ReadPortal());
    CheckPortal(inoutHandle.ReadPortal());
    CheckPortal(outputHandleAsPtr.ReadPortal());
    CheckPortal(inoutHandleAsPtr.ReadPortal());

    std::cout << "Try to invoke with an input array of the wrong size." << std::endl;
    inputHandle.Allocate(ARRAY_SIZE / 2, viskores::CopyFlag::On);
    bool exceptionThrown = false;
    try
    {
      dispatcher.Invoke(inputHandle, outputHandle, inoutHandle);
    }
    catch (viskores::cont::ErrorBadValue& error)
    {
      std::cout << "  Caught expected error: " << error.GetMessage() << std::endl;
      exceptionThrown = true;
    }
    VISKORES_TEST_ASSERT(exceptionThrown, "Dispatcher did not throw expected exception.");
  }
};

template <typename WorkletType>
struct DoVariantTestWorklet
{
  template <typename T>
  VISKORES_CONT void operator()(T) const
  {
    std::cout << "Set up data." << std::endl;
    T inputArray[ARRAY_SIZE];

    for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
    {
      inputArray[index] = static_cast<T>(TestValue(index, T()) + T(100));
    }

    viskores::cont::ArrayHandle<T> inputHandle =
      viskores::cont::make_ArrayHandle(inputArray, ARRAY_SIZE, viskores::CopyFlag::On);
    viskores::cont::ArrayHandle<T> outputHandle;
    viskores::cont::ArrayHandle<T> inoutHandle;


    std::cout << "Create and run dispatcher with unknown arrays." << std::endl;
    viskores::worklet::DispatcherMapField<WorkletType> dispatcher;

    using UncertainArrayType =
      viskores::cont::UncertainArrayHandle<viskores::List<T>, VISKORES_DEFAULT_STORAGE_LIST>;
    UncertainArrayType inputVariant(inputHandle);

    { //Verify we can pass by value
      viskores::cont::ArrayCopy(inputHandle, inoutHandle);
      viskores::cont::UnknownArrayHandle outputVariant(outputHandle);
      viskores::cont::UnknownArrayHandle inoutVariant(inoutHandle);
      dispatcher.Invoke(
        inputVariant
          .template ResetTypes<viskores::List<T>, viskores::List<VISKORES_DEFAULT_STORAGE_TAG>>(),
        outputVariant.ResetTypes<viskores::List<T>, viskores::List<VISKORES_DEFAULT_STORAGE_TAG>>(),
        inoutVariant.ResetTypes<viskores::List<T>, viskores::List<VISKORES_DEFAULT_STORAGE_TAG>>());
      CheckPortal(outputHandle.ReadPortal());
      CheckPortal(inoutHandle.ReadPortal());
    }

    { //Verify we can pass by pointer
      UncertainArrayType outputVariant(outputHandle);
      UncertainArrayType inoutVariant(inoutHandle);

      viskores::cont::ArrayCopy(inputHandle, inoutHandle);
      dispatcher.Invoke(&inputVariant, outputHandle, inoutHandle);
      CheckPortal(outputHandle.ReadPortal());
      CheckPortal(inoutHandle.ReadPortal());

      viskores::cont::ArrayCopy(inputHandle, inoutHandle);
      dispatcher.Invoke(inputHandle, &outputVariant, inoutHandle);
      CheckPortal(outputHandle.ReadPortal());
      CheckPortal(inoutHandle.ReadPortal());

      viskores::cont::ArrayCopy(inputHandle, inoutHandle);
      dispatcher.Invoke(inputHandle, outputHandle, &inoutVariant);
      CheckPortal(outputHandle.ReadPortal());
      CheckPortal(inoutHandle.ReadPortal());
    }
  }
};

template <typename WorkletType>
struct DoTestWorklet
{
  template <typename T>
  VISKORES_CONT void operator()(T t) const
  {
    DoStaticTestWorklet<WorkletType> sw;
    sw(t);
    DoVariantTestWorklet<WorkletType> dw;
    dw(t);
  }
};

void TestWorkletMapField(viskores::cont::DeviceAdapterId id)
{
  std::cout << "Testing Map Field on device adapter: " << id.GetName() << std::endl;

  viskores::testing::Testing::TryTypes(mapfield::DoTestWorklet<TestMapFieldWorklet>(),
                                       viskores::TypeListCommon());
}

} // mapfield namespace

int UnitTestWorkletMapField(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::RunOnDevice(mapfield::TestWorkletMapField, argc, argv);
}
