// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// This tests the fix to issue #18307 (incorrect sign when a point is located
// directly above an edge of a cube)

#include "vtkCubeSource.h"
#include "vtkImplicitPolyDataDistance.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"

bool isPointInsideCube(double x, double y, double z)
{
  // Check if point (x, y, z) lies within a cube centered at (0,0,0) with side length 1
  return (x >= -0.5 && x <= 0.5) && (y >= -0.5 && y <= 0.5) && (z >= -0.5 && z <= 0.5);
}

int TestImplicitPolyDataDistanceCube(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Create a cube with side length 1.0
  const double length = 1.0;
  vtkNew<vtkCubeSource> cube;
  cube->SetXLength(length);
  cube->SetYLength(length);
  cube->SetZLength(length);
  cube->Update();

  // Retrieve the cube's polydata
  vtkSmartPointer<vtkPolyData> cubePolydata = cube->GetOutput();

  // Initialize distance function for the cube
  vtkNew<vtkImplicitPolyDataDistance> signedDistance;
  signedDistance->SetInput(cubePolydata);

  // Grid step size for sampling points
  const double step = 0.05;

  // Calculate number of steps in each dimension
  int numSteps = static_cast<int>(2 * length / step) + 1;

  // Iterate through a grid of points around the cube
  for (int i = 0; i < numSteps; ++i)
  {
    const double z = -length + i * step;
    for (int j = 0; j < numSteps; ++j)
    {
      const double y = -length + j * step;
      for (int k = 0; k < numSteps; ++k)
      {
        const double x = -length + k * step;
        const double dist = signedDistance->EvaluateFunction(x, y, z);
        if (isPointInsideCube(x, y, z) != (dist <= 0))
        {
          return EXIT_FAILURE;
        }
      }
    }
  }

  return EXIT_SUCCESS;
}
