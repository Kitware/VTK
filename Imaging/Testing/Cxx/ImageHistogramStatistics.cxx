/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageHistogramStatistics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test the vtkImageHistogramStatistics class
//
// The command line arguments are:
// -I        => run in interactive mode

#include "vtkPNGReader.h"
#include "vtkImageCast.h"
#include "vtkImageHistogramStatistics.h"
#include "vtkImageAccumulate.h"

#include "vtkTestUtilities.h"

#include <math.h>

int ImageHistogramStatistics(int argc, char *argv[])
{
  vtkPNGReader *reader = vtkPNGReader::New();

  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/fullhead15.png");

  reader->SetFileName(fname);
  delete[] fname;

  // Use float data to get the most code coverage
  vtkImageCast *imageCast = vtkImageCast::New();
  imageCast->SetOutputScalarTypeToFloat();
  imageCast->SetInputConnection(reader->GetOutputPort());

  double minValTest = 0;
  double maxValTest = 3714;
  double meanValTest = 635.8066572717137;
  double medianTest = 190.9279926756695;
  double stdevTest = 660.9126299774935;
  double tol = 1e-6;

  vtkImageHistogramStatistics *statistics = vtkImageHistogramStatistics::New();
  statistics->SetInputConnection(imageCast->GetOutputPort());
  statistics->GenerateHistogramImageOff();
  statistics->Update();

  double minVal = statistics->GetMinimum();
  double maxVal = statistics->GetMaximum();
  double meanVal = statistics->GetMean();
  double median = statistics->GetMedian();
  double stdev = statistics->GetStandardDeviation();

// uncomment to test vtkImageAccumulate instead
/*
  vtkImageAccumulate *accumulate = vtkImageAccumulate::New();
  accumulate->SetInputConnection(reader->GetOutputPort());
  accumulate->Update();

  double minVal = accumulate->GetMin()[0];
  double maxVal = accumulate->GetMax()[0];
  double meanVal = accumulate->GetMean()[0];
  double median = medianTest;
  double stdev = accumulate->GetStandardDeviation()[0];
*/

  bool retVal = true;

  if (fabs((minVal - minValTest)/maxValTest) > tol)
    {
    cout.precision(16);
    cout << "minVal " << minVal << " should be " << minValTest << endl;
    retVal = false;
    }
  if (fabs((maxVal - maxValTest)/maxValTest) > tol)
    {
    cout.precision(16);
    cout << "maxVal " << maxVal << " should be " << maxValTest << endl;
    retVal = false;
    }
  if (fabs((meanVal - meanValTest)/maxValTest) > tol)
    {
    cout.precision(16);
    cout << "meanVal " << meanVal << " should be " << meanValTest << endl;
    retVal = false;
    }
  if (fabs((median - medianTest)/maxValTest) > tol)
    {
    cout.precision(16);
    cout << "median " << median << " should be " << medianTest << endl;
    retVal = false;
    }
  if (fabs((stdev - stdevTest)/maxValTest) > tol)
    {
    cout.precision(16);
    cout << "stdev " << stdev << " should be " << stdevTest << endl;
    retVal = false;
    }

  reader->Delete();
  imageCast->Delete();
  statistics->Delete();

  return !retVal;
}
