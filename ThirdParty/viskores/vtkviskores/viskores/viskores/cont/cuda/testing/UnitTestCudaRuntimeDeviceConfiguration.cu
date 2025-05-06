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
#include <viskores/cont/cuda/DeviceAdapterCuda.h>
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
TestingRuntimeDeviceConfiguration<viskores::cont::DeviceAdapterTagCuda>::TestRuntimeConfig()
{
  auto deviceOptions = TestingRuntimeDeviceConfiguration::DefaultInitializeConfigOptions();
  int numDevices = 0;
  VISKORES_CUDA_CALL(cudaGetDeviceCount(&numDevices));
  viskores::Id selectedDevice = numDevices > 0 ? numDevices - 1 : 0;
  deviceOptions.ViskoresDeviceInstance.SetOption(selectedDevice);
  auto& config =
    RuntimeDeviceInformation{}.GetRuntimeConfiguration(DeviceAdapterTagCuda(), deviceOptions);
  viskores::Id setDevice;
  VISKORES_TEST_ASSERT(config.GetDeviceInstance(setDevice) ==
                         internal::RuntimeDeviceConfigReturnCode::SUCCESS,
                       "Failed to get device instance");
  VISKORES_TEST_ASSERT(setDevice == selectedDevice,
                       "RTC's setDevice != selectedDevice cuda direct! " +
                         std::to_string(setDevice) + " != " + std::to_string(selectedDevice));
  viskores::Id maxDevices;
  VISKORES_TEST_ASSERT(config.GetMaxDevices(maxDevices) ==
                         internal::RuntimeDeviceConfigReturnCode::SUCCESS,
                       "Failed to get max devices");
  VISKORES_TEST_ASSERT(maxDevices == numDevices,
                       "RTC's maxDevices != numDevices cuda direct! " + std::to_string(maxDevices) +
                         " != " + std::to_string(numDevices));
  std::vector<cudaDeviceProp> cudaProps;
  dynamic_cast<internal::RuntimeDeviceConfiguration<viskores::cont::DeviceAdapterTagCuda>&>(config)
    .GetCudaDeviceProp(cudaProps);
  VISKORES_TEST_ASSERT(maxDevices == static_cast<viskores::Id>(cudaProps.size()),
                       "CudaProp's size != maxDevices! " + std::to_string(cudaProps.size()) +
                         " != " + std::to_string(maxDevices));
}

} // namespace viskores::cont::testing
} // namespace viskores::cont
} // namespace viskores

int UnitTestCudaRuntimeDeviceConfiguration(int argc, char* argv[])
{
  return viskores::cont::testing::TestingRuntimeDeviceConfiguration<
    viskores::cont::DeviceAdapterTagCuda>::Run(argc, argv);
}
