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

#include <viskores/cont/DeviceAdapterList.h>
#include <viskores/cont/RuntimeDeviceTracker.h>
#include <viskores/cont/Timer.h>

#include <viskores/cont/testing/Testing.h>

#include <chrono>
#include <thread>

namespace
{

using TimerTestDevices = viskores::ListAppend<VISKORES_DEFAULT_DEVICE_ADAPTER_LIST,
                                              viskores::List<viskores::cont::DeviceAdapterTagAny>>;

constexpr long long waitTimeMilliseconds = 5;

struct Waiter
{
  std::chrono::high_resolution_clock::time_point Start = std::chrono::high_resolution_clock::now();
  long long ExpectedTimeMilliseconds = 0;

  viskores::Float64 Wait()
  {
    // Update when we want to wait to.
    this->ExpectedTimeMilliseconds += waitTimeMilliseconds;
    viskores::Float64 expectedTimeSeconds =
      viskores::Float64(this->ExpectedTimeMilliseconds) / 1000;

    long long elapsedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                                      std::chrono::high_resolution_clock::now() - this->Start)
                                      .count();

    long long millisecondsToSleep = this->ExpectedTimeMilliseconds - elapsedMilliseconds;

    std::cout << "  Sleeping for " << millisecondsToSleep << "ms (to " << expectedTimeSeconds
              << "s)\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(millisecondsToSleep));

    return expectedTimeSeconds;
  }
};

void CheckTime(const viskores::cont::Timer& timer, viskores::Float64 expectedTime)
{
  viskores::Float64 elapsedTime = timer.GetElapsedTime();
  VISKORES_TEST_ASSERT(
    elapsedTime > (expectedTime - 0.001), "Timer did not capture full wait. ", elapsedTime);
}

void DoTimerCheck(viskores::cont::Timer& timer)
{
  // Before starting the timer, synchronize the device. Some timers do not record
  // the start time as the time `Start` is called. Rather, if operations are still
  // pending on the device, the timer will start recording after those operations
  // complete. To make sure there are no pending operations, call `Synchronize`.
  timer.Synchronize();

  std::cout << "  Starting timer\n";
  timer.Start();
  VISKORES_TEST_ASSERT(timer.Started(), "Timer fails to track started status");
  VISKORES_TEST_ASSERT(!timer.Stopped(), "Timer fails to track non stopped status");

  Waiter waiter;

  viskores::Float64 expectedTime = 0.0;
  CheckTime(timer, expectedTime);

  expectedTime = waiter.Wait();

  CheckTime(timer, expectedTime);

  std::cout << "  Make sure timer is still running\n";
  VISKORES_TEST_ASSERT(!timer.Stopped(), "Timer fails to track stopped status");

  expectedTime = waiter.Wait();

  CheckTime(timer, expectedTime);

  std::cout << "  Stop the timer\n";
  timer.Stop();
  VISKORES_TEST_ASSERT(timer.Stopped(), "Timer fails to track stopped status");

  CheckTime(timer, expectedTime);

  waiter.Wait(); // Do not advanced expected time

  std::cout << "  Check that timer legitimately stopped\n";
  CheckTime(timer, expectedTime);
}

struct TimerCheckFunctor
{
  void operator()(viskores::cont::DeviceAdapterId device) const
  {
    if ((device != viskores::cont::DeviceAdapterTagAny()) &&
        !viskores::cont::GetRuntimeDeviceTracker().CanRunOn(device))
    {
      // A timer will not work if set on a device that is not supported. Just skip this test.
      return;
    }

    {
      viskores::cont::Timer timer(device);
      DoTimerCheck(timer);
    }
    {
      viskores::cont::Timer timer;
      timer.Reset(device);
      DoTimerCheck(timer);
    }
    {
      viskores::cont::GetRuntimeDeviceTracker().DisableDevice(device);
      viskores::cont::Timer timer(device);
      viskores::cont::GetRuntimeDeviceTracker().ResetDevice(device);
      DoTimerCheck(timer);
    }
    {
      viskores::cont::ScopedRuntimeDeviceTracker scoped(device);
      viskores::cont::Timer timer(device);
      timer.Start();
      VISKORES_TEST_ASSERT(timer.Started(), "Timer fails to track started status");
      //simulate a device failing
      scoped.DisableDevice(device);
      Waiter waiter;
      waiter.Wait();
      CheckTime(timer, 0.0);
    }
  }
};

void DoTimerTest()
{
  std::cout << "Check default timer\n";
  viskores::cont::Timer timer;
  DoTimerCheck(timer);

  viskores::ListForEach(TimerCheckFunctor(), TimerTestDevices());
}

} // anonymous namespace

int UnitTestTimer(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DoTimerTest, argc, argv);
}
