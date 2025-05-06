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

#include <viskores/cont/Initialize.h>

#include <viskores/cont/Logging.h>
#include <viskores/cont/RuntimeDeviceTracker.h>
#include <viskores/cont/internal/OptionParser.h>
#include <viskores/cont/internal/OptionParserArguments.h>

#include <viskores/thirdparty/diy/environment.h>

#include <cstdlib>
#include <memory>
#include <sstream>

namespace opt = viskores::cont::internal::option;

namespace
{

struct ViskoresDeviceArg : public opt::Arg
{
  static opt::ArgStatus IsDevice(const opt::Option& option, bool msg)
  {
    // Device must be specified if option is present:
    if (option.arg == nullptr)
    {
      if (msg)
      {
        VISKORES_LOG_ALWAYS_S(viskores::cont::LogLevel::Error,
                              "Missing device after option '"
                                << std::string(option.name, static_cast<size_t>(option.namelen))
                                << "'.\nValid devices are: "
                                << ViskoresDeviceArg::GetValidDeviceNames() << "\n");
      }
      return opt::ARG_ILLEGAL;
    }

    auto id = viskores::cont::make_DeviceAdapterId(option.arg);

    if (!ViskoresDeviceArg::DeviceIsAvailable(id))
    {
      VISKORES_LOG_ALWAYS_S(viskores::cont::LogLevel::Error,
                            "Unavailable device specificed after option '"
                              << std::string(option.name, static_cast<size_t>(option.namelen))
                              << "': '" << option.arg << "'.\nValid devices are: "
                              << ViskoresDeviceArg::GetValidDeviceNames() << "\n");
      return opt::ARG_ILLEGAL;
    }

    return opt::ARG_OK;
  }

  static bool DeviceIsAvailable(viskores::cont::DeviceAdapterId id)
  {
    if (id == viskores::cont::DeviceAdapterTagAny{})
    {
      return true;
    }

    if (id.GetValue() <= 0 || id.GetValue() >= VISKORES_MAX_DEVICE_ADAPTER_ID ||
        id == viskores::cont::DeviceAdapterTagUndefined{})
    {
      return false;
    }

    auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
    bool result = false;
    try
    {
      result = tracker.CanRunOn(id);
    }
    catch (...)
    {
      result = false;
    }
    return result;
  }
  static std::string GetValidDeviceNames()
  {
    std::ostringstream names;
    names << "\"Any\" ";

    for (viskores::Int8 i = 0; i < VISKORES_MAX_DEVICE_ADAPTER_ID; ++i)
    {
      auto id = viskores::cont::make_DeviceAdapterId(i);
      if (ViskoresDeviceArg::DeviceIsAvailable(id))
      {
        names << "\"" << id.GetName() << "\" ";
      }
    }
    return names.str();
  }
};

} // namespace

