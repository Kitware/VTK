/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImplicitPolyDataDistance.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include <vtkSmartPointer.h>

#include <vtkImplicitPolyDataDistance.h>
#include <vtkPlaneSource.h>

#include <vector>

int TestImplicitPolyDataDistance(int, char*[])
{
  vtkSmartPointer<vtkPlaneSource> plane =
    vtkSmartPointer<vtkPlaneSource>::New();
  plane->SetXResolution(5);
  plane->SetYResolution(5);
  plane->Update();

  vtkSmartPointer<vtkImplicitPolyDataDistance> distance =
    vtkSmartPointer<vtkImplicitPolyDataDistance>::New();

  distance->SetInput(plane->GetOutput());

  // Evaluate at a few points

  std::vector<double *> probes;
  {
  double *probe = new double[3];
  probe[0] = probe[1] = 0.0;
  probe[2] = 1.0;
  probes.push_back(probe);
  }
  {
  double *probe = new double[3];
  probe[0] = probe[1] = 0.0;
  probe[2] = -1.0;
  probes.push_back(probe);
  }
  {
  double *probe = new double[3];
  probe[0] = 1.0;
  probe[1] = 1.0;
  probe[2] = 1.0;
  probes.push_back(probe);
  }
  // Probe at each of the polydata points
  for (int i = 0; i < plane->GetOutput()->GetNumberOfPoints(); ++i)
    {
    double *point = new double[3];
    plane->GetOutput()->GetPoint(i, point);
    probes.push_back(point);
    }

  std::vector<double*>::iterator it = probes.begin();
  for ( ; it != probes.end(); ++it)
    {
    double *p = *it;
    double result = distance->EvaluateFunction(p);
    double gradient[3];
    distance->EvaluateGradient(p, gradient);
    std::cout << "(" << p[0] << ", "  << p[1] << ", " << p[2] << "): "
              << result
              << " gradient(" << gradient[0] << ", "
              << gradient[1] << ", "
              << gradient[2] << ")"
              << std::endl;
    }

  distance->Print(std::cout);

  it = probes.begin();
  for ( ; it != probes.end(); ++it)
    {
    delete [] *it;
    }
  return EXIT_SUCCESS;
}
