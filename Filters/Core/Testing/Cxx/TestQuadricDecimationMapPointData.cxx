// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkActor.h>
#include <vtkAffineArray.h>
#include <vtkCamera.h>
#include <vtkDataSet.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkQuadricDecimation.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>

#include <cmath>
#include <cstdlib>
#include <iostream>

int TestQuadricDecimationMapPointData(int argc, char* argv[])
{

  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius(1.0);
  sphere->SetThetaResolution(70);
  sphere->SetPhiResolution(70);
  sphere->Update();

  {
    vtkPolyData* output = vtkPolyData::SafeDownCast(sphere->GetOutput(0));
    std::cout << "NCells before decimation: " << output->GetNumberOfCells() << std::endl;
    vtkNew<vtkDoubleArray> scalars;
    scalars->SetName("Analytical");
    scalars->SetNumberOfComponents(1);
    scalars->SetNumberOfTuples(output->GetNumberOfPoints());
    auto points = output->GetPoints()->GetData();
    auto ptRange = vtk::DataArrayValueRange<3>(points);
    auto dRange = vtk::DataArrayValueRange<1>(scalars);
    for (vtkIdType iP = 0; iP < output->GetNumberOfPoints(); ++iP)
    {
      dRange[iP] = std::sin(3.0 * (ptRange[iP * 3] + ptRange[iP * 3 + 1] + ptRange[iP * 3 + 2]));
    }

    vtkNew<vtkAffineArray<vtkIdType>> affine;
    affine->SetNumberOfTuples(output->GetNumberOfPoints());
    affine->ConstructBackend(1, 0);
    affine->SetName("IdsTypeArray");

    output->GetPointData()->AddArray(affine);
    output->GetPointData()->AddArray(scalars);
    output->GetPointData()->SetScalars(scalars);
  }

  vtkNew<vtkQuadricDecimation> decimator;
  decimator->SetInputConnection(sphere->GetOutputPort());
  decimator->SetTargetReduction(0.90);
  decimator->SetVolumePreservation(true);
  decimator->SetMapPointData(true);

  decimator->Update();

  int nCellsAfter = 0;
  {
    vtkDataSet* output = vtkDataSet::SafeDownCast(decimator->GetOutput(0));
    nCellsAfter = output->GetNumberOfCells();
    std::cout << "NCells after decimation: " << nCellsAfter << std::endl;

    auto idArr = output->GetPointData()->GetArray("IdsTypeArray");
    std::cout << idArr->GetTuple1(0) << std::endl;
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
  actor->GetProperty()->SetRepresentationToSurface();

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->Render();

  vtkCamera* camera = renderer->GetActiveCamera();
  camera->SetPosition(-1.5, 1.5, 1.5);
  renderer->ResetCamera();

  return (vtkRegressionTester::Test(argc, argv, renWin, 10) == vtkRegressionTester::PASSED)
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}
