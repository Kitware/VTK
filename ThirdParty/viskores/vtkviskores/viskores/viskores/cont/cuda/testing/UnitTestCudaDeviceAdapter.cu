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
#include <viskores/cont/cuda/internal/testing/Testing.h>
#include <viskores/cont/testing/TestingDeviceAdapter.h>

int UnitTestCudaDeviceAdapter(int argc, char* argv[])
{
  auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  tracker.ForceDevice(viskores::cont::DeviceAdapterTagCuda{});
  int result =
    viskores::cont::testing::TestingDeviceAdapter<viskores::cont::DeviceAdapterTagCuda>::Run(argc,
                                                                                             argv);
  return viskores::cont::cuda::internal::Testing::CheckCudaBeforeExit(result);
}
