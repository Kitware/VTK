/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPointSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkMinimalStandardRandomSequence.h>
#include <vtkPointSource.h>
#include <vtkSmartPointer.h>

int TestPointSource(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkMinimalStandardRandomSequence> randomSequence
    = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  vtkSmartPointer<vtkPointSource> pointSource
    = vtkSmartPointer<vtkPointSource>::New();
  pointSource->SetDistributionToUniform();
  pointSource->SetNumberOfPoints(16);

  pointSource->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);

  double center[3];
  for(unsigned int i = 0; i < 3; ++i)
    {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
    }
  pointSource->SetCenter(center);

  randomSequence->Next();
  double radius = randomSequence->GetValue();
  pointSource->SetRadius(radius);

  pointSource->Update();

  vtkSmartPointer<vtkPolyData> polyData = pointSource->GetOutput();
  vtkSmartPointer<vtkPoints> points = polyData->GetPoints();

  if(points->GetDataType() != VTK_FLOAT)
    {
    return EXIT_FAILURE;
    }

  pointSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

  for(unsigned int i = 0; i < 3; ++i)
    {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
    }
  pointSource->SetCenter(center);

  randomSequence->Next();
  radius = randomSequence->GetValue();
  pointSource->SetRadius(radius);

  pointSource->Update();

  polyData = pointSource->GetOutput();
  points = polyData->GetPoints();

  if(points->GetDataType() != VTK_DOUBLE)
    {
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
