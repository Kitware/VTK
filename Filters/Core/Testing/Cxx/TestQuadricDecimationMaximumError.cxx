// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkQuadricDecimation.h>
#include <vtkSphereSource.h>

#include <cstdlib>
#include <iostream>

int TestQuadricDecimationMaximumError(int, char*[])
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius(1.0);
  sphere->SetThetaResolution(70);
  sphere->SetPhiResolution(70);
  sphere->Update();

  int nCellsBefore = 0;
  {
    vtkDataSet* output = vtkDataSet::SafeDownCast(sphere->GetOutput(0));
    nCellsBefore = output->GetNumberOfCells();
    std::cout << "NCells before decimation: " << nCellsBefore << std::endl;
  }

  vtkNew<vtkQuadricDecimation> decimator;
  decimator->SetInputConnection(sphere->GetOutputPort());
  decimator->SetTargetReduction(0.90);
  decimator->SetVolumePreservation(true);
  decimator->SetMaximumError(0.0);
  decimator->Update();

  int nCellsAfter = 0;
  {
    vtkDataSet* output = vtkDataSet::SafeDownCast(decimator->GetOutput(0));
    nCellsAfter = output->GetNumberOfCells();
    std::cout << "NCells after decimation: " << nCellsAfter << std::endl;
  }

  if (nCellsAfter != nCellsBefore)
  {
    std::cout << "Decimation maximum error not respected!" << std::endl;
    return EXIT_FAILURE;
  }

  decimator->SetMaximumError(0.0000001);
  decimator->Update();

  {
    vtkDataSet* output = vtkDataSet::SafeDownCast(decimator->GetOutput(0));
    nCellsAfter = output->GetNumberOfCells();
    std::cout << "NCells after decimation: " << nCellsAfter << std::endl;
  }

  if (nCellsAfter != 2780)
  {
    std::cout << "Decimation maximum error not respected!" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
