/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTubeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This is just a simple test. vtkTubeFilter internally uses
// vtkProbeFilter, which is tested thoroughly in other tests.

#include "vtkTubeFilter.h"

#include "vtkActor.h"
#include "vtkDataObject.h"
#include "vtkImageGradient.h"
#include "vtkPolyDataMapper.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStreamTracer.h"
#include "vtkTestUtilities.h"

#include <iostream>

int TestVaryRadiusTubeFilter(int argc, char* argv[])
{
  // Create Pipeline
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-10, 10, -10, 10, -10, 10);
  wavelet->SetCenter(0, 0, 0);

  vtkNew<vtkImageGradient> gradient;
  gradient->SetInputConnection(wavelet->GetOutputPort());
  gradient->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "RTData");
  gradient->SetDimensionality(3);

  vtkNew<vtkPolyData> seedsScalar;
  vtkNew<vtkPoints> seedPointsScalar;
  seedPointsScalar->InsertNextPoint(0.0, 0, 0);
  seedPointsScalar->InsertNextPoint(1.0, 0, 0);
  seedPointsScalar->InsertNextPoint(2.0, 0, 0);
  seedPointsScalar->InsertNextPoint(3.0, 0, 0);
  seedPointsScalar->InsertNextPoint(4.0, 0, 0);
  seedPointsScalar->InsertNextPoint(5.0, 0, 0);
  seedPointsScalar->InsertNextPoint(6.0, 0, 0);
  seedPointsScalar->InsertNextPoint(7.0, 0, 0);
  seedPointsScalar->InsertNextPoint(8.0, 0, 0);
  seedsScalar->SetPoints(seedPointsScalar);

  vtkNew<vtkStreamTracer> streamScalar;
  streamScalar->SetInputConnection(gradient->GetOutputPort());
  streamScalar->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "RTDataGradient");
  streamScalar->SetSourceData(seedsScalar);
  streamScalar->SetIntegrationDirection(2); // BOTH
  streamScalar->SetIntegratorType(2);       // Runge-Kutta 4-5

  vtkNew<vtkTubeFilter> tubeScalar;
  tubeScalar->SetInputConnection(streamScalar->GetOutputPort());
  tubeScalar->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "RTData");
  tubeScalar->SetInputArrayToProcess(
    1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "RTDataGradient");
  tubeScalar->SetRadiusFactor(0.1);
  tubeScalar->SetVaryRadiusToVaryRadiusByScalar();
  tubeScalar->Update();

  vtkNew<vtkPolyDataMapper> mapperScalar;
  mapperScalar->SetInputData(tubeScalar->GetOutput());

  vtkNew<vtkActor> actorScalar;
  actorScalar->SetMapper(mapperScalar);

  vtkNew<vtkPolyData> seedsVector;
  vtkNew<vtkPoints> seedPointsVector;
  seedPointsVector->InsertNextPoint(0.0, -4.0, 0);
  seedPointsVector->InsertNextPoint(1.0, -4.0, 0);
  seedPointsVector->InsertNextPoint(2.0, -4.0, 0);
  seedPointsVector->InsertNextPoint(3.0, -4.0, 0);
  seedPointsVector->InsertNextPoint(4.0, -4.0, 0);
  seedPointsVector->InsertNextPoint(5.0, -4.0, 0);
  seedPointsVector->InsertNextPoint(6.0, -4.0, 0);
  seedPointsVector->InsertNextPoint(7.0, -4.0, 0);
  seedPointsVector->InsertNextPoint(8.0, -4.0, 0);
  seedsVector->SetPoints(seedPointsVector);

  vtkNew<vtkStreamTracer> streamVector;
  streamVector->SetInputConnection(gradient->GetOutputPort());
  streamVector->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "RTDataGradient");
  streamVector->SetSourceData(seedsVector);
  streamVector->SetIntegrationDirection(2); // BOTH
  streamVector->SetIntegratorType(2);       // Runge-Kutta 4-5

  vtkNew<vtkTubeFilter> tubeVector;
  tubeVector->SetInputConnection(streamVector->GetOutputPort());
  tubeVector->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "RTData");
  tubeVector->SetInputArrayToProcess(
    1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "RTDataGradient");
  tubeVector->SetRadiusFactor(0.1);
  tubeVector->SetVaryRadiusToVaryRadiusByVector();
  tubeVector->Update();

  vtkNew<vtkPolyDataMapper> mapperVector;
  mapperVector->SetInputData(tubeVector->GetOutput());

  vtkNew<vtkActor> actorVector;
  actorVector->SetMapper(mapperVector);

  vtkNew<vtkPolyData> seedsVectorNorm;
  vtkNew<vtkPoints> seedPointsVectorNorm;
  seedPointsVectorNorm->InsertNextPoint(0.0, 4.0, 0);
  seedPointsVectorNorm->InsertNextPoint(1.0, 4.0, 0);
  seedPointsVectorNorm->InsertNextPoint(2.0, 4.0, 0);
  seedPointsVectorNorm->InsertNextPoint(3.0, 4.0, 0);
  seedPointsVectorNorm->InsertNextPoint(4.0, 4.0, 0);
  seedPointsVectorNorm->InsertNextPoint(5.0, 4.0, 0);
  seedPointsVectorNorm->InsertNextPoint(6.0, 4.0, 0);
  seedPointsVectorNorm->InsertNextPoint(7.0, 4.0, 0);
  seedPointsVectorNorm->InsertNextPoint(8.0, 4.0, 0);
  seedsVectorNorm->SetPoints(seedPointsVectorNorm);

  vtkNew<vtkStreamTracer> streamVectorNorm;
  streamVectorNorm->SetInputConnection(gradient->GetOutputPort());
  streamVectorNorm->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "RTDataGradient");
  streamVectorNorm->SetSourceData(seedsVectorNorm);
  streamVectorNorm->SetIntegrationDirection(2); // BOTH
  streamVectorNorm->SetIntegratorType(2);       // Runge-Kutta 4-5

  vtkNew<vtkTubeFilter> tubeVectorNorm;
  tubeVectorNorm->SetInputConnection(streamVectorNorm->GetOutputPort());
  tubeVectorNorm->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "RTData");
  tubeVectorNorm->SetInputArrayToProcess(
    1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "RTDataGradient");
  tubeVectorNorm->SetRadiusFactor(0.1);
  tubeVectorNorm->SetVaryRadiusToVaryRadiusByVectorNorm();
  tubeVectorNorm->Update();

  vtkNew<vtkPolyDataMapper> mapperVectorNorm;
  mapperVectorNorm->SetInputData(tubeVectorNorm->GetOutput());

  vtkNew<vtkActor> actorVectorNorm;
  actorVectorNorm->SetMapper(mapperVectorNorm);

  vtkNew<vtkRenderer> renderer;

  vtkNew<vtkRenderWindow> renderWindow;

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;

  renderer->AddActor(actorScalar);
  renderer->AddActor(actorVector);
  renderer->AddActor(actorVectorNorm);
  renderer->SetBackground(0.5, 0.5, 0.5);

  renderWindow->AddRenderer(renderer);
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->ResetCamera();
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return retVal ? EXIT_SUCCESS : EXIT_FAILURE;
}
