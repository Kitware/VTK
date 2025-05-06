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

#include <viskores/cont/testing/Testing.h>

namespace
{

template <typename... T>
void CheckArgs(int argc, char* argv[], T&&... args)
{
  constexpr std::size_t numArgs = sizeof...(args) + 1;

  std::array<std::string, numArgs> expectedArgs = { { "program-name", args... } };

  std::cout << "  expected args:";
  for (std::size_t i = 0; i < numArgs; ++i)
  {
    std::cout << " " << expectedArgs[i];
  }
  std::cout << std::endl;

  std::cout << "  received args:";
  for (int i = 0; i < argc; ++i)
  {
    std::cout << " " << argv[i];
  }
  std::cout << std::endl;

  VISKORES_TEST_ASSERT(
    numArgs == static_cast<std::size_t>(argc), "Got wrong number of arguments (", argc, ")");

  for (std::size_t i = 0; i < numArgs; ++i)
  {
    VISKORES_TEST_ASSERT(expectedArgs[i] == argv[i], "Arg ", i, " wrong");
  }

  std::cout << std::endl;
}

void InitializeZeroArguments()
{
  std::cout << "Initialize with no arguments" << std::endl;
  viskores::cont::Initialize();
}

void InitializeNoOptions()
{
  std::cout << "Initialize without any options" << std::endl;

  int argc;
  char** argv;
  viskores::cont::testing::Testing::MakeArgsAddProgramName(argc, argv);
  viskores::cont::InitializeResult result = viskores::cont::Initialize(argc, argv);
  CheckArgs(argc, argv);

  std::cout << "Usage statement returned from Initialize:" << std::endl;
  std::cout << result.Usage << std::endl;
}

void InitializeStandardOptions()
{
  std::cout << "Initialize with some standard options" << std::endl;

  int argc;
  char** argv;
  viskores::cont::testing::Testing::MakeArgsAddProgramName(argc, argv, "--viskores-device", "Any");
  viskores::cont::Initialize(argc, argv, viskores::cont::InitializeOptions::Strict);
  CheckArgs(argc, argv);
}

void InitializeCustomOptions()
{
  std::cout << "Initialize with some custom options and arguments" << std::endl;

  int argc;
  char** argv;
  viskores::cont::testing::Testing::MakeArgsAddProgramName(
    argc, argv, "--foo", "-bar", "baz", "buz");
  viskores::cont::Initialize(argc, argv);
  CheckArgs(argc, argv, "--foo", "-bar", "baz", "buz");

  viskores::cont::testing::Testing::MakeArgsAddProgramName(
    argc, argv, "--foo", "-bar", "--", "baz", "buz");
  viskores::cont::Initialize(argc, argv);
  CheckArgs(argc, argv, "--foo", "-bar", "--", "baz", "buz");
}

void InitializeMixedOptions()
{
  std::cout << "Initialize with options both for Viskores and some that are not." << std::endl;

  int argc;
  char** argv;
  viskores::cont::testing::Testing::MakeArgsAddProgramName(
    argc, argv, "--foo", "--viskores-device", "Any", "--bar", "baz");
  viskores::cont::Initialize(argc, argv, viskores::cont::InitializeOptions::AddHelp);
  CheckArgs(argc, argv, "--foo", "--bar", "baz");

  viskores::cont::testing::Testing::MakeArgsAddProgramName(argc,
                                                           argv,
                                                           "--foo",
                                                           "--viskores-log-level",
                                                           "OFF",
                                                           "--",
                                                           "--viskores-device",
                                                           "Any",
                                                           "--bar",
                                                           "baz");
  viskores::cont::Initialize(argc, argv);
  CheckArgs(argc, argv, "--foo", "--", "--viskores-device", "Any", "--bar", "baz");

  viskores::cont::testing::Testing::MakeArgsAddProgramName(
    argc, argv, "--viskores-device", "Any", "foo");
  viskores::cont::Initialize(argc, argv);
  CheckArgs(argc, argv, "foo");
}

void InitializeCustomOptionsWithArgs()
{
  std::cout << "Calling program has option --foo that takes arg bar." << std::endl;

  int argc;
  char** argv;
  viskores::cont::testing::Testing::MakeArgsAddProgramName(
    argc, argv, "--viskores-device", "Any", "--foo=bar", "--baz");
  viskores::cont::Initialize(argc, argv);
  CheckArgs(argc, argv, "--foo=bar", "--baz");

  viskores::cont::testing::Testing::MakeArgsAddProgramName(
    argc, argv, "--foo=bar", "--baz", "--viskores-device", "Any");
  viskores::cont::Initialize(argc, argv);
  CheckArgs(argc, argv, "--foo=bar", "--baz");

  viskores::cont::testing::Testing::MakeArgsAddProgramName(
    argc, argv, "--viskores-device", "Any", "--foo", "bar", "--baz");
  viskores::cont::Initialize(argc, argv);
  CheckArgs(argc, argv, "--foo", "bar", "--baz");

  viskores::cont::testing::Testing::MakeArgsAddProgramName(
    argc, argv, "--foo", "bar", "--baz", "--viskores-device", "Any");
  viskores::cont::Initialize(argc, argv);
  CheckArgs(argc, argv, "--foo", "bar", "--baz");
}

void InitializeRuntimeDeviceConfigurationWithArgs()
{
  int argc;
  char** argv;
  viskores::cont::testing::Testing::MakeArgsAddProgramName(argc,
                                                           argv,
                                                           "--viskores-device",
                                                           "Any",
                                                           "--viskores-num-threads",
                                                           "100",
                                                           "--viskores-device-instance",
                                                           "2");
  viskores::cont::Initialize(argc, argv);
  CheckArgs(argc, argv);
}

void InitializeWithHelp()
{
  std::cout << "Pass help flag to initialize" << std::endl;

  int argc;
  char** argv;
  viskores::cont::testing::Testing::MakeArgsAddProgramName(argc, argv, "--viskores-help");
  viskores::cont::Initialize(argc, argv);

  VISKORES_TEST_FAIL("Help argument did not exit as expected.");
}

void DoInitializeTests()
{
  // Technically, by the time we get here, we have already called Initialize once.
  std::cout << "Note: This test calls viskores::cont::Initialize multiple times to test"
            << std::endl
            << "it under different circumstances. You may get warnings/errors about" << std::endl
            << "that, particularly from the logging interface." << std::endl;

  InitializeZeroArguments();
  InitializeNoOptions();
  InitializeStandardOptions();
  InitializeCustomOptions();
  InitializeMixedOptions();
  InitializeCustomOptionsWithArgs();
  InitializeRuntimeDeviceConfigurationWithArgs();

  // This should be the last function called as it should exit with a zero status.
  InitializeWithHelp();
}

} // anonymous namespace

int UnitTestInitialize(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DoInitializeTests, argc, argv);
}
