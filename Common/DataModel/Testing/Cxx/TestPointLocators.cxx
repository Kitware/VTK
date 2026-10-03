// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAbstractPointLocator.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkKdTree.h"
#include "vtkKdTreePointLocator.h"
#include "vtkLocatorInterface.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkOctreePointLocator.h"
#include "vtkPointLocator.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkStaticPointLocator.h"
#include "vtkStaticPointLocator2D.h"
#include "vtkStructuredGrid.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <set>
#include <vector>

// returns true if 2 points are equidistant from x, within a tolerance
bool ArePointsEquidistant(double x[3], vtkIdType id1, vtkIdType id2, vtkPointSet* grid)
{
  if (id1 == id2)
  {
    return true;
  }
  float firstDist2 = vtkMath::Distance2BetweenPoints(x, grid->GetPoint(id1));
  float secondDist2 = vtkMath::Distance2BetweenPoints(x, grid->GetPoint(id2));

  float differenceDist2 = firstDist2 - secondDist2;
  if (differenceDist2 < 0)
  {
    differenceDist2 = -differenceDist2;
  }

  if (differenceDist2 / (firstDist2 + secondDist2) > .00001)
  {
    std::cerr << "Results do not match (first dist2=" << firstDist2
              << " , second dist2=" << secondDist2 << ") ";
    return false;
  }
  return true;
}

// checks that every point in firstList has a matching point (based
// on distance) in secondList
bool DoesListHaveProperPoints(
  double x[3], vtkIdList* firstList, vtkIdList* secondList, vtkPointSet* grid)
{
  for (vtkIdType uid = 0; uid < firstList->GetNumberOfIds(); uid++)
  {
    int found = 0;
    for (vtkIdType kid = 0; kid < secondList->GetNumberOfIds(); kid++)
    {
      if (firstList->GetId(uid) == secondList->GetId(kid))
      {
        found = 1;
        break;
      }
    }
    if (!found)
    {
      for (vtkIdType kid = 0; kid < secondList->GetNumberOfIds(); kid++)
      {
        if (ArePointsEquidistant(x, firstList->GetId(uid), secondList->GetId(kid), grid))
        {
          found = 1;
          break;
        }
      }
    }
    if (!found)
    {
      return false;
    }
  }
  return true;
}

