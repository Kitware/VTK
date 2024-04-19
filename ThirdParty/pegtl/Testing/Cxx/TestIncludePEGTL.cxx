#include "vtk_pegtl.h"
// clang-format off
#include VTK_PEGTL(pegtl/contrib/tracer.hpp)
// clang-format on

#include <cstdlib>

int TestIncludePEGTL(int /*argc*/, char* /*argv*/[])
{
  return EXIT_SUCCESS;
}
