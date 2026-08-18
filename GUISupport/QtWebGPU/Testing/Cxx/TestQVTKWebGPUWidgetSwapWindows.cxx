// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests swapping QVTKWebGPUWidget between different parent widgets.

#include "QVTKWebGPUWidget.h"

#include "vtkNew.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

#include <QApplication>
#include <QBoxLayout>
#include <QEventLoop>
#include <QTimer>
#include <QWidget>

#include <iostream>

#include <vtkActor.h>
#include <vtkConeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkSphereSource.h>

namespace
{
void ProcessEventsAndWait(int msec)
{
  QEventLoop loop;
  QTimer::singleShot(msec, &loop, SLOT(quit()));
  loop.exec();
}
}

int TestQVTKWebGPUWidgetSwapWindows(int argc, char* argv[])
{
  QApplication app(argc, argv);

  // Set up frame with two horizontally stacked panels,
  // each containing a QVTKWebGPUWidget
  QWidget frame;
  frame.setWindowTitle("TestQVTKWebGPUWidgetSwapWindows");
  QHBoxLayout* layout = new QHBoxLayout(&frame);

  // Left panel with red background
  QWidget* leftPanel = new QWidget(&frame);
  QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
  QVTKWebGPUWidget* leftVTKWidget = new QVTKWebGPUWidget(leftPanel);

  vtkWebGPURenderWindow* leftRenWin = leftVTKWidget->renderWindow();
  if (!leftRenWin)
  {
    std::cerr << "Failed to get left render window" << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkWebGPURenderer> leftRenderer;
  leftRenderer->SetBackground(1.0, 0.0, 0.0); // Red
  leftRenWin->AddRenderer(leftRenderer);
  leftLayout->addWidget(leftVTKWidget);

  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper);
  leftRenderer->AddActor(sphereActor);
  leftRenderer->ResetCamera();

  // Right panel with green background
  QWidget* rightPanel = new QWidget(&frame);
  QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
  QVTKWebGPUWidget* rightVTKWidget = new QVTKWebGPUWidget(rightPanel);

  vtkWebGPURenderWindow* rightRenWin = rightVTKWidget->renderWindow();
  if (!rightRenWin)
  {
    std::cerr << "Failed to get right render window" << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkWebGPURenderer> rightRenderer;
  rightRenderer->SetBackground(0.0, 1.0, 0.0); // Green
  rightRenWin->AddRenderer(rightRenderer);
  rightLayout->addWidget(rightVTKWidget);

  vtkNew<vtkConeSource> cone;
  vtkNew<vtkPolyDataMapper> coneMapper;
  coneMapper->SetInputConnection(cone->GetOutputPort());
  vtkNew<vtkActor> coneActor;
  coneActor->SetMapper(coneMapper);
  rightRenderer->AddActor(coneActor);
  rightRenderer->ResetCamera();

  layout->addWidget(leftPanel);
  layout->addWidget(rightPanel);

  // Show frame and process events
  frame.resize(600, 300);
  frame.show();
  leftRenWin->Render();
  rightRenWin->Render();
  ProcessEventsAndWait(500);

  // Verify initial state
  std::cout << "Initial state - Left widget parent: " << leftVTKWidget->parentWidget()
            << ", Right widget parent: " << rightVTKWidget->parentWidget() << std::endl;

  // Swap QVTKWebGPUWidgets between panels
  rightLayout->removeWidget(rightVTKWidget);
  leftLayout->removeWidget(leftVTKWidget);

  rightVTKWidget->setParent(leftPanel);
  leftVTKWidget->setParent(rightPanel);

  rightLayout->addWidget(leftVTKWidget);
  leftLayout->addWidget(rightVTKWidget);

  // Process events again
  leftRenWin->Render();
  rightRenWin->Render();
  ProcessEventsAndWait(500);

  // Verify swapped state
  std::cout << "After swap - Left widget parent: " << leftVTKWidget->parentWidget()
            << ", Right widget parent: " << rightVTKWidget->parentWidget() << std::endl;

  // Verify the widgets are now in their new parents
  if (leftVTKWidget->parentWidget() != rightPanel)
  {
    std::cerr << "Left widget was not properly moved to right panel" << std::endl;
    return EXIT_FAILURE;
  }

  if (rightVTKWidget->parentWidget() != leftPanel)
  {
    std::cerr << "Right widget was not properly moved to left panel" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