// This test compares results for different point locators since they should
// all return the same results (within a tolerance)
int ComparePointLocators(vtkAbstractPointLocator* locator1, vtkAbstractPointLocator* locator2)
{
  int rval = 0;
  int i, j, k, kOffset, jOffset, offset;
  float x[3];
  static const int dims[3] = { 39, 31, 31 };

  // Create the structured grid.
  vtkStructuredGrid* sgrid = vtkStructuredGrid::New();
  sgrid->SetDimensions(dims);

  // We also create the points.
  vtkPoints* points = vtkPoints::New();
  points->Reserve(dims[0] * dims[1] * dims[2]);

  for (k = 0; k < dims[2]; k++)
  {
    x[2] = 1.0 + k * 1.2;
    kOffset = k * dims[0] * dims[1];
    for (j = 0; j < dims[1]; j++)
    {
      x[1] = sqrt(10. + j * 2.);
      jOffset = j * dims[0];
      for (i = 0; i < dims[0]; i++)
      {
        x[0] = 1 + i * i * .5;
        offset = i + jOffset + kOffset;
        points->InsertPoint(offset, x);
      }
    }
  }
  sgrid->SetPoints(points);
  points->Delete();

  locator1->SetDataSet(sgrid);
  locator2->SetDataSet(sgrid);

  double bounds[6];
  sgrid->GetBounds(bounds);
  for (i = 0; i < 3; i++)
  {
    // expand the search so we are looking for points inside and outside the BB
    bounds[i * 2] *= .5;
    bounds[i * 2 + 1] *= 1.2;
  }
  int numSearchPoints = 20;
  vtkIdList* locator1List = vtkIdList::New();
  vtkIdList* locator2List = vtkIdList::New();
  for (i = 0; i < numSearchPoints; i++)
  {
    double point[3] = { (bounds[0] + (bounds[1] - bounds[0]) * i / numSearchPoints),
      (bounds[2] + (bounds[3] - bounds[2]) * i / numSearchPoints),
      (bounds[4] + (bounds[5] - bounds[4]) * i / numSearchPoints) };
    vtkIdType locator1Pt = locator1->FindClosestPoint(point);
    vtkIdType locator2Pt = locator2->FindClosestPoint(point);
    if (!ArePointsEquidistant(point, locator1Pt, locator2Pt, sgrid))
    {
      std::cerr << " from FindClosestPoint.\n";
      rval++;
    }
    int N = 1 + i * 250 / numSearchPoints; // test different amounts of points to search for
    locator1->FindClosestNPoints(N, point, locator1List);
    locator2->FindClosestNPoints(N, point, locator2List);
    if (!ArePointsEquidistant(point, locator1Pt, locator1List->GetId(0), sgrid))
    {
      std::cerr
        << "for comparing FindClosestPoint and first result of FindClosestNPoints for locator1.\n";
      rval++;
    }
    if (!ArePointsEquidistant(point, locator2Pt, locator2List->GetId(0), sgrid))
    {
      std::cerr
        << "for comparing FindClosestPoint and first result of FindClosestNPoints for locator2.\n";
      rval++;
    }

    for (j = 0; j < N; j++)
    {
      if (!ArePointsEquidistant(point, locator2List->GetId(j), locator1List->GetId(j), sgrid))
      {
        std::cerr << "for point " << j << " for ClosestNPoints search.\n";
        rval++;
      }
    }
    double radius = 10;
    locator1->FindPointsWithinRadius(radius, point, locator1List);
    locator2->FindPointsWithinRadius(radius, point, locator2List);
    if (!DoesListHaveProperPoints(point, locator1List, locator2List, sgrid) ||
      !DoesListHaveProperPoints(point, locator2List, locator1List, sgrid))
    {
      std::cerr << "Problem with FindPointsWithinRadius\n";
      rval++;
    }

    double dist2;
    locator1Pt = locator1->FindClosestPointWithinRadius(radius, point, dist2);
    locator2Pt = locator2->FindClosestPointWithinRadius(radius, point, dist2);
    if (locator1Pt < 0 || locator2Pt < 0)
    {
      if (locator1Pt >= 0 || locator2Pt >= 0)
      {
        std::cerr << "Inconsistent results for FindClosestPointWithinRadius\n";
        rval++;
      }
    }
    else if (!ArePointsEquidistant(point, locator1Pt, locator2Pt, sgrid))
    {
      std::cerr << "Incorrect result for FindClosestPointWithinRadius.\n";
      rval++;
    }
    if (locator1Pt >= 0)
    {
      locator1List->Reset();
      locator1List->InsertNextId(locator1Pt);
      if (!DoesListHaveProperPoints(point, locator1List, locator2List, sgrid))
      {
        std::cerr
          << "Inconsistent results for FindClosestPointWithinRadius and FindPointsWithRadius\n";
        rval++;
      }
    }
  }

  locator1List->Delete();
  locator2List->Delete();

  sgrid->Delete();

  return rval; // returns 0 if all tests passes
}

// This test does a brute force test on the KdTree point locator
// to make sure that at least one of the point locators used
// above gives a correct result for FindClosestPoint().
int TestKdTreePointLocator()
{
  int rval = 0;
  vtkIdType num_points = 1000;
  vtkIdType num_test_points = 100;

  vtkIdType idA;
  vtkIdType closest_id = -1;
  vtkIdType point;
  vtkIdType test_point;

  double pointA[3];
  double pointB[3];

  std::mt19937 rng(std::random_device{}());
  std::uniform_real_distribution<double> dist(0.0, 1.0);

  vtkPoints* A = vtkPoints::New();
  A->SetDataTypeToDouble();
  A->SetNumberOfPoints(num_points);
  for (point = 0; point < num_points; ++point)
  {
    pointA[0] = dist(rng);
    pointA[1] = dist(rng);
    pointA[2] = dist(rng);
    A->SetPoint(point, pointA);
  }

  vtkKdTree* kd = vtkKdTree::New();
  kd->BuildLocatorFromPoints(A);

  for (test_point = 0; test_point < num_test_points; ++test_point)
  {
    double min_dist2 = 10.0;
    pointB[0] = dist(rng);
    pointB[1] = dist(rng);
    pointB[2] = dist(rng);
    for (point = 0; point < num_points; ++point)
    {
      double dist2;
      double dx, dy, dz;
      A->GetPoint(point, pointA);
      dx = pointA[0] - pointB[0];
      dy = pointA[1] - pointB[1];
      dz = pointA[2] - pointB[2];
      dist2 = dx * dx + dy * dy + dz * dz;
      if (dist2 < min_dist2)
      {
        closest_id = point;
        min_dist2 = dist2;
      }
    }
    double ld2;
    idA = kd->FindClosestPoint(pointB, ld2);
    float diff = static_cast<float>(ld2) - static_cast<float>(min_dist2);
    if (ld2 == 0)
    {
      ld2 = 1; // avoid divide by zero error below
    }
    if ((idA != closest_id) && (diff / ld2 > .00001))
    {
      std::cerr << "KdTree found the closest point to be " << ld2
                << " away but a brute force method returned a closer distance of " << min_dist2
                << std::endl;
      rval++;
    }
  }

  kd->Delete();
  A->Delete();

  return rval;
}

