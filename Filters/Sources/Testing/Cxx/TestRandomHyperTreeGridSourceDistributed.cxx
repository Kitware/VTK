// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRandomHyperTreeGridSource.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeRenderManager.h"
#include "vtkHyperTreeGridGeometry.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"

#include <sstream>
double colors[8][3] = { { 1.0, 1.0, 1.0 }, { 0.0, 1.0, 1.0 }, { 1.0, 0.0, 1.0 }, { 1.0, 1.0, 0.0 },
  { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 0.7, 0.3, 0.3 } };

int TestRandomHyperTreeGridSourceDistributed(int argc, char* argv[])
{

  // Setup mpi
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);

  const int myId = controller->GetLocalProcessId();
  const int numProcs = controller->GetNumberOfProcesses();
  std::string threadName = "rank-" + std::to_string(controller->GetLocalProcessId());
  threadName += "\n";
  std::cout << threadName << endl;
  vtkLogger::SetThreadName(threadName);
  //=======================================================

  // Initialize logger.
  vtkLogger::Init(argc, argv);
  vtkLogIfF(INFO, controller->GetLocalProcessId() == 0, "total num-ranks=%d", numProcs);

  // Setup parallel rendering
  vtkNew<vtkCompositeRenderManager> prm;
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::Take(prm->MakeRenderer());
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::Take(prm->MakeRenderWindow());
  renWin->AddRenderer(renderer);
  renWin->DoubleBufferOn();
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  prm->SetRenderWindow(renWin);
  prm->SetController(controller);

  // Setup source
  int result = 1;
  float maskedFraction = 1.0 / (2.0 * float(numProcs));
  vtkNew<vtkRandomHyperTreeGridSource> source;
  source->SetDimensions(5, 5, 2); // GridCell 4, 4, 1
  source->SetSeed(371399);
  source->SetSplitFraction(0.25);
  source->SetMaskedFraction(maskedFraction);
  source->Update();

  if (source->GetActualMaskedCellFraction() > maskedFraction)
  {
    std::cout << "The masked cell proportion is " << source->GetActualMaskedCellFraction()
              << " and it should be less or equal than " << maskedFraction << std::endl;
    result = 0;
  }

  vtkNew<vtkHyperTreeGridGeometry> geom;
  geom->SetInputConnection(source->GetOutputPort());

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(geom->GetOutputPort());
  mapper->SetPiece(myId);
  mapper->SetNumberOfPieces(numProcs);
  mapper->SelectColorArray("Piece");
  mapper->SetScalarRange(0, numProcs - 1);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToSurface();
  actor->GetProperty()->EdgeVisibilityOn();
  actor->GetProperty()->SetColor(colors[myId]);

  renderer->AddActor(actor);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(1.3);
  if (myId == 0)
  {
    prm->ResetAllCameras();
    renderer->GetActiveCamera()->SetPosition(50.0, 40.0, 30.0);
    renderer->GetActiveCamera()->SetFocalPoint(0.0, 0.0, 0.0);
    renderer->ResetCameraClippingRange();

    renWin->Render();
    result = vtkRegressionTester::Test(argc, argv, renWin, 10);
    if (result == vtkRegressionTester::DO_INTERACTOR)
    {
      prm->StartInteractor();
    }
    controller->TriggerBreakRMIs();
  }
  else
  {
    prm->StartServices();
  }
  controller->Barrier();
  controller->Broadcast(&result, 1, 0);

  controller->Finalize();
  return result == 1 ? EXIT_SUCCESS : EXIT_FAILURE;
}