namespace viskores
{
namespace cont
{

VISKORES_CONT
InitializeResult Initialize(int& argc, char* argv[], InitializeOptions opts)
{
  InitializeResult config;
  const std::string loggingFlagName = "viskores-log-level";
  const std::string loggingFlag = "--" + loggingFlagName;
  const std::string loggingHelp = "  " + loggingFlag +
    " <#|INFO|WARNING|ERROR|FATAL|OFF> \tSpecify a log level (when logging is enabled).";

  // initialize logging and diy first -- they'll pop off the options they consume:
  if (argc == 0 || argv == nullptr)
  {
    viskores::cont::InitLogging();
  }
  else
  {
    viskores::cont::InitLogging(argc, argv, loggingFlag, "VISKORES_LOG_LEVEL");
  }
  if (!viskoresdiy::mpi::environment::initialized())
  {
    if (argc == 0 || argv == nullptr)
    {
      // If initialized, will be deleted on program exit (calling MPI_Finalize if necessary)
      static viskoresdiy::mpi::environment diyEnvironment;
    }
    else
    {
      // If initialized, will be deleted on program exit (calling MPI_Finalize if necessary)
      static viskoresdiy::mpi::environment diyEnvironment(argc, argv);
    }
  }

  { // Parse Viskores options
    std::vector<opt::Descriptor> usage;
    if ((opts & InitializeOptions::AddHelp) != InitializeOptions::None)
    {
      // Because we have the AddHelp option, we will add both --help and --viskores-help to
      // the list of arguments. Use the first entry for introduction on the usage.
      usage.push_back(
        { opt::OptionIndex::HELP, 0, "", "viskores-help", opt::Arg::None, "Usage information:\n" });
      usage.push_back({ opt::OptionIndex::HELP,
                        0,
                        "h",
                        "help",
                        opt::Arg::None,
                        "  --help, --viskores-help, -h \tPrint usage information." });
    }
    else
    {
      usage.push_back({ opt::OptionIndex::HELP,
                        0,
                        "",
                        "viskores-help",
                        opt::Arg::None,
                        "  --viskores-help \tPrint usage information." });
    }
    usage.push_back({ opt::OptionIndex::DEVICE,
                      0,
                      "",
                      "viskores-device",
                      ViskoresDeviceArg::IsDevice,
                      "  --viskores-device <dev> \tForce device to dev. Omit device to list "
                      "available devices." });
    usage.push_back({ opt::OptionIndex::LOGLEVEL,
                      0,
                      "",
                      loggingFlagName.c_str(),
                      opt::ViskoresArg::Required,
                      loggingHelp.c_str() });

    // Bring in extra args used by the runtime device configuration options
    viskores::cont::internal::RuntimeDeviceConfigurationOptions runtimeDeviceOptions(usage);

    // Required to collect unknown arguments.
    usage.push_back({ opt::OptionIndex::UNKNOWN, 0, "", "", opt::ViskoresArg::UnknownOption, "" });
    usage.push_back({ 0, 0, 0, 0, 0, 0 });

    {
      std::stringstream streamBuffer;
      opt::printUsage(streamBuffer, usage.data());
      config.Usage = streamBuffer.str();
      // Remove trailing newline as one more than we want is added.
      config.Usage = config.Usage.substr(0, config.Usage.length() - 1);
    }

    // Remove argv[0] (executable name) if present:
    int viskoresArgc = argc > 0 ? argc - 1 : 0;
    char** viskoresArgv = viskoresArgc > 0 ? argv + 1 : argv;

    opt::Stats stats(usage.data(), viskoresArgc, viskoresArgv);
    std::unique_ptr<opt::Option[]> options{ new opt::Option[stats.options_max] };
    std::unique_ptr<opt::Option[]> buffer{ new opt::Option[stats.buffer_max] };
    opt::Parser parse(usage.data(), viskoresArgc, viskoresArgv, options.get(), buffer.get());

    if (parse.error())
    {
      std::cerr << config.Usage;
      exit(1);
    }

    if (options[opt::OptionIndex::HELP])
    {
      std::cerr << config.Usage;
      exit(0);
    }

    // The RuntimeDeviceConfiguration must be completed before calling GetRuntimeDeviceTracker()
    // for all the devices. This is because GetRuntimeDeviceTracker will construct a given
    // device's DeviceAdapterRuntimeDetector to determine if it exists and this constructor may
    // call `GetRuntimeConfiguration` for the specific device in order to query things such as
    // available threads/devices.
    {
      runtimeDeviceOptions.Initialize(options.get());
      viskores::cont::RuntimeDeviceInformation runtimeDevice;
      runtimeDevice.GetRuntimeConfiguration(
        viskores::cont::DeviceAdapterTagAny{}, runtimeDeviceOptions, argc, argv);
    }

    // Check for device on command line.
    if (options[opt::OptionIndex::DEVICE])
    {
      const char* arg = options[opt::OptionIndex::DEVICE].arg;
      config.Device = viskores::cont::make_DeviceAdapterId(arg);
    }
    // If not on command line, check for device in environment variable.
    if (config.Device == viskores::cont::DeviceAdapterTagUndefined{})
    {
      const char* deviceEnv = std::getenv("VISKORES_DEVICE");
      if (deviceEnv != nullptr)
      {
        auto id = viskores::cont::make_DeviceAdapterId(std::getenv("VISKORES_DEVICE"));
        if (ViskoresDeviceArg::DeviceIsAvailable(id))
        {
          config.Device = id;
        }
        else
        {
          // Got invalid device. Log an error, but continue to do the default action for
          // the device (i.e., ignore the environment variable setting).
          VISKORES_LOG_S(viskores::cont::LogLevel::Error,
                         "Invalid device `"
                           << deviceEnv
                           << "` specified in VISKORES_DEVICE environment variable. Ignoring.");
          VISKORES_LOG_S(viskores::cont::LogLevel::Error,
                         "Valid devices are: " << ViskoresDeviceArg::GetValidDeviceNames());
        }
      }
    }
    // If still not defined, check to see if "any" device should be added.
    if ((config.Device == viskores::cont::DeviceAdapterTagUndefined{}) &&
        (opts & InitializeOptions::DefaultAnyDevice) != InitializeOptions::None)
    {
      config.Device = viskores::cont::DeviceAdapterTagAny{};
    }
    // Set the state for the device selected.
    if (config.Device == viskores::cont::DeviceAdapterTagUndefined{})
    {
      if ((opts & InitializeOptions::RequireDevice) != InitializeOptions::None)
      {
        auto devices = ViskoresDeviceArg::GetValidDeviceNames();
        VISKORES_LOG_S(viskores::cont::LogLevel::Fatal, "Device not given on command line.");
        std::cerr << "Target device must be specified via --viskores-device.\n"
                     "Valid devices: "
                  << devices << std::endl;
        if ((opts & InitializeOptions::AddHelp) != InitializeOptions::None)
        {
          std::cerr << config.Usage;
        }
        exit(1);
      }
      else
      {
        // No device specified. Do nothing and let Viskores decide what it is going to do.
      }
    }
    else if (config.Device == viskores::cont::DeviceAdapterTagAny{})
    {
      viskores::cont::GetRuntimeDeviceTracker().Reset();
    }
    else
    {
      viskores::cont::GetRuntimeDeviceTracker().ForceDevice(config.Device);
    }


    for (const opt::Option* opt = options[opt::OptionIndex::UNKNOWN]; opt != nullptr;
         opt = opt->next())
    {
      if ((opts & InitializeOptions::ErrorOnBadOption) != InitializeOptions::None)
      {
        std::cerr << "Unknown option: " << opt->name << std::endl;
        if ((opts & InitializeOptions::AddHelp) != InitializeOptions::None)
        {
          std::cerr << config.Usage;
        }
        exit(1);
      }
    }

    for (int nonOpt = 0; nonOpt < parse.nonOptionsCount(); ++nonOpt)
    {
      VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                     "Unknown argument to Initialize: " << parse.nonOption(nonOpt) << "\n");
      if ((opts & InitializeOptions::ErrorOnBadArgument) != InitializeOptions::None)
      {
        std::cerr << "Unknown argument: " << parse.nonOption(nonOpt) << std::endl;
        if ((opts & InitializeOptions::AddHelp) != InitializeOptions::None)
        {
          std::cerr << config.Usage;
        }
        exit(1);
      }
    }

    // Now go back through the arg list and remove anything that is not in the list of
    // unknown options or non-option arguments.
    int destArg = 1;
    for (int srcArg = 1; srcArg < argc; ++srcArg)
    {
      std::string thisArg{ argv[srcArg] };
      bool copyArg = false;

      // Special case: "--" gets removed by optionparser but should be passed.
      if (thisArg == "--")
      {
        copyArg = true;
      }
      for (const opt::Option* opt = options[opt::OptionIndex::UNKNOWN]; !copyArg && opt != nullptr;
           opt = opt->next())
      {
        if (thisArg == opt->name)
        {
          copyArg = true;
        }
        if ((opt->arg != nullptr) && (thisArg == opt->arg))
        {
          copyArg = true;
        }
        // Special case: optionparser sometimes removes a single "-" from an option
        if (thisArg.substr(1) == opt->name)
        {
          copyArg = true;
        }
      }
      for (int nonOpt = 0; !copyArg && nonOpt < parse.nonOptionsCount(); ++nonOpt)
      {
        if (thisArg == parse.nonOption(nonOpt))
        {
          copyArg = true;
        }
      }
      if (copyArg)
      {
        if (destArg != srcArg)
        {
          argv[destArg] = argv[srcArg];
        }
        ++destArg;
      }
    }
    argc = destArg;
  }

  return config;
}

VISKORES_CONT
InitializeResult Initialize()
{
  int argc = 0;
  char** argv = nullptr;
  return Initialize(argc, argv);
}
}
} // end namespace viskores::cont
