// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkPlane.h>
#include <vtkPolyDataPlaneClipper.h>
#include <vtkSphereSource.h>

#include <iostream>

int TestPolyDataPlaneClipper(int, char*[])
{
  vtkNew<vtkSphereSource> source;
  source->SetCenter(0.0, 0.0, 0.0);
  source->SetRadius(0.5);
  source->Update();

  vtkNew<vtkPlane> plane;
  plane->SetOrigin(0.2, 0.0, 0.0);
  plane->SetNormal(-1.0, 0.0, 0.0);

  vtkNew<vtkPolyDataPlaneClipper> clipper;
  clipper->SetInputConnection(source->GetOutputPort());
  clipper->SetPlane(plane);
  clipper->CappingOn();
  clipper->Update();

  vtkSmartPointer<vtkPolyData> clippedOutput = clipper->GetOutput(0);
  vtkSmartPointer<vtkPolyData> cap = clipper->GetOutput(1);

  double expectedBounds[6] = { -0.487464, 0.2, -0.487464, 0.487464, -0.5, 0.5 };
  double expectedNumberOfPoints = 64;
  double expectedNumberOfCells = 82;

  double expectedNumberOfCapPoints = 64;
  double expectedNumberOfCapCells = 26;

  double bounds[6];
  clippedOutput->GetBounds(bounds);
  for (int i = 0; i < 6; i++)
  {
    if (std::abs(bounds[i] - expectedBounds[i]) > 1e-6)
    {
      std::cout << "Clipped output bounds (" << bounds[0] << ", " << bounds[1] << ", " << bounds[2]
                << ", " << bounds[3] << ", " << bounds[4] << ", " << bounds[5]
                << ") differ from expected bounds (" << expectedBounds[0] << ", "
                << expectedBounds[1] << ", " << expectedBounds[2] << ", " << expectedBounds[3]
                << ", " << expectedBounds[4] << ", " << expectedBounds[5] << ")" << std::endl;
      return EXIT_FAILURE;
    }
  }

  double numberOfPoints = clippedOutput->GetNumberOfPoints();
  if (expectedNumberOfPoints != numberOfPoints)
  {
    std::cout << "Clipped output number of points (" << numberOfPoints
              << ") differ from expected number (" << expectedNumberOfPoints << ")" << std::endl;
  }

  double numberOfCells = clippedOutput->GetNumberOfCells();
  if (expectedNumberOfCells != numberOfCells)
  {
    std::cout << "Clipped output number of cells (" << numberOfCells
              << ") differ from expected number (" << expectedNumberOfCells << ")" << std::endl;
  }

  double numberOfCapPoints = cap->GetNumberOfPoints();
  if (expectedNumberOfCapPoints != numberOfCapPoints)
  {
    std::cout << "Cap number of points (" << numberOfCapPoints << ") differ from expected number ("
              << expectedNumberOfCapPoints << ")" << std::endl;
  }

  double numberOfCapCells = cap->GetNumberOfCells();
  if (expectedNumberOfCapCells != numberOfCapCells)
  {
    std::cout << "Cap number of cells (" << numberOfCapCells << ") differ from expected number ("
              << expectedNumberOfCapCells << ")" << std::endl;
  }

  return EXIT_SUCCESS;
}
