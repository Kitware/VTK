// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkInteractorStyleManipulator.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollectionSource.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTesting.h"
#include "vtkTexture.h"
#include "vtkTrackballRotate.h"

#include "InteractorStyleTestUtils.h"

#include <cstdlib>

int TestTrackballRotate(int argc, char* argv[])
{
  vtkNew<vtkPartitionedDataSetCollectionSource> source;
  source->SetNumberOfShapes(1);
  vtkNew<vtkCompositePolyDataMapper> mapper;
  mapper->SetInputConnection(source->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);

  vtkNew<vtkInteractorStyleManipulator> style;
  vtkNew<vtkTrackballRotate> manipulator;
  style->AddManipulator(manipulator);
  interactor->SetInteractorStyle(style);

  renderWindow->Render();

  bool success = true;
  auto* camera = renderer->GetActiveCamera();
  std::array<double, 3> expectedFocalPoint, expectedPosition, expectedViewUp;
  std::array<double, 3> actualFocalPoint, actualPosition, actualViewUp;

  interactor->SetEventInformation(150, 150, 0, 0, 0, 0);
  interactor->InvokeEvent(vtkCommand::LeftButtonPressEvent);
  for (int i = 150; i >= 0; i -= 10)
  {
    interactor->SetEventInformation(i, 150, 0, 0, 0, 0);
    interactor->InvokeEvent(vtkCommand::MouseMoveEvent);
  }
  interactor->InvokeEvent(vtkCommand::LeftButtonReleaseEvent);
  renderWindow->Render();

  expectedFocalPoint = { -0.150135, -0.00152028, -0.481953 };
  camera->GetFocalPoint(actualFocalPoint.data());
  success &= assertTuplesEqual(expectedFocalPoint, actualFocalPoint, "focal point after rotate");
  expectedPosition = { -0.150135, -0.00152028, -6.27176 };
  camera->GetPosition(actualPosition.data());
  success &= assertTuplesEqual(expectedPosition, actualPosition, "position after rotate");
  expectedViewUp = { 0.0, 1.0, 0.0 };
  camera->GetViewUp(actualViewUp.data());
  success &= assertTuplesEqual(expectedViewUp, actualViewUp, "view up after rotate");

  // Exercise a non-zero center of rotation. The drag below azimuths the camera
  // by 360 * 150 / 300 = 180 degrees about the view-up axis (0, 1, 0) passing
  // through the center, so the camera position and focal point must end up
  // mirrored about the center in the x-z plane.
  const std::array<double, 3> center = { 0.5, 0.25, -0.5 };
  style->SetCenterOfRotation(center[0], center[1], center[2]);

  std::array<double, 3> positionBefore, focalPointBefore;
  camera->GetPosition(positionBefore.data());
  camera->GetFocalPoint(focalPointBefore.data());

  interactor->SetEventInformation(150, 150, 0, 0, 0, 0);
  interactor->InvokeEvent(vtkCommand::LeftButtonPressEvent);
  for (int i = 150; i >= 0; i -= 10)
  {
    interactor->SetEventInformation(i, 150, 0, 0, 0, 0);
    interactor->InvokeEvent(vtkCommand::MouseMoveEvent);
  }
  interactor->InvokeEvent(vtkCommand::LeftButtonReleaseEvent);
  renderWindow->Render();

  expectedPosition = { 2.0 * center[0] - positionBefore[0], positionBefore[1],
    2.0 * center[2] - positionBefore[2] };
  camera->GetPosition(actualPosition.data());
  success &= assertTuplesEqual(
    expectedPosition, actualPosition, "position after rotate about non-zero center");
  expectedFocalPoint = { 2.0 * center[0] - focalPointBefore[0], focalPointBefore[1],
    2.0 * center[2] - focalPointBefore[2] };
  camera->GetFocalPoint(actualFocalPoint.data());
  success &= assertTuplesEqual(
    expectedFocalPoint, actualFocalPoint, "focal point after rotate about non-zero center");
  expectedViewUp = { 0.0, 1.0, 0.0 };
  camera->GetViewUp(actualViewUp.data());
  success &=
    assertTuplesEqual(expectedViewUp, actualViewUp, "view up after rotate about non-zero center");

  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);
  if (testing->IsInteractiveModeSpecified())
  {
    interactor->Start();
  }
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
