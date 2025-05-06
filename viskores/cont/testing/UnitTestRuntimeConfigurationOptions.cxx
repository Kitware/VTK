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
#include <viskores/cont/internal/OptionParser.h>
#include <viskores/cont/internal/RuntimeDeviceConfigurationOptions.h>
#include <viskores/cont/internal/RuntimeDeviceOption.h>

#include <viskores/cont/testing/Testing.h>

#include <stdlib.h>
#include <utility>
#include <vector>

namespace internal = viskores::cont::internal;
namespace opt = internal::option;

namespace
{

enum
{
  UNKNOWN,
  TEST
};

std::unique_ptr<opt::Option[]> GetOptions(int& argc,
                                          char** argv,
                                          std::vector<opt::Descriptor>& usage)
{
  if (argc == 0 || argv == nullptr)
  {
    return nullptr;
  }

  opt::Stats stats(usage.data(), argc, argv);
  std::unique_ptr<opt::Option[]> options{ new opt::Option[stats.options_max] };
  std::unique_ptr<opt::Option[]> buffer{ new opt::Option[stats.buffer_max] };
  opt::Parser parse(usage.data(), argc, argv, options.get(), buffer.get());

  return options;
}

void TestRuntimeDeviceOptionHappy()
{
  std::vector<opt::Descriptor> usage;
  usage.push_back(
    { TEST, 0, "", "test-option", opt::ViskoresArg::Required, " --test-option <val>" });
  usage.push_back({ UNKNOWN, 0, "", "", opt::ViskoresArg::UnknownOption, "" });
  usage.push_back({ 0, 0, 0, 0, 0, 0 });

  const std::string env{ "TEST_OPTION" };
  viskores::cont::testing::Testing::UnsetEnv(env);

  // Basic no value initialize
  {
    internal::RuntimeDeviceOption testOption(TEST, env);
    testOption.Initialize(nullptr);
    VISKORES_TEST_ASSERT(!testOption.IsSet(), "test option should not be set");
  }

  viskores::cont::testing::Testing::SetEnv(env, "1");

  // Initialize from environment
  {
    internal::RuntimeDeviceOption testOption(TEST, env);
    testOption.Initialize(nullptr);
    VISKORES_TEST_ASSERT(testOption.IsSet(), "Option set through env");
    VISKORES_TEST_ASSERT(testOption.GetSource() == internal::RuntimeDeviceOptionSource::ENVIRONMENT,
                         "Option should be set");
    VISKORES_TEST_ASSERT(testOption.GetValue() == 1, "Option value should be 1");
  }

  int argc;
  char** argv;
  viskores::cont::testing::Testing::MakeArgs(argc, argv, "--test-option", "2");
  auto options = GetOptions(argc, argv, usage);
  VISKORES_TEST_ASSERT(options[TEST], "should be and option");

  // Initialize from argument with priority over environment
  {
    internal::RuntimeDeviceOption testOption(TEST, env);

    testOption.Initialize(options.get());
    VISKORES_TEST_ASSERT(testOption.IsSet(), "Option should be set");
    VISKORES_TEST_ASSERT(testOption.GetSource() ==
                           internal::RuntimeDeviceOptionSource::COMMAND_LINE,
                         "Option should be set");
    VISKORES_TEST_ASSERT(testOption.GetValue() == 2, "Option value should be 1");
  }

  // Initialize then set manually
  {
    internal::RuntimeDeviceOption testOption(TEST, env);

    testOption.Initialize(options.get());
    testOption.SetOption(3);
    VISKORES_TEST_ASSERT(testOption.IsSet(), "Option should be set");
    VISKORES_TEST_ASSERT(testOption.GetSource() == internal::RuntimeDeviceOptionSource::IN_CODE,
                         "Option should be set");
    VISKORES_TEST_ASSERT(testOption.GetValue() == 3, "Option value should be 3");
  }

  viskores::cont::testing::Testing::UnsetEnv(env);
}

void TestRuntimeDeviceOptionError()
{
  std::vector<opt::Descriptor> usage;
  usage.push_back(
    { TEST, 0, "", "test-option", opt::ViskoresArg::Required, " --test-option <val>" });
  usage.push_back({ UNKNOWN, 0, "", "", opt::ViskoresArg::UnknownOption, "" });
  usage.push_back({ 0, 0, 0, 0, 0, 0 });

  const std::string env{ "TEST_OPTION" };
  viskores::cont::testing::Testing::UnsetEnv(env);

  bool threw = true;

  // Parse a non integer
  {
    internal::RuntimeDeviceOption testOption(TEST, env);
    viskores::cont::testing::Testing::SetEnv(env, "bad");
    try
    {
      testOption.Initialize(nullptr);
      threw = false;
    }
    catch (const viskores::cont::ErrorBadValue& error)
    {
      VISKORES_TEST_ASSERT(
        error.GetMessage() ==
          "Value 'bad' failed to parse as integer from source: 'ENVIRONMENT: " + env + "'",
        "message: " + error.GetMessage());
    }

    VISKORES_TEST_ASSERT(threw, "Should have thrown");
  }

  // Parse an integer that's too large
  {
    internal::RuntimeDeviceOption testOption(TEST, env);
    viskores::cont::testing::Testing::SetEnv(env, "9938489298493882949384989");
    try
    {
      testOption.Initialize(nullptr);
      threw = false;
    }
    catch (const viskores::cont::ErrorBadValue& error)
    {
      VISKORES_TEST_ASSERT(
        error.GetMessage() ==
          "Value '9938489298493882949384989' out of range for source: 'ENVIRONMENT: " + env + "'",
        "message: " + error.GetMessage());
    }

    VISKORES_TEST_ASSERT(threw, "Should have thrown");
  }

  // Parse an integer with some stuff on the end
  {
    internal::RuntimeDeviceOption testOption(TEST, env);
    viskores::cont::testing::Testing::SetEnv(env, "100bad");
    try
    {
      testOption.Initialize(nullptr);
      threw = false;
    }
    catch (const viskores::cont::ErrorBadValue& error)
    {
      VISKORES_TEST_ASSERT(error.GetMessage() ==
                             "Value '100bad' from source: 'ENVIRONMENT: " + env +
                               "' has dangling characters, throwing",
                           "message: " + error.GetMessage());
    }

    VISKORES_TEST_ASSERT(threw, "Should have thrown");
  }

  viskores::cont::testing::Testing::UnsetEnv(env);
}

void TestConfigOptionValues(const internal::RuntimeDeviceConfigurationOptions& configOptions)
{
  VISKORES_TEST_ASSERT(configOptions.IsInitialized(),
                       "runtime config options should be initialized");

  VISKORES_TEST_ASSERT(configOptions.ViskoresNumThreads.IsSet(), "num threads should be set");
  VISKORES_TEST_ASSERT(configOptions.ViskoresDeviceInstance.IsSet(),
                       "device instance should be set");

  VISKORES_TEST_ASSERT(configOptions.ViskoresNumThreads.GetValue() == 100,
                       "num threads should == 100");
  VISKORES_TEST_ASSERT(configOptions.ViskoresDeviceInstance.GetValue() == 1,
                       "device instance should == 1");
}

void TestRuntimeDeviceConfigurationOptions()
{
  {
    std::vector<opt::Descriptor> usage;
    usage.push_back({ 0, 0, "", "need", opt::ViskoresArg::Required, "" });
    usage.push_back({ 1, 0, "", "filler", opt::ViskoresArg::Required, "" });
    usage.push_back({ 2, 0, "", "args", opt::ViskoresArg::Required, "" });
    usage.push_back({ 3, 0, "", "to", opt::ViskoresArg::Required, "" });
    usage.push_back({ 4, 0, "", "pass", opt::ViskoresArg::Required, "" });
    internal::RuntimeDeviceConfigurationOptions configOptions(usage);

    usage.push_back({ opt::OptionIndex::UNKNOWN, 0, "", "", opt::ViskoresArg::UnknownOption, "" });
    usage.push_back({ 0, 0, 0, 0, 0, 0 });

    int argc;
    char** argv;
    viskores::cont::testing::Testing::MakeArgs(
      argc, argv, "--viskores-num-threads", "100", "--viskores-device-instance", "1");
    auto options = GetOptions(argc, argv, usage);

    VISKORES_TEST_ASSERT(!configOptions.IsInitialized(),
                         "runtime config options should not be initialized");
    configOptions.Initialize(options.get());
    TestConfigOptionValues(configOptions);
  }

  {
    int argc;
    char** argv;
    viskores::cont::testing::Testing::MakeArgs(
      argc, argv, "--viskores-num-threads", "100", "--viskores-device-instance", "1");
    internal::RuntimeDeviceConfigurationOptions configOptions(argc, argv);
    TestConfigOptionValues(configOptions);
  }
}

void TestRuntimeConfigurationOptions()
{
  TestRuntimeDeviceOptionHappy();
  TestRuntimeDeviceOptionError();
  TestRuntimeDeviceConfigurationOptions();
}

} // namespace

int UnitTestRuntimeConfigurationOptions(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestRuntimeConfigurationOptions, argc, argv);
}
