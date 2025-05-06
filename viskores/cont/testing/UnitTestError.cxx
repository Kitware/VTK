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

#include <viskores/cont/Error.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/Logging.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

void RecursiveFunction(int recurse)
{
  if (recurse < 5)
  {
    RecursiveFunction(++recurse);
  }
  else
  {
    throw viskores::cont::ErrorBadValue("Too much recursion");
  }
}

void ValidateError(const viskores::cont::Error& error)
{
  std::string message = "Too much recursion";
  std::string stackTrace = error.GetStackTrace();
  std::stringstream stackTraceStream(stackTrace);
  std::string tmp;
  size_t count = 0;
  while (std::getline(stackTraceStream, tmp))
  {
    count++;
  }

  // StackTrace may be unavailable on certain Devices
  if (stackTrace == "(Stack trace unavailable)")
  {
    VISKORES_TEST_ASSERT(count == 1, "Logging disabled, stack trace shouldn't be available");
  }
  else
  {
#if defined(NDEBUG)
    // The compiler can optimize out the recursion and other function calls in release
    // mode, but the backtrace should contain atleast one entry.
    std::string assert_msg = "No entries in the stack frame\n" + stackTrace;
    VISKORES_TEST_ASSERT(count >= 1, assert_msg);
#else
    std::string assert_msg = "Expected more entries in the stack frame\n" + stackTrace;
    VISKORES_TEST_ASSERT(count > 2, assert_msg);
#endif
  }
  VISKORES_TEST_ASSERT(test_equal(message, error.GetMessage()), "Message was incorrect");
  VISKORES_TEST_ASSERT(test_equal(message + "\n" + stackTrace, std::string(error.what())),
                       "what() was incorrect");
}

void DoErrorTest()
{
  VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Check base error messages");
  try
  {
    RecursiveFunction(0);
  }
  catch (const viskores::cont::Error& e)
  {
    ValidateError(e);
  }
}

} // anonymous namespace

int UnitTestError(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DoErrorTest, argc, argv);
}
