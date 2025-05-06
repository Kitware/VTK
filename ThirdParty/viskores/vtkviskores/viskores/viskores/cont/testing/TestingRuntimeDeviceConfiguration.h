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
#ifndef viskores_cont_testing_TestingRuntimeDeviceConfiguration_h
#define viskores_cont_testing_TestingRuntimeDeviceConfiguration_h

#include <viskores/cont/DeviceAdapterTag.h>
#include <viskores/cont/RuntimeDeviceInformation.h>
#include <viskores/cont/RuntimeDeviceTracker.h>
#include <viskores/cont/internal/RuntimeDeviceConfiguration.h>
#include <viskores/cont/internal/RuntimeDeviceConfigurationOptions.h>
#include <viskores/cont/testing/Testing.h>

namespace internal = viskores::cont::internal;

namespace viskores
{
namespace cont
{
namespace testing
{

template <class DeviceAdapterTag>
struct TestingRuntimeDeviceConfiguration
{

  VISKORES_CONT
  static internal::RuntimeDeviceConfigurationOptions DefaultInitializeConfigOptions()
  {
    internal::RuntimeDeviceConfigurationOptions runtimeDeviceOptions{};
    runtimeDeviceOptions.ViskoresNumThreads.SetOption(8);
    runtimeDeviceOptions.ViskoresDeviceInstance.SetOption(2);
    runtimeDeviceOptions.Initialize(nullptr);
    VISKORES_TEST_ASSERT(runtimeDeviceOptions.IsInitialized(),
                         "Failed to default initialize runtime config options.");
    return runtimeDeviceOptions;
  }

  VISKORES_CONT static void TestRuntimeConfig(){};

  struct TestRunner
  {
    VISKORES_CONT
    void operator()() const { TestRuntimeConfig(); }
  };

  static VISKORES_CONT int Run(int, char*[])
  {
    // For the Kokkos version of this test we don't not want Initialize to be called
    // so we directly execute the testing functor instead of calling Run
    return viskores::cont::testing::Testing::ExecuteFunction(TestRunner{});
  }
};

} // namespace viskores::cont::testing
} // namespace viskores::cont
} // namespace viskores

#endif // viskores_cont_testing_TestingRuntimeDeviceConfiguration_h
