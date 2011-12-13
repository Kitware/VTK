/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLineSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkConvexHull2D.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkMath.h"
#include <limits>

#ifndef ABS
#define ABS(x) ((x) < 0 ? -(x) : (x))
#endif

template<class A>
bool fuzzyCompare1Dweak(A a, A b)
{
  return ABS(a - b) < 0.000001;
}

template<class A>
bool fuzzyCompare2Dweak(A a[2], A b[2])
{
  return fuzzyCompare1Dweak(a[0], b[0]) &&
         fuzzyCompare1Dweak(a[1], b[1]);
}

int TestConvexHull2D(int argc, char* argv[])
{
  (void)argc;
  (void)argv;
  double expectedPoint[3];
  double retrievedPoint[3];

  //---------------------------------------------------------------------------
  // A single point - expected output is a 2x2 square centred on the origin
  vtkSmartPointer<vtkPoints> inPoints = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkPoints> outPoints = vtkSmartPointer<vtkPoints>::New();
  inPoints->SetNumberOfPoints(1);
  inPoints->SetPoint(0, 0.0, 0.0, 0.0);
  vtkConvexHull2D::CalculateConvexHull(inPoints, outPoints, 2.0);
  if (outPoints->GetNumberOfPoints() != 4)
    {
    std::cerr << "Error: Single point - expected 4 output points but got " <<
      outPoints->GetNumberOfPoints() << "." << std::endl;
    return EXIT_FAILURE;
    }

  outPoints->GetPoint(0, retrievedPoint);
  expectedPoint[0] = -1.0; expectedPoint[1] = -1.0; expectedPoint[2] = 0.0;
  if (!fuzzyCompare2Dweak(expectedPoint, retrievedPoint))
    {
    std::cerr << "Error: Single point - unexpected output value for point 0." << std::endl;
    return EXIT_FAILURE;
    }
  outPoints->GetPoint(1, retrievedPoint);
  expectedPoint[0] = 1.0; expectedPoint[1] = -1.0; expectedPoint[2] = 0.0;
  if (!fuzzyCompare2Dweak(expectedPoint, retrievedPoint))
    {
    std::cerr << "Error: Single point - unexpected output value for point 1." << std::endl;
    return EXIT_FAILURE;
    }
  outPoints->GetPoint(2, retrievedPoint);
  expectedPoint[0] = 1.0; expectedPoint[1] = 1.0; expectedPoint[2] = 0.0;
  if (!fuzzyCompare2Dweak(expectedPoint, retrievedPoint))
    {
    std::cerr << "Error: Single point - unexpected output value for point 2." << std::endl;
    return EXIT_FAILURE;
    }
  outPoints->GetPoint(3, retrievedPoint);
  expectedPoint[0] = -1.0; expectedPoint[1] = 1.0; expectedPoint[2] = 0.0;
  if (!fuzzyCompare2Dweak(expectedPoint, retrievedPoint))
    {
    std::cerr << "Error: Single point - unexpected output value for point 3." << std::endl;
    return EXIT_FAILURE;
    }

  //---------------------------------------------------------------------------
  // Two points in a line - expected output is a 4x2 rectangle centred on the origin
  inPoints = vtkSmartPointer<vtkPoints>::New();
  outPoints = vtkSmartPointer<vtkPoints>::New();

  inPoints->SetNumberOfPoints(2);
  inPoints->SetPoint(0, -2.0, 0.0, 0.0);
  inPoints->SetPoint(1, 2.0, 0.0, 0.0);
  vtkConvexHull2D::CalculateConvexHull(inPoints, outPoints, 2.0);
  if (outPoints->GetNumberOfPoints() != 4)
    {
    std::cerr << "Error: Two points in a line - expected 4 output points but got " <<
      outPoints->GetNumberOfPoints() << "." << std::endl;
    return EXIT_FAILURE;
    }

  outPoints->GetPoint(0, retrievedPoint);
  expectedPoint[0] = -2.0; expectedPoint[1] = -1.0; expectedPoint[2] = 0.0;
  if (!fuzzyCompare2Dweak(expectedPoint, retrievedPoint))
    {
    std::cerr << "Error: Two points in a line - unexpected output value for point 0." << std::endl;
    return EXIT_FAILURE;
    }
  outPoints->GetPoint(1, retrievedPoint);
  expectedPoint[0] = 2.0; expectedPoint[1] = -1.0; expectedPoint[2] = 0.0;
  if (!fuzzyCompare2Dweak(expectedPoint, retrievedPoint))
    {
    std::cerr << "Error: Two points in a line - unexpected output value for point 1." << std::endl;
    return EXIT_FAILURE;
    }
  outPoints->GetPoint(2, retrievedPoint);
  expectedPoint[0] = 2.0; expectedPoint[1] = 1.0; expectedPoint[2] = 0.0;
  if (!fuzzyCompare2Dweak(expectedPoint, retrievedPoint))
    {
    std::cerr << "Error: Two points in a line - unexpected output value for point 2." << std::endl;
    return EXIT_FAILURE;
    }
  outPoints->GetPoint(3, retrievedPoint);
  expectedPoint[0] = -2.0; expectedPoint[1] = 1.0; expectedPoint[2] = 0.0;
  if (!fuzzyCompare2Dweak(expectedPoint, retrievedPoint))
    {
    std::cerr << "Error: Two points in a line - unexpected output value for point 3." << std::endl;
    return EXIT_FAILURE;
    }

  //---------------------------------------------------------------------------
  // Five points - expected output is a 2x2 rotated rectangle centred on the origin
  inPoints = vtkSmartPointer<vtkPoints>::New();
  outPoints = vtkSmartPointer<vtkPoints>::New();

  inPoints->SetNumberOfPoints(5);
  inPoints->SetPoint(0, 1.9, 2.0, 0.0);
  inPoints->SetPoint(1, 2.1, 2.0, 0.0);
  inPoints->SetPoint(2, 2.0, 2.1, 0.0);
  inPoints->SetPoint(3, 2.0, 1.9, 0.0);
  inPoints->SetPoint(4, 2.0, 2.0, 0.0);
  vtkConvexHull2D::CalculateConvexHull(inPoints, outPoints, 2.0);
  if (outPoints->GetNumberOfPoints() != 4)
    {
    std::cerr << "Error: Five points - expected 4 output points but got " <<
      outPoints->GetNumberOfPoints() << "." << std::endl;
    return EXIT_FAILURE;
    }

  outPoints->GetPoint(0, retrievedPoint);
  expectedPoint[0] = 2.0; expectedPoint[1] = 1.0; expectedPoint[2] = 0.0;
  if (!fuzzyCompare2Dweak(expectedPoint, retrievedPoint))
    {
    std::cerr << "Error: Five points - unexpected output value for point 0." << std::endl;
    return EXIT_FAILURE;
    }
  outPoints->GetPoint(1, retrievedPoint);
  expectedPoint[0] = 3.0; expectedPoint[1] = 2.0; expectedPoint[2] = 0.0;
  if (!fuzzyCompare2Dweak(expectedPoint, retrievedPoint))
    {
    std::cerr << "Error: Five points - unexpected output value for point 1." << std::endl;
    return EXIT_FAILURE;
    }
  outPoints->GetPoint(2, retrievedPoint);
  expectedPoint[0] = 2.0; expectedPoint[1] = 3.0; expectedPoint[2] = 0.0;
  if (!fuzzyCompare2Dweak(expectedPoint, retrievedPoint))
    {
    std::cerr << "Error: Five points - unexpected output value for point 2." << std::endl;
    return EXIT_FAILURE;
    }
  outPoints->GetPoint(3, retrievedPoint);
  expectedPoint[0] = 1.0; expectedPoint[1] = 2.0; expectedPoint[2] = 0.0;
  if (!fuzzyCompare2Dweak(expectedPoint, retrievedPoint))
    {
    std::cerr << "Error: Five points - unexpected output value for point 3." << std::endl;
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
