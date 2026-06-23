//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/cont/kokkos/internal/RuntimeDeviceConfigurationKokkos.h>

namespace
{

VISKORES_CONT
viskores::cont::internal::RuntimeDeviceConfigReturnCode GetArgFromList(
  const std::vector<std::string>& argList,
  const std::string& argName,
  viskores::Id& value)
{
  size_t pos;
  try
  {
    for (auto argItr = argList.rbegin(); argItr != argList.rend(); argItr++)
    {
      if (argItr->rfind(argName, 0) == 0)
      {
        if (argItr->size() == argName.size())
        {
          value = std::stoi(*(--argItr), &pos, 10);
          return viskores::cont::internal::RuntimeDeviceConfigReturnCode::SUCCESS;
        }
        else
        {
          value = std::stoi(argItr->substr(argName.size() + 1), &pos, 10);
          return viskores::cont::internal::RuntimeDeviceConfigReturnCode::SUCCESS;
        }
      }
    }
  }
  catch (const std::invalid_argument&)
  {
    VISKORES_LOG_S(
      viskores::cont::LogLevel::Error,
      "Unable to get arg " + argName +
        "from kokkos argList, invalid argument thrown... This shouldn't have happened");
    return viskores::cont::internal::RuntimeDeviceConfigReturnCode::INVALID_VALUE;
  }
  catch (const std::out_of_range&)
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Error,
                   "Unable to get arg " + argName +
                     "from kokkos argList, out of range thrown... This shouldn't have happened");
    return viskores::cont::internal::RuntimeDeviceConfigReturnCode::INVALID_VALUE;
  }
  return viskores::cont::internal::RuntimeDeviceConfigReturnCode::NOT_APPLIED;
}

} // namespace anonymous

namespace viskores
{
namespace cont
{
namespace internal
{

VISKORES_CONT viskores::cont::DeviceAdapterId
RuntimeDeviceConfiguration<viskores::cont::DeviceAdapterTagKokkos>::GetDevice() const
{
  return viskores::cont::DeviceAdapterTagKokkos{};
}

VISKORES_CONT RuntimeDeviceConfigReturnCode
RuntimeDeviceConfiguration<viskores::cont::DeviceAdapterTagKokkos>::SetThreads(
  const viskores::Id& value)
{
  if (Kokkos::is_initialized())
  {
    VISKORES_LOG_S(
      viskores::cont::LogLevel::Warn,
      "SetThreads was called but Kokkos was already initailized! Updates will not be applied.");
    return RuntimeDeviceConfigReturnCode::NOT_APPLIED;
  }
  this->KokkosArguments.insert(this->KokkosArguments.begin(),
                               "--kokkos-num-threads=" + std::to_string(value));
  return RuntimeDeviceConfigReturnCode::SUCCESS;
}

VISKORES_CONT RuntimeDeviceConfigReturnCode
RuntimeDeviceConfiguration<viskores::cont::DeviceAdapterTagKokkos>::SetDeviceInstance(
  const viskores::Id& value)
{
  if (Kokkos::is_initialized())
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                   "SetDeviceInstance was called but Kokkos was already initailized! Updates will "
                   "not be applied.");
    return RuntimeDeviceConfigReturnCode::NOT_APPLIED;
  }
  this->KokkosArguments.insert(this->KokkosArguments.begin(),
                               "--kokkos-device-id=" + std::to_string(value));
  return RuntimeDeviceConfigReturnCode::SUCCESS;
}

VISKORES_CONT RuntimeDeviceConfigReturnCode
RuntimeDeviceConfiguration<viskores::cont::DeviceAdapterTagKokkos>::GetThreads(
  viskores::Id& value) const
{
  return GetArgFromList(this->KokkosArguments, "--kokkos-num-threads", value);
}

VISKORES_CONT RuntimeDeviceConfigReturnCode
RuntimeDeviceConfiguration<viskores::cont::DeviceAdapterTagKokkos>::GetDeviceInstance(
  viskores::Id& value) const
{
  return GetArgFromList(this->KokkosArguments, "--kokkos-device-id", value);
}

VISKORES_CONT void RuntimeDeviceConfiguration<
  viskores::cont::DeviceAdapterTagKokkos>::ParseExtraArguments(int& argc, char* argv[])
{
  if (argc > 0 && argv)
  {
    this->KokkosArguments.insert(this->KokkosArguments.end(), argv, argv + argc);
  }
}

VISKORES_CONT void
RuntimeDeviceConfiguration<viskores::cont::DeviceAdapterTagKokkos>::InitializeSubsystem()
{
  if (!Kokkos::is_initialized())
  {
    std::vector<char*> argv;
    for (auto& arg : this->KokkosArguments)
    {
      argv.push_back(&arg[0]);
    }
    int size = argv.size();
    Kokkos::initialize(size, argv.data());
    std::atexit(
      []()
      {
        // Avoid finalizing Kokkos if something else has finalized.
        if (Kokkos::is_initialized())
        {
          VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Finalizing Kokkos.");
          Kokkos::finalize();
        }
        else
        {
          VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                         "Detected external finalization of Kokkos.");
        }
      });
  }
  else
  {
    VISKORES_LOG_S(
      viskores::cont::LogLevel::Warn,
      "Attempted to Re-initialize Kokkos! The Kokkos subsystem can only be initialized once");
  }
}

} // namespace viskores::cont::internal
} // namespace viskores::cont
} // namespace viskores
