// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLocatorInterface.h"
#include "vtkMath.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkShellBinIterator.h"
#include "vtkStaticPointLocator.h"
#include "vtkTimerLog.h"

#include <iostream>

int TestShellBinIterator(int, char*[])
{
  int nPts = 1000000;

  // Populate a list of points and query locations
  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(nPts);
  for (int i = 0; i < nPts; ++i)
  {
    points->SetPoint(i, vtkMath::Random(-1, 1), vtkMath::Random(-1, 1), vtkMath::Random(-1, 1));
  }

  vtkNew<vtkPolyData> polydata;
  polydata->SetPoints(points);

  vtkNew<vtkStaticPointLocator> staticLocator;
  staticLocator->SetDataSet(polydata);
  staticLocator->BuildLocator();

  // Create the iterator. We choose the point x so as to produce an
  // insertion location @(70,70,70).
  vtkShellBinIterator siter(staticLocator);
  double x[3] = { 0.4, 0.425, 0.425 };
  vtkDist2TupleArray results;
  siter.Begin(100, x, results);

  // Iterate to the end of the fourth level, and get the results
  for (auto binNum = 1; binNum < 707; ++binNum)
  {
    siter.Next(VTK_FLOAT_MAX, nullptr, results);
  }
  std::cout << "Results size: " << results.size() << endl;

  // Return as appropriate
  if (results.size() != 2)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
