#include "vtk_kissfft.h"
// clang-format off
#include VTK_KISSFFT_HEADER(kiss_fft.h)
#include VTK_KISSFFT_HEADER(tools/kiss_fftr.h)
// clang-format on

#include <cstdlib>

int TestIncludeKissFFT(int /*argc*/, char* /*argv*/[])
{
  return EXIT_SUCCESS;
}
