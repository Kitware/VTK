/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestConeSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkConeSource.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkSmartPointer.h>

int TestConeSource(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkMinimalStandardRandomSequence> randomSequence
    = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  vtkSmartPointer<vtkConeSource> coneSource
    = vtkSmartPointer<vtkConeSource>::New();
  coneSource->SetResolution(8);
  coneSource->CappingOn();

  coneSource->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);

  double center[3];
  for(unsigned int i = 0; i < 3; ++i)
    {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
    }
  coneSource->SetCenter(center);

  double direction[3];
  for(unsigned int i = 0; i < 3; ++i)
    {
    randomSequence->Next();
    direction[i] = randomSequence->GetValue();
    }
  coneSource->SetDirection(direction);

  randomSequence->Next();
  double height = randomSequence->GetValue();
  coneSource->SetHeight(height);

  randomSequence->Next();
  double radius = randomSequence->GetValue();
  coneSource->SetRadius(radius);

  coneSource->Update();

  vtkSmartPointer<vtkPolyData> polyData = coneSource->GetOutput();
  vtkSmartPointer<vtkPoints> points = polyData->GetPoints();

  if(points->GetDataType() != VTK_FLOAT)
    {
    return EXIT_FAILURE;
    }

  coneSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

  for(unsigned int i = 0; i < 3; ++i)
    {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
    }
  coneSource->SetCenter(center);

  for(unsigned int i = 0; i < 3; ++i)
    {
    randomSequence->Next();
    direction[i] = randomSequence->GetValue();
    }
  coneSource->SetDirection(direction);

  randomSequence->Next();
  height = randomSequence->GetValue();
  coneSource->SetHeight(height);

  randomSequence->Next();
  radius = randomSequence->GetValue();
  coneSource->SetRadius(radius);

  coneSource->Update();

  polyData = coneSource->GetOutput();
  points = polyData->GetPoints();

  if(points->GetDataType() != VTK_DOUBLE)
    {
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
