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

#include <viskores/cont/RuntimeDeviceInformation.h>

#include <viskores/cont/DeviceAdapterTag.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

template <bool>
struct DoesExist;

template <typename DeviceAdapterTag>
void detect_if_exists(DeviceAdapterTag tag)
{
  using DeviceAdapterTraits = viskores::cont::DeviceAdapterTraits<DeviceAdapterTag>;
  std::cout << "testing runtime support for " << DeviceAdapterTraits::GetName() << std::endl;
  DoesExist<tag.IsEnabled> exist;
  exist.Exist(tag);
}

template <>
struct DoesExist<false>
{
  template <typename DeviceAdapterTag>
  void Exist(DeviceAdapterTag) const
  {

    //runtime information for this device should return false
    viskores::cont::RuntimeDeviceInformation runtime;
    VISKORES_TEST_ASSERT(runtime.Exists(DeviceAdapterTag()) == false,
                         "A backend with zero compile time support, can't have runtime support");
  }

  void Exist(viskores::cont::DeviceAdapterTagCuda) const
  {
    //Since we are in a C++ compilation unit the Device Adapter
    //trait should be false. But CUDA could still be enabled.
    //That is why we check VISKORES_ENABLE_CUDA.
    viskores::cont::RuntimeDeviceInformation runtime;
#ifdef VISKORES_ENABLE_CUDA
    VISKORES_TEST_ASSERT(runtime.Exists(viskores::cont::DeviceAdapterTagCuda()) == true,
                         "with cuda backend enabled, runtime support should be enabled");
#else
    VISKORES_TEST_ASSERT(runtime.Exists(viskores::cont::DeviceAdapterTagCuda()) == false,
                         "with cuda backend disabled, runtime support should be disabled");
#endif
  }

#ifdef VISKORES_KOKKOS_CUDA
  void Exist(viskores::cont::DeviceAdapterTagKokkos) const
  {
    //Since we are in a C++ compilation unit the Device Adapter
    //trait should be false. But Kokkos could still be enabled.
    //That is why we check VISKORES_ENABLE_KOKKOS.
    viskores::cont::RuntimeDeviceInformation runtime;
#ifdef VISKORES_ENABLE_KOKKOS
    VISKORES_TEST_ASSERT(runtime.Exists(viskores::cont::DeviceAdapterTagKokkos()) == true,
                         "with kokkos backend enabled, runtime support should be enabled");
#else
    VISKORES_TEST_ASSERT(runtime.Exists(viskores::cont::DeviceAdapterTagKokkos()) == false,
                         "with kokkos backend disabled, runtime support should be disabled");
#endif
  }
#endif
};

template <>
struct DoesExist<true>
{
  template <typename DeviceAdapterTag>
  void Exist(DeviceAdapterTag) const
  {
    //runtime information for this device should return true
    viskores::cont::RuntimeDeviceInformation runtime;
    VISKORES_TEST_ASSERT(runtime.Exists(DeviceAdapterTag()) == true,
                         "A backend with compile time support, should have runtime support");
  }
};

void Detection()
{
  using SerialTag = ::viskores::cont::DeviceAdapterTagSerial;
  using OpenMPTag = ::viskores::cont::DeviceAdapterTagOpenMP;
  using TBBTag = ::viskores::cont::DeviceAdapterTagTBB;
  using CudaTag = ::viskores::cont::DeviceAdapterTagCuda;
  using KokkosTag = ::viskores::cont::DeviceAdapterTagKokkos;

  //Verify that for each device adapter we compile code for, that it
  //has valid runtime support.
  detect_if_exists(SerialTag());
  detect_if_exists(OpenMPTag());
  detect_if_exists(CudaTag());
  detect_if_exists(TBBTag());
  detect_if_exists(KokkosTag());
}

} // anonymous namespace

int UnitTestRuntimeDeviceInformation(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Detection, argc, argv);
}
