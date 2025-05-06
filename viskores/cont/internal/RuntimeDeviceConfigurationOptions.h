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
#ifndef viskores_cont_internal_RuntimeDeviceConfigurationOptions_h
#define viskores_cont_internal_RuntimeDeviceConfigurationOptions_h

#include <viskores/cont/viskores_cont_export.h>

#include <viskores/cont/internal/OptionParserArguments.h>
#include <viskores/cont/internal/RuntimeDeviceOption.h>

#include <vector>

namespace viskores
{
namespace cont
{
namespace internal
{

/// Provides a default set of RuntimeDeviceOptions that viskores currently supports setting.
/// Each option provided in this class should have a corresponding `Set*` method in the
/// RuntimeDeviceConfiguration.
class VISKORES_CONT_EXPORT RuntimeDeviceConfigurationOptions
{
public:
  VISKORES_CONT RuntimeDeviceConfigurationOptions();

  /// Calls the default constructor and additionally pushes back additional command line
  /// options to the provided usage vector for integration with the viskores option parser.
  VISKORES_CONT RuntimeDeviceConfigurationOptions(std::vector<option::Descriptor>& usage);

  /// Allows the caller to initialize these runtime config arguments directly from
  /// command line arguments
  VISKORES_CONT RuntimeDeviceConfigurationOptions(int& argc, char* argv[]);

  VISKORES_CONT virtual ~RuntimeDeviceConfigurationOptions() noexcept;

  /// Calls Initialize for each of this class's current configuration options and marks
  /// the options as initialized.
  VISKORES_CONT void Initialize(const option::Option* options);
  VISKORES_CONT bool IsInitialized() const;

  RuntimeDeviceOption ViskoresNumThreads;
  RuntimeDeviceOption ViskoresDeviceInstance;

protected:
  /// Sets the option indices and environment varaible names for the viskores supported options.
  /// If useOptionIndex is set the OptionParserArguments enum for option indices will be used,
  /// otherwise ints from 0 - numOptions will be used.
  VISKORES_CONT RuntimeDeviceConfigurationOptions(const bool& useOptionIndex);

private:
  bool Initialized;
};

} // namespace viskores::cont::internal
} // namespace viskores::cont
} // namespace viskores

#endif // viskores_cont_internal_RuntimeDeviceConfigurationOptions_h
