// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// this program tests the Tetra cell. The test is still very superficial for now.

#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkStringFormatter.h"
#include "vtkTetra.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <limits>
#include <string>

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

  // Testing vtkTetra::EvaluatePosition with points in different relative configurations

  // Vertices of a tetrahedron with obtuse dihedral angle and not aligned with coordinate axes.
  double pt0[3] = { 1, 0, 0 };
  double pt1[3] = { 2, 0, 0 };
  double pt2[3] = { 0, 1, 1 };
  double pt3[3] = { 0, 1, 0 };

  // Points to test the relative position and the closest point
  struct TestPointData
  {
    double p[3];
    double closestP[3];
    int inside;
  };
  std::vector<TestPointData> data = {
    // In the tetrahedron boundary:
    // * vertices
    { { 1., 0., 0. }, { 1., 0., 0. }, 1 },
    { { 2., 0., 0. }, { 2., 0., 0. }, 1 },
    { { 0., 1., 1. }, { 0., 1., 1. }, 1 },
    { { 0., 1., 0. }, { 0., 1., 0. }, 1 },
    // * edges
    { { 1., 0., 0. }, { 1., 0., 0. }, 1 },
    { { 1., 0.5, 0.5 }, { 1., 0.5, 0.5 }, 1 },
    { { 0.5, 0.5, 0.5 }, { 0.5, 0.5, 0.5 }, 1 },
    { { 0.5, 0.5, 0. }, { 0.5, 0.5, 0. }, 1 },
    { { 1., 0.5, 0. }, { 1., 0.5, 0. }, 1 },
    { { 0., 1., 0.5 }, { 0., 1., 0.5 }, 1 },
    // * triangular faces
    { { 1., 0.35, 0.35 }, { 1., 0.35, 0.35 }, 1 },
    { { 0.5, 0.5, 0.25 }, { 0.5, 0.5, 0.25 }, 1 },
    { { 0.5, 0.75, 0.3 }, { 0.5, 0.75, 0.3 }, 1 },
    { { 0.8, 0.4, 0. }, { 0.8, 0.4, 0. }, 1 },
    // In the tetrahedron strict interior:
    { { 0.2, 0.85, 0.5 }, { 0.2, 0.85, 0.5 }, 1 },
    { { 0.5, 0.75, 0.5 }, { 0.5, 0.75, 0.5 }, 1 },
    { { 0.8, 0.4, 0.1 }, { 0.8, 0.4, 0.1 }, 1 },
    { { 1., 0.1, 0.05 }, { 1., 0.1, 0.05 }, 1 },
    { { 1., 0.4, 0.2 }, { 1., 0.4, 0.2 }, 1 },
    { { 1.4, 0.2, 0.1 }, { 1.4, 0.2, 0.1 }, 1 },
    { { 1.8, 0.05, 0.02 }, { 1.8, 0.05, 0.02 }, 1 },
    // Outside the tetrahedron, projecting to face 0-1-2
    { { 1.01, 0.01, 0.97 }, { 1.01, 0.49, 0.49 }, 0 },
    { { 0.5, 0.15, 0.95 }, { 0.5, 0.55, 0.55 }, 0 },
    { { 0.5, 0.3, 1. }, { 0.5, 0.65, 0.65 }, 0 },
    // Outside the tetrahedron, projecting to face 0-1-3
    { { 0.8, 0.4, -1. }, { 0.8, 0.4, 0. }, 0 },
    { { 1.6, 0.1, -0.5 }, { 1.6, 0.1, 0. }, 0 },
    // Outside the tetrahedron, projecting to face 0-2-3
    { { -0.3, -0.7, 0.2 }, { 0.7, 0.3, 0.2 }, 0 },
    { { -0.8, -0.2, 0.7 }, { 0.2, 0.8, 0.7 }, 0 },
    // Outside the tetrahedron, projecting to face 1-2-3
    { { 2.1, 1.2, 0.1 }, { 1.6, 0.2, 0.1 }, 0 },
    { { 1.1, 1.7, 0.6 }, { 0.6, 0.7, 0.6 }, 0 },
    // Outside the tetrahedron, projecting to edge 0-1
    { { 1.01, -0.5, 0.4 }, { 1.01, 0., 0. }, 0 },
    { { 1.999, -0.2, -0.4 }, { 1.999, 0., 0. }, 0 },
    // Outside the tetrahedron, projecting to edge 0-2
    { { -0.1, 0.6, 1. }, { 0.1, 0.9, 0.9 }, 0 },
    { { 0.86, -0.42, 0.58 }, { 0.9, 0.1, 0.1 }, 0 },
    // Outside the tetrahedron, projecting to edge 0-3
    { { 0.2, -0.6, -0.5 }, { 0.9, 0.1, 0. }, 0 },
    { { 0.8, 0., -0.5 }, { 0.9, 0.1, 0. }, 0 },
    // Outside the tetrahedron, projecting to edge 1-2
    { { 2.3, 0.9, 0.3 }, { 1.8, 0.1, 0.1 }, 0 },
    { { 0.4, 0.6, 1.6 }, { 0.2, 0.9, 0.9 }, 0 },
    // Outside the tetrahedron, projecting to edge 1-3
    { { 2., 0.5, -0.4 }, { 1.8, 0.1, 0. }, 0 },
    { { 0.2, 0.9, -0.3 }, { 0.2, 0.9, 0. }, 0 },
    // Outside the tetrahedron, projecting to edge 2-3
    { { -0.3, 1.2, 0.1 }, { 0., 1., 0.1 }, 0 },
    { { -0.3, 0.75, 0.99 }, { 0., 1., 0.99 }, 0 },
    // Outside the triangle, projecting to vertex 0
    { { 0.9, -0.5, -0.5 }, { 1., 0., 0. }, 0 },
    { { 0.9, -0.5, 0.3 }, { 1., 0., 0. }, 0 },
    // Outside the triangle, projecting to vertex 1
    { { 2.1, -0.5, -0.5 }, { 2., 0., 0. }, 0 },
    { { 3., 0.9, 0.9 }, { 2., 0., 0. }, 0 },
    // Outside the triangle, projecting to vertex 2
    { { -1., 1., 1.2 }, { 0., 1., 1. }, 0 },
    { { -0.5, 0.5, 1.2 }, { 0., 1., 1. }, 0 },
    // Outside the triangle, projecting to vertex 3
    { { -0.5, 0.5, -0.1 }, { 0., 1., 0. }, 0 },
    { { -0.5, 1.3, -0.1 }, { 0., 1., 0. }, 0 },
  };

  // The results should be independent of the order of the tetrahedral vertices:
  std::vector<int> pId = { 0, 1, 2, 3 };
  do
  {
    vtkNew<vtkTetra> tetra2;
    vtkPoints* pts = tetra2->GetPoints();
    pts->SetPoint(pId[0], pt0);
    pts->SetPoint(pId[1], pt1);
    pts->SetPoint(pId[2], pt2);
    pts->SetPoint(pId[3], pt3);

    double closestPoint[3];
    double dist2;
    for (unsigned int i = 0; i < data.size(); ++i)
    {
      int inside = tetra2->EvaluatePosition(
        data[i].p, closestPoint, subId, pcoords.data(), dist2, weights.data());
      if (inside != data[i].inside)
      {
        std::cerr << "ERROR: EvaluatePosition on point " << vtk::format("{}", data[i].p)
                  << std::endl
                  << "       and vertex permutation " << vtk::format("{}", pId) << std::endl
                  << "       returns value " << inside << std::endl
                  << "       not coinciding with insideGT = " << data[i].inside << std::endl;
        ;
        return EXIT_FAILURE;
      }
      if (vtkMath::Distance2BetweenPoints(closestPoint, data[i].closestP) > 1e-12)
      {
        std::cerr << "ERROR: EvaluatePosition on point " << vtk::format("{}", data[i].p)
                  << std::endl
                  << "       and vertex permutation " << vtk::format("{}", pId) << std::endl
                  << "       gives the closest point " << vtk::format("{}", closestPoint)
                  << std::endl
                  << "       when it should be " << vtk::format("{}", data[i].closestP)
                  << std::endl;
        return EXIT_FAILURE;
      }
    }
  } while (std::next_permutation(pId.begin(), pId.end()));

  return EXIT_SUCCESS;
}
