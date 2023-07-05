// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkResliceCursorRepresentation.h"

#include "vtkMathUtilities.h"

#include <array>

bool areEquals(double p1[3], double p2[3])
{
  return vtkMathUtilities::FuzzyCompare<double>(p1[0], p2[0]) &&
    vtkMathUtilities::FuzzyCompare(p1[1], p2[1]) && vtkMathUtilities::FuzzyCompare(p1[2], p2[2]);
}

bool areEquals(double p1[3], double p2[3], double epsilon)
{
  return vtkMathUtilities::FuzzyCompare<double>(p1[0], p2[0], epsilon) &&
    vtkMathUtilities::FuzzyCompare(p1[1], p2[1], epsilon) &&
    vtkMathUtilities::FuzzyCompare(p1[2], p2[2], epsilon);
}

int vtkResliceCursorRepresentationTest1(int, char*[])
{
  // BoundPlane natural basis simple
  std::array<double, 3> origin = { -1, -1, 0 };
  std::array<double, 3> point1 = { 2, -1, 0 };
  std::array<double, 3> point2 = { -1, 2, 0 };
  std::array<double, 6> bounds = { 0, 1, 0, 1, -1, 1 };

  int intersect = vtkResliceCursorRepresentation::BoundPlane(
    bounds.data(), origin.data(), point1.data(), point2.data());

  std::array<double, 3> originExpected = { 0, 0, 0 };
  std::array<double, 3> point1Expected = { 1, 0, 0 };
  std::array<double, 3> point2Expected = { 0, 1, 0 };

  if (intersect != 1 || !areEquals(origin.data(), originExpected.data()) ||
    !areEquals(point1.data(), point1Expected.data()) ||
    !areEquals(point2.data(), point2Expected.data()))
  {
    std::cerr << "Error during boundPlane natural basis simple" << std::endl;
    return EXIT_FAILURE;
  }

  // BoundPlane natural basis with offset
  origin = std::array<double, 3>({ -1, -1, -1.5 });
  point1 = std::array<double, 3>({ 2, -1, -1.5 });
  point2 = std::array<double, 3>({ -1, 2, -1.5 });
  bounds = std::array<double, 6>({ 0, 1, 1, 2, -2, -1 });

  intersect = vtkResliceCursorRepresentation::BoundPlane(
    bounds.data(), origin.data(), point1.data(), point2.data());

  originExpected = std::array<double, 3>({ 0, 1, -1.5 });
  point1Expected = std::array<double, 3>({ 1, 1, -1.5 });
  point2Expected = std::array<double, 3>({ 0, 2, -1.5 });

  if (intersect != 1 || !areEquals(origin.data(), originExpected.data()) ||
    !areEquals(point1.data(), point1Expected.data()) ||
    !areEquals(point2.data(), point2Expected.data()))
  {
    std::cerr << "Error during boundPlane natural basis with offset" << std::endl;
    return EXIT_FAILURE;
  }

  // BoundPlane oriented
  origin = std::array<double, 3>({ 0, 0, 0 });
  point1 = std::array<double, 3>({ 1, 1, 0 });
  point2 = std::array<double, 3>({ 0, 0, 1 });
  bounds = std::array<double, 6>({ 0, 1, 0, 1, 0, 1 });

  intersect = vtkResliceCursorRepresentation::BoundPlane(
    bounds.data(), origin.data(), point1.data(), point2.data());

  originExpected = std::array<double, 3>({ 0, 0, 0 });
  point1Expected = std::array<double, 3>({ 1, 1, 0 });
  point2Expected = std::array<double, 3>({ 0, 0, 1 });

  double epsilon = 0.000000001;
  if (intersect != 1 || !areEquals(origin.data(), originExpected.data(), epsilon) ||
    !areEquals(point1.data(), point1Expected.data(), epsilon) ||
    !areEquals(point2.data(), point2Expected.data(), epsilon))
  {
    std::cerr << "Error during boundPlane oriented" << std::endl;
    return EXIT_FAILURE;
  }

  // BoundPlane no intersection
  origin = std::array<double, 3>({ 0, 0, 0 });
  point1 = std::array<double, 3>({ 2, 0, 0 });
  point2 = std::array<double, 3>({ 0, 2, 0 });
  bounds = std::array<double, 6>({ 0, 1, 0, 1, 1, 2 });

  intersect = vtkResliceCursorRepresentation::BoundPlane(
    bounds.data(), origin.data(), point1.data(), point2.data());

  originExpected = std::array<double, 3>({ 0, 0, 0 });
  point1Expected = std::array<double, 3>({ 2, 0, 0 });
  point2Expected = std::array<double, 3>({ 0, 2, 0 });

  if (intersect == 1 || !areEquals(origin.data(), originExpected.data()) ||
    !areEquals(point1.data(), point1Expected.data()) ||
    !areEquals(point2.data(), point2Expected.data()))
  {
    std::cerr << "Error during boundPlane oriented" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
