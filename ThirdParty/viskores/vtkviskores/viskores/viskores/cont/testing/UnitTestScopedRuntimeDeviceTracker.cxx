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

#include <viskores/cont/RuntimeDeviceTracker.h>

#include <viskores/cont/DeviceAdapterTag.h>

#include <viskores/cont/testing/Testing.h>

#include <algorithm>
#include <array>
#include <thread>

namespace
{

void verify_state(viskores::cont::DeviceAdapterId tag,
                  std::array<bool, VISKORES_MAX_DEVICE_ADAPTER_ID>& defaults)
{
  auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  // presumable all other devices match the defaults
  for (viskores::Int8 i = 1; i < VISKORES_MAX_DEVICE_ADAPTER_ID; ++i)
  {
    const auto deviceId = viskores::cont::make_DeviceAdapterId(i);
    if (deviceId != tag)
    {
      VISKORES_TEST_ASSERT(defaults[static_cast<std::size_t>(i)] == tracker.CanRunOn(deviceId),
                           "ScopedRuntimeDeviceTracker didn't properly setup state correctly");
    }
  }
}

void verify_state_thread(viskores::cont::DeviceAdapterId tag,
                         std::array<bool, VISKORES_MAX_DEVICE_ADAPTER_ID>& defaults,
                         const viskores::cont::RuntimeDeviceTracker& tracker)
{
  // Each thread has its own RuntimeDeviceTracker (to allow you to control different devices
  // on different threads). But that means that each thread creates its own tracker. We
  // want all the threads to respect the runtime set up on the main thread, so copy the state
  // of that tracker (passed as an argument) to this thread.
  viskores::cont::GetRuntimeDeviceTracker().CopyStateFrom(tracker);

  verify_state(tag, defaults);
}

void verify_srdt_support(viskores::cont::DeviceAdapterId tag,
                         std::array<bool, VISKORES_MAX_DEVICE_ADAPTER_ID>& force,
                         std::array<bool, VISKORES_MAX_DEVICE_ADAPTER_ID>& enable,
                         std::array<bool, VISKORES_MAX_DEVICE_ADAPTER_ID>& disable)
{
  viskores::cont::RuntimeDeviceInformation runtime;
  const bool haveSupport = runtime.Exists(tag);
  if (haveSupport)
  {
    viskores::cont::ScopedRuntimeDeviceTracker tracker(
      tag, viskores::cont::RuntimeDeviceTrackerMode::Force);
    VISKORES_TEST_ASSERT(tracker.CanRunOn(tag) == haveSupport, "");
    verify_state(tag, force);
    std::thread(verify_state_thread, tag, std::ref(force), std::ref(tracker)).join();
  }

  if (haveSupport)
  {
    viskores::cont::ScopedRuntimeDeviceTracker tracker(
      tag, viskores::cont::RuntimeDeviceTrackerMode::Enable);
    VISKORES_TEST_ASSERT(tracker.CanRunOn(tag) == haveSupport);
    verify_state(tag, enable);
    std::thread(verify_state_thread, tag, std::ref(enable), std::ref(tracker)).join();
  }

  {
    viskores::cont::ScopedRuntimeDeviceTracker tracker(
      tag, viskores::cont::RuntimeDeviceTrackerMode::Disable);
    VISKORES_TEST_ASSERT(tracker.CanRunOn(tag) == false, "");
    verify_state(tag, disable);
    std::thread(verify_state_thread, tag, std::ref(disable), std::ref(tracker)).join();
  }
}

void VerifyScopedRuntimeDeviceTracker()
{
  // This test requires all valid devices to be on.
  viskores::cont::GetRuntimeDeviceTracker().Reset();

  std::array<bool, VISKORES_MAX_DEVICE_ADAPTER_ID> all_off;
  std::array<bool, VISKORES_MAX_DEVICE_ADAPTER_ID> all_on;
  std::array<bool, VISKORES_MAX_DEVICE_ADAPTER_ID> defaults;

  all_off.fill(false);
  viskores::cont::RuntimeDeviceInformation runtime;
  auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  for (viskores::Int8 i = 1; i < VISKORES_MAX_DEVICE_ADAPTER_ID; ++i)
  {
    auto deviceId = viskores::cont::make_DeviceAdapterId(i);
    defaults[static_cast<std::size_t>(i)] = tracker.CanRunOn(deviceId);
    all_on[static_cast<std::size_t>(i)] = runtime.Exists(deviceId);
  }

  using SerialTag = ::viskores::cont::DeviceAdapterTagSerial;
  using OpenMPTag = ::viskores::cont::DeviceAdapterTagOpenMP;
  using TBBTag = ::viskores::cont::DeviceAdapterTagTBB;
  using CudaTag = ::viskores::cont::DeviceAdapterTagCuda;
  using KokkosTag = ::viskores::cont::DeviceAdapterTagKokkos;
  using AnyTag = ::viskores::cont::DeviceAdapterTagAny;

  //Verify that for each device adapter we compile code for, that it
  //has valid runtime support.
  verify_srdt_support(SerialTag(), all_off, all_on, defaults);
  verify_srdt_support(OpenMPTag(), all_off, all_on, defaults);
  verify_srdt_support(CudaTag(), all_off, all_on, defaults);
  verify_srdt_support(TBBTag(), all_off, all_on, defaults);
  verify_srdt_support(KokkosTag(), all_off, all_on, defaults);

  // Verify that all the ScopedRuntimeDeviceTracker changes
  // have been reverted
  verify_state(AnyTag(), defaults);


  verify_srdt_support(AnyTag(), all_on, all_on, all_off);

  // Verify that all the ScopedRuntimeDeviceTracker changes
  // have been reverted
  verify_state(AnyTag(), defaults);
}

} // anonymous namespace

int UnitTestScopedRuntimeDeviceTracker(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(VerifyScopedRuntimeDeviceTracker, argc, argv);
}
