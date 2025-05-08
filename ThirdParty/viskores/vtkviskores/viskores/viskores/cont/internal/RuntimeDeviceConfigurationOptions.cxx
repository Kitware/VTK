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
#include <viskores/cont/internal/RuntimeDeviceConfigurationOptions.h>

#include <memory>
#include <sstream>

namespace viskores
{
namespace cont
{
namespace internal
{

namespace
{
void AppendOptionDescriptors(std::vector<option::Descriptor>& usage,
                             const bool& useOptionIndex = true)
{
  usage.push_back({ useOptionIndex ? static_cast<uint32_t>(option::OptionIndex::NUM_THREADS) : 0,
                    0,
                    "",
                    "viskores-num-threads",
                    option::ViskoresArg::Required,
                    "  --viskores-num-threads <dev> \tSets the number of threads to use for the "
                    "selected device" });
  usage.push_back({ useOptionIndex ? static_cast<uint32_t>(option::OptionIndex::NUMA_REGIONS) : 1,
                    0,
                    "",
                    "viskores-numa-regions",
                    option::ViskoresArg::Required,
                    "  --viskores-numa-regions <dev> \tSets the number of numa regions when using "
                    "kokkos/OpenMP (deprecated, has no effect)" });
  usage.push_back(
    { useOptionIndex ? static_cast<uint32_t>(option::OptionIndex::DEVICE_INSTANCE) : 2,
      0,
      "",
      "viskores-device-instance",
      option::ViskoresArg::Required,
      "  --viskores-device-instance <dev> \tSets the device instance to use when using "
      "kokkos/cuda" });
}
} // anonymous namespace

RuntimeDeviceConfigurationOptions::RuntimeDeviceConfigurationOptions(const bool& useOptionIndex)
  : ViskoresNumThreads(useOptionIndex ? option::OptionIndex::NUM_THREADS : 0,
                       "VISKORES_NUM_THREADS")
  , ViskoresDeviceInstance(useOptionIndex ? option::OptionIndex::DEVICE_INSTANCE : 2,
                           "VISKORES_DEVICE_INSTANCE")
  , Initialized(false)
{
}

RuntimeDeviceConfigurationOptions::RuntimeDeviceConfigurationOptions()
  : RuntimeDeviceConfigurationOptions(true)
{
}


RuntimeDeviceConfigurationOptions::RuntimeDeviceConfigurationOptions(
  std::vector<option::Descriptor>& usage)
  : RuntimeDeviceConfigurationOptions(true)
{
  AppendOptionDescriptors(usage);
}

RuntimeDeviceConfigurationOptions::RuntimeDeviceConfigurationOptions(int& argc, char* argv[])
  : RuntimeDeviceConfigurationOptions(false)
{
  std::vector<option::Descriptor> usage;
  AppendOptionDescriptors(usage, false);
  usage.push_back(
    { option::OptionIndex::UNKNOWN, 0, "", "", option::ViskoresArg::UnknownOption, "" });
  usage.push_back({ 0, 0, 0, 0, 0, 0 });

  option::Stats stats(usage.data(), argc, argv);
  std::unique_ptr<option::Option[]> options{ new option::Option[stats.options_max] };
  std::unique_ptr<option::Option[]> buffer{ new option::Option[stats.buffer_max] };
  option::Parser parse(usage.data(), argc, argv, options.get(), buffer.get());

  if (parse.error())
  {
    std::stringstream streamBuffer;
    option::printUsage(streamBuffer, usage.data());
    std::cerr << streamBuffer.str();
    exit(1);
  }

  this->Initialize(options.get());
}

RuntimeDeviceConfigurationOptions::~RuntimeDeviceConfigurationOptions() noexcept = default;

void RuntimeDeviceConfigurationOptions::Initialize(const option::Option* options)
{
  this->ViskoresNumThreads.Initialize(options);
  this->ViskoresDeviceInstance.Initialize(options);
  this->Initialized = true;
}

bool RuntimeDeviceConfigurationOptions::IsInitialized() const
{
  return this->Initialized;
}

} // namespace viskores::cont::internal
} // namespace viskores::cont
} // namespace viskores
