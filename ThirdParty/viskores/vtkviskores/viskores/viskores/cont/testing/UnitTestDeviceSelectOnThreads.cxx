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

#include <viskores/cont/Initialize.h>
#include <viskores/cont/RuntimeDeviceTracker.h>

#include <viskores/cont/testing/Testing.h>

#include <future>
#include <vector>

namespace
{

bool CheckLocalRuntime()
{
  if (!viskores::cont::GetRuntimeDeviceTracker().CanRunOn(viskores::cont::DeviceAdapterTagSerial{}))
  {
    std::cout << "Serial device not runable" << std::endl;
    return false;
  }

  for (viskores::Int8 deviceIndex = 0; deviceIndex < VISKORES_MAX_DEVICE_ADAPTER_ID; ++deviceIndex)
  {
    viskores::cont::DeviceAdapterId device = viskores::cont::make_DeviceAdapterId(deviceIndex);
    if (!device.IsValueValid() || (deviceIndex == VISKORES_DEVICE_ADAPTER_SERIAL))
    {
      continue;
    }
    if (viskores::cont::GetRuntimeDeviceTracker().CanRunOn(device))
    {
      std::cout << "Device " << device.GetName() << " declared as runnable" << std::endl;
      return false;
    }
  }

  return true;
}

void DoTest()
{
  VISKORES_TEST_ASSERT(
    CheckLocalRuntime(),
    "Runtime check failed on main thread. Did you try to set a device argument?");

  // Now check on a new thread. The runtime is a thread-local object so that each thread can
  // use its own device. But when you start a thread, you want the default to be what the
  // user selected on the main thread.
  VISKORES_TEST_ASSERT(std::async(std::launch::async, CheckLocalRuntime).get(),
                       "Runtime loses defaults in spawned thread.");
}

} // anonymous namespace

int UnitTestDeviceSelectOnThreads(int argc, char* argv[])
{
  // This test is checking to make sure that a device selected in the command line
  // argument is the default for all threads. We will test this by adding an argument
  // to select the serial device, which is always available. The test might fail if
  // a different device is also selected.
  std::string deviceSelectString("--viskores-device=serial");
  std::vector<char> deviceSelectArg(deviceSelectString.size());
  std::copy(deviceSelectString.begin(), deviceSelectString.end(), deviceSelectArg.begin());
  deviceSelectArg.push_back('\0');

  std::vector<char*> newArgs;
  for (int i = 0; i < argc; ++i)
  {
    if (std::strncmp(argv[i], "--viskores-device", 13) != 0)
    {
      newArgs.push_back(argv[i]);
    }
  }
  newArgs.push_back(deviceSelectArg.data());
  newArgs.push_back(nullptr);

  int newArgc = static_cast<int>(newArgs.size() - 1);

  return viskores::cont::testing::Testing::Run(DoTest, newArgc, newArgs.data());
}
