/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSuperquadricSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkMinimalStandardRandomSequence.h>
#include <vtkSmartPointer.h>
#include <vtkSuperquadricSource.h>

int TestSuperquadricSource(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkMinimalStandardRandomSequence> randomSequence
    = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  vtkSmartPointer<vtkSuperquadricSource> superquadricSource
    = vtkSmartPointer<vtkSuperquadricSource>::New();
  superquadricSource->SetThetaResolution(8);
  superquadricSource->SetPhiResolution(8);
  superquadricSource->SetThetaRoundness(1.0);
  superquadricSource->SetPhiRoundness(1.0);
  superquadricSource->SetYAxisOfSymmetry();
  superquadricSource->ToroidalOff();

  superquadricSource->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);

  double center[3];
  for(unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
  }
  superquadricSource->SetCenter(center);

  double scale[3];
  for(unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    scale[i] = randomSequence->GetValue();
  }
  superquadricSource->SetScale(scale);

  superquadricSource->Update();

  vtkSmartPointer<vtkPolyData> polyData = superquadricSource->GetOutput();
  vtkSmartPointer<vtkPoints> points = polyData->GetPoints();

  if(points->GetDataType() != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  superquadricSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

  for(unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
  }
  superquadricSource->SetCenter(center);

  for(unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    scale[i] = randomSequence->GetValue();
  }
  superquadricSource->SetScale(scale);

  superquadricSource->Update();

  polyData = superquadricSource->GetOutput();
  points = polyData->GetPoints();

  if(points->GetDataType() != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
