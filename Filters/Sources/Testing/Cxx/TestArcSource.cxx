// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkArcSource.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkSmartPointer.h>

int TestArcSource(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkMinimalStandardRandomSequence> randomSequence =
    vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  vtkSmartPointer<vtkArcSource> arcSource = vtkSmartPointer<vtkArcSource>::New();
  arcSource->SetAngle(90.0);
  arcSource->SetResolution(8);
  arcSource->NegativeOff();
  arcSource->UseNormalAndAngleOn();

  arcSource->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);

  double normal[3];
  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    normal[i] = randomSequence->GetValue();
  }
  arcSource->SetNormal(normal);

  double polarVector[3];
  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    polarVector[i] = randomSequence->GetValue();
  }
  arcSource->SetPolarVector(polarVector);

  arcSource->Update();

  vtkSmartPointer<vtkPolyData> polyData = arcSource->GetOutput();
  vtkSmartPointer<vtkPoints> points = polyData->GetPoints();

  if (points->GetDataType() != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  arcSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    normal[i] = randomSequence->GetValue();
  }
  arcSource->SetNormal(normal);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    polarVector[i] = randomSequence->GetValue();
  }
  arcSource->SetPolarVector(polarVector);

  arcSource->Update();

  polyData = arcSource->GetOutput();
  points = polyData->GetPoints();

  if (points->GetDataType() != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
