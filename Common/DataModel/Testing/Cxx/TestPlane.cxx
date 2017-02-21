/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPlane.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDataArrayAccessor.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"

#include <limits>

#ifndef ABS
#define ABS(x) ((x) < 0 ? -(x) : (x))
#endif

template<class A>
bool fuzzyCompare1D(A a, A b)
{
  return ABS(a - b) < std::numeric_limits<A>::epsilon();
}

template<class A>
bool fuzzyCompare3D(A a[3], A b[3])
{
  return fuzzyCompare1D(a[0], b[0]) &&
         fuzzyCompare1D(a[1], b[1]) &&
         fuzzyCompare1D(a[2], b[2]);
}

int TestPlane(int,char *[])
{
  // Test ProjectVector (vector is out of plane)
  {
  vtkSmartPointer<vtkPlane> plane =
    vtkSmartPointer<vtkPlane>::New();
  plane->SetOrigin(0.0, 0.0, 0.0);
  plane->SetNormal(0,0,1);

  std::cout << "Testing ProjectVector" << std::endl;
  double v[3] = {1,2,3};
  double projection[3];
  double correct[3] = {1., 2., 0};
  plane->ProjectVector(v, projection);
  if(!fuzzyCompare3D(projection,correct))
  {
    std::cerr << "ProjectVector failed! Should be (1., 2., 0) but it is ("
                  << projection[0] << " " << projection[1] << " " << projection[2] << ")" << std::endl;
    return EXIT_FAILURE;
  }
  }

  // Test ProjectVector where vector is already in plane
  {
  vtkSmartPointer<vtkPlane> plane =
    vtkSmartPointer<vtkPlane>::New();
  plane->SetOrigin(0.0, 0.0, 0.0);
  plane->SetNormal(0,0,1);

  std::cout << "Testing ProjectVector" << std::endl;
  double v[3] = {1,2,0};
  double projection[3];
  double correct[3] = {1., 2., 0};
  plane->ProjectVector(v, projection);
  if(!fuzzyCompare3D(projection,correct))
  {
    std::cerr << "ProjectVector failed! Should be (1., 2., 0) but it is ("
                  << projection[0] << " " << projection[1] << " " << projection[2] << ")" << std::endl;
    return EXIT_FAILURE;
  }
  }

  // Test ProjectVector where vector is orthogonal to plane
  {
  vtkSmartPointer<vtkPlane> plane =
    vtkSmartPointer<vtkPlane>::New();
  plane->SetOrigin(0.0, 0.0, 0.0);
  plane->SetNormal(0,0,1);

  std::cout << "Testing ProjectVector" << std::endl;
  double v[3] = {0,0,1};
  double projection[3];
  double correct[3] = {0., 0., 0.};
  plane->ProjectVector(v, projection);
  if(!fuzzyCompare3D(projection,correct))
  {
    std::cerr << "ProjectVector failed! Should be (0., 0., 0) but it is ("
                  << projection[0] << " " << projection[1] << " " << projection[2] << ")" << std::endl;
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

    //Generate a grid of points
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

    vtkDataArray* input = points->GetData();
    vtkNew<vtkFloatArray> arrayOutput;
    arrayOutput->SetNumberOfComponents(1);
    arrayOutput->SetNumberOfTuples(nPoints);

    std::cout << "Testing FunctionValue:\n";
    // calcuate function values with the vtkDataArray interface
    plane->FunctionValue(input, arrayOutput.GetPointer());

    //Calculate the same points using a loop over points.
    vtkNew<vtkFloatArray> loopOutput;
    loopOutput->SetNumberOfComponents(1);
    loopOutput->SetNumberOfTuples(nPoints);
    vtkDataArrayAccessor<vtkFloatArray> output(loopOutput.GetPointer());
    vtkDataArrayAccessor<vtkFloatArray> pts(vtkFloatArray::SafeDownCast(input));

    for(vtkIdType pt = 0; pt < nPoints; ++pt)
    {
      double x[3];
      x[0] = pts.Get(pt,0);
      x[1] = pts.Get(pt,1);
      x[2] = pts.Get(pt,2);
      output.Set(pt, 0, plane->FunctionValue(x));
    }

    for (vtkIdType i = 0; i < nPoints; ++i)
    {
      if(!vtkMathUtilities::FuzzyCompare(arrayOutput->GetTypedComponent(i, 0), loopOutput->GetTypedComponent(i, 0)))
      {
        std::cerr << "Array and point interfaces returning different results at index " << i << ": "
                  << arrayOutput->GetTypedComponent(i, 0) << " vs " << loopOutput->GetTypedComponent(i, 0) << '\n';
        return EXIT_FAILURE;
      }
    }
  }
  return EXIT_SUCCESS;
}
