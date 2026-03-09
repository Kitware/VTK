// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests QVTKWebGPUWidget

#include "QVTKWebGPUWidget.h"

#include "vtkActor.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTesting.h"
#include "vtkWebGPURenderWindow.h"

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

int TestQVTKWebGPUWidget(int argc, char* argv[])
{
  QApplication app(argc, argv);

  vtkNew<vtkTesting> vtktesting;
  vtktesting->AddArguments(argc, argv);

  // Create the QVTKWebGPUWidget
  QVTKWebGPUWidget widget;
  widget.setWindowTitle("TestQVTKWebGPUWidget");

  // Get the render window
  vtkWebGPURenderWindow* renderWindow = widget.renderWindow();
  if (!renderWindow)
  {
    std::cerr << "Failed to get render window from QVTKWebGPUWidget" << std::endl;
    return EXIT_FAILURE;
  }

  // Create a renderer
  vtkNew<vtkRenderer> renderer;
  renderer->SetGradientBackground(true);
  renderer->SetBackground(0.2, 0.2, 0.2);
  renderer->SetBackground2(0.7, 0.7, 0.7);
  renderWindow->AddRenderer(renderer);

  // Create a simple sphere
  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius(1.0);
  sphere->SetThetaResolution(32);
  sphere->SetPhiResolution(32);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  // Show the widget
  widget.resize(300, 300);
  widget.show();

  // Process events and let the widget render
  process_events_and_wait(1000);

  // Verify window size
  const int* windowSize = renderWindow->GetSize();
  if (windowSize[0] <= 0 || windowSize[1] <= 0)
  {
    std::cerr << "Invalid render window size: " << windowSize[0] << "x" << windowSize[1]
              << std::endl;
    return EXIT_FAILURE;
  }

  // Verify screen size
  const int* screenSize = renderWindow->GetScreenSize();
  if (screenSize[0] < windowSize[0] || screenSize[1] < windowSize[1])
  {
    std::cout << "Expected vtkWebGPURenderWindow::GetScreenSize() "
                 "dimensions to be larger than the render window size"
              << std::endl;
    return EXIT_FAILURE;
  }

  // Perform regression test
  vtktesting->SetRenderWindow(renderWindow);

  int retVal = vtktesting->RegressionTest(0.05);
  switch (retVal)
  {
    case vtkTesting::DO_INTERACTOR:
      return QApplication::exec();
    case vtkTesting::FAILED:
    case vtkTesting::NOT_RUN:
      return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
