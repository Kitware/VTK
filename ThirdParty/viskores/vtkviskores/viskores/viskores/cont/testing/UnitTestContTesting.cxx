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

// This meta-test makes sure that the testing environment is properly reporting
// errors.

#include <viskores/Assert.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

void TestFail()
{
  VISKORES_TEST_FAIL("I expect this error.");
}

void BadTestAssert()
{
  VISKORES_TEST_ASSERT(0 == 1, "I expect this error.");
}

void GoodAssert()
{
  VISKORES_TEST_ASSERT(1 == 1, "Always true.");
  VISKORES_ASSERT(1 == 1);
}

} // anonymous namespace

int UnitTestContTesting(int argc, char* argv[])
{
  std::cout << "-------\nThis call should fail." << std::endl;
  if (viskores::cont::testing::Testing::Run(TestFail, argc, argv) == 0)
  {
    std::cout << "Did not get expected fail!" << std::endl;
    return 1;
  }
  std::cout << "-------\nThis call should fail." << std::endl;
  if (viskores::cont::testing::Testing::Run(BadTestAssert, argc, argv) == 0)
  {
    std::cout << "Did not get expected fail!" << std::endl;
    return 1;
  }

  std::cout << "-------\nThis call should pass." << std::endl;
  // This is what your main function typically looks like.
  return viskores::cont::testing::Testing::Run(GoodAssert, argc, argv);
}
