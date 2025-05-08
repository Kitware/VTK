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
#include <viskores/cont/internal/RuntimeDeviceOption.h>

#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/Logging.h>

#include <string>

namespace viskores
{
namespace cont
{
namespace internal
{

namespace
{

VISKORES_CONT viskores::Id ParseOption(const std::string& input, const std::string& source)
{
  try
  {
    size_t pos;
    auto value = std::stoi(input, &pos, 10);
    if (pos != input.size())
    {
      throw viskores::cont::ErrorBadValue("Value '" + input + "' from source: '" + source +
                                          "' has dangling characters, throwing");
    }
    return value;
  }
  catch (const std::invalid_argument&)
  {
    throw viskores::cont::ErrorBadValue(
      "Value '" + input + "' failed to parse as integer from source: '" + source + "'");
  }
  catch (const std::out_of_range&)
  {
    throw viskores::cont::ErrorBadValue("Value '" + input + "' out of range for source: '" +
                                        source + "'");
  }
}

} // namespace

RuntimeDeviceOption::RuntimeDeviceOption(const viskores::Id& index, const std::string& envName)
  : Index(index)
  , EnvName(envName)
  , Source(RuntimeDeviceOptionSource::NOT_SET)
{
}

RuntimeDeviceOption::~RuntimeDeviceOption() noexcept = default;

void RuntimeDeviceOption::Initialize(const option::Option* options)
{
  this->SetOptionFromEnvironment();
  this->SetOptionFromOptionsArray(options);
}

void RuntimeDeviceOption::SetOptionFromEnvironment()
{
  if (std::getenv(EnvName.c_str()) != nullptr)
  {
    this->Value = ParseOption(std::getenv(EnvName.c_str()), "ENVIRONMENT: " + EnvName);
    this->Source = RuntimeDeviceOptionSource::ENVIRONMENT;
  }
}

void RuntimeDeviceOption::SetOptionFromOptionsArray(const option::Option* options)
{
  if (options != nullptr && options[this->Index])
  {
    this->Value = ParseOption(options[this->Index].arg,
                              "COMMAND_LINE: " + std::string{ options[this->Index].name });
    this->Source = RuntimeDeviceOptionSource::COMMAND_LINE;
  }
}

void RuntimeDeviceOption::SetOption(const viskores::Id& value)
{
  this->Value = value;
  this->Source = RuntimeDeviceOptionSource::IN_CODE;
}

viskores::Id RuntimeDeviceOption::GetValue() const
{
  if (!this->IsSet())
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                   "GetValue() called on Argument '" << this->EnvName << "' when it was not set.");
  }
  return this->Value;
}

RuntimeDeviceOptionSource RuntimeDeviceOption::GetSource() const
{
  return this->Source;
}

bool RuntimeDeviceOption::IsSet() const
{
  return this->Source != RuntimeDeviceOptionSource::NOT_SET;
}

} // namespace viskores::cont::internal
} // namespace viskores::cont
} // namespace viskores
