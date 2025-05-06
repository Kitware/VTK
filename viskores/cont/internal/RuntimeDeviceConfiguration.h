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
#ifndef viskores_cont_internal_RuntimeDeviceConfiguration_h
#define viskores_cont_internal_RuntimeDeviceConfiguration_h

#include <viskores/cont/viskores_cont_export.h>

#include <viskores/cont/DeviceAdapterTag.h>
#include <viskores/cont/internal/RuntimeDeviceConfigurationOptions.h>

#include <vector>

namespace viskores
{
namespace cont
{
namespace internal
{

enum class RuntimeDeviceConfigReturnCode
{
  SUCCESS,
  OUT_OF_BOUNDS,
  INVALID_FOR_DEVICE,
  INVALID_VALUE,
  NOT_APPLIED
};

class VISKORES_CONT_EXPORT RuntimeDeviceConfigurationBase
{
public:
  VISKORES_CONT virtual ~RuntimeDeviceConfigurationBase() noexcept;
  VISKORES_CONT virtual viskores::cont::DeviceAdapterId GetDevice() const = 0;

  /// Calls the various `Set*` methods in this class with the provided set of config
  /// options which can either be manually provided or automatically initialized
  /// from command line arguments and environment variables via viskores::cont::Initialize.
  /// Each `Set*` method is called only if the corresponding viskores option is set, and a
  /// warning is logged based on the value of the `RuntimeDeviceConfigReturnCode` returned
  /// via the `Set*` method.
  VISKORES_CONT void Initialize(const RuntimeDeviceConfigurationOptions& configOptions);
  VISKORES_CONT void Initialize(const RuntimeDeviceConfigurationOptions& configOptions,
                                int& argc,
                                char* argv[]);

  /// The following public methods should be overriden in each individual device.
  /// A method should return INVALID_FOR_DEVICE if the overriden device does not
  /// support the particular set method.
  VISKORES_CONT virtual RuntimeDeviceConfigReturnCode SetThreads(const viskores::Id& value);
  VISKORES_CONT virtual RuntimeDeviceConfigReturnCode SetDeviceInstance(const viskores::Id& value);

  /// The following public methods are overriden in each individual device and store the
  /// values that were set via the above Set* methods for the given device.
  VISKORES_CONT virtual RuntimeDeviceConfigReturnCode GetThreads(viskores::Id& value) const;
  VISKORES_CONT virtual RuntimeDeviceConfigReturnCode GetDeviceInstance(viskores::Id& value) const;

  /// The following public methods should be overriden as needed for each individual device
  /// as they describe various device parameters.
  VISKORES_CONT virtual RuntimeDeviceConfigReturnCode GetMaxThreads(viskores::Id& value) const;
  VISKORES_CONT virtual RuntimeDeviceConfigReturnCode GetMaxDevices(viskores::Id& value) const;

protected:
  /// An overriden method that can be used to perform extra command line argument parsing
  /// for cases where a specific device may use additional command line arguments. At the
  /// moment Kokkos is the only device that overrides this method.
  /// Note: This method assumes that viskores arguments have already been parsed and removed
  ///       from argv.
  VISKORES_CONT virtual void ParseExtraArguments(int& argc, char* argv[]);

  /// An overriden method that can be used to perform extra initialization after Extra
  /// Arguments are parsed and the Initialized ConfigOptions are used to call the various
  /// Set* methods at the end of Initialize. Particuarly useful when initializing
  /// additional subystems (like Kokkos).
  VISKORES_CONT virtual void InitializeSubsystem();
};

template <typename DeviceAdapterTag>
class RuntimeDeviceConfiguration;

} // namespace viskores::cont::internal
} // namespace viskores::cont
} // namespace viskores

#endif // viskores_cont_internal_RuntimeDeviceConfiguration_h
