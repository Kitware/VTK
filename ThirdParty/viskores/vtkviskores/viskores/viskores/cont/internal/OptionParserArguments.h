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
#ifndef viskores_cont_internal_OptionParserArguments_h
#define viskores_cont_internal_OptionParserArguments_h

#include <viskores/cont/Logging.h>
#include <viskores/cont/internal/OptionParser.h>

namespace viskores
{
namespace cont
{
namespace internal
{
namespace option
{

/// All options supported by the parser are stored here for usage in multiple modules
enum OptionIndex
{
  // special option for dealing with unknown arguments
  UNKNOWN,

  // general viskores arguments
  HELP,
  DEVICE,
  LOGLEVEL, // not parsed by this parser, but by loguru

  // All RuntimeDeviceConfiguration specific options
  NUM_THREADS,
  NUMA_REGIONS,
  DEVICE_INSTANCE
};

struct ViskoresArg : public option::Arg
{
  static option::ArgStatus Required(const option::Option& option, bool msg)
  {
    if (option.arg == nullptr)
    {
      if (msg)
      {
        VISKORES_LOG_ALWAYS_S(viskores::cont::LogLevel::Error,
                              "Missing argument after option '"
                                << std::string(option.name, static_cast<size_t>(option.namelen))
                                << "'.\n");
      }
      return option::ARG_ILLEGAL;
    }
    else
    {
      return option::ARG_OK;
    }
  }

  // Method used for guessing whether an option that do not support (perhaps that calling
  // program knows about it) has an option attached to it (which should also be ignored).
  static option::ArgStatus UnknownOption(const option::Option& option, bool msg)
  {
    // If we don't have an arg, obviously we don't have an arg.
    if (option.arg == nullptr)
    {
      return option::ARG_NONE;
    }

    // The option::Arg::Optional method will return that the ARG is OK if and only if
    // the argument is attached to the option (e.g. --foo=bar). If that is the case,
    // then we definitely want to report that the argument is OK.
    if (option::Arg::Optional(option, msg) == option::ARG_OK)
    {
      return option::ARG_OK;
    }

    // Now things get tricky. Maybe the next argument is an option or maybe it is an
    // argument for this option. We will guess that if the next argument does not
    // look like an option, we will treat it as such.
    if (option.arg[0] == '-')
    {
      return option::ARG_NONE;
    }
    else
    {
      return option::ARG_OK;
    }
  }
};

} // namespace viskores::cont::internal::option
} // namespace viskores::cont::internal
} // namespace viskores::cont
} // namespace viskores

#endif // viskores_cont_internal_OptionParserArguments_h
