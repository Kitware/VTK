// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// this program tests the Tetra cell. The test is still very superficial for now.

#include "vtkLogger.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkTetra.h"

#include <array>
#include <cassert>
#include <limits>

//-----------------------------------------------------------------------------
#define VTK_REQUIRE(cond, msg)                                                                     \
  do                                                                                               \
  {                                                                                                \
    if (!(cond))                                                                                   \
    {                                                                                              \
      vtkLogF(ERROR, "'%s' => %s", #cond, msg);                                                    \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false);

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
int TestTetra(int, char*[])
{
  constexpr double tol = 0.000001;

  vtkNew<vtkTetra> tetra;
  tetra->GetPoints()->SetPoint(0, 0.0, 0.0, 0.0);
  tetra->GetPoints()->SetPoint(1, 1.0, 0.0, 0.0);
  tetra->GetPoints()->SetPoint(2, 0.0, 1.0, 0.0);
  tetra->GetPoints()->SetPoint(3, 0.0, 0.0, 1.0);

  // Testing vtkTetra::IntersectWithLine and vtktetra::InterpolateFunctions
  std::array<double, 3> x, pcoords;
  std::array<double, 4> weights;
  double t;
  int subId;
  std::array<double, 3> p2 = { 0.25, 0.25, 0.25 };
  std::array<double, 3> p1 = { -0.25, 0.25, 0.25 };
  int res = tetra->IntersectWithLine(p1.data(), p2.data(), tol, t, x.data(), pcoords.data(), subId);
  VTK_REQUIRE(res > 0, "vtkTetra::IntersectWithLine FAILED: couldn't find intersection");
  VTK_REQUIRE(
    vtkMathUtilities::NearlyEqual(t, 0.5, tol), "vtkTetra::IntersectWithLine FAILED: wrong t");
  VTK_REQUIRE(
    FuzzyCompare(x, { 0.0, 0.25, 0.25 }, tol), "vtkTetra::IntersectWithLine FAILED: wrong x");
  VTK_REQUIRE(FuzzyCompare(pcoords, { 0.0, 0.25, 0.25 }, tol),
    "vtkTetra::IntersectWithLine FAILED: wrong pcoords");
  tetra->InterpolateFunctions(pcoords.data(), weights.data());
  VTK_REQUIRE(FuzzyCompare(weights, { 0.5, 0.0, 0.25, 0.25 }, tol),
    "vtkTetra::IntersectWithLine FAILED: wrong interpolation function");

  p1 = { 0.25, -0.25, 0.25 };
  p2 = { 0.25, 0.25, 0.25 };
  res = tetra->IntersectWithLine(p1.data(), p2.data(), tol, t, x.data(), pcoords.data(), subId);
  VTK_REQUIRE(res > 0, "vtkTetra::IntersectWithLine FAILED: couldn't find intersection");
  VTK_REQUIRE(
    vtkMathUtilities::NearlyEqual(t, 0.5, tol), "vtkTetra::IntersectWithLine FAILED: wrong t");
  VTK_REQUIRE(
    FuzzyCompare(x, { 0.25, 0.0, 0.25 }, tol), "vtkTetra::IntersectWithLine FAILED: wrong x");
  VTK_REQUIRE(FuzzyCompare(pcoords, { 0.25, 0.0, 0.25 }, tol),
    "vtkTetra::IntersectWithLine FAILED: wrong pcoords");
  tetra->InterpolateFunctions(pcoords.data(), weights.data());
  VTK_REQUIRE(FuzzyCompare(weights, { 0.5, 0.25, 0.0, 0.25 }, tol),
    "vtkTetra::IntersectWithLine FAILED: wrong interpolation function");

  p1 = { 0.25, 0.25, -0.25 };
  p2 = { 0.25, 0.25, 0.25 };
  res = tetra->IntersectWithLine(p1.data(), p2.data(), tol, t, x.data(), pcoords.data(), subId);
  VTK_REQUIRE(res > 0, "vtkTetra::IntersectWithLine FAILED: couldn't find intersection");
  VTK_REQUIRE(
    vtkMathUtilities::NearlyEqual(t, 0.5, tol), "vtkTetra::IntersectWithLine FAILED: wrong t");
  VTK_REQUIRE(
    FuzzyCompare(x, { 0.25, 0.25, 0.0 }, tol), "vtkTetra::IntersectWithLine FAILED: wrong x");
  VTK_REQUIRE(FuzzyCompare(pcoords, { 0.25, 0.25, 0.0 }, tol),
    "vtkTetra::IntersectWithLine FAILED: wrong pcoords");
  tetra->InterpolateFunctions(pcoords.data(), weights.data());
  VTK_REQUIRE(FuzzyCompare(weights, { 0.5, 0.25, 0.25, 0.0 }, tol),
    "vtkTetra::IntersectWithLine FAILED: wrong interpolation function");

  constexpr double athird = 1.0 / 3;
  constexpr double asixth = 1.0 / 6;
  p1 = { 0.5, 0.5, 0.5 };
  p2 = { asixth, asixth, asixth };
  res = tetra->IntersectWithLine(p1.data(), p2.data(), tol, t, x.data(), pcoords.data(), subId);
  VTK_REQUIRE(res > 0, "vtkTetra::IntersectWithLine FAILED: couldn't find intersection");
  VTK_REQUIRE(
    vtkMathUtilities::NearlyEqual(t, 0.5, tol), "vtkTetra::IntersectWithLine FAILED: wrong t");
  VTK_REQUIRE(FuzzyCompare(x, { athird, athird, athird }, tol),
    "vtkTetra::IntersectWithLine FAILED: wrong x");
  VTK_REQUIRE(FuzzyCompare(pcoords, { athird, athird, athird }, tol),
    "vtkTetra::IntersectWithLine FAILED: wrong pcoords");
  tetra->InterpolateFunctions(pcoords.data(), weights.data());
  VTK_REQUIRE(FuzzyCompare(weights, { 0.0, athird, athird, athird }, tol),
    "vtkTetra::IntersectWithLine FAILED: wrong interpolation function");

  return EXIT_SUCCESS;
}
