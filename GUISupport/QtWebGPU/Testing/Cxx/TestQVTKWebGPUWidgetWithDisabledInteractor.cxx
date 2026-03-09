// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests QVTKWebGPUWidget with a vtkRenderWindowInteractor that has its
// EnableRender flag disabled.

#include "QVTKWebGPUWidget.h"

#include "vtkActor.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTesting.h"
#include "vtkWebGPURenderWindow.h"

#include <QApplication>
#include <QEventLoop>
#include <QImage>
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

int TestQVTKWebGPUWidgetWithDisabledInteractor(int argc, char* argv[])
{
  QApplication app(argc, argv);

  auto vtktesting = vtkSmartPointer<vtkTesting>::New();
  vtktesting->AddArguments(argc, argv);

  // Create the QVTKWebGPUWidget
  QVTKWebGPUWidget widget;
  widget.setWindowTitle("TestQVTKWebGPUWidgetWithDisabledInteractor");

  vtkWebGPURenderWindow* renWin = widget.renderWindow();
  if (!renWin)
  {
    std::cerr << "Failed to get render window from QVTKWebGPUWidget" << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkRenderer> ren;
  ren->GradientBackgroundOn();
  ren->SetBackground(0.2, 0.2, 0.2);
  ren->SetBackground2(0.7, 0.7, 0.7);
  renWin->AddRenderer(ren);
  renWin->Render();

  widget.resize(100, 100);
  widget.show();
  process_events_and_wait(500);

  // Set interactor to not call Render() on the vtkRenderWindow. Clients might
  // set this to enforce a specified framerate by rendering only when a timer
  // fires, for example.
  renWin->GetInteractor()->EnableRenderOff();

  vtkNew<vtkSphereSource> source;
  source->SetRadius(1.0);
  source->SetThetaResolution(32);
  source->SetPhiResolution(32);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(source->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  ren->AddActor(actor);
  ren->ResetCamera();
  renWin->Render(); // this will render a sphere at 100x100.

  // Resize widget. This should NOT retrigger a VTK render since
  // the interactor is disabled. We should still see the rendering result from
  // earlier.
  widget.resize(300, 300);
  widget.show();
  process_events_and_wait(500);

  // Get output image filename
  const std::string tempDir(vtktesting->GetTempDirectory());
  std::string fileName(vtktesting->GetValidImageFileName());
  auto slashPos = fileName.rfind('/');
  if (slashPos != std::string::npos)
  {
    fileName = fileName.substr(slashPos + 1);
  }
  fileName = tempDir + '/' + fileName;

  // Capture widget using Qt. Don't use vtkTesting to capture the image, because
  // this should test what the widget displays, not what VTK renders.
  const QImage image = widget.grab().toImage();
  if (!image.save(QString::fromStdString(fileName)))
  {
    std::cout << "ERROR: Saving image failed" << std::endl;
    return EXIT_FAILURE;
  }

  int retVal = vtktesting->RegressionTest(fileName, vtkRegressionTester::ErrorThreshold);
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
