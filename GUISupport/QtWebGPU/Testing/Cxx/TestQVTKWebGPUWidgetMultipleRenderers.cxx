// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests QVTKWebGPUWidget with multiple renderers in different viewports.

#include "QVTKWebGPUWidget.h"

#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRendererCollection.h"
#include "vtkSphereSource.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

#include <QApplication>
#include <QEventLoop>
#include <QTimer>

#include <iostream>

namespace
{
void process_events_and_wait(int msec)
{
  QEventLoop loop;
  QTimer::singleShot(msec, &loop, SLOT(quit()));
  loop.exec();
}
}

int TestQVTKWebGPUWidgetMultipleRenderers(int argc, char* argv[])
{
  QApplication app(argc, argv);

  // Create the QVTKWebGPUWidget
  QVTKWebGPUWidget widget;
  widget.setWindowTitle("TestQVTKWebGPUWidgetMultipleRenderers");

  vtkWebGPURenderWindow* renWin = widget.renderWindow();
  if (!renWin)
  {
    std::cerr << "Failed to get render window from QVTKWebGPUWidget" << std::endl;
    return EXIT_FAILURE;
  }

  // Create left renderer with a sphere (left half of window)
  vtkNew<vtkWebGPURenderer> leftRenderer;
  leftRenderer->SetViewport(0.0, 0.0, 0.5, 1.0);
  leftRenderer->SetBackground(0.1, 0.1, 0.4);
  renWin->AddRenderer(leftRenderer);

  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius(1.0);
  sphere->SetThetaResolution(32);
  sphere->SetPhiResolution(32);

  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper);
  leftRenderer->AddActor(sphereActor);
  leftRenderer->ResetCamera();

  // Create right renderer with a cone (right half of window)
  vtkNew<vtkWebGPURenderer> rightRenderer;
  rightRenderer->SetViewport(0.5, 0.0, 1.0, 1.0);
  rightRenderer->SetBackground(0.4, 0.1, 0.1);
  renWin->AddRenderer(rightRenderer);

  vtkNew<vtkConeSource> cone;
  cone->SetRadius(1.0);
  cone->SetHeight(2.0);
  cone->SetResolution(32);

  vtkNew<vtkPolyDataMapper> coneMapper;
  coneMapper->SetInputConnection(cone->GetOutputPort());

  vtkNew<vtkActor> coneActor;
  coneActor->SetMapper(coneMapper);
  rightRenderer->AddActor(coneActor);
  rightRenderer->ResetCamera();

  // Show the widget
  widget.resize(400, 300);
  widget.show();
  process_events_and_wait(500);

  // Verify we have 2 renderers in the render window
  if (renWin->GetRenderers()->GetNumberOfItems() != 2)
  {
    std::cerr << "Expected 2 renderers, got " << renWin->GetRenderers()->GetNumberOfItems()
              << std::endl;
    return EXIT_FAILURE;
  }

  // Verify the render window has a valid size
  const int* windowSize = renWin->GetSize();
  if (windowSize[0] <= 0 || windowSize[1] <= 0)
  {
    std::cerr << "Invalid render window size: " << windowSize[0] << "x" << windowSize[1]
              << std::endl;
    return EXIT_FAILURE;
  }

  // Render and verify no errors
  renWin->Render();
  process_events_and_wait(500);

  // Verify viewports are set correctly
  double leftVP[4];
  leftRenderer->GetViewport(leftVP);
  if (leftVP[0] != 0.0 || leftVP[1] != 0.0 || leftVP[2] != 0.5 || leftVP[3] != 1.0)
  {
    std::cerr << "Left renderer viewport is incorrect: " << leftVP[0] << ", " << leftVP[1] << ", "
              << leftVP[2] << ", " << leftVP[3] << std::endl;
    return EXIT_FAILURE;
  }

  double rightVP[4];
  rightRenderer->GetViewport(rightVP);
  if (rightVP[0] != 0.5 || rightVP[1] != 0.0 || rightVP[2] != 1.0 || rightVP[3] != 1.0)
  {
    std::cerr << "Right renderer viewport is incorrect: " << rightVP[0] << ", " << rightVP[1]
              << ", " << rightVP[2] << ", " << rightVP[3] << std::endl;
    return EXIT_FAILURE;
  }

  // Verify each renderer has exactly one actor
  if (leftRenderer->VisibleActorCount() != 1)
  {
    std::cerr << "Left renderer should have 1 visible actor, got "
              << leftRenderer->VisibleActorCount() << std::endl;
    return EXIT_FAILURE;
  }

  if (rightRenderer->VisibleActorCount() != 1)
  {
    std::cerr << "Right renderer should have 1 visible actor, got "
              << rightRenderer->VisibleActorCount() << std::endl;
    return EXIT_FAILURE;
  }

  // Verify interactor is accessible
  if (!widget.interactor())
  {
    std::cerr << "Widget interactor is null" << std::endl;
    return EXIT_FAILURE;
  }

  // Test removing a renderer
  renWin->RemoveRenderer(rightRenderer);
  if (renWin->GetRenderers()->GetNumberOfItems() != 1)
  {
    std::cerr << "Expected 1 renderer after removal, got "
              << renWin->GetRenderers()->GetNumberOfItems() << std::endl;
    return EXIT_FAILURE;
  }

  renWin->Render();
  process_events_and_wait(500);

  // Re-add renderer
  renWin->AddRenderer(rightRenderer);
  if (renWin->GetRenderers()->GetNumberOfItems() != 2)
  {
    std::cerr << "Expected 2 renderers after re-adding, got "
              << renWin->GetRenderers()->GetNumberOfItems() << std::endl;
    return EXIT_FAILURE;
  }

  renWin->Render();
  process_events_and_wait(500);

  std::cout << "All multiple renderers tests passed." << std::endl;
  return EXIT_SUCCESS;
}
