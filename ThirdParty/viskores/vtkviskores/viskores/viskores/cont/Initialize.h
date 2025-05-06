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
#ifndef viskores_cont_Initialize_h
#define viskores_cont_Initialize_h

#include <viskores/cont/DeviceAdapterTag.h>
#include <viskores/cont/viskores_cont_export.h>
#include <viskores/internal/ExportMacros.h>

#include <string>
#include <type_traits>
#include <vector>

namespace viskores
{
namespace cont
{

struct InitializeResult
{
  /// The device passed into `--viskores-device` argument. If no device was specified, then
  /// this value is set to `DeviceAdapterTagUndefined`. Note that if the user specifies
  /// "any" device, then this value can be set to `DeviceAdapterTagAny`, which is a
  /// pseudo-tag that allows any supported device.
  DeviceAdapterId Device = DeviceAdapterTagUndefined{};

  /// A usage statement for arguments parsed by Viskores. If the calling code wants to print
  /// a usage statement documenting the options that can be provided on the command line,
  /// then this string can be added to document the options supported by Viskores.
  std::string Usage;
};

enum class InitializeOptions
{
  /// Placeholder used when no options are enabled. This is the value used when the third argument
  /// to `viskores::cont::Initialize` is not provided.
  None = 0x00,

  /// Issue an error if the device argument is not specified.
  RequireDevice = 0x01,

  /// If no device is specified, treat it as if the user gave `--viskores-device=Any`. This means that
  /// `DeviceAdapterTagUndefined` will never be returned in the result.
  DefaultAnyDevice = 0x02,

  /// Add a help argument. If `-h` or `--viskores-help` is provided, prints a usage statement. Of course,
  /// the usage statement will only print out arguments processed by Viskores, which is why help is not
  /// given by default. Alternatively, a string with usage help is returned from `viskores::cont::Initialize`
  /// so that the calling program can provide Viskores's help in its own usage statement.
  AddHelp = 0x04,

  /// If an unknown option is encountered, the program terminates with an error and a usage
  /// statement is printed. If this option is not provided, any unknown options are returned
  /// in `argv`. If this option is used, it is a good idea to use AddHelp as well.
  ErrorOnBadOption = 0x08,

  /// If an extra argument is encountered, the program terminates with an error and a usage
  /// statement is printed. If this option is not provided, any unknown arguments are returned
  /// in `argv`.
  ErrorOnBadArgument = 0x10,

  /// If supplied, Initialize treats its own arguments as the only ones supported by the
  /// application and provides an error if not followed exactly. This is a convenience
  /// option that is a combination of `ErrorOnBadOption`, `ErrorOnBadArgument`, and `AddHelp`.
  Strict = ErrorOnBadOption | ErrorOnBadArgument | AddHelp
};

// Allow options to be used as a bitfield
inline InitializeOptions operator|(const InitializeOptions& lhs, const InitializeOptions& rhs)
{
  using T = std::underlying_type<InitializeOptions>::type;
  return static_cast<InitializeOptions>(static_cast<T>(lhs) | static_cast<T>(rhs));
}
inline InitializeOptions operator&(const InitializeOptions& lhs, const InitializeOptions& rhs)
{
  using T = std::underlying_type<InitializeOptions>::type;
  return static_cast<InitializeOptions>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

/**
 * Initialize the Viskores library, parsing arguments when provided:
 * - Sets log level names when logging is configured.
 * - Sets the calling thread as the main thread for logging purposes.
 * - Sets the default log level to the argument provided to `--viskores-log-level`.
 * - Forces usage of the device name passed to `--viskores-device`.
 * - Prints usage when `-h` or `--viskores-help` is passed.
 *
 * The parameterless version only sets up log level names.
 *
 * Additional options may be supplied via the @a opts argument, such as
 * requiring the `--viskores-device` option.
 *
 * Results are available in the returned InitializeResult.
 *
 * @note This method may call exit() on parse error.
 * @{
 */
VISKORES_CONT_EXPORT
VISKORES_CONT
InitializeResult Initialize(int& argc,
                            char* argv[],
                            InitializeOptions opts = InitializeOptions::None);
VISKORES_CONT_EXPORT
VISKORES_CONT
InitializeResult Initialize();
/**@}*/
}
} // end namespace viskores::cont


#endif // viskores_cont_Initialize_h
