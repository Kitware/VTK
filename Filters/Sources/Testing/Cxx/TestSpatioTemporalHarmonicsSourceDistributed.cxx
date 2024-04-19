// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeRenderManager.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkImageData.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkPieceScalars.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSpatioTemporalHarmonicsSource.h"

int TestSpatioTemporalHarmonicsSourceDistributed(int argc, char* argv[])
{
  // Setup mpi
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);

  const int myId = controller->GetLocalProcessId();
  const int numProcs = controller->GetNumberOfProcesses();

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

  // Create source
  vtkNew<vtkSpatioTemporalHarmonicsSource> source;
  vtkNew<vtkDataSetSurfaceFilter> toPolyData;
  toPolyData->SetInputConnection(source->GetOutputPort());

  vtkNew<vtkPieceScalars> pieceScalars;
  pieceScalars->SetInputConnection(toPolyData->GetOutputPort());
  pieceScalars->SetScalarModeToCellData();

  // Execute pipeline and render
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(pieceScalars->GetOutputPort());
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SetPiece(myId);
  mapper->SetNumberOfPieces(numProcs);
  mapper->SelectColorArray("Piece");
  mapper->SetScalarRange(0, numProcs - 1);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  int retVal;
  if (myId == 0)
  {
    prm->ResetAllCameras();
    renderer->GetActiveCamera()->SetPosition(50.0, 40.0, 30.0);
    renderer->GetActiveCamera()->SetFocalPoint(0.0, 0.0, 0.0);
    renderer->ResetCameraClippingRange();

    renWin->Render();
    retVal = vtkRegressionTester::Test(argc, argv, renWin, 10);
    if (retVal == vtkRegressionTester::DO_INTERACTOR)
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
  controller->Broadcast(&retVal, 1, 0);

  controller->Finalize();

  return !retVal;
}
