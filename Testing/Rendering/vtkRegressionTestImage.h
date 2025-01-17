// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkRegressionTestImage_h
#define vtkRegressionTestImage_h

// Includes and a macro necessary for saving the image produced by a cxx
// example program. This capability is critical for regression testing.
// This function returns 1 if test passed, 0 if test failed.

#include "vtkTesting.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkRegressionTester : public vtkTesting
{
public:
  // 0.05 threshold is arbitrary but found to
  // allow most graphics system variances to pass
  // when they should and fail when they should
  static constexpr double ErrorThreshold = 0.05;

protected:
  vtkRegressionTester() = default;
  ~vtkRegressionTester() override = default;

private:
  vtkRegressionTester(const vtkRegressionTester&) = delete;
  void operator=(const vtkRegressionTester&) = delete;
};

#define vtkRegressionTestImage(rw)                                                                 \
  vtkTesting::Test(argc, argv, rw, vtkRegressionTester::ErrorThreshold)

#define vtkRegressionTestImageThreshold(rw, t) vtkTesting::Test(argc, argv, rw, t)

#define vtkRegressionTestPassForMesaLessThan(rw, major, minor, patch)                              \
  do                                                                                               \
  {                                                                                                \
    int mesaVersion[3] = {};                                                                       \
    if (vtkTesting::GetMesaVersion(rw, mesaVersion))                                               \
    {                                                                                              \
      if (mesaVersion[0] < major || (mesaVersion[0] == major && mesaVersion[1] < minor) ||         \
        (mesaVersion[0] == major && mesaVersion[1] == minor && mesaVersion[2] < patch))            \
      {                                                                                            \
        return EXIT_SUCCESS;                                                                       \
      }                                                                                            \
    }                                                                                              \
  } while (false)

VTK_ABI_NAMESPACE_END
#endif // vtkRegressionTestImage_h
// VTK-HeaderTest-Exclude: vtkRegressionTestImage.h
