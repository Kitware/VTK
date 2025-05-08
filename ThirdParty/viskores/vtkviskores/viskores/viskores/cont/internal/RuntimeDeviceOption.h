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
#ifndef viskores_cont_internal_RuntimeDeviceOption_h
#define viskores_cont_internal_RuntimeDeviceOption_h

#include <viskores/cont/viskores_cont_export.h>

#include <viskores/cont/internal/OptionParser.h>
#include <viskores/cont/internal/OptionParserArguments.h>

namespace viskores
{
namespace cont
{
namespace internal
{

enum class RuntimeDeviceOptionSource
{
  COMMAND_LINE,
  ENVIRONMENT,
  IN_CODE,
  NOT_SET
};

class VISKORES_CONT_EXPORT RuntimeDeviceOption
{
public:
  /// Constructs a RuntimeDeviceOption, sets the Source to NOT_SET
  /// params:
  ///   index - index location of this command line argument in an option::Option array
  ///   envName - The environment variable name of this option
  VISKORES_CONT RuntimeDeviceOption(const viskores::Id& index, const std::string& envName);

  VISKORES_CONT virtual ~RuntimeDeviceOption() noexcept;

  /// Initializes this option's value from the environment and then the provided options
  /// array in that order. The options array is expected to be filled in using the
  /// viskores::cont::internal::option::OptionIndex with the usage vector defined in
  /// viskores::cont::Initialize.
  VISKORES_CONT void Initialize(const option::Option* options);

  /// Sets the Value to the environment variable of the constructed EnvName
  VISKORES_CONT void SetOptionFromEnvironment();

  /// Grabs and sets the option value using the constructed Index
  VISKORES_CONT void SetOptionFromOptionsArray(const option::Option* options);

  /// Directly set the value for this option
  VISKORES_CONT void SetOption(const viskores::Id& value);

  VISKORES_CONT viskores::Id GetValue() const;
  VISKORES_CONT RuntimeDeviceOptionSource GetSource() const;
  VISKORES_CONT bool IsSet() const;

private:
  const viskores::Id Index;
  const std::string EnvName;
  RuntimeDeviceOptionSource Source;
  viskores::Id Value;
};

} // namespace viskores::cont::internal
} // namespace viskores::cont
} // namespace viskores

#endif // viskores_cont_internal_RuntimeDeviceOption_h
