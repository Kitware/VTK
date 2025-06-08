// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkDoubleArray.h>
#include <vtkPlaneSource.h>
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
#include <vtkTriangleFilter.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLPolyDataWriter.h>

#include <cstdlib>
#include <iostream>

int TestQuadricDecimationSetPointAttributeArray(int argc, char* argv[])
{
  vtkNew<vtkPlaneSource> planeSource;
  planeSource->SetXResolution(100);
  planeSource->SetYResolution(100);
  planeSource->Update();

  vtkPolyData* plane = planeSource->GetOutput();
  vtkPoints* points = plane->GetPoints();
  vtkIdType numPoints = points->GetNumberOfPoints();

  vtkNew<vtkDoubleArray> scalars;
  scalars->SetName("Analytical");
  scalars->SetNumberOfComponents(1);
  scalars->SetNumberOfTuples(numPoints);

  auto ptRange = vtk::DataArrayValueRange<3>(points->GetData());
  auto dRange = vtk::DataArrayValueRange<1>(scalars);
  for (vtkIdType iP = 0; iP < numPoints; ++iP)
  {
    dRange[iP] = 2.5 - 2.5 * std::cos(20 * ptRange[iP * 3] + 8 * ptRange[iP * 3 + 1]);
  }

  // Add scalar array to point data
  plane->GetPointData()->SetScalars(scalars);

  vtkNew<vtkTriangleFilter> triangulate;
  triangulate->SetInputData(plane);
  triangulate->Update();
  triangulate->GetOutput()->GetPointData()->SetActiveScalars("Analytical");

  vtkNew<vtkQuadricDecimation> decimator;
  decimator->SetInputConnection(triangulate->GetOutputPort());
  decimator->SetRegularize(false);
  decimator->SetTargetReduction(0.95);
  decimator->AttributeErrorMetricOn();
  decimator->ScalarsAttributeOn();
  decimator->SetScalarsWeight(1.0);
  decimator->VectorsAttributeOff();
  decimator->NormalsAttributeOff();
  decimator->VolumePreservationOn();
  decimator->WeighBoundaryConstraintsByLengthOn();
  decimator->SetMapPointData(true);
  decimator->Update();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(decimator->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetEdgeVisibility(true);
  mapper->SetScalarRange(decimator->GetOutput()->GetPointData()->GetScalars()->GetRange());

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  vtkCamera* camera = renderer->GetActiveCamera();
  camera->SetParallelProjection(true);
  camera->SetParallelScale(0.5);
  renWin->Render();

  return (vtkRegressionTester::Test(argc, argv, renWin, 0.1) == vtkRegressionTester::PASSED)
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}
