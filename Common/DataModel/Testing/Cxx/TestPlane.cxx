// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"

#include <cmath>
#include <limits>

template <class A>
bool fuzzyCompare1D(A a, A b)
{
  return std::abs(a - b) < std::numeric_limits<A>::epsilon();
}

template <class A>
bool fuzzyCompare3D(A a[3], A b[3])
{
  return fuzzyCompare1D(a[0], b[0]) && fuzzyCompare1D(a[1], b[1]) && fuzzyCompare1D(a[2], b[2]);
}

int TestPlane(int, char*[])
{
  // Test ProjectVector (vector is out of plane)
  {
    vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
    plane->SetOrigin(0.0, 0.0, 0.0);
    plane->SetNormal(0, 0, 1);

    std::cout << "Testing ProjectVector" << std::endl;
    double v[3] = { 1, 2, 3 };
    double projection[3];
    double correct[3] = { 1., 2., 0 };
    plane->ProjectVector(v, projection);
    if (!fuzzyCompare3D(projection, correct))
    {
      std::cerr << "ProjectVector failed! Should be (1., 2., 0) but it is (" << projection[0] << " "
                << projection[1] << " " << projection[2] << ")" << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Test ProjectVector where vector is already in plane
  {
    vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
    plane->SetOrigin(0.0, 0.0, 0.0);
    plane->SetNormal(0, 0, 1);

    std::cout << "Testing ProjectVector" << std::endl;
    double v[3] = { 1, 2, 0 };
    double projection[3];
    double correct[3] = { 1., 2., 0 };
    plane->ProjectVector(v, projection);
    if (!fuzzyCompare3D(projection, correct))
    {
      std::cerr << "ProjectVector failed! Should be (1., 2., 0) but it is (" << projection[0] << " "
                << projection[1] << " " << projection[2] << ")" << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Test ProjectVector where vector is orthogonal to plane
  {
    vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
    plane->SetOrigin(0.0, 0.0, 0.0);
    plane->SetNormal(0, 0, 1);

    std::cout << "Testing ProjectVector" << std::endl;
    double v[3] = { 0, 0, 1 };
    double projection[3];
    double correct[3] = { 0., 0., 0. };
    plane->ProjectVector(v, projection);
    if (!fuzzyCompare3D(projection, correct))
    {
      std::cerr << "ProjectVector failed! Should be (0., 0., 0) but it is (" << projection[0] << " "
                << projection[1] << " " << projection[2] << ")" << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Test AxisAligned
  {
    vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
    plane->SetOrigin(0.0, 0.0, 0.0);
    plane->SetNormal(0.5, 0.8, 0.2);
    plane->SetAxisAligned(false);

    std::cout << "Testing AxisAligned" << std::endl;
    double x[3] = { 1.0, 1.0, 1.0 };
    double res = plane->EvaluateFunction(x);
    if (!fuzzyCompare1D(res, 1.5))
    {
      std::cerr << "AxisAligned failed! Should be 1.5 but is " << res << std::endl;
      return EXIT_FAILURE;
    }
    plane->SetAxisAligned(true);
    res = plane->EvaluateFunction(x);
    if (!fuzzyCompare1D(res, 1.0))
    {
      std::cerr << "AxisAligned failed! Should be 1.0 but is " << res << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Test Offset
  {
    vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
    plane->SetOrigin(0.0, 0.0, 0.0);
    plane->SetNormal(0.5, 0.5, 0.5);
    plane->SetOffset(0);

    std::cout << "Testing Offset" << std::endl;
    double x[3] = { 1.0, 1.0, 1.0 };
    double res = plane->EvaluateFunction(x);
    if (!fuzzyCompare1D(res, 1.5))
    {
      std::cerr << "Offset failed! Should be 1.5 but is " << res << std::endl;
      return EXIT_FAILURE;
    }
    plane->SetOffset(0.5);
    res = plane->EvaluateFunction(x);
    if (!fuzzyCompare1D(res, 1.125))
    {
      std::cerr << "Offset failed! Should be 1.125 but is " << res << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Test Push
  {
    vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
    plane->SetOrigin(5.0, 5.0, 5.0);
    plane->SetNormal(1.0, 0.0, 0.0);
    plane->SetAxisAligned(true);

    double x[3] = { 5.0, 5.0, 5.0 };
    double res = plane->EvaluateFunction(x);

    if (!fuzzyCompare1D(res, 0.0))
    {
      std::cerr << "Push failed! Should be 0.0 but is " << res << std::endl;
      return EXIT_FAILURE;
    }

    plane->Push(1.0);
    res = plane->EvaluateFunction(x);

    if (!fuzzyCompare1D(res, -1.0))
    {
      std::cerr << "Push failed! Should be -1.0 but is " << res << std::endl;
      return EXIT_FAILURE;
    }
  }

  {
    vtkNew<vtkPlane> plane;
    plane->SetOrigin(0.0, 0.0, 0.0);
    plane->SetNormal(0.0, 0.0, 1.0);

    vtkIdType nPointsPerDimension = 11;
    vtkIdType nPoints = std::pow(nPointsPerDimension, 3);
    vtkNew<vtkPoints> points;
    points->SetNumberOfPoints(nPoints);

    // Generate a grid of points
    float in[3];
    float minX = -1.0f, minY = -1.0f, minZ = -1.0f;
    float increment = 2.0f / (static_cast<float>(nPointsPerDimension) - 1.0f);
    vtkIdType pos = 0;
    for (int z = 0; z < nPointsPerDimension; ++z)
    {
      in[2] = minZ + static_cast<float>(z) * increment;
      for (int y = 0; y < nPointsPerDimension; ++y)
      {
        in[1] = minY + static_cast<float>(y) * increment;
        for (int x = 0; x < nPointsPerDimension; ++x)
        {
          in[0] = minX + static_cast<float>(x) * increment;
          points->SetPoint(pos++, in);
        }
      }
    }
    assert(pos == nPoints);

    vtkFloatArray* input = vtkArrayDownCast<vtkFloatArray>(points->GetData());
    vtkNew<vtkFloatArray> arrayOutput;
    arrayOutput->SetNumberOfComponents(1);
    arrayOutput->SetNumberOfTuples(nPoints);

    std::cout << "Testing FunctionValue:\n";
    // calculate function values with the vtkDataArray interface
    plane->FunctionValue(input, arrayOutput);

    // Calculate the same points using a loop over points.
    vtkNew<vtkFloatArray> loopOutput;
    loopOutput->SetNumberOfComponents(1);
    loopOutput->SetNumberOfTuples(nPoints);

    for (vtkIdType pt = 0; pt < nPoints; ++pt)
    {
      double x[3];
      x[0] = input->GetTypedComponent(pt, 0);
      x[1] = input->GetTypedComponent(pt, 1);
      x[2] = input->GetTypedComponent(pt, 2);
      loopOutput->SetTypedComponent(pt, 0, plane->FunctionValue(x));
    }

    for (vtkIdType i = 0; i < nPoints; ++i)
    {
      if (!vtkMathUtilities::FuzzyCompare(
            arrayOutput->GetTypedComponent(i, 0), loopOutput->GetTypedComponent(i, 0)))
      {
        std::cerr << "Array and point interfaces returning different results at index " << i << ": "
                  << arrayOutput->GetTypedComponent(i, 0) << " vs "
                  << loopOutput->GetTypedComponent(i, 0) << '\n';
        return EXIT_FAILURE;
      }
    }
  }
  return EXIT_SUCCESS;
}
