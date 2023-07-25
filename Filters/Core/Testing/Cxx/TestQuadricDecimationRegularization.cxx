// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkDataSet.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkQuadricDecimation.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>

#include <cstdlib>
#include <iostream>

int TestQuadricDecimationRegularization(int argc, char* argv[])
{

  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius(1.0);
  sphere->SetThetaResolution(70);
  sphere->SetPhiResolution(70);
  sphere->Update();

  {
    vtkDataSet* output = vtkDataSet::SafeDownCast(sphere->GetOutput(0));
    std::cout << "NCells before decimation: " << output->GetNumberOfCells() << std::endl;
  }

  vtkNew<vtkQuadricDecimation> decimator;
  decimator->SetInputConnection(sphere->GetOutputPort());
  decimator->SetTargetReduction(0.90);
  decimator->SetVolumePreservation(true);
  decimator->SetRegularize(true);
  decimator->SetRegularization(0.05);

  decimator->Update();

  int nCellsAfter = 0;
  {
    vtkDataSet* output = vtkDataSet::SafeDownCast(decimator->GetOutput(0));
    nCellsAfter = output->GetNumberOfCells();
    std::cout << "NCells after decimation: " << nCellsAfter << std::endl;
  }

  if (nCellsAfter != 952)
  {
    std::cout << "Decimation target not achieved!" << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(decimator->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToWireframe();

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->Render();

  vtkCamera* camera = renderer->GetActiveCamera();
  camera->SetPosition(1.5, 1.5, 1.5);
  renderer->ResetCamera();

  return (vtkRegressionTester::Test(argc, argv, renWin, 10) == vtkRegressionTester::PASSED)
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}
