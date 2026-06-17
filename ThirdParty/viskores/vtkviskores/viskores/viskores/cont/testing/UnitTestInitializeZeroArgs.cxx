//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/cont/Initialize.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

void InitializeZeroArguments()
{
  std::cout << "Initialize with no arguments" << std::endl;
  VISKORES_TEST_ASSERT(!viskores::cont::IsInitialized());
  viskores::cont::Initialize();
  VISKORES_TEST_ASSERT(viskores::cont::IsInitialized());
}

} // anonymous namespace

int UnitTestInitializeZeroArgs(int, char*[])
{
  return viskores::cont::testing::Testing::ExecuteFunction(InitializeZeroArguments);
}
