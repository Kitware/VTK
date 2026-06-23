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

int InitializeWithHelp()
{
  std::cout << "Pass help flag to initialize" << std::endl;
  VISKORES_TEST_ASSERT(!viskores::cont::IsInitialized());

  int argc;
  char** argv;
  viskores::cont::testing::Testing::MakeArgsAddProgramName(argc, argv, "--viskores-help");
  viskores::cont::Initialize(argc, argv);

  VISKORES_TEST_FAIL("Help argument did not exit as expected.");
  return 1;
}

} // anonymous namespace

int UnitTestInitializeWithHelp(int, char*[])
{
  return viskores::cont::testing::Testing::ExecuteFunction(InitializeWithHelp);
}