// Whether results holds exactly the points of FindPointsInShell() in the shell
// (minDist2, maxDist2], with the same distances.
bool IsSameAsShell(vtkAbstractPointLocator* loc, const double x[3],
  const vtkDist2TupleArray& results, double minDist2, double maxDist2, vtkDoubleArray* petals)
{
  vtkDist2TupleArray shell;
  loc->FindPointsInShell(x, shell, minDist2, maxDist2, true, petals);
  auto byId = [](const vtkDist2Tuple& a, const vtkDist2Tuple& b) { return a.Id < b.Id; };
  vtkDist2TupleArray a(results);
  std::sort(a.begin(), a.end(), byId);
  std::sort(shell.begin(), shell.end(), byId);
  return a.size() == shell.size() &&
    std::equal(a.begin(), a.end(), shell.begin(),
      [](const vtkDist2Tuple& t1, const vtkDist2Tuple& t2)
      { return t1.Id == t2.Id && t1.Dist2 == t2.Dist2; });
}

// Brute-force validation of vtkKdTreePointLocator::FindNPointsInShell(): checks
// that the returned set is exactly the points with minDist2 < dist2 <= maxDist2,
// that the returned maxDist2 is the exact N-th-nearest valid distance (covering
// every point tied at that boundary), and that every returned point actually
// satisfies those bounds. The result must also be exactly the one of
// FindPointsInShell() with the returned maxDist2 (same ids, same distances),
// so that nested shells starting with it return every point once.
// vtkStaticPointLocator's FindNPointsInShell() must also return the points of
// its FindPointsInShell(), at least N of them.
int TestFindNPointsInShell()
{
  int rval = 0;
  vtkIdType numPoints = 2000;

  std::mt19937 rng(42); // fixed seed for reproducibility
  std::uniform_real_distribution<double> dist(0.0, 1.0);

  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(numPoints);
  for (vtkIdType i = 0; i < numPoints; i++)
  {
    double p[3] = { dist(rng), dist(rng), dist(rng) };
    points->SetPoint(i, p);
  }

  vtkNew<vtkPolyData> pd;
  pd->SetPoints(points);

  vtkNew<vtkKdTreePointLocator> locator;
  locator->SetDataSet(pd);
  locator->BuildLocator();

  // Cross-check against vtkStaticPointLocator's FindNPointsInShell(). Its
  // radius is an upper bound of the N-th nearest distance, so it may return
  // more points: this checks containment and bounds, not an exact match.
  vtkNew<vtkStaticPointLocator> staticLocator;
  staticLocator->SetDataSet(pd);
  staticLocator->BuildLocator();

  for (int trial = 0; trial < 30; trial++)
  {
    double x[3] = { dist(rng), dist(rng), dist(rng) };
    int N = 5 + (trial % 20);
    double minDist2 = (trial % 4) * 0.0005;

    vtkDist2TupleArray results;
    double maxDist2 = locator->FindNPointsInShell(N, x, results, minDist2, true);

    // Ground truth via brute force.
    std::vector<double> allDist2(numPoints);
    for (vtkIdType i = 0; i < numPoints; i++)
    {
      double p[3];
      points->GetPoint(i, p);
      double dx = p[0] - x[0], dy = p[1] - x[1], dz = p[2] - x[2];
      allDist2[i] = dx * dx + dy * dy + dz * dz;
    }
    std::vector<double> validDist2;
    for (double d : allDist2)
    {
      if (d > minDist2)
      {
        validDist2.push_back(d);
      }
    }
    std::sort(validDist2.begin(), validDist2.end());

    double expectedMax2 = 0.0;
    if (!validDist2.empty())
    {
      vtkIdType idx = std::min<vtkIdType>(N, static_cast<vtkIdType>(validDist2.size())) - 1;
      expectedMax2 = validDist2[idx];
    }

    std::set<vtkIdType> expectedIds;
    for (vtkIdType i = 0; i < numPoints; i++)
    {
      if (allDist2[i] > minDist2 && allDist2[i] <= expectedMax2 + 1e-12)
      {
        expectedIds.insert(i);
      }
    }

    std::set<vtkIdType> resultIds;
    for (const auto& t : results)
    {
      resultIds.insert(t.Id);
    }

    if (resultIds != expectedIds)
    {
      std::cerr << "FindNPointsInShell: result set mismatch for trial " << trial << " (N=" << N
                << ", minDist2=" << minDist2 << ", got " << resultIds.size() << " points, expected "
                << expectedIds.size() << ")\n";
      rval++;
    }
    if (std::abs(maxDist2 - expectedMax2) > 1e-9)
    {
      std::cerr << "FindNPointsInShell: maxDist2 mismatch for trial " << trial << " (got "
                << maxDist2 << ", expected " << expectedMax2 << ")\n";
      rval++;
    }
    for (const auto& t : results)
    {
      if (!(t.Dist2 > minDist2) || t.Dist2 > maxDist2 + 1e-9)
      {
        std::cerr << "FindNPointsInShell: point " << t.Id
                  << " out of [minDist2, maxDist2] bounds for trial " << trial << "\n";
        rval++;
      }
    }
    if (!IsSameAsShell(locator, x, results, minDist2, maxDist2, nullptr))
    {
      std::cerr << "FindNPointsInShell: differs from FindPointsInShell for trial " << trial << "\n";
      rval++;
    }

    // vtkStaticPointLocator cross-check: its result may overshoot ours, but
    // should never be a stricter (smaller/different) answer -- our exact
    // result set must be contained in its (possibly larger) one, and its
    // boundary distance must be at least as large as ours.
    vtkDist2TupleArray staticResults;
    double staticMaxDist2 = staticLocator->FindNPointsInShell(N, x, staticResults, minDist2, true);
    if (static_cast<int>(staticResults.size()) < N ||
      !IsSameAsShell(staticLocator, x, staticResults, minDist2, staticMaxDist2, nullptr))
    {
      std::cerr << "vtkStaticPointLocator::FindNPointsInShell: fewer than N points, or differs "
                   "from FindPointsInShell for trial "
                << trial << "\n";
      rval++;
    }

    std::set<vtkIdType> staticIds;
    for (const auto& t : staticResults)
    {
      staticIds.insert(t.Id);
    }

    for (vtkIdType kdId : resultIds)
    {
      if (staticIds.find(kdId) == staticIds.end())
      {
        std::cerr << "FindNPointsInShell: point " << kdId
                  << " returned by vtkKdTreePointLocator is missing from "
                     "vtkStaticPointLocator's (overshot) result for trial "
                  << trial << "\n";
        rval++;
      }
    }
    if (maxDist2 > staticMaxDist2 + 1e-9)
    {
      std::cerr << "FindNPointsInShell: vtkKdTreePointLocator's maxDist2 (" << maxDist2
                << ") exceeds vtkStaticPointLocator's (" << staticMaxDist2 << ") for trial "
                << trial << "\n";
      rval++;
    }
  }

  // Petals: brute-force validation of vtkKdTreePointLocator, and
  // vtkStaticPointLocator must return the points of its FindPointsInShell(),
  // at least N of them (or all the valid points if there are fewer).
  for (int trial = 0; trial < 30; trial++)
  {
    double x[3] = { dist(rng), dist(rng), dist(rng) };
    int N = 5 + (trial % 20);
    double minDist2 = (trial % 4) * 0.0005;

    // A couple of petal spheres somewhere near the query point, small enough
    // that they meaningfully restrict the candidate set.
    vtkNew<vtkDoubleArray> petals;
    petals->SetNumberOfComponents(4);
    petals->SetNumberOfTuples(2);
    petals->SetTuple4(0, x[0] + 0.1, x[1], x[2], 0.02);
    petals->SetTuple4(1, x[0] - 0.1, x[1] + 0.1, x[2], 0.02);

    vtkDist2TupleArray results;
    double maxDist2 = locator->FindNPointsInShell(N, x, results, minDist2, true, petals);

    auto inAnyPetal = [&](const double p[3])
    {
      for (vtkIdType s = 0; s < petals->GetNumberOfTuples(); s++)
      {
        double tuple[4];
        petals->GetTuple(s, tuple);
        double dx = p[0] - tuple[0], dy = p[1] - tuple[1], dz = p[2] - tuple[2];
        if (dx * dx + dy * dy + dz * dz <= tuple[3])
        {
          return true;
        }
      }
      return false;
    };

    // Ground truth via brute force, restricted to points inside a petal.
    std::vector<double> allDist2(numPoints);
    std::vector<bool> inPetal(numPoints);
    for (vtkIdType i = 0; i < numPoints; i++)
    {
      double p[3];
      points->GetPoint(i, p);
      double dx = p[0] - x[0], dy = p[1] - x[1], dz = p[2] - x[2];
      allDist2[i] = dx * dx + dy * dy + dz * dz;
      inPetal[i] = inAnyPetal(p);
    }
    std::vector<double> validDist2;
    for (vtkIdType i = 0; i < numPoints; i++)
    {
      if (allDist2[i] > minDist2 && inPetal[i])
      {
        validDist2.push_back(allDist2[i]);
      }
    }
    std::sort(validDist2.begin(), validDist2.end());

    double expectedMax2 = 0.0;
    if (!validDist2.empty())
    {
      vtkIdType idx = std::min<vtkIdType>(N, static_cast<vtkIdType>(validDist2.size())) - 1;
      expectedMax2 = validDist2[idx];
    }

    std::set<vtkIdType> expectedIds;
    for (vtkIdType i = 0; i < numPoints; i++)
    {
      if (allDist2[i] > minDist2 && inPetal[i] && allDist2[i] <= expectedMax2 + 1e-12)
      {
        expectedIds.insert(i);
      }
    }

    std::set<vtkIdType> resultIds;
    for (const auto& t : results)
    {
      resultIds.insert(t.Id);
    }

    if (resultIds != expectedIds)
    {
      std::cerr << "FindNPointsInShell (petals): result set mismatch for trial " << trial
                << " (N=" << N << ", minDist2=" << minDist2 << ", got " << resultIds.size()
                << " points, expected " << expectedIds.size() << ")\n";
      rval++;
    }
    if (std::abs(maxDist2 - expectedMax2) > 1e-9)
    {
      std::cerr << "FindNPointsInShell (petals): maxDist2 mismatch for trial " << trial << " (got "
                << maxDist2 << ", expected " << expectedMax2 << ")\n";
      rval++;
    }
    for (const auto& t : results)
    {
      double p[3];
      points->GetPoint(t.Id, p);
      if (!(t.Dist2 > minDist2) || t.Dist2 > maxDist2 + 1e-9 || !inAnyPetal(p))
      {
        std::cerr << "FindNPointsInShell (petals): point " << t.Id
                  << " out of bounds or outside all petals for trial " << trial << "\n";
        rval++;
      }
    }
    if (!IsSameAsShell(locator, x, results, minDist2, maxDist2, petals))
    {
      std::cerr << "FindNPointsInShell (petals): differs from FindPointsInShell for trial " << trial
                << "\n";
      rval++;
    }

    vtkDist2TupleArray staticResults;
    const double staticMaxDist2 =
      staticLocator->FindNPointsInShell(N, x, staticResults, minDist2, true, petals);
    const std::size_t minCount = std::min(static_cast<std::size_t>(N), validDist2.size());
    if (staticResults.size() < minCount ||
      !IsSameAsShell(staticLocator, x, staticResults, minDist2, staticMaxDist2, petals))
    {
      std::cerr << "vtkStaticPointLocator::FindNPointsInShell (petals): fewer than N points, or "
                   "differs from FindPointsInShell for trial "
                << trial << "\n";
      rval++;
    }
  }

  return rval;
}

