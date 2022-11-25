/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageDifference.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test the vtkImageDifference class

#include "vtkImageDifference.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPNGReader.h"
#include "vtkTestUtilities.h"

#include <iostream>
#include <limits>

namespace
{
bool CheckErrors(vtkImageDifference* differenceFilter, double expectedError,
  double expectedThresholdedError, const std::string& info)
{
  if (!vtkMathUtilities::FuzzyCompare(differenceFilter->GetError(), expectedError, 1e-8) ||
    !vtkMathUtilities::FuzzyCompare(
      differenceFilter->GetThresholdedError(), expectedThresholdedError, 1e-8))
  {
    std::cerr << std::setprecision(std::numeric_limits<double>::digits10);
    std::cerr << std::setprecision(17);
    std::cerr << "Unexpected vtkImageDifference errors with " << info << std::endl;
    std::cerr << "Expected error: " << expectedError << ", got: " << differenceFilter->GetError()
              << " " << (expectedError != differenceFilter->GetError()) << std::endl;
    std::cerr << "Expected thresholded error: " << expectedThresholdedError
              << ", got: " << differenceFilter->GetThresholdedError() << " "
              << (expectedThresholdedError != differenceFilter->GetThresholdedError()) << std::endl;
    return false;
  }
  return true;
}
}

int ImageDifference(int argc, char* argv[])
{
  char* fname1 = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ImageDiff1.png");
  char* fname2 = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ImageDiff2.png");

  vtkNew<vtkPNGReader> reader1;
  reader1->SetFileName(fname1);
  reader1->Update();

  vtkNew<vtkPNGReader> reader2;
  reader2->SetFileName(fname2);
  reader2->Update();

  delete[] fname1;
  delete[] fname2;

  vtkNew<vtkImageDifference> differenceFilter;
  differenceFilter->SetInputConnection(reader1->GetOutputPort());
  differenceFilter->SetImageConnection(reader2->GetOutputPort());

  // Default param
  differenceFilter->Update();
  if (!::CheckErrors(
        differenceFilter, 10600.898039215839, 0.97124183006535825, "default parameters"))
  {
    return EXIT_FAILURE;
  }

  // Testing symmetric
  differenceFilter->SetInputConnection(reader2->GetOutputPort());
  differenceFilter->SetImageConnection(reader1->GetOutputPort());
  differenceFilter->Update();
  if (!::CheckErrors(
        differenceFilter, 10600.898039215839, 0.97124183006535825, "symmetric testing"))
  {
    return EXIT_FAILURE;
  }

  // Zero threshold
  differenceFilter->SetThreshold(0);
  differenceFilter->Update();
  if (!::CheckErrors(differenceFilter, 9342.9607843138092, 9342.9607843138092, "zero threshold"))
  {
    return EXIT_FAILURE;
  }

  // Higher average threshold
  differenceFilter->SetThreshold(105);
  differenceFilter->SetAverageThresholdFactor(1.);
  differenceFilter->Update();
  if (!::CheckErrors(
        differenceFilter, 10594.431372549172, 0.22614379084967323, "higher average threshold"))
  {
    return EXIT_FAILURE;
  }

  // No averaging
  differenceFilter->SetAverageThresholdFactor(0.5);
  differenceFilter->SetAveraging(false);
  differenceFilter->Update();
  if (!::CheckErrors(differenceFilter, 10600.898039215839, 0.97124183006535825, "no averaging"))
  {
    return EXIT_FAILURE;
  }

  // No shift
  differenceFilter->SetAveraging(true);
  differenceFilter->SetAllowShift(false);
  differenceFilter->Update();
  if (!::CheckErrors(differenceFilter, 9587.1254901961565, 1.1986928104575143, "no shift"))
  {
    return EXIT_FAILURE;
  }

  // Multi param
  differenceFilter->SetThreshold(0);
  differenceFilter->SetAveraging(false);
  differenceFilter->Update();
  if (!::CheckErrors(
        differenceFilter, 9587.1254901961565, 9587.1254901961565, "multiple parameters changes"))
  {
    return EXIT_FAILURE;
  }

  // Identical
  differenceFilter->SetInputConnection(reader1->GetOutputPort());
  differenceFilter->Update();
  if (!::CheckErrors(differenceFilter, 0., 0., "identical images"))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
