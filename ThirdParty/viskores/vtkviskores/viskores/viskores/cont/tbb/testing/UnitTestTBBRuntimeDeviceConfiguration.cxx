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
#include <viskores/cont/tbb/DeviceAdapterTBB.h>
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
TestingRuntimeDeviceConfiguration<viskores::cont::DeviceAdapterTagTBB>::TestRuntimeConfig()
{
  auto deviceOptions = TestingRuntimeDeviceConfiguration::DefaultInitializeConfigOptions();
  viskores::Id maxThreads = 0;
  viskores::Id numThreads = 0;
#if TBB_VERSION_MAJOR >= 2020
  maxThreads = ::tbb::global_control::active_value(::tbb::global_control::max_allowed_parallelism);
  numThreads = maxThreads;
#else
  maxThreads = ::tbb::task_scheduler_init::default_num_threads();
  numThreads = maxThreads;
#endif
  numThreads = numThreads / 2;
  deviceOptions.ViskoresNumThreads.SetOption(numThreads);
  auto& config =
    RuntimeDeviceInformation{}.GetRuntimeConfiguration(DeviceAdapterTagTBB(), deviceOptions);
  viskores::Id setNumThreads;
  viskores::Id setMaxThreads;
  VISKORES_TEST_ASSERT(config.GetThreads(setNumThreads) ==
                         internal::RuntimeDeviceConfigReturnCode::SUCCESS,
                       "Failed to get num threads");
  VISKORES_TEST_ASSERT(setNumThreads == numThreads,
                       "RTC's numThreads != numThreads tbb direct! " +
                         std::to_string(setNumThreads) + " != " + std::to_string(numThreads));
  VISKORES_TEST_ASSERT(config.GetMaxThreads(setMaxThreads) ==
                         internal::RuntimeDeviceConfigReturnCode::SUCCESS,
                       "Failed to get max threads");
  VISKORES_TEST_ASSERT(setMaxThreads == maxThreads,
                       "RTC's maxThreads != maxThreads tbb direct! " +
                         std::to_string(setMaxThreads) + " != " + std::to_string(maxThreads));
}

} // namespace viskores::cont::testing
} // namespace viskores::cont
} // namespace viskores

int UnitTestTBBRuntimeDeviceConfiguration(int argc, char* argv[])
{
  return viskores::cont::testing::TestingRuntimeDeviceConfiguration<
    viskores::cont::DeviceAdapterTagTBB>::Run(argc, argv);
}
