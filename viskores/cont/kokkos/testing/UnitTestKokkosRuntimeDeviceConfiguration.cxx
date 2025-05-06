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
#include <viskores/cont/kokkos/DeviceAdapterKokkos.h>
#include <viskores/cont/testing/TestingRuntimeDeviceConfiguration.h>

namespace internal = viskores::cont::internal;

namespace viskores
{
namespace cont
{
namespace testing
{

template <>
VISKORES_CONT void
TestingRuntimeDeviceConfiguration<viskores::cont::DeviceAdapterTagKokkos>::TestRuntimeConfig()
{
  int argc;
  char** argv;
  viskores::cont::testing::Testing::MakeArgs(argc, argv, "--kokkos-print-configuration");
  auto deviceOptions = TestingRuntimeDeviceConfiguration::DefaultInitializeConfigOptions();
  deviceOptions.ViskoresDeviceInstance.SetOption(0);
  internal::RuntimeDeviceConfigurationBase& config =
    RuntimeDeviceInformation{}.GetRuntimeConfiguration(
      DeviceAdapterTagKokkos(), deviceOptions, argc, argv);
  VISKORES_TEST_ASSERT(Kokkos::is_initialized(), "Kokkos should be initialized at this point");

  // Test that args are set and the right arg priority is applied
  viskores::Id testValue;
  VISKORES_TEST_ASSERT(config.GetThreads(testValue) ==
                         internal::RuntimeDeviceConfigReturnCode::SUCCESS,
                       "Failed to get set threads");
  VISKORES_TEST_ASSERT(
    testValue == 8, "Set threads does not match expected value: 8 != " + std::to_string(testValue));
  VISKORES_TEST_ASSERT(config.GetDeviceInstance(testValue) ==
                         internal::RuntimeDeviceConfigReturnCode::SUCCESS,
                       "Failed to get set device instance");
  VISKORES_TEST_ASSERT(testValue == 0,
                       "Set device instance does not match expected value: 0 != " +
                         std::to_string(testValue));
  std::cout
    << "Ensure that with kokkos we can't re-initialize or set values after the first initialize"
    << std::endl;
  std::cout << "This should pop up a few warnings in the test logs" << std::endl;
  deviceOptions.ViskoresNumThreads.SetOption(16);
  deviceOptions.ViskoresDeviceInstance.SetOption(5);
  config.Initialize(deviceOptions);
  VISKORES_TEST_ASSERT(config.SetThreads(1) == internal::RuntimeDeviceConfigReturnCode::NOT_APPLIED,
                       "Shouldn't be able to set threads after kokkos is initalized");
  VISKORES_TEST_ASSERT(config.SetDeviceInstance(1) ==
                         internal::RuntimeDeviceConfigReturnCode::NOT_APPLIED,
                       "Shouldn't be able to set device instnace after kokkos is initalized");

  // make sure all the values are the same
  VISKORES_TEST_ASSERT(config.GetThreads(testValue) ==
                         internal::RuntimeDeviceConfigReturnCode::SUCCESS,
                       "Failed to get set threads");
  VISKORES_TEST_ASSERT(
    testValue == 8, "Set threads does not match expected value: 8 != " + std::to_string(testValue));
  VISKORES_TEST_ASSERT(config.GetDeviceInstance(testValue) ==
                         internal::RuntimeDeviceConfigReturnCode::SUCCESS,
                       "Failed to get set device instance");
  VISKORES_TEST_ASSERT(testValue == 0,
                       "Set device instance does not match expected value: 0 != " +
                         std::to_string(testValue));
}

} // namespace viskores::cont::testing
} // namespace viskores::cont
} // namespace viskores

int UnitTestKokkosRuntimeDeviceConfiguration(int argc, char* argv[])
{
  return viskores::cont::testing::TestingRuntimeDeviceConfiguration<
    viskores::cont::DeviceAdapterTagKokkos>::Run(argc, argv);
}
