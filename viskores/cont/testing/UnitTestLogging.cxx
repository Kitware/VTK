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
#include <viskores/cont/testing/Testing.h>

#include <chrono>
#include <thread>

namespace
{

void DoWork()
{
  VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Info);
  VISKORES_LOG_F(viskores::cont::LogLevel::Info, "Sleeping for 5 milliseconds...");
  std::this_thread::sleep_for(std::chrono::milliseconds{ 5 });
}

void Scopes(int level = 0)
{
  VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Info, "Called Scope (level=%d)", level);

  DoWork();

  VISKORES_LOG_IF_F(viskores::cont::LogLevel::Info,
                    level % 2 != 0,
                    "Printing extra log message because level is odd (%d)",
                    level);
  if (level < 5)
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Recursing to level " << level + 1);
    Scopes(level + 1);
  }
  else
  {
    VISKORES_LOG_F(viskores::cont::LogLevel::Warn, "Reached limit for Scopes test recursion.");
  }
}

void UserDefined()
{
  VISKORES_DEFINE_USER_LOG_LEVEL(CustomLevel, 0);
  VISKORES_DEFINE_USER_LOG_LEVEL(CustomLevel2, 2);
  VISKORES_DEFINE_USER_LOG_LEVEL(AnotherCustomLevel2, 2);
  VISKORES_DEFINE_USER_LOG_LEVEL(BigLevel, 300);

  viskores::cont::SetStderrLogLevel(viskores::cont::LogLevel::UserLast);
  VISKORES_LOG_S(CustomLevel, "CustomLevel");
  VISKORES_LOG_S(CustomLevel2, "CustomLevel2");
  VISKORES_LOG_S(AnotherCustomLevel2, "AnotherCustomLevel2");

  viskores::cont::SetStderrLogLevel(viskores::cont::LogLevel::UserFirst);
  VISKORES_LOG_S(BigLevel, "BigLevel"); // should log nothing

  viskores::cont::SetStderrLogLevel(viskores::cont::LogLevel::UserLast);
  VISKORES_LOG_S(BigLevel, "BigLevel");
}

void RunTests()
{
  VISKORES_LOG_F(viskores::cont::LogLevel::Info, "Running tests.");

  VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Running Scopes test...");
  Scopes();

  VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Running UserDefined test...");
  UserDefined();
}

} // end anon namespace

int UnitTestLogging(int, char*[])
{
  // Test that parameterless init works:
  VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Log before intialize");
  viskores::cont::InitLogging();

  RunTests();
  return 0;
}
