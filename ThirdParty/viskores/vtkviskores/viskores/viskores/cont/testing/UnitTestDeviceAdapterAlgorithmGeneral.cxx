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

// This test makes sure that the algorithms specified in
// DeviceAdapterAlgorithmGeneral.h are working correctly. It does this by
// creating a test device adapter that uses the serial device adapter for the
// base schedule/scan/sort algorithms and using the general algorithms for
// everything else. Because this test is based of the serial device adapter,
// make sure that UnitTestDeviceAdapterSerial is working before trying to debug
// this one.

// It's OK to compile this without the device compiler.
#ifndef VISKORES_NO_ERROR_ON_MIXED_CUDA_CXX_TAG
#define VISKORES_NO_ERROR_ON_MIXED_CUDA_CXX_TAG 1
#endif

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/RuntimeDeviceTracker.h>
#include <viskores/cont/internal/DeviceAdapterAlgorithmGeneral.h>
#include <viskores/cont/serial/DeviceAdapterSerial.h>

#include <viskores/cont/testing/TestingDeviceAdapter.h>

// Hijack the serial device id so that precompiled units (like memory management) still work.
VISKORES_VALID_DEVICE_ADAPTER(TestAlgorithmGeneral, VISKORES_DEVICE_ADAPTER_SERIAL);

namespace viskores
{
namespace cont
{

template <>
struct DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagTestAlgorithmGeneral>
  : viskores::cont::internal::DeviceAdapterAlgorithmGeneral<
      DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagTestAlgorithmGeneral>,
      viskores::cont::DeviceAdapterTagTestAlgorithmGeneral>
{
private:
  using Algorithm = viskores::cont::DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagSerial>;

  using DeviceAdapterTagTestAlgorithmGeneral = viskores::cont::DeviceAdapterTagTestAlgorithmGeneral;

public:
  template <class Functor>
  VISKORES_CONT static void Schedule(Functor functor, viskores::Id numInstances)
  {
    Algorithm::Schedule(functor, numInstances);
  }

  template <class Functor>
  VISKORES_CONT static void Schedule(Functor functor, viskores::Id3 rangeMax)
  {
    Algorithm::Schedule(functor, rangeMax);
  }

  VISKORES_CONT static void Synchronize() { Algorithm::Synchronize(); }
};

template <>
class DeviceAdapterRuntimeDetector<viskores::cont::DeviceAdapterTagTestAlgorithmGeneral>
{
public:
  /// Returns true as the General Algorithm Device can always be used.
  VISKORES_CONT bool Exists() const { return true; }
};


namespace internal
{

template <>
class DeviceAdapterMemoryManager<viskores::cont::DeviceAdapterTagTestAlgorithmGeneral>
  : public viskores::cont::internal::DeviceAdapterMemoryManagerShared
{
  VISKORES_CONT viskores::cont::DeviceAdapterId GetDevice() const override
  {
    return viskores::cont::DeviceAdapterTagTestAlgorithmGeneral{};
  }
};

}
}
} // namespace viskores::cont::internal

int UnitTestDeviceAdapterAlgorithmGeneral(int argc, char* argv[])
{
  //need to enable DeviceAdapterTagTestAlgorithmGeneral as it
  //is not part of the default set of devices
  auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  tracker.ResetDevice(viskores::cont::DeviceAdapterTagTestAlgorithmGeneral{});

  return viskores::cont::testing::TestingDeviceAdapter<
    viskores::cont::DeviceAdapterTagTestAlgorithmGeneral>::Run(argc, argv);
}
