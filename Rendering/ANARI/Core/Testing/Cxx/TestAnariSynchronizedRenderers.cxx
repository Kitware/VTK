// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <cstdlib>

#include "vtkActor.h"
#include "vtkAnariDevice.h"
#include "vtkAnariRenderWindow.h"
#include "vtkAnariRenderer.h"
#include "vtkCamera.h"
#include "vtkCommunicator.h"
#include "vtkMPIController.h"
#include "vtkMathUtilities.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPolyDataMapper.h"
#include "vtkSphereSource.h"
#include "vtkSynchronizedRenderers.h"
#include "vtkTesting.h"
#include "vtkWindowToImageFilter.h"

int TestAnariSynchronizedRenderers(int argc, char* argv[])
{
  vtkMPIController* controller = vtkMPIController::New();
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);

  const int rank = controller->GetLocalProcessId();
  const int numRanks = controller->GetNumberOfProcesses();

  vtkNew<vtkAnariRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);

  vtkNew<vtkOpenGLRenderer> renderer;
  renderer->SetBackground(0.2, 0.2, 0.2);
  renderer->SetBackgroundAlpha(1.0);
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius(5.0);
  sphere->SetPhiResolution(32);
  sphere->SetThetaResolution(32);
  sphere->UpdatePiece(rank, numRanks, 0);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  vtkNew<vtkSynchronizedRenderers> synchronizedRenderers;
  synchronizedRenderers->SetRenderer(renderer);
  synchronizedRenderers->SetParallelController(controller);
  synchronizedRenderers->SetRootProcessId(0);
  synchronizedRenderers->SetParallelRendering(true);
  synchronizedRenderers->WriteBackImagesOff();
  synchronizedRenderers->FixBackgroundOff();

  // Barney wants 'mpi' subdevice, helide does not care
  renderWindow->GetAnariDevice()->SetupAnariDeviceFromLibrary("environment", "mpi", false);

  // Parameters for raytracing back-ends
  renderWindow->GetAnariRenderer()->SetParameterf("ambientRadiance", 1.0f);
  renderWindow->GetAnariRenderer()->SetParameteri("pixelSamples", 80);
  renderWindow->GetAnariRenderer()->SetParameterb("denoise", true);

  std::array<double, 3> camPosRank0 = { 1.0, 2.0, 20.0 };
  std::array<double, 3> expectedFocalPoint = { 0.0, 0.0, 0.0 };
  vtkCamera* camera = renderer->GetActiveCamera();
  if (rank == 0)
  {
    camera->SetPosition(camPosRank0.data());
  }
  else
  {
    camera->SetPosition(-20.0, 4.0, 1.0);
  }
  camera->SetFocalPoint(expectedFocalPoint.data());
  camera->SetViewUp(0.0, 1.0, 0.0);
  camera->SetClippingRange(0.1, 100.0);

  renderWindow->Render();

  // SyncRenderers should make sure that camera positions are coherent
  std::array<double, 3> camPos;
  camera->GetPosition(camPos.data());
  int camMatch = 1;
  if (!vtkMathUtilities::FuzzyCompare(camPos[0], camPosRank0[0]) ||
    !vtkMathUtilities::FuzzyCompare(camPos[1], camPosRank0[1]) ||
    !vtkMathUtilities::FuzzyCompare(camPos[2], camPosRank0[2]))
  {
    camMatch = 0;
  }

  int allCamMatch = 1;
  controller->AllReduce(&camMatch, &allCamMatch, 1, vtkCommunicator::MIN_OP);
  if (allCamMatch == 0)
  {
    vtkLog(ERROR, "Inconsistent camera positions between ranks");
    controller->Finalize();
    controller->Delete();
    return EXIT_FAILURE;
  }

  int imageResult = vtkTesting::NOT_RUN;
  if (rank == 0)
  {
    vtkNew<vtkWindowToImageFilter> capture;
    capture->SetInput(renderWindow);
    capture->SetShouldRerender(false);
    capture->ReadFrontBufferOff();
    capture->Update();

    // Helide is not MPI-aware, so technically this only really relevant with Barney.
    vtkNew<vtkTesting> testing;
    for (int cc = 0; cc < argc; ++cc)
    {
      testing->AddArgument(argv[cc]);
    }
    if (testing->IsValidImageSpecified())
    {
      imageResult = testing->RegressionTest(capture, 10.0);
    }
  }
  controller->Broadcast(&imageResult, 1, 0);

  controller->Finalize();
  controller->Delete();
  return imageResult == vtkTesting::PASSED ? EXIT_SUCCESS : EXIT_FAILURE;
}
