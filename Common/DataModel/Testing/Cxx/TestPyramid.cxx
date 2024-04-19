// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// this program tests the Pyramid cell. The test only includes centroid testing for now.

#include "vtkLogger.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPyramid.h"

#include <array>

//-----------------------------------------------------------------------------
#define VTK_REQUIRE(cond, msg)                                                                     \
  do                                                                                               \
  {                                                                                                \
    if (!(cond))                                                                                   \
    {                                                                                              \
      vtkLogF(ERROR, "'%s' => %s", #cond, msg);                                                    \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

//-----------------------------------------------------------------------------
template <typename T>
bool FuzzyCompare(T x, T y, double tol = 0.0)
{
  for (unsigned long i = 0; i < x.size(); ++i)
  {
    if (!vtkMathUtilities::FuzzyCompare(x[i], y[i], tol))
    {
      return false;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
int TestPyramid(int, char*[])
{
  constexpr double tol = 0.000001;

  vtkNew<vtkPyramid> pyramid;
  pyramid->GetPoints()->SetPoint(0, 1.0, 1.0, 1.0);
  pyramid->GetPoints()->SetPoint(1, -1.0, 1.0, 1.0);
  pyramid->GetPoints()->SetPoint(2, -1.0, -1.0, 1.0);
  pyramid->GetPoints()->SetPoint(3, 1.0, -1.0, 1.0);
  pyramid->GetPoints()->SetPoint(4, 0.0, 0.0, 0.0);

  // Testing vtkPyramid::ComputeCentroid and vtkPyramid::GetCentroid
  std::array<double, 3> centroid;
  bool res = pyramid->GetCentroid(centroid.data());
  VTK_REQUIRE(res, "vtkPyramid::GetCentroid FAILED: couldn't determine centroid");
  VTK_REQUIRE(FuzzyCompare(centroid, { 0.0, 0.0, 0.75 }, tol),
    "vtkPyramid::GetCentroid FAILED: wrong centroid");
  return EXIT_SUCCESS;
}
