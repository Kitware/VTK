/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPLagrangianParticleTracker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkCompositeRenderManager.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkLagrangianMatidaIntegrationModel.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkOutlineFilter.h"
#include "vtkPLagrangianParticleTracker.h"
#include "vtkPointData.h"
#include "vtkPointSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRungeKutta2.h"
#include "vtkTrivialProducer.h"

struct PLagrangianParticleTrackerArgs_tmp
{
  int* retVal;
  int argc;
  char** argv;
};

// This will be called by all processes
void MainPLagrangianParticleTracker(vtkMultiProcessController *controller, void *arg)
{
  PLagrangianParticleTrackerArgs_tmp* args =
    reinterpret_cast<PLagrangianParticleTrackerArgs_tmp*>(arg);

  int myId = controller->GetLocalProcessId();
  int numProcs = controller->GetNumberOfProcesses();

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

  // Create seeds with point source
  vtkNew<vtkPointSource> seeds;
  seeds->SetNumberOfPoints(10);
  seeds->SetRadius(4);
  seeds->Update();
  vtkPolyData* seedPD = seeds->GetOutput();
  vtkPointData* seedData = seedPD->GetPointData();

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

  // Create input (flow) from wavelet
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->UpdateInformation();
  wavelet->UpdatePiece(myId, numProcs, 0);
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

  // Create input outline
  vtkNew<vtkOutlineFilter> outline;
  outline->SetInputData(waveletImg);

  vtkNew<vtkPolyDataMapper> outlineMapper;
  outlineMapper->SetInputConnection(outline->GetOutputPort());
  outlineMapper->SetImmediateModeRendering(1);
  outlineMapper->UseLookupTableScalarRangeOn();
  outlineMapper->SetScalarVisibility(0);
  outlineMapper->SetScalarModeToDefault();

  vtkNew<vtkActor> outlineActor;
  outlineActor->SetMapper(outlineMapper.Get());
  renderer->AddActor(outlineActor.Get());

  // Create Integrator
  vtkNew<vtkRungeKutta2> integrator;

  // Create Integration Model
  vtkNew<vtkLagrangianMatidaIntegrationModel> integrationModel;
  integrationModel->SetInputArrayToProcess(0, 1, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, "InitialVelocity");
  integrationModel->SetInputArrayToProcess(2, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_CELLS, "");
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
  tracker->SetIntegrator(integrator.Get());
  tracker->SetIntegrationModel(integrationModel.Get());
  tracker->SetInputData(waveletImg);
  tracker->SetStepFactor(0.1);
  tracker->SetSourceData(seedPD);
  // Show tracker result
  vtkNew<vtkPolyDataMapper> trackerMapper;
  trackerMapper->SetInputConnection(tracker->GetOutputPort());
  vtkNew<vtkActor> trackerActor;
  trackerActor->SetMapper(trackerMapper.Get());
  renderer->AddActor(trackerActor.Get());

  // Check result
  vtkNew<vtkCompositeRenderManager> compManager;
  compManager->SetRenderWindow(renderWindow.Get());
  compManager->SetController(controller);
  compManager->InitializePieces();

  if (myId)
  {
    compManager->InitializeRMIs();
    controller->ProcessRMIs();
    controller->Receive(args->retVal, 1, 0, 33);
  }
  else
  {
    renderWindow->Render();
    *(args->retVal) =
      vtkRegressionTester::Test(args->argc, args->argv, renderWindow.Get(), 10);
    for (int i = 1; i < numProcs; i++)
    {
      controller->TriggerRMI(i, vtkMultiProcessController::BREAK_RMI_TAG);
      controller->Send(args->retVal, 1, i, 33);
    }
  }

  if (*(args->retVal) == vtkRegressionTester::DO_INTERACTOR)
  {
    compManager->StartInteractor();
  }
}

int TestPLagrangianParticleTracker(int argc, char* argv[])
{
  vtkNew<vtkMPIController> contr;
  contr->Initialize(&argc, &argv);

  // When using MPI, the number of processes is determined
  // by the external program which launches this application.
  // However, when using threads, we need to set it ourselves.
  if (contr->IsA("vtkThreadedController"))
  {
    // Set the number of processes to 2 for this example.
    contr->SetNumberOfProcesses(2);
  }

  // Added for regression test.
  // ----------------------------------------------
  int retVal;
  PLagrangianParticleTrackerArgs_tmp args;
  args.retVal = &retVal;
  args.argc = argc;
  args.argv = argv;
  // ----------------------------------------------

  contr->SetSingleMethod(MainPLagrangianParticleTracker, &args);
  contr->SingleMethodExecute();

  contr->Finalize();

  return !retVal;
}
