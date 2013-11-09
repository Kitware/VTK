/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCubeSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkCubeSource.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkSmartPointer.h>

int TestCubeSource(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkMinimalStandardRandomSequence> randomSequence
    = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  vtkSmartPointer<vtkCubeSource> cubeSource
    = vtkSmartPointer<vtkCubeSource>::New();

  cubeSource->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);

  double center[3];
  for(unsigned int i = 0; i < 3; ++i)
    {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
    }
  cubeSource->SetCenter(center);

  randomSequence->Next();
  double xLength = randomSequence->GetValue();
  cubeSource->SetXLength(xLength);

  randomSequence->Next();
  double yLength = randomSequence->GetValue();
  cubeSource->SetYLength(yLength);

  randomSequence->Next();
  double zLength = randomSequence->GetValue();
  cubeSource->SetZLength(zLength);

  cubeSource->Update();

  vtkSmartPointer<vtkPolyData> polyData = cubeSource->GetOutput();
  vtkSmartPointer<vtkPoints> points = polyData->GetPoints();

  if(points->GetDataType() != VTK_FLOAT)
    {
    return EXIT_FAILURE;
    }

  cubeSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

  for(unsigned int i = 0; i < 3; ++i)
    {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
    }
  cubeSource->SetCenter(center);

  randomSequence->Next();
  xLength = randomSequence->GetValue();
  cubeSource->SetXLength(xLength);

  randomSequence->Next();
  yLength = randomSequence->GetValue();
  cubeSource->SetYLength(yLength);

  randomSequence->Next();
  zLength = randomSequence->GetValue();
  cubeSource->SetZLength(zLength);

  cubeSource->Update();

  polyData = cubeSource->GetOutput();
  points = polyData->GetPoints();

  if(points->GetDataType() != VTK_DOUBLE)
    {
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
