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

#include <viskores/cont/testing/Testing.h>

namespace
{

enum TestOptionsIndex
{
  TEST_UNKNOWN,
  DATADIR,     // base dir containing test data files
  BASELINEDIR, // base dir for regression test images
  WRITEDIR,    // base dir for generated regression test images
};

} // anonymous namespace

namespace viskores
{
namespace cont
{
namespace testing
{

std::string Testing::GetTestDataBasePath()
{
  return SetAndGetTestDataBasePath();
}

std::string Testing::DataPath(const std::string& filename)
{
  return GetTestDataBasePath() + filename;
}

std::string Testing::GetRegressionTestImageBasePath()
{
  return SetAndGetRegressionImageBasePath();
}

std::string Testing::RegressionImagePath(const std::string& filename)
{
  return GetRegressionTestImageBasePath() + filename;
}

std::string Testing::GetWriteDirBasePath()
{
  return SetAndGetWriteDirBasePath();
}

std::string Testing::WriteDirPath(const std::string& filename)
{
  return GetWriteDirBasePath() + filename;
}

void Testing::SetEnv(const std::string& var, const std::string& value)
{
  static std::vector<std::pair<std::string, std::string>> envVars{};
#ifdef _WIN32
  auto iter = envVars.emplace(envVars.end(), var, value);
  _putenv_s(iter->first.c_str(), iter->second.c_str());
#else
  setenv(var.c_str(), value.c_str(), 1);
#endif
}

void Testing::UnsetEnv(const std::string& var)
{
#ifdef _WIN32
  SetEnv(var, "");
#else
  unsetenv(var.c_str());
#endif
}

std::string& Testing::SetAndGetTestDataBasePath(std::string path)
{
  static std::string TestDataBasePath;

  if (!path.empty())
  {
    TestDataBasePath = path;
    if ((TestDataBasePath.back() != '/') && (TestDataBasePath.back() != '\\'))
    {
      TestDataBasePath = TestDataBasePath + "/";
    }
  }

  if (TestDataBasePath.empty())
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Error,
                   "TestDataBasePath was never set, was --viskores-data-dir set correctly? (hint: "
                   "../data/data)");
  }

  return TestDataBasePath;
}

std::string& Testing::SetAndGetRegressionImageBasePath(std::string path)
{
  static std::string RegressionTestImageBasePath;

  if (!path.empty())
  {
    RegressionTestImageBasePath = path;
    if ((RegressionTestImageBasePath.back() != '/') && (RegressionTestImageBasePath.back() != '\\'))
    {
      RegressionTestImageBasePath = RegressionTestImageBasePath + '/';
    }
  }

  if (RegressionTestImageBasePath.empty())
  {
    VISKORES_LOG_S(
      viskores::cont::LogLevel::Error,
      "RegressionTestImageBasePath was never set, was --viskores-baseline-dir set correctly? "
      "(hint: ../data/baseline)");
  }

  return RegressionTestImageBasePath;
}

std::string& Testing::SetAndGetWriteDirBasePath(std::string path)
{
  static std::string WriteDirBasePath;

  if (!path.empty())
  {
    WriteDirBasePath = path;
    if ((WriteDirBasePath.back() != '/') && (WriteDirBasePath.back() != '\\'))
    {
      WriteDirBasePath = WriteDirBasePath + '/';
    }
  }

  return WriteDirBasePath;
}

void Testing::ParseAdditionalTestArgs(int& argc, char* argv[])
{
  std::vector<opt::Descriptor> usage;

  usage.push_back({ DATADIR,
                    0,
                    "",
                    "viskores-data-dir",
                    opt::ViskoresArg::Required,
                    "  --viskores-data-dir "
                    "<data-dir-path> \tPath to the "
                    "base data directory in the Viskores "
                    "src dir." });
  usage.push_back({ BASELINEDIR,
                    0,
                    "",
                    "viskores-baseline-dir",
                    opt::ViskoresArg::Required,
                    "  --viskores-baseline-dir "
                    "<baseline-dir-path> "
                    "\tPath to the base dir "
                    "for regression test "
                    "images" });
  usage.push_back({ WRITEDIR,
                    0,
                    "",
                    "viskores-write-dir",
                    opt::ViskoresArg::Required,
                    "  --viskores-write-dir "
                    "<write-dir-path> "
                    "\tPath to the write dir "
                    "to store generated "
                    "regression test images" });

  // Required to collect unknown arguments when help is off.
  usage.push_back({ TEST_UNKNOWN, 0, "", "", opt::ViskoresArg::UnknownOption, "" });
  usage.push_back({ 0, 0, 0, 0, 0, 0 });


  // Remove argv[0] (executable name) if present:
  int viskoresArgc = argc > 0 ? argc - 1 : 0;
  char** viskoresArgv = argc > 0 ? argv + 1 : argv;

  opt::Stats stats(usage.data(), viskoresArgc, viskoresArgv);
  std::unique_ptr<opt::Option[]> options{ new opt::Option[stats.options_max] };
  std::unique_ptr<opt::Option[]> buffer{ new opt::Option[stats.buffer_max] };
  opt::Parser parse(usage.data(), viskoresArgc, viskoresArgv, options.get(), buffer.get());

  if (parse.error())
  {
    std::cerr << "Internal Initialize parser error" << std::endl;
    exit(1);
  }

  if (options[DATADIR])
  {
    SetAndGetTestDataBasePath(options[DATADIR].arg);
  }

  if (options[BASELINEDIR])
  {
    SetAndGetRegressionImageBasePath(options[BASELINEDIR].arg);
  }

  if (options[WRITEDIR])
  {
    SetAndGetWriteDirBasePath(options[WRITEDIR].arg);
  }

  for (const opt::Option* opt = options[TEST_UNKNOWN]; opt != nullptr; opt = opt->next())
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                   "Unknown option to internal Initialize: " << opt->name << "\n");
  }

  for (int nonOpt = 0; nonOpt < parse.nonOptionsCount(); ++nonOpt)
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                   "Unknown argument to internal Initialize: " << parse.nonOption(nonOpt) << "\n");
  }
}

} // namespace testing
} // namespace cont
} // namespace viskores
