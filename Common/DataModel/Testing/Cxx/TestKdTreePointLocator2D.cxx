// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Check the queries of vtkKdTreePointLocator against brute force, on points of
// a plane of constant z (the locator then builds a 2D tree), and on points
// that are not in a plane (a 3D tree). The distances of the 2D tree are in the
// x-y plane: the z of the queries and of the petals is ignored.

#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkKdTreePointLocator.h"
#include "vtkLocatorInterface.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <random>
#include <set>
#include <vector>

namespace
{
using Points = std::vector<std::array<double, 3>>;

double Dist2(const std::array<double, 3>& p, const double x[3], bool flat)
{
  if (flat)
  {
    return (p[0] - x[0]) * (p[0] - x[0]) + (p[1] - x[1]) * (p[1] - x[1]);
  }
  return vtkMath::Distance2BetweenPoints(p.data(), x);
}

bool Check(const char* name, const Points& pts, bool flat)
{
  const vtkIdType n = static_cast<vtkIdType>(pts.size());
  vtkNew<vtkPoints> vtkPts;
  vtkPts->SetDataTypeToDouble();
  for (const auto& p : pts)
  {
    vtkPts->InsertNextPoint(p.data());
  }
  vtkNew<vtkPolyData> data;
  data->SetPoints(vtkPts);

  vtkNew<vtkKdTreePointLocator> locator;
  locator->SetDataSet(data);
  locator->BuildLocator();

  if (locator->GetDimension() != (flat ? 2 : 3))
  {
    std::cout << name << ": wrong dimension " << locator->GetDimension() << "\n";
    return false;
  }

  std::mt19937 gen(7);
  std::uniform_real_distribution<double> uni(0.0, 100.0);
  const double zs[4] = { 3.0, 0.0, 7.5, -2.0 };
  int errors = 0;
  for (int q = 0; q < 80; ++q)
  {
    // Queries in the plane of the points, and out of it.
    const double x[3] = { uni(gen), uni(gen), flat ? zs[q % 4] : uni(gen) };
    std::vector<double> d2(n);
    for (vtkIdType i = 0; i < n; ++i)
    {
      d2[i] = Dist2(pts[i], x, flat);
    }
    const double minD2 = *std::min_element(d2.begin(), d2.end());

    // Closest point, with and without a radius.
    const vtkIdType closest = locator->FindClosestPoint(x);
    if (closest < 0 || d2[closest] != minD2)
    {
      ++errors;
    }
    double dist2 = 0.0;
    const double radius = std::sqrt(minD2) * 1.0001 + 1e-12;
    const vtkIdType within = locator->FindClosestPointWithinRadius(radius, x, dist2);
    if (within < 0 || std::abs(dist2 - minD2) > 1e-9 * (1.0 + minD2))
    {
      ++errors;
    }
    if (locator->FindClosestPointWithinRadius(std::sqrt(minD2) * 0.99 - 1e-9, x, dist2) >= 0 &&
      minD2 > 1e-9)
    {
      ++errors;
    }

    // The N closest points.
    vtkNew<vtkIdList> ids;
    locator->FindClosestNPoints(7, x, ids);
    std::vector<double> got, expected(d2);
    for (vtkIdType i = 0; i < ids->GetNumberOfIds(); ++i)
    {
      got.push_back(d2[ids->GetId(i)]);
    }
    std::sort(got.begin(), got.end());
    std::sort(expected.begin(), expected.end());
    expected.resize(7);
    if (got.size() != 7 || !std::equal(got.begin(), got.end(), expected.begin()))
    {
      ++errors;
    }

    // The points within a radius (not too close to the boundary).
    const double R = 25.0;
    locator->FindPointsWithinRadius(R, x, ids);
    std::set<vtkIdType> within1;
    for (vtkIdType i = 0; i < ids->GetNumberOfIds(); ++i)
    {
      within1.insert(ids->GetId(i));
    }
    for (vtkIdType i = 0; i < n; ++i)
    {
      if (std::abs(d2[i] - R * R) > 1e-9 && (d2[i] < R * R) != (within1.count(i) > 0))
      {
        ++errors;
      }
    }

    // The points in the shell (minDist2, maxDist2], with and without a petal:
    // a circle for the 2D tree, a sphere for the 3D one.
    const double lo = 400.0, hi = 900.0;
    vtkNew<vtkDoubleArray> petals;
    if (flat)
    {
      petals->SetNumberOfComponents(3);
      petals->InsertNextTuple3(50.0, 50.0, 30.0 * 30.0);
    }
    else
    {
      petals->SetNumberOfComponents(4);
      petals->InsertNextTuple4(50.0, 50.0, 50.0, 30.0 * 30.0);
    }
    vtkDist2TupleArray shell;
    for (int usePetals = 0; usePetals < 2; ++usePetals)
    {
      locator->FindPointsInShell(x, shell, lo, hi, true, usePetals ? petals.GetPointer() : nullptr);
      std::set<vtkIdType> expectedShell;
      for (vtkIdType i = 0; i < n; ++i)
      {
        bool inPetal = true;
        if (usePetals)
        {
          const double c[3] = { 50.0, 50.0, 50.0 };
          inPetal = Dist2(pts[i], c, flat) <= 900.0;
        }
        if (d2[i] > lo && d2[i] <= hi && inPetal)
        {
          expectedShell.insert(i);
        }
      }
      std::set<vtkIdType> gotShell;
      double previous = -1.0;
      for (const auto& t : shell)
      {
        gotShell.insert(t.Id);
        if (t.Dist2 != d2[t.Id] || t.Dist2 < previous)
        {
          ++errors;
        }
        previous = t.Dist2;
      }
      if (gotShell != expectedShell || shell.size() != gotShell.size())
      {
        ++errors;
      }
    }
  }

  std::cout << name << ": " << errors << " errors\n";
  return errors == 0;
}
}

int TestKdTreePointLocator2D(int, char*[])
{
  std::mt19937 gen(3);
  std::uniform_real_distribution<double> uni(0.0, 100.0);

  Points flat, solid;
  for (int i = 0; i < 2000; ++i)
  {
    flat.push_back({ uni(gen), uni(gen), 3.0 });
    solid.push_back({ uni(gen), uni(gen), uni(gen) });
  }
  // Coincident points
  for (int i = 0; i < 50; ++i)
  {
    flat[i] = flat[50 + i];
    solid[i] = solid[50 + i];
  }

  bool ok = Check("points in a plane (2D tree)", flat, true);
  ok = Check("points in a volume (3D tree)", solid, false) && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
