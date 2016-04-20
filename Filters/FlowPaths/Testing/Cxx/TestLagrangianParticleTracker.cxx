/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLagrangianParticleTracker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLagrangianMatidaIntegrationModel.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDataSetMapper.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDoubleArray.h"
#include "vtkGlyph3D.h"
#include "vtkImageData.h"
#include "vtkImageDataToPointSet.h"
#include "vtkLagrangianParticleTracker.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataGroupFilter.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPointSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRungeKutta2.h"
#include "vtkSphereSource.h"

int TestLagrangianParticleTracker(int, char*[])
{
  // Create a point source
  vtkNew<vtkPointSource> seeds;
  seeds->SetNumberOfPoints(10);
  seeds->SetRadius(4);
  seeds->Update();
  vtkPolyData* seedPD = seeds->GetOutput();
  vtkPointData* seedData = seedPD->GetPointData();

  // Create seed data
  vtkNew<vtkDoubleArray> partVel;
  partVel->SetNumberOfComponents(3);
  partVel->SetNumberOfTuples(seedPD->GetNumberOfPoints());
  partVel->SetName("InitialVelocity");

  vtkNew<vtkDoubleArray> partDens;
  partDens->SetNumberOfComponents(1);
  partDens->SetNumberOfTuples(seedPD->GetNumberOfPoints());
  partDens->SetName("ParticleDensity");

  vtkNew<vtkDoubleArray> partDiam;
  partDiam->SetNumberOfComponents(1);
  partDiam->SetNumberOfTuples(seedPD->GetNumberOfPoints());
  partDiam->SetName("ParticleDiameter");

  partVel->FillComponent(0, 2);
  partVel->FillComponent(1, 5);
  partVel->FillComponent(2, 1);
  partDens->FillComponent(0, 1920);
  partDiam->FillComponent(0, 0.1);

  seedData->AddArray(partVel.Get());
  seedData->AddArray(partDens.Get());
  seedData->AddArray(partDiam.Get());

  // Create a wavelet
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->Update();
  vtkImageData* waveletImg = wavelet->GetOutput();

  vtkCellData* cd = waveletImg->GetCellData();

  // Create flow data
  vtkNew<vtkDoubleArray> flowVel;
  flowVel->SetNumberOfComponents(3);
  flowVel->SetNumberOfTuples(waveletImg->GetNumberOfCells());
  flowVel->SetName("FlowVelocity");

  vtkNew<vtkDoubleArray> flowDens ;
  flowDens->SetNumberOfComponents(1);
  flowDens->SetNumberOfTuples(waveletImg->GetNumberOfCells());
  flowDens->SetName("FlowDensity");

  vtkNew<vtkDoubleArray> flowDynVisc ;
  flowDynVisc->SetNumberOfComponents(1);
  flowDynVisc->SetNumberOfTuples(waveletImg->GetNumberOfCells());
  flowDynVisc->SetName("FlowDynamicViscosity");

  flowVel->FillComponent(0, -0.3);
  flowVel->FillComponent(1, -0.3);
  flowVel->FillComponent(2, -0.3);
  flowDens->FillComponent(0, 1000);
  flowDynVisc->FillComponent(0, 0.894);

  cd->AddArray(flowVel.Get());
  cd->AddArray(flowDens.Get());
  cd->AddArray(flowDynVisc.Get());

  // Create surface
  vtkNew<vtkDataSetSurfaceFilter> surface;
  surface->SetInputConnection(wavelet->GetOutputPort());
  surface->Update();
  vtkPolyData* surfacePd = surface->GetOutput();

  // Create Surface data
  vtkNew<vtkDoubleArray> surfaceTypeTerm;
  surfaceTypeTerm->SetNumberOfComponents(1);
  surfaceTypeTerm->SetName("SurfaceType");
  surfaceTypeTerm->SetNumberOfTuples(surfacePd->GetNumberOfCells());
  surfaceTypeTerm->FillComponent(0,
    vtkLagrangianBasicIntegrationModel::SURFACE_TYPE_TERM);
  surfacePd->GetCellData()->AddArray(surfaceTypeTerm.Get());

  // Create plane passThrough
  vtkNew<vtkPlaneSource> surfacePass;
  surfacePass->SetOrigin(-10, -10, 0);
  surfacePass->SetPoint1(10, -10, 0);
  surfacePass->SetPoint2(-10, 10, 0);
  surfacePass->Update();
  vtkPolyData* passPd = surfacePass->GetOutput();

  // Create Surface data
  vtkNew<vtkDoubleArray> surfaceTypePass;
  surfaceTypePass->SetNumberOfComponents(1);
  surfaceTypePass->SetName("SurfaceType");
  surfaceTypePass->SetNumberOfTuples(passPd->GetNumberOfCells());
  surfaceTypePass->FillComponent(0,
    vtkLagrangianBasicIntegrationModel::SURFACE_TYPE_PASS);
  passPd->GetCellData()->AddArray(surfaceTypePass.Get());

  // Create plane passThrough
  vtkNew<vtkPlaneSource> surfaceBounce;
  surfaceBounce->SetOrigin(-2, -2, -2);
  surfaceBounce->SetPoint1(5, -2, -2);
  surfaceBounce->SetPoint2(-2, 5, -2);
  surfaceBounce->Update();
  vtkPolyData* bouncePd = surfaceBounce->GetOutput();

  // Create Surface data
  vtkNew<vtkDoubleArray> surfaceTypeBounce;
  surfaceTypeBounce->SetNumberOfComponents(1);
  surfaceTypeBounce->SetName("SurfaceType");
  surfaceTypeBounce->SetNumberOfTuples(bouncePd->GetNumberOfCells());
  surfaceTypeBounce->FillComponent(0,
    vtkLagrangianBasicIntegrationModel::SURFACE_TYPE_BOUNCE);
  bouncePd->GetCellData()->AddArray(surfaceTypeBounce.Get());

  vtkNew<vtkMultiBlockDataGroupFilter> groupSurface;
  groupSurface->AddInputDataObject(surfacePd);
  groupSurface->AddInputDataObject(passPd);
  groupSurface->AddInputDataObject(bouncePd);

  vtkNew<vtkMultiBlockDataGroupFilter> groupFlow;
  groupFlow->AddInputDataObject(waveletImg);

  vtkNew<vtkImageDataToPointSet> ugFlow;
  ugFlow->AddInputData(waveletImg);

  vtkNew<vtkMultiBlockDataGroupFilter> groupSeed;
  groupSeed->AddInputDataObject(seedPD);
  groupSeed->AddInputDataObject(seedPD);

  // Create Integrator
  vtkNew<vtkRungeKutta2> integrator;

  // Create Integration Model
  vtkNew<vtkLagrangianMatidaIntegrationModel> integrationModel;
  integrationModel->SetInputArrayToProcess(0, 1, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, "InitialVelocity");
  integrationModel->SetInputArrayToProcess(2, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "SurfaceType");
  integrationModel->SetInputArrayToProcess(3, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "FlowVelocity");
  integrationModel->SetInputArrayToProcess(4, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "FlowDensity");
  integrationModel->SetInputArrayToProcess(5, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "FlowDynamicViscosity");
  integrationModel->SetInputArrayToProcess(6, 1, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, "ParticleDiameter");
  integrationModel->SetInputArrayToProcess(7, 1, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, "ParticleDensity");

  // Put in tracker
  vtkNew<vtkLagrangianParticleTracker> tracker;
  tracker->SetIntegrator(NULL);
  tracker->SetIntegrationModel(NULL);
  tracker->Print(cout);
  if (tracker->GetSource() != 0 || tracker->GetSurface() != 0)
  {
    std::cerr << "Incorrect Input Initialization" << std::endl;
    return EXIT_FAILURE;
  }
  tracker->SetIntegrator(integrator.Get());
  if (tracker->GetIntegrator() != integrator.Get())
  {
    std::cerr << "Incorrect Integrator" << std::endl;
    return EXIT_FAILURE;
  }

  tracker->SetIntegrationModel(integrationModel.Get());
  if (tracker->GetIntegrationModel() != integrationModel.Get())
  {
    std::cerr << "Incorrect Integration Model" << std::endl;
    return EXIT_FAILURE;
  }

  tracker->SetInputConnection(groupFlow->GetOutputPort());
  tracker->SetStepFactor(0.1);
  tracker->SetStepFactorMin(0.1);
  tracker->SetStepFactorMax(0.1);
  tracker->SetMaximumNumberOfSteps(150);
  tracker->SetSourceConnection(groupSeed->GetOutputPort());
  tracker->SetSurfaceData(surfacePd);
  tracker->SetCellLengthComputationMode(
    vtkLagrangianParticleTracker::STEP_CUR_CELL_VEL_DIR);
  tracker->AdaptiveStepReintegrationOn();
  tracker->UseParticlePathsRenderingThresholdOn();
  tracker->SetParticlePathsRenderingPointsThreshold(100);
  tracker->CreateOutOfDomainParticleOn();
  tracker->Update();
  tracker->SetInputConnection(ugFlow->GetOutputPort());
  tracker->SetMaximumNumberOfSteps(30);
  tracker->SetCellLengthComputationMode(
    vtkLagrangianParticleTracker::STEP_CUR_CELL_DIV_THEO);
  tracker->Update();
  tracker->SetInputData(waveletImg);
  tracker->SetSourceData(seedPD);
  tracker->SetMaximumNumberOfSteps(300);
  tracker->SetSurfaceConnection(groupSurface->GetOutputPort());
  tracker->SetCellLengthComputationMode(
    vtkLagrangianParticleTracker::STEP_LAST_CELL_VEL_DIR);
  tracker->AdaptiveStepReintegrationOff();
  tracker->UseParticlePathsRenderingThresholdOff();
  tracker->CreateOutOfDomainParticleOff();
  tracker->Update();
  if (tracker->GetStepFactor() != 0.1)
  {
    std::cerr << "Incorrect StepFactor" << std::endl;
    return EXIT_FAILURE;
  }
  if (tracker->GetStepFactorMin() != 0.1)
  {
    std::cerr << "Incorrect StepFactorMin" << std::endl;
    return EXIT_FAILURE;
  }
  if (tracker->GetStepFactorMax() != 0.1)
  {
    std::cerr << "Incorrect StepFactorMax" << std::endl;
    return EXIT_FAILURE;
  }
  if (tracker->GetMaximumNumberOfSteps() != 300)
  {
    std::cerr << "Incorrect StepFactorMax" << std::endl;
    return EXIT_FAILURE;
  }
  if (tracker->GetCellLengthComputationMode() != vtkLagrangianParticleTracker::STEP_LAST_CELL_VEL_DIR)
  {
    std::cerr << "Incorrect CellLengthComputationMode" << std::endl;
    return EXIT_FAILURE;
  }
  if (tracker->GetAdaptiveStepReintegration())
  {
    std::cerr << "Incorrect AdaptiveStepReintegration" << std::endl;
    return EXIT_FAILURE;
  }
  if (tracker->GetUseParticlePathsRenderingThreshold())
  {
    std::cerr << "Incorrect UseParticlePathsRenderingThreshold" << std::endl;
    return EXIT_FAILURE;
  }
  if (tracker->GetParticlePathsRenderingPointsThreshold() != 100)
  {
    std::cerr << "Incorrect ParticlePathsRenderingThreshold" << std::endl;
    return EXIT_FAILURE;
  }
  if (tracker->GetCreateOutOfDomainParticle())
  {
    std::cerr << "Incorrect CreateOutOfDomainParticle" << std::endl;
    return EXIT_FAILURE;
  }
  tracker->Print(cout);
  if (tracker->GetSource() != seedPD)
  {
    std::cerr << "Incorrect Source" << std::endl;
    return EXIT_FAILURE;
  }
  if (tracker->GetSurface() != groupSurface->GetOutput())
  {
    std::cerr << "Incorrect Surface" << std::endl;
    return EXIT_FAILURE;
  }

  // Glyph for interaction points
  vtkNew<vtkSphereSource> sphereGlyph;
  sphereGlyph->SetRadius(0.1);

  vtkNew<vtkPoints> points;
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 1, 1);
  points->InsertNextPoint(2, 2, 2);
  vtkNew<vtkPolyData> polydata;
  polydata->SetPoints(points.Get());

  vtkNew<vtkGlyph3D> glyph;
  glyph->SetSourceConnection(sphereGlyph->GetOutputPort());
  vtkMultiBlockDataSet* mbInter = vtkMultiBlockDataSet::SafeDownCast(
    tracker->GetOutput(1));
  glyph->SetInputData(mbInter->GetBlock(1));

  // Setup actor and mapper
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(vtkPolyData::SafeDownCast(tracker->GetOutput()));

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.Get());

  vtkNew<vtkPolyDataMapper> surfaceMapper;
  surfaceMapper->SetInputConnection(surfaceBounce->GetOutputPort());
  vtkNew<vtkActor> surfaceActor;
  surfaceActor->SetMapper(surfaceMapper.Get());
  vtkNew<vtkPolyDataMapper> surfaceMapper2;
  surfaceMapper2->SetInputConnection(surfacePass->GetOutputPort());
  vtkNew<vtkActor> surfaceActor2;
  surfaceActor2->SetMapper(surfaceMapper2.Get());

  vtkNew<vtkPolyDataMapper> glyphMapper;
  glyphMapper->SetInputConnection(glyph->GetOutputPort());
  vtkNew<vtkActor> glyphActor;
  glyphActor->SetMapper(glyphMapper.Get());

  // Setup camera
  vtkNew<vtkCamera> camera;
  camera->SetFocalPoint(0, 0, -1);
  camera->SetViewUp(0, 0, 1);
  camera->SetPosition(0, -40, 0);

  // Setup render window, renderer, and interactor
  vtkNew<vtkRenderer> renderer;
  renderer->SetActiveCamera(camera.Get());
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer.Get());
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow.Get());
  renderer->AddActor(actor.Get());
  renderer->AddActor(surfaceActor.Get());
  renderer->AddActor(surfaceActor2.Get());
  renderer->AddActor(glyphActor.Get());
  renderer->SetBackground(0.1, .5, 1);

  renderWindow->Render();
  renderWindowInteractor->Start();
  return EXIT_SUCCESS;
}
