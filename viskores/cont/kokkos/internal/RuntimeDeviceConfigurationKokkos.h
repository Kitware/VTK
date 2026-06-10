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
#ifndef viskores_cont_kokkos_internal_RuntimeDeviceConfigurationKokkos_h
#define viskores_cont_kokkos_internal_RuntimeDeviceConfigurationKokkos_h

#include <viskores/cont/ErrorInternal.h>
#include <viskores/cont/Logging.h>
#include <viskores/cont/internal/RuntimeDeviceConfiguration.h>
#include <viskores/cont/kokkos/internal/DeviceAdapterTagKokkos.h>

VISKORES_THIRDPARTY_PRE_INCLUDE
#include <Kokkos_Core.hpp>
VISKORES_THIRDPARTY_POST_INCLUDE

#include <cstring>
#include <vector>

namespace viskores
{
namespace cont
{
namespace internal
{

template <>
class RuntimeDeviceConfiguration<viskores::cont::DeviceAdapterTagKokkos>
  : public viskores::cont::internal::RuntimeDeviceConfigurationBase
{
public:
  VISKORES_CONT viskores::cont::DeviceAdapterId GetDevice() const override final;

  VISKORES_CONT virtual RuntimeDeviceConfigReturnCode SetThreads(
    const viskores::Id& value) override final;

  VISKORES_CONT virtual RuntimeDeviceConfigReturnCode SetDeviceInstance(
    const viskores::Id& value) override final;

  VISKORES_CONT virtual RuntimeDeviceConfigReturnCode GetThreads(
    viskores::Id& value) const override final;

  VISKORES_CONT virtual RuntimeDeviceConfigReturnCode GetDeviceInstance(
    viskores::Id& value) const override final;

protected:
  /// Store a copy of the current arguments when initializing the Kokkos subsystem later
  /// Appends a copy of the argv values in the KokkosArguments vector: this assumes the
  /// argv values contain kokkos command line arguments (like --kokkos-num-threads, etc)
  VISKORES_CONT virtual void ParseExtraArguments(int& argc, char* argv[]) override final;

  /// Calls kokkos initiailze if kokkos has not been initialized yet and sets up an atexit
  /// to call kokkos finalize. Converts the KokkosArguments vector to a standard argc/argv
  /// list of arguments when calling kokkos initialize.
  ///
  /// When using viskores::Initialize, the standard order for kokkos argument priority is as
  /// follows (this assumes kokkos still prioritizes arguments found at the end of the
  /// argv list over similarly named arguements found earlier in the list):
  ///   1. Environment Variables
  ///   2. Kokkos Command Line Arguments
  ///   3. Viskores Interpreted Command Line Arguements
  VISKORES_CONT virtual void InitializeSubsystem() override final;

private:
  std::vector<std::string> KokkosArguments;
};

} // namespace viskores::cont::internal
} // namespace viskores::cont
} // namespace viskores

#endif //viskores_cont_kokkos_internal_RuntimeDeviceConfigurationKokkos_h
