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

#include "vtkLineSource.h"
#include "vtkSmartPointer.h"

#include "vtkTestUtilities.h"

#include <limits>

#ifndef ABS
#define ABS(x) ((x) < 0 ? -(x) : (x))
#endif

template<class A>
bool fuzzyCompare1D(A a, A b)
{
  return ABS(a - b) < std::numeric_limits<A>::epsilon();
}

int TestLineSource( int argc, char* argv[] )
{
  (void)argc;
  (void)argv;

  {
  // Test double functions
  double p1[3] = {1.0, 0.0, 0.0};
  double p2[3] = {0.0, 1.0, 0.0};

  vtkSmartPointer<vtkLineSource> lineSource =
    vtkSmartPointer<vtkLineSource>::New();
  lineSource->SetPoint1(p1);
  lineSource->SetPoint2(p2);
  lineSource->Update();

  double* p1retrieved = lineSource->GetPoint1();
  if(!fuzzyCompare1D(static_cast<float>(p1retrieved[0]), static_cast<float>(p1[0])) ||
     !fuzzyCompare1D(static_cast<float>(p1retrieved[1]), static_cast<float>(p1[1])) ||
     !fuzzyCompare1D(static_cast<float>(p1retrieved[2]), static_cast<float>(p1[2])))
    {
    std::cerr << "Error: p1(double) was not retrieved properly!" << std::endl;
    return EXIT_FAILURE;
    }


  double* p2retrieved = lineSource->GetPoint2();
  if(!fuzzyCompare1D(static_cast<float>(p2retrieved[0]), static_cast<float>(p2[0])) ||
     !fuzzyCompare1D(static_cast<float>(p2retrieved[1]), static_cast<float>(p2[1])) ||
     !fuzzyCompare1D(static_cast<float>(p2retrieved[2]), static_cast<float>(p2[2])))
    {
    std::cerr << "Error: p2(double) was not retrieved properly!" << std::endl;
    return EXIT_FAILURE;
    }
  }

  {
  // Test float functions
  float p1[3] = {1.0, 0.0, 0.0};
  float p2[3] = {0.0, 1.0, 0.0};

  vtkSmartPointer<vtkLineSource> lineSource =
    vtkSmartPointer<vtkLineSource>::New();
  lineSource->SetPoint1(p1);
  lineSource->SetPoint2(p2);
  lineSource->Update();

  double* p1retrieved = lineSource->GetPoint1();
  if(!fuzzyCompare1D(static_cast<float>(p1retrieved[0]), static_cast<float>(p1[0])) ||
     !fuzzyCompare1D(static_cast<float>(p1retrieved[1]), static_cast<float>(p1[1])) ||
     !fuzzyCompare1D(static_cast<float>(p1retrieved[2]), static_cast<float>(p1[2])))
    {
    std::cerr << "Error: p1(float) was not retrieved properly!" << std::endl;
    return EXIT_FAILURE;
    }


  double* p2retrieved = lineSource->GetPoint2();
  if(!fuzzyCompare1D(static_cast<float>(p2retrieved[0]), static_cast<float>(p2[0])) ||
     !fuzzyCompare1D(static_cast<float>(p2retrieved[1]), static_cast<float>(p2[1])) ||
     !fuzzyCompare1D(static_cast<float>(p2retrieved[2]), static_cast<float>(p2[2])))
    {
    std::cerr << "Error: p2(float) was not retrieved properly!" << std::endl;
    return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
