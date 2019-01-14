/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSphereSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkMinimalStandardRandomSequence.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>

int TestSphereSource(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkMinimalStandardRandomSequence> randomSequence
    = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  vtkSmartPointer<vtkSphereSource> sphereSource
    = vtkSmartPointer<vtkSphereSource>::New();
  sphereSource->SetThetaResolution(8);
  sphereSource->SetPhiResolution(8);
  sphereSource->SetStartTheta(0.0);
  sphereSource->SetEndTheta(360.0);
  sphereSource->SetStartPhi(0.0);
  sphereSource->SetEndPhi(180.0);
  sphereSource->LatLongTessellationOff();

  sphereSource->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);

  double center[3];
  for(unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
  }
  sphereSource->SetCenter(center);

  randomSequence->Next();
  double radius = randomSequence->GetValue();
  sphereSource->SetRadius(radius);

  sphereSource->Update();

  vtkSmartPointer<vtkPolyData> polyData = sphereSource->GetOutput();
  vtkSmartPointer<vtkPoints> points = polyData->GetPoints();

  if(points->GetDataType() != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  sphereSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

  for(unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
  }
  sphereSource->SetCenter(center);

  randomSequence->Next();
  radius = randomSequence->GetValue();
  sphereSource->SetRadius(radius);

  sphereSource->Update();

  polyData = sphereSource->GetOutput();
  points = polyData->GetPoints();

  if(points->GetDataType() != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