// Brute-force validation of vtkStaticPointLocator2D::FindNPointsInShell() and
// FindPointsInShell(), on
// points in a plane: the distances are measured in the x-y plane. The locator
// may return more than the N nearest points, but every point that it returns
// is in the shell (minDist2, maxDist2], and every point of that shell is
// returned.
int TestStaticPointLocator2DShells()
{
  int rval = 0;
  constexpr vtkIdType numPoints = 2000;

  std::mt19937 rng(17);
  std::uniform_real_distribution<double> dist(0.0, 1.0);

  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(numPoints);
  for (vtkIdType i = 0; i < numPoints; i++)
  {
    points->SetPoint(i, dist(rng), dist(rng), 0.5);
  }
  vtkNew<vtkPolyData> pd;
  pd->SetPoints(points);

  vtkNew<vtkStaticPointLocator2D> locator;
  locator->SetDataSet(pd);
  locator->BuildLocator();

  for (int trial = 0; trial < 30; trial++)
  {
    double x[3] = { dist(rng), dist(rng), 0.5 };
    std::vector<double> allDist2(numPoints);
    for (vtkIdType i = 0; i < numPoints; i++)
    {
      double p[3];
      points->GetPoint(i, p);
      allDist2[i] = (p[0] - x[0]) * (p[0] - x[0]) + (p[1] - x[1]) * (p[1] - x[1]);
    }

    const int N = 5 + (trial % 20);
    const double minDist2 = (trial % 4) * 0.0005;
    vtkDist2TupleArray results;
    const double maxDist2 = locator->FindNPointsInShell(N, x, results, minDist2, true);

    std::set<vtkIdType> expectedIds;
    for (vtkIdType i = 0; i < numPoints; i++)
    {
      if (allDist2[i] > minDist2 && allDist2[i] <= maxDist2)
      {
        expectedIds.insert(i);
      }
    }
    std::set<vtkIdType> resultIds;
    for (const auto& t : results)
    {
      resultIds.insert(t.Id);
    }
    if (resultIds != expectedIds || static_cast<int>(resultIds.size()) < N ||
      !IsSameAsShell(locator, x, results, minDist2, maxDist2, nullptr))
    {
      std::cerr << "vtkStaticPointLocator2D::FindNPointsInShell: result set mismatch for trial "
                << trial << " (got " << resultIds.size() << " points, expected "
                << expectedIds.size() << ")\n";
      rval++;
    }

    // FindPointsInShell(): exactly the points of the annulus (minDist2,
    // maxDist2], sorted, and with and without a petal (a circle).
    vtkNew<vtkDoubleArray> petals;
    petals->SetNumberOfComponents(3);
    petals->InsertNextTuple3(0.5, 0.5, 0.09);

    // FindNPointsInShell() with the petal: the points of FindPointsInShell().
    const double petalMaxDist2 = locator->FindNPointsInShell(N, x, results, minDist2, true, petals);
    if (!IsSameAsShell(locator, x, results, minDist2, petalMaxDist2, petals))
    {
      std::cerr << "vtkStaticPointLocator2D::FindNPointsInShell (petals): differs from "
                   "FindPointsInShell for trial "
                << trial << "\n";
      rval++;
    }
    for (int usePetals = 0; usePetals < 2; ++usePetals)
    {
      const double lo = 0.01, hi = 0.04;
      locator->FindPointsInShell(
        x, results, lo, hi, true, usePetals ? petals.GetPointer() : nullptr);
      std::set<vtkIdType> expectedShell;
      for (vtkIdType i = 0; i < numPoints; i++)
      {
        double p[3];
        points->GetPoint(i, p);
        const double petalD2 = (p[0] - 0.5) * (p[0] - 0.5) + (p[1] - 0.5) * (p[1] - 0.5);
        if (allDist2[i] > lo && allDist2[i] <= hi && (!usePetals || petalD2 <= 0.09))
        {
          expectedShell.insert(i);
        }
      }
      std::set<vtkIdType> shellIds;
      double previous = -1.0;
      for (const auto& t : results)
      {
        shellIds.insert(t.Id);
        if (t.Dist2 != allDist2[t.Id] || t.Dist2 < previous)
        {
          std::cerr << "vtkStaticPointLocator2D::FindPointsInShell: wrong or unsorted distance "
                    << "for trial " << trial << "\n";
          rval++;
        }
        previous = t.Dist2;
      }
      if (shellIds != expectedShell || results.size() != shellIds.size())
      {
        std::cerr << "vtkStaticPointLocator2D::FindPointsInShell: result set mismatch for trial "
                  << trial << " (got " << results.size() << " points, expected "
                  << expectedShell.size() << ")\n";
        rval++;
      }
    }
  }

  return rval;
}

