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
#include <viskores/cont/Error.h>
#include <viskores/cont/ErrorBadAllocation.h>
#include <viskores/cont/ErrorBadDevice.h>
#include <viskores/cont/ErrorBadType.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/RuntimeDeviceTracker.h>
#include <viskores/cont/TryExecute.h>
#include <viskores/cont/serial/DeviceAdapterSerial.h>

#include <viskores/cont/testing/Testing.h>

#include <exception>

namespace
{

static constexpr viskores::Id ARRAY_SIZE = 10;

class ErrorDeviceIndependent : public viskores::cont::Error
{
public:
  ErrorDeviceIndependent(const std::string& msg)
    : viskores::cont::Error(msg, true)
  {
  }
};

class ErrorDeviceDependent : public viskores::cont::Error
{
public:
  ErrorDeviceDependent(const std::string& msg)
    : viskores::cont::Error(msg, false)
  {
  }
};

struct TryExecuteTestFunctor
{
  viskores::IdComponent NumCalls;

  VISKORES_CONT
  TryExecuteTestFunctor()
    : NumCalls(0)
  {
  }

  template <typename Device>
  VISKORES_CONT bool operator()(Device,
                                const viskores::cont::ArrayHandle<viskores::FloatDefault>& in,
                                viskores::cont::ArrayHandle<viskores::FloatDefault>& out)
  {
    using Algorithm = viskores::cont::DeviceAdapterAlgorithm<Device>;
    Algorithm::Copy(in, out);
    this->NumCalls++;
    return true;
  }
};

template <typename ExceptionT>
struct TryExecuteTestErrorFunctor
{
  template <typename Device>
  VISKORES_CONT bool operator()(Device)
  {
    throw ExceptionT("Test message");
  }
};

template <typename DeviceList>
void TryExecuteTests(DeviceList, bool expectSuccess)
{
  viskores::cont::ArrayHandle<viskores::FloatDefault> inArray;
  viskores::cont::ArrayHandle<viskores::FloatDefault> outArray;

  inArray.Allocate(ARRAY_SIZE);
  SetPortal(inArray.WritePortal());

  TryExecuteTestFunctor functor;

  bool result = viskores::cont::TryExecute(functor, DeviceList(), inArray, outArray);

  if (expectSuccess)
  {
    VISKORES_TEST_ASSERT(result, "Call returned failure when expected success.");
    VISKORES_TEST_ASSERT(functor.NumCalls == 1, "Bad number of calls");
    CheckPortal(outArray.ReadPortal());
  }
  else
  {
    VISKORES_TEST_ASSERT(!result, "Call returned true when expected failure.");
  }

  //verify the ability to pass rvalue functors
  viskores::cont::ArrayHandle<viskores::FloatDefault> outArray2;
  result = viskores::cont::TryExecute(TryExecuteTestFunctor(), DeviceList(), inArray, outArray2);
  if (expectSuccess)
  {
    VISKORES_TEST_ASSERT(result, "Call returned failure when expected success.");
    CheckPortal(outArray2.ReadPortal());
  }
  else
  {
    VISKORES_TEST_ASSERT(!result, "Call returned true when expected failure.");
  }
}

struct EdgeCaseFunctor
{
  template <typename DeviceList>
  bool operator()(DeviceList, int, float, bool) const
  {
    return true;
  }
  template <typename DeviceList>
  bool operator()(DeviceList) const
  {
    return true;
  }
};

void TryExecuteAllEdgeCases()
{
  using ValidDevice = viskores::cont::DeviceAdapterTagSerial;
  using SingleValidList = viskores::List<ValidDevice>;

  std::cout << "TryExecute no Runtime, no Device, no parameters." << std::endl;
  viskores::cont::TryExecute(EdgeCaseFunctor());

  std::cout << "TryExecute no Runtime, no Device, with parameters." << std::endl;
  viskores::cont::TryExecute(EdgeCaseFunctor(), int{ 42 }, float{ 3.14f }, bool{ true });

  std::cout << "TryExecute with Device, no parameters." << std::endl;
  viskores::cont::TryExecute(EdgeCaseFunctor(), SingleValidList());

  std::cout << "TryExecute with Device, with parameters." << std::endl;
  viskores::cont::TryExecute(
    EdgeCaseFunctor(), SingleValidList(), int{ 42 }, float{ 3.14f }, bool{ true });
}

template <typename ExceptionType>
void RunErrorTest(bool shouldFail, bool shouldThrow, bool shouldDisable)
{
  using Device = viskores::cont::DeviceAdapterTagSerial;
  using Functor = TryExecuteTestErrorFunctor<ExceptionType>;

  // Initialize this one to what we expect -- it won't get set if we throw.
  bool succeeded = !shouldFail;
  bool threw = false;
  bool disabled = false;

  viskores::cont::ScopedRuntimeDeviceTracker scopedTracker(Device{});

  try
  {
    succeeded = viskores::cont::TryExecute(Functor{});
    threw = false;
  }
  catch (...)
  {
    threw = true;
  }

  auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  disabled = !tracker.CanRunOn(Device{});

  std::cout << "Failed: " << !succeeded << " "
            << "Threw: " << threw << " "
            << "Disabled: " << disabled << "\n"
            << std::endl;

  VISKORES_TEST_ASSERT(shouldFail == !succeeded, "TryExecute return status incorrect.");
  VISKORES_TEST_ASSERT(threw == shouldThrow, "TryExecute throw behavior incorrect.");
  VISKORES_TEST_ASSERT(disabled == shouldDisable,
                       "TryExecute device-disabling behavior incorrect.");
}

void TryExecuteErrorTests()
{
  std::cout << "Test ErrorBadAllocation." << std::endl;
  RunErrorTest<viskores::cont::ErrorBadAllocation>(true, false, true);

  std::cout << "Test ErrorBadDevice." << std::endl;
  RunErrorTest<viskores::cont::ErrorBadDevice>(true, false, true);

  std::cout << "Test ErrorBadType." << std::endl;
  RunErrorTest<viskores::cont::ErrorBadType>(true, false, false);

  std::cout << "Test ErrorBadValue." << std::endl;
  RunErrorTest<viskores::cont::ErrorBadValue>(true, true, false);

  std::cout << "Test custom viskores Error (dev indep)." << std::endl;
  RunErrorTest<ErrorDeviceIndependent>(true, true, false);

  std::cout << "Test custom viskores Error (dev dep)." << std::endl;
  RunErrorTest<ErrorDeviceDependent>(true, false, false);

  std::cout << "Test std::exception." << std::endl;
  RunErrorTest<std::runtime_error>(true, false, false);

  std::cout << "Test throw non-exception." << std::endl;
  RunErrorTest<std::string>(true, false, false);
}

static void Run()
{
  // This test requires all available devices to be enabled.
  viskores::cont::GetRuntimeDeviceTracker().Reset();

  using ValidDevice = viskores::cont::DeviceAdapterTagSerial;
  using InvalidDevice = viskores::cont::DeviceAdapterTagUndefined;

  TryExecuteAllEdgeCases();

  std::cout << "Try a list with a single entry." << std::endl;
  using SingleValidList = viskores::List<ValidDevice>;
  TryExecuteTests(SingleValidList(), true);

  std::cout << "Try a list with two valid devices." << std::endl;
  using DoubleValidList = viskores::List<ValidDevice, ValidDevice>;
  TryExecuteTests(DoubleValidList(), true);

  std::cout << "Try a list with only invalid device." << std::endl;
  using SingleInvalidList = viskores::List<InvalidDevice>;
  TryExecuteTests(SingleInvalidList(), false);

  std::cout << "Try a list with an invalid and valid device." << std::endl;
  using InvalidAndValidList = viskores::List<InvalidDevice, ValidDevice>;
  TryExecuteTests(InvalidAndValidList(), true);

  TryExecuteErrorTests();
}

} // anonymous namespace

int UnitTestTryExecute(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
