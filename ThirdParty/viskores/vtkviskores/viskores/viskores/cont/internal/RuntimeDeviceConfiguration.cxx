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
#include <viskores/cont/Logging.h>
#include <viskores/cont/internal/RuntimeDeviceConfiguration.h>

namespace viskores
{
namespace cont
{
namespace internal
{

namespace
{
VISKORES_CONT
std::string RuntimeDeviceConfigReturnCodeToString(const RuntimeDeviceConfigReturnCode& code)
{
  switch (code)
  {
    case RuntimeDeviceConfigReturnCode::SUCCESS:
      return "SUCCESS";
    case RuntimeDeviceConfigReturnCode::OUT_OF_BOUNDS:
      return "OUT_OF_BOUNDS";
    case RuntimeDeviceConfigReturnCode::INVALID_FOR_DEVICE:
      return "INVALID_FOR_DEVICE";
    case RuntimeDeviceConfigReturnCode::INVALID_VALUE:
      return "INVALID_VALUE";
    case RuntimeDeviceConfigReturnCode::NOT_APPLIED:
      return "NOT_APPLIED";
    default:
      return "";
  }
}

VISKORES_CONT
void LogReturnCode(const RuntimeDeviceConfigReturnCode& code,
                   const std::string& function,
                   const viskores::Id& value,
                   const std::string& deviceName)
{
  // Note that we intentionally are not logging a warning for INVALID_FOR_DEVICE. When a
  // user provides a command line argument, it gets sent to all possible devices during
  // `Initialize` regardless of whether it is used. The user does not need a lot of
  // useless warnings about (for example) the serial device not supporting parameters
  // intended for a real parallel device.
  if ((code != RuntimeDeviceConfigReturnCode::INVALID_FOR_DEVICE) &&
      (code != RuntimeDeviceConfigReturnCode::SUCCESS))
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                   function << " for device: " << deviceName
                            << " had code: " << RuntimeDeviceConfigReturnCodeToString(code)
                            << " with value: " << value);
  }
#ifndef VISKORES_ENABLE_LOGGING
  (void)function;
  (void)value;
  (void)deviceName;
#endif
}

template <typename SetFunc>
VISKORES_CONT void InitializeOption(RuntimeDeviceOption option,
                                    SetFunc setFunc,
                                    const std::string& funcName,
                                    const std::string& deviceName)
{
  if (option.IsSet())
  {
    auto value = option.GetValue();
    auto code = setFunc(value);
    LogReturnCode(code, funcName, value, deviceName);
  }
}

} // namespace anonymous

RuntimeDeviceConfigurationBase::~RuntimeDeviceConfigurationBase() noexcept = default;

void RuntimeDeviceConfigurationBase::Initialize(
  const RuntimeDeviceConfigurationOptions& configOptions)
{
  InitializeOption(
    configOptions.ViskoresNumThreads,
    [&](const viskores::Id& value) { return this->SetThreads(value); },
    "SetThreads",
    this->GetDevice().GetName());
  InitializeOption(
    configOptions.ViskoresDeviceInstance,
    [&](const viskores::Id& value) { return this->SetDeviceInstance(value); },
    "SetDeviceInstance",
    this->GetDevice().GetName());
  this->InitializeSubsystem();
}

void RuntimeDeviceConfigurationBase::Initialize(
  const RuntimeDeviceConfigurationOptions& configOptions,
  int& argc,
  char* argv[])
{
  this->ParseExtraArguments(argc, argv);
  this->Initialize(configOptions);
}

RuntimeDeviceConfigReturnCode RuntimeDeviceConfigurationBase::SetThreads(const viskores::Id&)
{
  return RuntimeDeviceConfigReturnCode::INVALID_FOR_DEVICE;
}

RuntimeDeviceConfigReturnCode RuntimeDeviceConfigurationBase::SetDeviceInstance(const viskores::Id&)
{
  return RuntimeDeviceConfigReturnCode::INVALID_FOR_DEVICE;
}

RuntimeDeviceConfigReturnCode RuntimeDeviceConfigurationBase::GetThreads(viskores::Id&) const
{
  return RuntimeDeviceConfigReturnCode::INVALID_FOR_DEVICE;
}

RuntimeDeviceConfigReturnCode RuntimeDeviceConfigurationBase::GetDeviceInstance(viskores::Id&) const
{
  return RuntimeDeviceConfigReturnCode::INVALID_FOR_DEVICE;
}

RuntimeDeviceConfigReturnCode RuntimeDeviceConfigurationBase::GetMaxThreads(viskores::Id&) const
{
  return RuntimeDeviceConfigReturnCode::INVALID_FOR_DEVICE;
}

RuntimeDeviceConfigReturnCode RuntimeDeviceConfigurationBase::GetMaxDevices(viskores::Id&) const
{
  return RuntimeDeviceConfigReturnCode::INVALID_FOR_DEVICE;
}

void RuntimeDeviceConfigurationBase::ParseExtraArguments(int&, char*[]) {}
void RuntimeDeviceConfigurationBase::InitializeSubsystem() {}


} // namespace viskores::cont::internal
} // namespace viskores::cont
} // namespace viskores