// The points at the radius are within it, for every locator: those of
// FindPointsWithinRadius(), of FindPointsInShell() and of
// FindClosestPointWithinRadius(). The squared distances are exact.
int TestRadiusBoundary()
{
  int rval = 0;

  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 0, 0);
  points->InsertNextPoint(0, 2, 0);
  points->InsertNextPoint(0, 0, 3);
  points->InsertNextPoint(0, -2, 0);
  vtkNew<vtkPolyData> pd;
  pd->SetPoints(points);

  const double origin[3] = { 0, 0, 0 };
  const std::set<vtkIdType> withinTwo = { 0, 1, 2, 4 };
  const double above[3] = { 0, 0, 6 }; // 3 away from point 3

  vtkNew<vtkPointLocator> uniformLocator;
  vtkNew<vtkStaticPointLocator> staticLocator;
  vtkNew<vtkOctreePointLocator> octreeLocator;
  vtkNew<vtkKdTreePointLocator> kdTreeLocator;
  for (vtkAbstractPointLocator* locator : std::vector<vtkAbstractPointLocator*>{
         uniformLocator, staticLocator, octreeLocator, kdTreeLocator })
  {
    locator->SetDataSet(pd);
    locator->BuildLocator();
    const char* name = locator->GetClassName();

    vtkNew<vtkIdList> ids;
    locator->FindPointsWithinRadius(2.0, origin, ids);
    const std::set<vtkIdType> radiusIds(ids->begin(), ids->end());
    if (radiusIds != withinTwo)
    {
      std::cerr << name << "::FindPointsWithinRadius: misses the points at the radius\n";
      rval++;
    }

    vtkDist2TupleArray results;
    locator->FindPointsInShell(origin, results, -0.1, 4.0);
    std::set<vtkIdType> shellIds;
    for (const auto& t : results)
    {
      shellIds.insert(t.Id);
    }
    if (shellIds != withinTwo)
    {
      std::cerr << name << "::FindPointsInShell: misses the points at the radius\n";
      rval++;
    }

    double dist2 = -1.0;
    if (locator->FindClosestPointWithinRadius(3.0, above, dist2) != 3 || dist2 != 9.0)
    {
      std::cerr << name << "::FindClosestPointWithinRadius: misses the point at the radius\n";
      rval++;
    }
    if (locator->FindClosestPointWithinRadius(std::nextafter(3.0, 0.0), above, dist2) != -1)
    {
      std::cerr << name << "::FindClosestPointWithinRadius: finds a point beyond the radius\n";
      rval++;
    }
  }

  return rval;
}

