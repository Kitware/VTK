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
#ifndef viskores_cont_RuntimeDeviceInformation_h
#define viskores_cont_RuntimeDeviceInformation_h

#include <viskores/cont/DeviceAdapterTag.h>
#include <viskores/cont/internal/DeviceAdapterMemoryManager.h>
#include <viskores/cont/internal/RuntimeDeviceConfiguration.h>
#include <viskores/internal/ExportMacros.h>

namespace viskores
{
namespace cont
{

/// A class that can be used to determine if a given device adapter
/// is supported on the current machine at runtime. This is very important
/// for device adapters where a physical hardware requirements such as a GPU
/// or a Accelerator Card is needed for support to exist.
///
///
class VISKORES_CONT_EXPORT RuntimeDeviceInformation
{
public:
  /// Returns the name corresponding to the device adapter id. If @a id is
  /// not recognized, `InvalidDeviceId` is returned. Queries for a
  /// name are all case-insensitive.
  VISKORES_CONT
  DeviceAdapterNameType GetName(DeviceAdapterId id) const;

  /// Returns the id corresponding to the device adapter name. If @a name is
  /// not recognized, DeviceAdapterTagUndefined is returned.
  VISKORES_CONT
  DeviceAdapterId GetId(DeviceAdapterNameType name) const;

  /// Returns true if the given device adapter is supported on the current
  /// machine.
  ///
  VISKORES_CONT
  bool Exists(DeviceAdapterId id) const;

  /// Returns a reference to a `DeviceAdapterMemoryManager` that will work with the
  /// given device. This method will throw an exception if the device id is not a
  /// real device (for example `DeviceAdapterTagAny`). If the device in question is
  /// not valid, a `DeviceAdapterMemoryManager` will be returned, but attempting to
  /// call any of the methods will result in a runtime exception.
  ///
  VISKORES_CONT
  viskores::cont::internal::DeviceAdapterMemoryManagerBase& GetMemoryManager(
    DeviceAdapterId id) const;

  /// Returns a reference to a `RuntimeDeviceConfiguration` that will work with the
  /// given device. If the device in question is not valid, a placeholder
  /// `InvalidRuntimeDeviceConfiguration` will be returned. Attempting to
  /// call any of the methods of this object will result in a runtime exception.
  /// The fully loaded version of this method is automatically called at the end
  /// of `vkmt::cont::Initialize` which performs automated setup of all runtime
  /// devices using parsed viskores arguments.
  ///
  /// params:
  ///   id - The specific device to retreive the RuntimeDeviceConfiguration options for
  ///   configOptions - Viskores provided options that should be included when initializing
  ///                   a given RuntimeDeviceConfiguration
  ///   argc - The number of command line arguments to parse when Initializing
  ///          a given RuntimeDeviceConfiguration
  ///   argv - The extra command line arguments to parse when Initializing a given
  ///          RuntimeDeviceConfiguration. This argument is mainlued used in conjuction
  ///          with Kokkos config arg parsing to include specific --kokkos command
  ///          line flags and environment variables.
  VISKORES_CONT
  viskores::cont::internal::RuntimeDeviceConfigurationBase& GetRuntimeConfiguration(
    DeviceAdapterId id,
    const viskores::cont::internal::RuntimeDeviceConfigurationOptions& configOptions,
    int& argc,
    char* argv[] = nullptr) const;

  VISKORES_CONT
  viskores::cont::internal::RuntimeDeviceConfigurationBase& GetRuntimeConfiguration(
    DeviceAdapterId id,
    const viskores::cont::internal::RuntimeDeviceConfigurationOptions& configOptions) const;

  VISKORES_CONT
  viskores::cont::internal::RuntimeDeviceConfigurationBase& GetRuntimeConfiguration(
    DeviceAdapterId id) const;
};
} // namespace viskores::cont
} // namespace viskores

#endif //viskores_cont_RuntimeDeviceInformation_h
