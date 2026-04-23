// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//
// Validates that vtkHilbertCurveSorter produces a valid permutation with
// improved spatial locality compared to random ordering.

#include "vtkHilbertCurveSorter.h"
#include "vtkIdList.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkNew.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"

#include <cmath>
#include <iostream>
#include <vector>

namespace
{

//------------------------------------------------------------------------------
// Compute the average distance between consecutive points in the given order.
double AverageConsecutiveDistance(vtkPoints* points, vtkIdList* order)
{
  vtkIdType n = order->GetNumberOfIds();
  if (n <= 1)
  {
    return 0.0;
  }
  double totalDist = 0.0;
  for (vtkIdType i = 0; i < n - 1; i++)
  {
    double p1[3], p2[3];
    points->GetPoint(order->GetId(i), p1);
    points->GetPoint(order->GetId(i + 1), p2);
    double dx = p1[0] - p2[0];
    double dy = p1[1] - p2[1];
    double dz = p1[2] - p2[2];
    totalDist += sqrt(dx * dx + dy * dy + dz * dz);
  }
  return totalDist / (n - 1);
}

//------------------------------------------------------------------------------
bool TestPermutationValidity(vtkIdList* order, vtkIdType numPoints, const char* label)
{
  if (order->GetNumberOfIds() != numPoints)
  {
    std::cerr << label << ": expected " << numPoints << " ids, got " << order->GetNumberOfIds()
              << std::endl;
    return false;
  }

  // Check that every index [0, numPoints) appears exactly once.
  std::vector<char> seen(numPoints, 0);
  for (vtkIdType i = 0; i < numPoints; i++)
  {
    vtkIdType id = order->GetId(i);
    if (id < 0 || id >= numPoints)
    {
      std::cerr << label << ": id " << id << " out of range" << std::endl;
      return false;
    }
    if (seen[id])
    {
      std::cerr << label << ": duplicate id " << id << std::endl;
      return false;
    }
    seen[id] = 1;
  }
  return true;
}

//------------------------------------------------------------------------------
bool TestCase(vtkIdType numPoints, int dims, int seed, const char* label)
{
  std::cout << "[" << label << "] " << numPoints << " pts, " << dims << "D ... ";

  vtkNew<vtkMinimalStandardRandomSequence> rng;
  rng->SetSeed(seed);

  vtkNew<vtkPoints> points;
  points->SetDataType(VTK_DOUBLE);
  points->SetNumberOfPoints(numPoints);
  for (vtkIdType i = 0; i < numPoints; i++)
  {
    double p[3];
    rng->Next();
    p[0] = rng->GetValue();
    rng->Next();
    p[1] = rng->GetValue();
    if (dims == 3)
    {
      rng->Next();
      p[2] = rng->GetValue();
    }
    else
    {
      p[2] = 0.0;
    }
    points->SetPoint(i, p);
  }
  vtkNew<vtkPointSet> pointsSet;
  pointsSet->SetPoints(points);

  // Compute Hilbert order.
  vtkNew<vtkHilbertCurveSorter> sorter;
  sorter->SetInputData(pointsSet);
  sorter->ComputePermutationOnlyOn();
  sorter->Update();
  auto order = sorter->GetPermutation();

  // 1. Check permutation validity.
  if (!::TestPermutationValidity(order, numPoints, label))
  {
    return false;
  }

  // 2. Check spatial locality improvement.
  // Build sequential order for comparison.
  vtkNew<vtkIdList> sequential;
  sequential->SetNumberOfIds(numPoints);
  for (vtkIdType i = 0; i < numPoints; i++)
  {
    sequential->SetId(i, i);
  }

  double seqDist = ::AverageConsecutiveDistance(points, sequential);
  double hilDist = ::AverageConsecutiveDistance(points, order);

  // The Hilbert order should have significantly smaller average
  // consecutive distance than random (sequential on random points).
  if (hilDist >= seqDist && numPoints > 10)
  {
    std::cerr << "FAIL: Hilbert dist (" << hilDist << ") >= sequential dist (" << seqDist << ")"
              << std::endl;
    return false;
  }

  std::cout << "PASS (locality: " << hilDist / seqDist << "x)" << std::endl;
  return true;
}

} // anonymous namespace

int TestHilbertCurveSorter(int, char*[])
{
  bool allPassed = true;

  // Small cases
  allPassed &= ::TestCase(10, 2, 42, "2D-10");
  allPassed &= ::TestCase(10, 3, 42, "3D-10");

  // Medium cases
  allPassed &= ::TestCase(1000, 2, 123, "2D-1k");
  allPassed &= ::TestCase(1000, 3, 123, "3D-1k");

  // Larger cases
  allPassed &= ::TestCase(10000, 2, 12345, "2D-10k");
  allPassed &= ::TestCase(10000, 3, 12345, "3D-10k");

  return allPassed ? EXIT_SUCCESS : EXIT_FAILURE;
}