// FindPointsInShell() returns exactly the points of the shell, compared with
// brute force, for every locator: also those at the outer radius, when they are
// on the boundaries of the buckets of the static locators (points of an integer
// grid, with buckets of size 1), or when the locator keeps the points in float
// (vtkOctreePointLocator, with random points). The points of a plane are also
// checked with vtkStaticPointLocator2D.
int TestShellBoundaries()
{
  int rval = 0;
  std::mt19937 rng(7);
  std::uniform_real_distribution<double> dist(0.0, 8.0);

  for (int planar = 0; planar < 2; ++planar)
  {
    vtkNew<vtkPoints> points;
    points->SetDataTypeToDouble();
    for (int k = 0; k <= (planar ? 0 : 8); ++k)
    {
      for (int j = 0; j <= 8; ++j)
      {
        for (int i = 0; i <= 8; ++i)
        {
          points->InsertNextPoint(i, j, planar ? 0.5 : k);
        }
      }
    }
    for (int i = 0; i < 500; ++i)
    {
      points->InsertNextPoint(dist(rng), dist(rng), planar ? 0.5 : dist(rng));
    }
    const vtkIdType numPoints = points->GetNumberOfPoints();
    vtkNew<vtkPolyData> pd;
    pd->SetPoints(points);

    vtkNew<vtkStaticPointLocator> staticLocator;
    staticLocator->AutomaticOff();
    staticLocator->SetDivisions(8, 8, planar ? 1 : 8);
    vtkNew<vtkStaticPointLocator2D> staticLocator2D;
    staticLocator2D->AutomaticOff();
    staticLocator2D->SetDivisions(8, 8);
    vtkNew<vtkKdTreePointLocator> kdTreeLocator;
    vtkNew<vtkPointLocator> uniformLocator;
    vtkNew<vtkOctreePointLocator> octreeLocator;
    std::vector<vtkAbstractPointLocator*> locators = { staticLocator, kdTreeLocator, uniformLocator,
      octreeLocator };
    if (planar)
    {
      locators.push_back(staticLocator2D);
    }
    for (vtkAbstractPointLocator* locator : locators)
    {
      locator->SetDataSet(pd);
      locator->BuildLocator();
    }

    std::vector<double> allDist2(numPoints);
    for (int trial = 0; trial < 100; ++trial)
    {
      // From a point of the grid, or from a random point, with radii that are
      // distances of points, or integers for the outer one.
      double x[3];
      points->GetPoint(trial % 2 ? trial : numPoints - 1 - trial, x);
      for (vtkIdType i = 0; i < numPoints; ++i)
      {
        double p[3];
        points->GetPoint(i, p);
        allDist2[i] = vtkMath::Distance2BetweenPoints(p, x);
      }
      const double minDist2 = trial % 3 == 0 ? -1.0 : allDist2[(7 * trial) % numPoints];
      const double maxDist2 = trial % 4 < 2 ? 1.0 + trial % 9 : allDist2[(13 * trial) % numPoints];
      if (maxDist2 <= minDist2)
      {
        continue;
      }
      std::set<vtkIdType> expectedIds;
      for (vtkIdType i = 0; i < numPoints; ++i)
      {
        if (allDist2[i] > minDist2 && allDist2[i] <= maxDist2)
        {
          expectedIds.insert(i);
        }
      }

      for (vtkAbstractPointLocator* locator : locators)
      {
        vtkDist2TupleArray results;
        locator->FindPointsInShell(x, results, minDist2, maxDist2, false);
        std::set<vtkIdType> resultIds;
        for (const auto& t : results)
        {
          resultIds.insert(t.Id);
        }
        if (resultIds != expectedIds || results.size() != resultIds.size())
        {
          std::cerr << locator->GetClassName() << "::FindPointsInShell: got " << results.size()
                    << " points, expected " << expectedIds.size() << " for trial " << trial
                    << (planar ? " (planar)\n" : "\n");
          rval++;
        }
      }
    }
  }

  return rval;
}

int TestPointLocators(int, char*[])
{
  vtkKdTreePointLocator* kdTreeLocator = vtkKdTreePointLocator::New();
  vtkPointLocator* uniformLocator = vtkPointLocator::New();

  std::cout << "Comparing vtkPointLocator to vtkKdTreePointLocator.\n";
  int rval = ComparePointLocators(uniformLocator, kdTreeLocator);

  vtkOctreePointLocator* octreeLocator = vtkOctreePointLocator::New();

  std::cout << "Comparing vtkOctreePointLocator to vtkKdTreePointLocator.\n";
  rval += ComparePointLocators(octreeLocator, kdTreeLocator);

  kdTreeLocator->Delete();
  uniformLocator->Delete();
  octreeLocator->Delete();

  rval += TestKdTreePointLocator();
  rval += TestFindNPointsInShell();
  rval += TestStaticPointLocator2DShells();
  rval += TestRadiusBoundary();
  rval += TestShellBoundaries();

  return rval;
}
