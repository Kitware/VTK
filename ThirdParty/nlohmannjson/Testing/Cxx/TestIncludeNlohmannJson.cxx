#include "vtk_nlohmannjson.h"
// clang-format off
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

#include <cstdlib>

int TestIncludeNlohmannJson(int /*argc*/, char* /*argv*/[])
{
  return EXIT_SUCCESS;
}
